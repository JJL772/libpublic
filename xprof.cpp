/*
Copyright (C) 2020 Jeremy Lorelli

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#include "xprof.h"
#include "public.h"
#include "crtlib.h"
#include "static_helpers.h"
#undef min
#undef max
#undef GetCurrentTime
#include <stack>
#include <stdio.h>
#include <ostream>
#include <iomanip>

#ifdef HAVE_ITTNOTIFY
#include "ittnotify/ittnotify.h"
#endif

CXProf* g_pXProf = NULL;

struct xprof_node_desc_t
{
	const char*	   name;
	unsigned long long budget;
};

static xprof_node_desc_t g_categories[] = {{XPROF_CATEGORY_OTHER, 0},
					   {XPROF_CATEGORY_MATH, 0},
					   {XPROF_CATEGORY_RENDERING, 0},
					   {XPROF_CATEGORY_MAINUI, 0},
					   {XPROF_CATEGORY_PHYSICS, 0},
					   {XPROF_CATEGORY_SOUND, 0},
					   {XPROF_CATEGORY_MAPLOAD, 0},
					   {XPROF_CATEGORY_TEXLOAD, 0},
					   {XPROF_CATEGORY_MODELOAD, 0},
					   {XPROF_CATEGORY_SOUNDLOAD, 0},
					   {XPROF_CATEGORY_FILESYSTEM, 0},
					   {XPROF_CATEGORY_CRTFUNC, 0},
					   {XPROF_CATEGORY_NETWORK, 0},
					   {XPROF_CATEGORY_CVAR, 0},
					   {XPROF_CATEGORY_CONCOMMAND, 0},
					   {XPROF_CATEGORY_SCRIPTING, 0},
					   {XPROF_CATEGORY_KVPARSE, 0},
					   {XPROF_CATEGORY_PARSING, 0},
					   {XPROF_CATEGORY_GAME_CLIENT_INIT, 0},
					   {XPROF_CATEGORY_GAME_SERVER_INIT, 0},
					   {XPROF_CATEGORY_CLIENT_THINK, 0},
					   {XPROF_CATEGORY_SERVER_THINK, 0},
					   {XPROF_CATEGORY_UNZIP, 0},
					   {XPROF_CATEGORY_LZSS, 0},
					   {XPROF_CATEGORY_COMMON, 0},
					   {XPROF_CATEGORY_FRAME, 0}};

CXProf& GlobalXProf()
{
	if (!g_pXProf)
		g_pXProf = new CXProf();
	return *g_pXProf;
}

void kill_xprof() { delete g_pXProf; }

static CStaticDestructionWrapper<kill_xprof> g_xprofKiller;

/* Constructor is NOT thread-safe, obviously! */
CXProf::CXProf()
	: m_enabled(true), m_lastFrameTime(), m_flags(0), m_init(false), m_fpsCounterBufferSize(XPROF_DEFAULT_FRAMEBUFFER_SIZE),
	  m_fpsCounterDataBuffer(), m_fpsCounterTotalSamples(0), m_fpsCounterSampleInterval(1.0f), m_features(XProfFeatures())
{
	for (int i = 0; i < MAX_NODESTACKS; i++)
	{
		m_nodeStack[i]	      = std::stack<CXProfNode*>();
		m_nodeStackThreads[i] = 0;
	}
	for (int i = 0; i < (sizeof(g_categories) / sizeof(xprof_node_desc_t)); i++)
	{
		this->AddCategoryNode(g_categories[i].name, g_categories[i].budget);
	}
	m_init = true;
}

CXProf::~CXProf()
{
	for (auto hook : m_shutdownHooks)
		hook(*this);

	if (m_flags & XPROF_DUMP_ON_EXIT)
	{
		for (int i = 0; i < (sizeof(g_categories) / sizeof(g_categories[0])); i++)
			this->DumpCategoryTree(g_categories[i].name);
	}
}

