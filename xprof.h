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
#pragma once

#include "containers/list.h"
#include "containers/array.h"
#include "containers/ringbuffer.h"
#include "platformspec.h"
#include "threadtools.h"

#include <initializer_list>
#include <stdio.h>
#include <stack>
#include <memory.h>

#define MAX_NODESTACK_DEPTH 32
#define MAX_NODESTACKS	    64

#define XPROF_CATEGORY_OTHER		"Other"
#define XPROF_CATEGORY_MATH		"MathFuncs"
#define XPROF_CATEGORY_RENDERING	"Rendering"
#define XPROF_CATEGORY_MAINUI		"MainUI"
#define XPROF_CATEGORY_PHYSICS		"PhysicsLoading"
#define XPROF_CATEGORY_SOUND		"SoundLoading"
#define XPROF_CATEGORY_MAPLOAD		"MapLoading"
#define XPROF_CATEGORY_TEXLOAD		"TextureLoading"
#define XPROF_CATEGORY_MODELOAD		"ModelLoading"
#define XPROF_CATEGORY_SOUNDLOAD	"SoundLoading"
#define XPROF_CATEGORY_FILESYSTEM	"Filesystem"
#define XPROF_CATEGORY_CRTFUNC		"CrtFunctions"
#define XPROF_CATEGORY_NETWORK		"Network"
#define XPROF_CATEGORY_CVAR		"ConsoleVar"
#define XPROF_CATEGORY_CONCOMMAND	"ConsoleCommand"
#define XPROF_CATEGORY_SCRIPTING	"Scripting"
#define XPROF_CATEGORY_KVPARSE		"KeyValuesParsing"
#define XPROF_CATEGORY_PARSING		"FileParsing"
#define XPROF_CATEGORY_GAME_CLIENT_INIT "ClientInit"
#define XPROF_CATEGORY_GAME_SERVER_INIT "ServerInit"
#define XPROF_CATEGORY_CLIENT_THINK	"ClientThink"
#define XPROF_CATEGORY_SERVER_THINK	"ServerThink"
#define XPROF_CATEGORY_UNZIP		"Unzip"
#define XPROF_CATEGORY_LZSS		"LZSS"
#define XPROF_CATEGORY_COMMON		"Common"
#define XPROF_CATEGORY_FRAME		"Frame"

/* If we have the vtune library, enable external instrumentation */
/* in the future, more external instrumentation libraries may be used */
#if defined(HAVE_ITTNOTIFY)
#define XPROF_INSTRUMENT 1
#endif

/*

/* XProf Flags */
#define XPROF_DUMP_ON_EXIT	(1 << 0)
#define XPROF_RECORD_FRAME_DATA (1 << 1) /* Record frame times and insert them into a ring buffer?? */

/* Number of seconds of frame data to record by default */
/* This will equal a total of 16kb of data (XProfFrameData is 16 bytes) */
#define XPROF_DEFAULT_FRAMEBUFFER_SIZE 1024

#undef GetCurrentTime

namespace xprof
{
constexpr inline unsigned long long SecondsToNs(unsigned long long sec) { return sec * 1000000000; }

constexpr inline unsigned long long SecondsToUs(unsigned long long sec) { return sec * 1000000; }

constexpr inline unsigned long long SecondsToMs(unsigned long long sec) { return sec * 1000; }

constexpr inline unsigned long long MsToNs(unsigned long long sec) { return sec * 1000000; }

constexpr inline unsigned long long FpsToNs(unsigned long long fps) { return (1000 / fps) * 1000000; }

constexpr inline float NsToMsF(unsigned long long ns) { return (ns / (float)1000000); }
} // namespace xprof

/* Holds a total of 16 bytes of frame data for a 1 second run time of the engine */
struct XProfFrameData
{
	float  max_time;   // max frame time
	float  avg;	   // average frame time
	float  min_time;   // min frame time
	int    num_frames; // frames that passed during this sample point (not saved to file)
	double timestamp;  // timestamp in seconds since unix epoch. Corresponds to the *start* of the sample

	void clear()
	{
		max_time = avg = 0.0f;
		min_time       = 1e9f;
		num_frames     = 0;
		timestamp      = 0.0;
	}
};

/* Struct with a list of features that xprof can enable or disable */
struct XProfFeatures
{
	/**
	 * XProf has an integrated frame time counter that is used to log FPS data to a file
	 * for better profiling out of engine.
	 * This has little overhead
	 */
	bool EnableFrameTimeCounter : 1;

	/**
	 * If your application is multithreaded, always enable this. In the event that you're
	 * building a primarily single-threaded application, you should leave this off.
	 * By default, it's on
	 * TODO: Actually implement!
	 */
	bool EnableThreadSafety : 1;
};

