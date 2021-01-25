/**
 *
 * unittestlib.h - Simple unit testing library, depending only on C++ STL stuff
 *
 */
#pragma once

#include <list>
#include <vector>
#include <string>
#include <chrono>
#include <stack>
#include <functional>

/* Okay I lied about the STL dependencies.. */
#include "logger.h"


class CUnitTest
{
private:
	bool m_failed;
	std::string m_name;
	std::string m_failedName;
	bool m_submitted;
	unsigned long long m_id = 0;
	class CUnitTestSuite* m_testSuite;

	friend class CUnitTestSuite;

protected:
	virtual ~CUnitTest() {};

	CUnitTest(const std::string name, CUnitTestSuite* suite) :
		m_failed(false),
		m_name(name),
		m_testSuite(suite)
	{
	}
public:

	const std::string& Name() const { return m_name; }
	bool Failed() const { return m_failed; }
	bool Passed() const { return !m_failed; }
	const std::string& FailedLocation() const { return m_failedName; }

	bool AssertTrue(bool result, const std::string& failed_name = "")
	{
		if(m_failed)
			return false;
		if(!result)
		{
			m_failed = true;
			m_failedName = failed_name;
		}
		return result;
	}

	bool AssertFalse(bool result, const std::string& failed_name = "")
	{
		if(m_failed)
			return false;
		if(result)
		{
			m_failed = true;
			m_failedName = failed_name;
		}
		return !result;
	}

	/* Must be equal with epsilon for floating point math */
	template<class A, class B, class C>
	bool MustBeEqual(const A& a, const B& b, const C& epsilon, const std::string& failed_name = "")
	{
		return this->AssertTrue((a-epsilon <= b) && (a+epsilon >= b), failed_name);
	}

	template<class A, class B, class C>
	bool MustNotBeEqual(const A& a, const B& b, const C& epsilon, const std::string& failed_name = "")
	{
		return this->AssertTrue(!((a-epsilon <= b) && (a+epsilon >= b)), failed_name);
	}

	template<class A, class B>
	bool MustBeEqual(const A& a, const B& b, const std::string& failed_name = "")
	{
		return this->AssertTrue(a == b, failed_name);
	}

	template<class A, class B>
	bool MustNotBeEqual(const A& a, const B& b, const std::string& failed_name = "")
	{
		return this->AssertTrue(a != b, failed_name);
	}
};

class CUnitTestTimed : public CUnitTest
{
private:
	using TimeT = unsigned long long;
	using HighResClockT = std::chrono::high_resolution_clock;

	friend class CUnitTestSuite;

	struct TimedTest_t
	{
		TimedTest_t()
		{
			m_avgTime = 0;
			m_maxTime = std::numeric_limits<TimeT>::min();
			m_minTime = std::numeric_limits<TimeT>::max();
			m_name = "";
			m_iterations = 0;
		}
		TimeT m_avgTime;
		TimeT m_minTime;
		TimeT m_maxTime;
		unsigned int m_iterations;
		std::string m_name;
	};

	std::stack<TimedTest_t> m_testStack;
	TimedTest_t m_test;

protected:
	virtual ~CUnitTestTimed() {};

	CUnitTestTimed(const std::string& name, CUnitTestSuite* suite) :
		CUnitTest(name, suite)
	{

	}
public:

	inline void BeginTimed(const std::string& timed_name)
	{
		TimedTest_t test;
		test.m_name = timed_name;
		m_testStack.push(test);
	}

	inline void EndTimed()
	{
		if(!m_testStack.empty())
		{
			m_test = m_testStack.top();
			m_testStack.pop();
		}
	}

	/**
	 * Performs a timed-iterative test on a function. Averages the test time and computes maxs and mins and submits the
	 * results to a test buffer
	 * @param func The function to execute
	 * @param num_iterations Number of iterations to incur
	 * @param name Name of the iterative test
	 */
	void IteratedTest(std::function<void(void)> func, unsigned int num_iterations, const std::string& name)
	{
		if(num_iterations == 0)
			return;

		this->BeginTimed(name);
		for(unsigned int i = 0; i < num_iterations; i++)
		{
			auto t0 = HighResClockT::now();
			func();
			auto t1 = HighResClockT ::now();
			TimedTest_t& test = m_testStack.top();

			auto dt = t1.time_since_epoch() - t0.time_since_epoch();
			test.m_avgTime = (test.m_iterations * test.m_avgTime + dt.count()) / (test.m_iterations+1);
			test.m_iterations++;

			test.m_minTime = std::min(test.m_minTime, (TimeT)dt.count());
			test.m_maxTime = std::max(test.m_maxTime, (TimeT)dt.count());
		}
		this->EndTimed();
	}

	/**
	 * Performs a timed-iterative test on a function. Averages the test time and computes maxs and mins and submits the
	 * results to a test buffer
	 * @param prefunc Function to execute right before the real one
	 * @param func The function to execute
	 * @param num_iterations Number of iterations to incur
	 * @param name Name of the iterative test
	 */
	template<class T>
	void IteratedTest(std::function<T(void)> prefunc, std::function<void(T)> func, unsigned int num_iterations, const std::string& name)
	{
		if(num_iterations == 0)
			return;

		this->BeginTimed(name);
		for(unsigned int i = 0; i < num_iterations; i++)
		{
			T a = prefunc();
			auto t0 = HighResClockT::now();
			func(a);
			auto t1 = HighResClockT ::now();
			TimedTest_t& test = m_testStack.top();

			auto dt = t1.time_since_epoch() - t0.time_since_epoch();
			test.m_avgTime = (test.m_iterations * test.m_avgTime + dt.count()) / (test.m_iterations+1);
			test.m_iterations++;

			test.m_minTime = std::min(test.m_minTime, (TimeT)dt.count());
			test.m_maxTime = std::max(test.m_maxTime, (TimeT)dt.count());
		}
		this->EndTimed();
	}
};