void CXProf::AddCategoryNode(const char* name, unsigned long long budget)
{
	auto	    lock = m_mutex.RAIILock();
	CXProfNode* node = new CXProfNode(name, "", "", budget);
	node->m_parent	 = nullptr;
	node->m_root	 = node;
	node->m_category = name;
	node->m_function = name;
	m_nodes.push_back(node);
}

void CXProf::PushNode(CXProfNode* node)
{
	auto lock = m_mutex.RAIILock();

	/* first, get the nodestack index for this thread */
	auto threadid = platform::GetCurrentThreadId();
	int  index    = -1;
	for (int i = 0; i < MAX_NODESTACKS; i++)
	{
		if (m_nodeStackThreads[i] == threadid)
		{
			index = i;
			break;
		}
	}

	/* Not found? Loop until a free one is found */
	if (index == -1)
	{
		for (index = 0; index < MAX_NODESTACKS; index++)
		{
			if (m_nodeStackThreads[index] == 0)
			{
				m_nodeStackThreads[index] = threadid;
				break;
			}
		}
	}

	std::stack<CXProfNode*>* nodestack = &m_nodeStack[index];

	CXProfNode* parent = nullptr;

	if (nodestack->empty())
		parent = this->FindCategory(node->m_category);
	else
		parent = nodestack->top();

	if (!parent)
	{
		printf("Internal error while pushing node %s: Parent was nullptr. Category=%s\n", node->Name(), node->m_category);
		return;
	}

	if (!node->m_added)
	{
		node->m_parent = parent;
		node->m_added  = true;
		node->m_root   = parent->m_root;
		node->m_parent->m_children.push_back(node);
		node->m_timeBudget = node->m_timeBudget ? node->m_timeBudget : parent->m_timeBudget;
		node->m_category   = node->m_parent->m_category;
	}
	nodestack->push(node);
}

void CXProf::BeginFrame()
{
	auto lock    = this->m_mutex.RAIILock();
	m_frameStart = platform::GetCurrentTime();
}

void CXProf::EndFrame()
{
	auto lock	= this->m_mutex.RAIILock();
	m_lastFrameTime = platform::GetCurrentTime();
	float frameDt	= m_lastFrameTime.to_ms() - m_frameStart.to_ms();

	double currentTime = m_lastFrameTime.to_seconds();

	/**
	 * FRAME TIME HANDLING
	 */
	if (m_features.EnableFrameTimeCounter)
	{
		/* Initialize the ring buffer if it hasnt been already */
		if (m_fpsCounterDataBuffer.size() == 0)
			m_fpsCounterDataBuffer.resize(m_fpsCounterBufferSize);
		/* Update the frame samples */
		if (frameDt > m_fpsCounterCurrentSample.max_time)
			m_fpsCounterCurrentSample.max_time = frameDt;
		if (frameDt < m_fpsCounterCurrentSample.min_time)
			m_fpsCounterCurrentSample.min_time = frameDt;
		m_fpsCounterCurrentSample.num_frames++;
		m_fpsCounterCurrentSample.avg = ((float)(m_fpsCounterCurrentSample.num_frames - 1) * m_fpsCounterCurrentSample.avg + frameDt) /
						((float)m_fpsCounterCurrentSample.num_frames);

		/* Check if it's been a second since the last time EndFrame was called and insert into frame time buffer */
		double lastSecond = m_fpsCounterLastSampleTime.to_seconds();
		if (lastSecond == 0.0)
			m_fpsCounterLastSampleTime = platform::GetCurrentTime();
		else if (currentTime - lastSecond > m_fpsCounterSampleInterval)
		{
			m_fpsCounterLastSampleTime = m_lastFrameTime;
			m_fpsCounterDataBuffer.write(m_fpsCounterCurrentSample);
			m_fpsCounterCurrentSample.clear();
			m_fpsCounterTotalSamples++;
			m_fpsCounterCurrentSample.timestamp = currentTime;
		}
	}
}

void CXProf::PopNode()
{
	auto threadid = platform::GetCurrentThreadId();
	for (int i = 0; i < MAX_NODESTACKS; i++)
	{
		if (m_nodeStackThreads[i] == threadid)
		{
			if (m_nodeStack[i].empty())
				return;
			m_nodeStack[i].pop();
			return;
		}
	}
}