class EXPORT CXProf
{
private:
	/* Hirearcheal profiling data */
	List<class CXProfNode*>	      m_nodes;
	std::stack<class CXProfNode*> m_nodeStack[MAX_NODESTACKS];
	unsigned long long	      m_nodeStackThreads[MAX_NODESTACKS];

	/* General properties */
	XProfFeatures m_features;
	bool	      m_enabled;
	bool	      m_init;
	unsigned int  m_flags;

	/* For thread safety */
	mutable CThreadRecursiveMutex m_mutex;

	/* Properties that hold times */
	platform::time_t m_lastFrameTime;
	platform::time_t m_frameStart;

	/* FPS Counter Properties */
	size_t			   m_fpsCounterBufferSize;
	size_t			   m_fpsCounterTotalSamples;
	float			   m_fpsCounterSampleInterval; // For FPS counter
	platform::time_t	   m_fpsCounterLastSampleTime;
	XProfFrameData		   m_fpsCounterCurrentSample;
	RingBuffer<XProfFrameData> m_fpsCounterDataBuffer;

	/* List of shutdown hooks */
	List<void (*)(CXProf&)> m_shutdownHooks;

public:
	CXProf();
	~CXProf();

	/* Creates a new node category */
	/* THREAD SAFE */
	void AddCategoryNode(const char* name, unsigned long long budget);

	class CXProfNode* CreateNode(const char* category, const char* func, const char* file, unsigned long long budget);
	void		  PushNode(class CXProfNode* node);
	void		  PopNode();

	bool Initialized() const { return m_init; };

	/* Returns a pointer to the current node.
	 * NOT thread-safe. Lock before calling this! */
	class CXProfNode* CurrentNode();

	platform::time_t LastFrameTime() const { return m_lastFrameTime; };

	/* Begins/Ends a frame and records various info
	 * thread safe, BUT ONLY call from main thread */
	void BeginFrame();
	void EndFrame();

	/* Use to report memory allocations/frees */
	/* Thread-safe without locks */
	void ReportAlloc(size_t sz);
	void ReportRealloc(size_t oldsize, size_t newsize);
	void ReportFree();

	class CXProfNode* FindCategory(const char* name);

	/* Returns a list of category nodes */
	/* THREAD SAFE */
	List<class CXProfNode*> Nodes()
	{
		auto lock = m_mutex.RAIILock();
		return m_nodes;
	};

	/* Tree dump functions */
	/* NOT THREAD SAFE */
	void DumpAllNodes(int (*printFn)(const char*, ...) = printf);
	void DumpCategoryTree(const char* cat, int (*printFn)(const char*, ...) = printf);

	/* Dumps all data to JSON format. Pass it a buffer to write into */
	void DumpToJSON(std::ostream& stream);

	/* Enables or disables xprof */
	/* THREAD SAFE */
	bool Enabled() const;
	void Enable();
	void Disable();

	void ClearNodes();

	/* Change the size of the frame buffer counter */
	void   SetFrameCountBufferSize(size_t newsize);
	size_t FrameCountBufferSize();
	void   SetFrameSampleInterval(float seconds);
	float  GetFrameSampleInterval();

	/* Feature handling */
	XProfFeatures Features() const { return m_features; }
	void	      SetFeatures(XProfFeatures features);

	/* Hooking Functions */
	/* In case you need to do some reporting or cleanup right before xprof is shutdown */
	void HookShutdown(void (*f)(CXProf&)) { m_shutdownHooks.push_back(f); };
	void RemoveShutdownHook(void (*f)(CXProf&)) { m_shutdownHooks.remove(f); };

	/* Event submission */
	/* This will signal the profiler when an event happens */
	void SubmitEvent(const char* name);

private:
	void DumpNodeTreeInternal(class CXProfNode* node, int indent, int (*printFn)(const char*, ...) = printf);
};

class EXPORT CXProfNode
{
private:
	void*			m_pvt;
	CXProfNode*		m_parent; /* Pointer to the node that is one above this, or null */
	CXProfNode*		m_root;	  /* Pointer to the "category" node. Aka the absolute root. Nullptr if this is the category */
	List<CXProfNode*>	m_children;
	const char*		m_function;
	const char*		m_comment;
	bool			m_added;
	unsigned long long	m_timeBudget;
	unsigned long long	m_totalTime; /* Total time for THIS NODE */
	unsigned long long	m_absTotal;  /* Total time absolute. Not per-frame */
	unsigned long long	m_avgTime;   /* Average per-frame time */
	bool			m_logTests;
	unsigned int		m_testQueueSize;
	Array<class CXProfTest> m_testQueue;
	const char*		m_file;
	const char*		m_category;
	mutable CThreadSpinlock m_mutex;
	platform::time_t m_lastSampleTime; /* Used to record the last sample time of the node, which is used to determine if this was sampled in the
					      last frame or not */