class CUnitTestSuite
{
private:
	friend class CUnitTest;

	std::vector<CUnitTest*> m_tests;
	unsigned int m_failed = 0;
	unsigned int m_passed = 0;
	unsigned int m_totalComplete = 0;
	std::string m_name;
	unsigned long long m_idCounter = 0;

	LogChannel m_channel;

	CUnitTestSuite()
	{
		m_tests.reserve(20);
	}
public:
	/**
	 * Creates a new named test suite
	 * @param name
	 * @param log_chan If INVALID_CHANNEL_ID is passed (which is the default), use normal printf (no color support though)
	 * @return ALWAYS VALID pointer to a new test suite
	 */
	static CUnitTestSuite* Create(const std::string& name, LogChannel log_chan = Log::INVALID_CHANNEL_ID)
	{
		auto suite = new CUnitTestSuite();
		suite->m_name = name;
		suite->m_channel = log_chan;
		return suite;
	}

	/**
	 * Just a static method to destroy your test suite
	 * @param suite
	 */
	static void Destroy(CUnitTestSuite* suite)
	{
		delete suite;
	}

	~CUnitTestSuite()
	{
		for(auto t : m_tests) {
			delete t;
		}
	}

	/**
	 * Reports the results of all tests and returns a return code you can use for the main() method
	 * @return number of failed tests
	 */
	int Report()
	{
		const LogColor green = {100, 200, 100};
		const LogColor red = {200, 100, 100};
		const LogColor teal = {0, 200, 200};
		const LogColor yellow = {200, 200, 50};

		this->MsgC({220, 220, 220}, "==============================================================\n", m_name.c_str());

		for(auto test : m_tests)
		{
			this->MsgC(teal, "  %s\n", test->m_name.c_str());

			if(!test->m_submitted)
			{
				this->MsgC(red, "    NOT SUBMITTED/FINALIZED\n");
			}

			CUnitTestTimed* timed = nullptr;
			if((timed = dynamic_cast<CUnitTestTimed*>(test)))
			{
				auto avg = timed->m_test.m_avgTime;
				auto max = timed->m_test.m_maxTime;
				auto min = timed->m_test.m_minTime;
				this->Msg("    Average Time: %llu ns (%f us, %f ms)\n", avg, (avg/1e3f), (avg/1e6f));
				this->Msg("    Max Time: %llu ns (%f us, %f ms)\n", max, (max/1e3f), (max/1e6f));
				this->Msg("    Min Time: %llu ns (%f us, %f ms)\n", min, (min/1e3f), (min/1e6f));
			}

			if(test->m_failed)
			{
				this->MsgC({255, 0, 0}, "    FAILED [In section \"%s\"]\n", test->m_failedName.c_str());
			}
			else
			{
				this->MsgC(green, "    PASSED\n");
			}
		}

		LogColor color;
		if(m_failed > 0)
			color = red;
		else
			color = green;

		this->MsgC({220, 220, 200}, "\n==============================================================\n");

		this->MsgC({200, 200, 200}, "  %s ", m_name.c_str());
		if(m_failed > 0)
			this->MsgC({255, 0, 0}, "^1SUITE FAILED!^0\n");
		else
			this->MsgC({0, 255, 0}, "SUITE PASSED!\n");

		this->MsgC({0, 200, 200}, "    %.2f%% tests passed (%u/%u)\n", (m_passed/(float)m_totalComplete)*100.f, m_passed, m_totalComplete);

		this->MsgC({220, 220, 220}, "==============================================================\n");

		return m_failed;
	}

	void Msg(const char* fmt, ...)
	{
		va_list vl;
		va_start(vl, fmt);
		if(m_channel != Log::INVALID_CHANNEL_ID)
			Log::Log(m_channel, ELogLevel::MSG_GENERAL, fmt, vl);
		else
			vprintf(fmt, vl);
		va_end(vl);
	}

	void MsgC(LogColor color, const char* fmt, ...)
	{
		va_list vl;
		va_start(vl, fmt);
		if(m_channel != Log::INVALID_CHANNEL_ID)
			Log::Log(m_channel, ELogLevel::MSG_GENERAL, color, fmt, vl);
		else
			vprintf(fmt, vl);
		va_end(vl);
	}

	CUnitTestTimed* CreateTimedTest(const std::string& name)
	{
		auto timed_test = new CUnitTestTimed(name, this);
		timed_test->m_id = m_idCounter++;
		m_tests.push_back(timed_test);
		return timed_test;
	}

	CUnitTest* CreateTest(const std::string& name)
	{
		auto test = new CUnitTest(name, this);
		test->m_id = m_idCounter++;
		m_tests.push_back(test);
		return test;
	}

	void Submit(CUnitTest* test)
	{
		for(auto x : m_tests)
		{
			if(x->m_id == test->m_id)
			{
				x->m_submitted = true;
				if(x->m_failed)
					m_failed++;
				else
					m_passed++;
				m_totalComplete++;
			}
		}
	}

	void Destroy()
	{
		delete this;
	}
};