CXProfNode* CXProf::CreateNode(const char* category, const char* func, const char* file, unsigned long long budget)
{
	CXProfNode* node = new CXProfNode(category, func, file, budget);
	this->PushNode(node);
	return node;
}

void CXProf::DumpAllNodes(int (*printFn)(const char*, ...))
{
	for (auto x : m_nodes)
	{
		DumpCategoryTree(x->m_category, printFn);
	}
}

class CXProfNode* CXProf::FindCategory(const char* category)
{
	for (auto x : this->m_nodes)
	{
		if (Q_strcmp(category, x->m_category) == 0)
			return x;
	}
	return nullptr;
}

void CXProf::DumpCategoryTree(const char* cat, int (*printFn)(const char*, ...))
{
	for (auto x : m_nodes)
	{
		if (Q_strcmp(x->m_category, cat) == 0)
		{
			this->DumpNodeTreeInternal(x, 0, printFn);
			return;
		}
	}
}

#define FILL_TABS                                                                                                                                    \
	for (int i = 0; i < indent; i++)                                                                                                             \
	printFn("\t")

void CXProf::DumpNodeTreeInternal(CXProfNode* node, int indent, int (*printFn)(const char*, ...))
{
	/* Print node name */
	FILL_TABS;
	printFn("%s\n", node->Name());

	/* Print total time */
	FILL_TABS;
	printFn("Total time: %llu us\n", node->m_absTotal / 1000);

	/* Print average time */
	FILL_TABS;
	printFn("Average per-frame time: %llu us\n", node->m_avgTime / 1000);

	/* Print total allocs */
	FILL_TABS;
	printFn("Total allocs: %llu for %llu bytes total\n", node->m_totalAllocs, node->m_totalAllocBytes);

	for (auto x : node->Children())
		DumpNodeTreeInternal(x, indent + 1);
}

void CXProf::ClearNodes()
{
	auto lock = m_mutex.RAIILock();
	for (auto x : this->m_nodes)
	{
		/* Only delete the non-category nodes */
		for (auto g : x->m_children)
		{
			delete g;
		}
	}
}

void CXProf::Disable()
{
	auto lock = m_mutex.RAIILock();
	m_enabled = false;
}

void CXProf::Enable()
{
	auto lock = m_mutex.RAIILock();
	m_enabled = true;
}

bool CXProf::Enabled() const
{
	auto lock = m_mutex.RAIILock();
	return m_enabled;
}

void CXProf::ReportAlloc(size_t sz)
{
	auto	    lock = m_mutex.RAIILock();
	CXProfNode* node = CurrentNode();
	if (!node)
		return;
	node->ReportAlloc(sz);
}

void CXProf::ReportRealloc(size_t oldsize, size_t newsize)
{
	auto	    lock = m_mutex.RAIILock();
	CXProfNode* node = CurrentNode();
	if (!node)
		return;
	node->ReportRealloc(oldsize, newsize);
}

void CXProf::ReportFree()
{
	auto	    lock = m_mutex.RAIILock();
	CXProfNode* node = CurrentNode();
	if (!node)
		return;
	node->ReportFree();
}