	unsigned long long m_numFrames; // Total number of frames

	/* Stuff for malloc tracking */
	unsigned long long m_allocBudget; /* Per-frame malloc budget */
	unsigned long long m_freeBudget;  /* Per-frame free budget */

	unsigned long long m_frameAllocs;     /* Number of allocations per frame */
	unsigned long long m_frameAllocBytes; /* Number of bytes allocated in this frame */
	unsigned long long m_frameFrees;      /* Number of calls to free */

	unsigned long long m_totalAllocs;
	unsigned long long m_totalAllocBytes;
	unsigned long long m_totalFrees;

	/* Averages for allocs and such */
	unsigned int m_avgAllocs;
	unsigned int m_avgAllocBytes;
	unsigned int m_avgFrees;

	friend class CXProf;

	/**
	 * Counter reports for the memory allocators and such
	 * These will be called by CXProf
	 */
	void ReportAlloc(size_t size);
	void ReportFree();
	void ReportRealloc(size_t old, size_t newsize);

public:
	CXProfNode(const char* category, const char* function, const char* file, unsigned long long budget, const char* comment = nullptr);

	~CXProfNode();

	CXProfNode(const CXProfNode& other)
	{
		/* Copy all-nontrivial types */
		memcpy((void*)this, &other, sizeof(CXProfNode));
		/* Copy non-trivial types */
		m_children	 = other.m_children;
		m_testQueue	 = other.m_testQueue;
		m_lastSampleTime = other.m_lastSampleTime;
	}

	CXProfNode(CXProfNode&& other) noexcept
	{
		/* Copy all-nontrivial types */
		memcpy((void*)this, &other, sizeof(CXProfNode));
		/* Copy non-trivial types */
		m_children	 = std::move(other.m_children);
		m_testQueue	 = std::move(other.m_testQueue);
		m_lastSampleTime = other.m_lastSampleTime;
	}

	/* Returns a copy of this node for reading */
	/* Also note that this will intentionally unset fields like m_parent, m_root and m_children */
	/* THREAD SAFE */
	CXProfNode LockRead()
	{
		auto lock = m_mutex.RAIILock();
		/* Make a copy of the node and then clear all fields out that we do not want the accessor to use */
		CXProfNode copy = *this;
		copy.m_parent = copy.m_root = nullptr;
		copy.m_testQueue.clear();
		copy.m_children.clear();
		return *this;
	}

	/**
	 * @brief Submit a new XProf time test to the node
	 */
	void SubmitTest(class CXProfTest* test);

	void		   SetBudget(unsigned long long time);
	unsigned long long GetBudget() const;

	/**
	 * @brief Get or reset the remaining budget constraints
	 */
	unsigned long long GetRemainingBudget() const;
	void		   ResetBudget();

	/**
	 * @brief Resets anything that might need to be reset
	 */
	void DoFrame();

	/**
	 * @brief Used to report test start for instrumentation that might be enabled.
	 * @param test
	 */
	void ReportTaskBegin(class CXProfTest* test);
	void ReportTaskEnd(class CXProfTest* test);

	/* Accessors */
	/* THREAD SAFE */
	const char*	  Name() const { return m_function; };
	const char*	  File() const { return m_file; };
	const char*	  Category() const { return m_category; };
	CXProfNode*	  Parent() const { return m_parent; };
	List<CXProfNode*> Children() const;
	void		  AddChild(CXProfNode* node);
	Array<CXProfTest> TestQueue() const;
};

EXPORT CXProf& GlobalXProf();

/* defined in header to avoid cross DLL calls */
class EXPORT CXProfTest
{
public:
	CXProfNode*	 node;
	platform::time_t start, stop;
	bool		 m_disabled;

	CXProfTest(CXProfNode* node) : m_disabled(false)
	{
		m_disabled = !GlobalXProf().Enabled() || !GlobalXProf().Initialized();
		if (m_disabled)
			return;

		/* Set the vars */
		this->node = node;
		start	   = platform::GetCurrentTime();

#ifdef XPROF_INSTRUMENT
		node->ReportTaskBegin(this);
#endif
		GlobalXProf().PushNode(node);
	}

	~CXProfTest()
	{
		if (m_disabled)
			return;

		stop = platform::GetCurrentTime();

		this->node->SubmitTest(this);
		GlobalXProf().PopNode();
	}
};

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#ifdef ENABLE_XPROF
#define XPROF_NODE(category)                                                                                                                         \
	static CXProfNode* __xprof_node = new CXProfNode((category), __PRETTY_FUNCTION__, __FILE__, 0);                                              \
	CXProfTest	   __xprof_test__##__COUNTER__(__xprof_node);
#else
#define XPROF_NODE(c)
#endif