class CXProfNode* CXProf::CurrentNode()
{
	auto threadid = platform::GetCurrentThreadId();
	int  index    = -1;
	for (int i = 0; i < MAX_NODESTACKS; i++)
	{
		if (m_nodeStackThreads[i] == threadid)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
		return nullptr;
	if (m_nodeStack[index].empty())
		return nullptr;

	return m_nodeStack[index].top();
}

void CXProf::SetFrameCountBufferSize(size_t newsize)
{
	m_fpsCounterDataBuffer.resize(newsize);
	m_fpsCounterBufferSize = newsize;
}

size_t CXProf::FrameCountBufferSize() { return m_fpsCounterBufferSize; }

void CXProf::DumpToJSON(std::ostream& stream)
{
	// Gotta increase the precision......
	stream << std::setprecision(1000);

	stream << "{";
	/* First part is to write out basic info */
	stream << "\"game_info\": {";
	stream << "\"name\": \"" << PROJECT_NAME << "\",";
	stream << "\"desc\": \"" << PROJECT_DESCRIPTION << "\",";
	stream << "\"version\": \"" << PROJECT_VERSION << "\"";
	stream << "},";

	stream << "\"system_info\": {";
	// TODO
	stream << "},";

	stream << "\"frame_times\": [";
	// Perform a read sequence here, but make sure to adjust the read head in case we wrapped around.
	// We do not want to lose any frame data.
	if (m_fpsCounterTotalSamples > m_fpsCounterDataBuffer.size())
		m_fpsCounterDataBuffer.set_read_index(m_fpsCounterDataBuffer.write_index());
	for (int i = 0; i < m_fpsCounterDataBuffer.size() && i < m_fpsCounterTotalSamples; i++)
	{
		XProfFrameData frame = m_fpsCounterDataBuffer.read();
		stream << "{\"max\": " << frame.max_time << ", \"min\": " << frame.min_time << ", \"avg\": " << frame.avg
		       << ", \"timestamp\": " << frame.timestamp << "}";
		if (i != m_fpsCounterDataBuffer.size() - 1 && i != m_fpsCounterTotalSamples - 1)
			stream << ",";
	}
	stream << "],";

	// Print out budget info. Nothing fancy here, still hirearchieal printing
	stream << "\"budget_info\": [";

	stream << "]";

	stream << "}";
}

void CXProf::SetFrameSampleInterval(float seconds)
{
	/* Make sure our sample time is a sensible amount. Having it at 0 could cause big problems */
	if (seconds < 0.05)
		return;
	m_fpsCounterSampleInterval = seconds;
}

float CXProf::GetFrameSampleInterval() { return m_fpsCounterSampleInterval; }

void CXProf::SetFeatures(XProfFeatures features)
{
	// Do any special on-enable handling here
	XProfFeatures oldFeatures = m_features;
	m_features		  = features;
}

void CXProf::SubmitEvent(const char* name)
{
#ifdef HAVE_ITTNOTIFY

#endif
}

/*=======================================================
 *
 *      CXProfNode
 *
 *======================================================= */

struct XProfNodePvt_t
{
	char m_domainName[512];
#ifdef HAVE_ITTNOTIFY
	__itt_domain*	     m_domain;
	__itt_string_handle* m_funcNameHandle;
#endif
};

CXProfNode::CXProfNode(const char* category, const char* function, const char* file, unsigned long long budget, const char* comment)
	: m_timeBudget(budget), m_function(function), m_file(file), m_logTests(false), m_parent(nullptr), m_root(nullptr), m_testQueueSize(0),
	  m_category(category), m_totalTime(0), m_added(false), m_lastSampleTime(), m_absTotal(0), m_totalAllocBytes(0), m_allocBudget(0),
	  m_frameAllocBytes(0), m_frameAllocs(0), m_frameFrees(0), m_totalAllocs(0), m_totalFrees(0), m_freeBudget(0), m_avgFrees(0),
	  m_avgAllocBytes(0), m_avgAllocs(0), m_numFrames(0), m_avgTime(0), m_comment(comment), m_pvt(nullptr)
{
	m_pvt		    = malloc(sizeof(XProfNodePvt_t));
	XProfNodePvt_t* pvt = (XProfNodePvt_t*)m_pvt;

	snprintf(pvt->m_domainName, sizeof(pvt->m_domainName), "%s:%s", function, category);
#ifdef HAVE_ITTNOTIFY
	pvt->m_domain	      = __itt_domain_create(pvt->m_domainName);
	pvt->m_funcNameHandle = __itt_string_handle_create(function);
#endif
}

CXProfNode::~CXProfNode() { /* m_pvt is not freed here because of possible issues with the ittnotify library */ }

unsigned long long CXProfNode::GetRemainingBudget() const
{
	auto		   lock	     = m_mutex.RAIILock();
	unsigned long long remaining = m_totalTime;
	for (auto node : this->m_children)
		remaining += node->m_totalTime;
	return remaining;
}

void CXProfNode::ResetBudget()
{
	auto lock	  = m_mutex.RAIILock();
	this->m_totalTime = 0;
	for (auto node : this->m_children)
	{
		node->ResetBudget();
	}
}

void CXProfNode::SubmitTest(CXProfTest* test)
{
	auto lock	       = m_mutex.RAIILock();
	this->m_lastSampleTime = platform::GetCurrentTime();

	unsigned long long elapsed = (test->stop.to_ns() - test->start.to_ns());
	this->m_totalTime += elapsed;
	this->m_absTotal += elapsed;
	DoFrame();
#ifdef XPROF_INSTRUMENT
	ReportTaskEnd(test);
#endif
}

void CXProfNode::SetBudget(unsigned long long int time)
{
	auto lock    = m_mutex.RAIILock();
	m_timeBudget = time;
}

unsigned long long CXProfNode::GetBudget() const
{
	auto lock = m_mutex.RAIILock();
	return m_timeBudget;
}

List<CXProfNode*> CXProfNode::Children() const
{
	auto lock = this->m_mutex.RAIILock();
	return this->m_children;
}

void CXProfNode::AddChild(CXProfNode* node)
{
	auto lock = this->m_mutex.RAIILock();
	m_children.push_back(node);
}

Array<CXProfTest> CXProfNode::TestQueue() const
{
	auto lock = this->m_mutex.RAIILock();
	return m_testQueue;
}

void CXProfNode::DoFrame()
{
	/* Check if the last frame reset time has taken place after our last sample. If so, reset the per-frame budget time and stuff */
	if (GlobalXProf().LastFrameTime() > this->m_lastSampleTime)
	{
		m_numFrames++;

		/* Update the average allocations and stuff */
		this->m_avgFrees      = ((m_numFrames - 1) * m_avgFrees + m_frameFrees) / m_numFrames;
		this->m_avgAllocs     = ((m_numFrames - 1) * m_avgAllocs + m_frameAllocs) / m_numFrames;
		this->m_avgAllocBytes = ((m_numFrames - 1) * m_avgAllocBytes + m_frameAllocBytes) / m_numFrames;

		/* Update frame time average */
		this->m_avgTime = ((m_numFrames - 1) * m_avgTime + m_totalTime) / m_numFrames;

		this->m_totalTime	= 0;
		this->m_frameAllocs	= 0;
		this->m_frameAllocBytes = 0;
		this->m_frameFrees	= 0;
	}
}

void CXProfNode::ReportAlloc(size_t size)
{
	auto lock = this->m_mutex.RAIILock();
	m_totalAllocs++;
	m_frameAllocs++;
	m_frameAllocBytes += size;
	m_totalAllocBytes += size;
	DoFrame();
}

void CXProfNode::ReportFree()
{
	auto lock = this->m_mutex.RAIILock();
	m_totalFrees++;
	m_frameFrees++;
	DoFrame();
}

void CXProfNode::ReportRealloc(size_t old, size_t newsize)
{
	auto lock = this->m_mutex.RAIILock();
	int  ds	  = newsize - old;
	m_totalAllocs++;
	m_frameAllocs++;
	m_totalFrees++; // Gonna count this as a free too. Need a better way to detect this
	m_totalAllocBytes += ds;
	m_frameAllocBytes += ds;
	DoFrame();
}

void CXProfNode::ReportTaskBegin(CXProfTest* test)
{
	auto pvt = static_cast<XProfNodePvt_t*>(m_pvt);
#ifdef HAVE_ITTNOTIFY
	__itt_task_begin(pvt->m_domain, __itt_null, __itt_null, pvt->m_funcNameHandle);
#endif
}

void CXProfNode::ReportTaskEnd(CXProfTest* test)
{
	auto pvt = static_cast<XProfNodePvt_t*>(m_pvt);
#ifdef HAVE_ITTNOTIFY
	__itt_task_end(pvt->m_domain);
#endif
}
