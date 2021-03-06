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
#ifdef _WIN32
#include "winplatform.h"
#endif

#include "threadtools.h"

#ifdef _POSIX
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#endif

#include <stdio.h>
#include <stdlib.h>

//===========================================
//
//      CThread
//
//===========================================

CThread::CThread(void* (*threadfn)(void*)) : m_threadfn(threadfn), m_run(false)
{
#ifdef _WIN32
	hThread = nullptr;
#else
	pthread_attr_init(&m_attr);
#endif
}

void CThread::Run(void* pvt)
{
#ifdef _WIN32
	if (m_run && hThread)
		TerminateThread(hThread, 1);
	DWORD dwThreadId;
	hThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(m_threadfn), pvt, 0, &dwThreadId);
	m_run	= true;
#else
	if (m_run)
	{
		pthread_cancel(m_thread);
	}
	m_run = true;
	pthread_create(&m_thread, &m_attr, m_threadfn, pvt);
#endif
}

void CThread::Terminate()
{
#ifdef _WIN32
	if (hThread)
		TerminateThread(hThread, 0);
#else
	pthread_kill(m_thread, SIGTERM);
#endif
}

void CThread::Kill()
{
#ifdef _WIN32
	if (hThread)
		TerminateThread(hThread, 0);
#else
	pthread_kill(m_thread, SIGKILL);
#endif
}

void CThread::Join()
{
#ifdef _WIN32
	if (hThread)
		WaitForSingleObject(hThread, INFINITE);
#else
	pthread_join(m_thread, &m_ret);
#endif
}

//===========================================
//
//      CThreadMutex
//
//===========================================

CThreadMutex::CThreadMutex()
{
#ifdef _WIN32
	m_mutex = CreateMutexA(NULL, TRUE, NULL);
#else
	pthread_mutexattr_init(&m_attr);
	pthread_mutex_init(&m_mutex, &m_attr);
#endif
}

CThreadMutex::~CThreadMutex()
{
#ifdef _WIN32
	CloseHandle(m_mutex);
#else
	pthread_mutex_destroy(&m_mutex);
	pthread_mutexattr_destroy(&m_attr);
#endif
}

void CThreadMutex::Lock()
{
#ifdef _WIN32
	WaitForSingleObject(m_mutex, INFINITE);
#else
	pthread_mutex_lock(&m_mutex);
#endif
}

bool CThreadMutex::TryLock()
{
#ifdef _WIN32
	DWORD dwRes = WaitForSingleObject(m_mutex, 0);
	return dwRes == WAIT_OBJECT_0;
#else
	return pthread_mutex_trylock(&m_mutex) == 0;
#endif
}

void CThreadMutex::Unlock()
{
#ifdef _WIN32
	ReleaseMutex(m_mutex);
#else
	pthread_mutex_unlock(&m_mutex);
#endif
}

//===========================================
//
//      CThreadRecursiveMutex
//
//===========================================

CThreadRecursiveMutex::CThreadRecursiveMutex()
{
#ifdef _WIN32
	m_mutex = CreateMutexA(NULL, TRUE, NULL);
#else
	pthread_mutexattr_init(&m_attr);
	pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &m_attr);
#endif
}

CThreadRecursiveMutex::~CThreadRecursiveMutex()
{
#ifdef _WIN32
	CloseHandle(m_mutex);
#else
	pthread_mutex_destroy(&m_mutex);
	pthread_mutexattr_destroy(&m_attr);
#endif
}

void CThreadRecursiveMutex::Lock()
{
#ifdef _WIN32
	WaitForSingleObject(m_mutex, INFINITE);
#else
	pthread_mutex_lock(&m_mutex);
#endif
}

bool CThreadRecursiveMutex::TryLock()
{
#ifdef _WIN32
	DWORD dwRes = WaitForSingleObject(m_mutex, 0);
	return dwRes == WAIT_OBJECT_0;
#else
	return pthread_mutex_trylock(&m_mutex) == 0;
#endif
}

void CThreadRecursiveMutex::Unlock()
{
#ifdef _WIN32
	ReleaseMutex(m_mutex);
#else
	pthread_mutex_unlock(&m_mutex);
#endif
}

//===========================================
//
//      CThreadSemaphore
//
//===========================================

CThreadSemaphore::CThreadSemaphore(const char* name, int max, bool shared) : m_name(name), m_max(max), m_shared(shared)
{
#ifdef _WIN32
	// NOTE: On windows, a semaphore's value is decreased by one each time it's obtained, and increased by a value each time it's released
	if (shared)
		m_sem = CreateSemaphoreExA(NULL, max, max, name, 0, 0);
	else
		m_sem = CreateSemaphoreA(NULL, max, max, name);
#else
	if (shared)
		m_sem = sem_open(name, O_CREAT);
	else
	{
		sem_init(&__m_sem, 0, max);
		m_sem = &__m_sem;
	}
#endif
}

CThreadSemaphore::~CThreadSemaphore()
{
#ifdef _WIN32
	CloseHandle(m_sem);
#else
	if (m_shared)
		sem_close(m_sem);
	else
		sem_destroy(m_sem);
#endif
}

void CThreadSemaphore::Lock()
{
#ifdef _WIN32
	WaitForSingleObject(m_sem, INFINITE);
#else
	sem_wait(m_sem);
#endif
}

void CThreadSemaphore::Unlock()
{
#ifdef _WIN32
	ReleaseSemaphore(m_sem, 1, NULL);
#else
	sem_post(m_sem);
#endif
}

bool CThreadSemaphore::TryLock()
{
#ifdef _WIN32
	DWORD res = WaitForSingleObject(m_sem, 0);
	return res == WAIT_OBJECT_0;
#else
	return sem_trywait(m_sem) == 0;
#endif
}

//===========================================
//
//      CThreadSpinSemaphore
//
//===========================================

CThreadSpinSemaphore::CThreadSpinSemaphore(int max) {}

CThreadSpinSemaphore::~CThreadSpinSemaphore() {}

void CThreadSpinSemaphore::Lock() {}

void CThreadSpinSemaphore::Unlock() {}

bool CThreadSpinSemaphore::TryLock() { return false; }

int CThreadSpinSemaphore::GetUsers() const { return 0; }

//===========================================
//
//      CThreadRWMutex
//
//===========================================

CThreadRWMutex::CThreadRWMutex()
{
#ifdef _WIN32

#else
	pthread_rwlockattr_init(&m_attr);
	pthread_rwlock_init(&m_mutex, &m_attr);
#endif
}

CThreadRWMutex::~CThreadRWMutex()
{
#ifdef _WIN32

#else
	pthread_rwlockattr_destroy(&m_attr);
	pthread_rwlock_destroy(&m_mutex);
#endif
}

void CThreadRWMutex::RLock()
{
#ifdef _WIN32
	m_r_mutex.Lock();
#else
	pthread_rwlock_rdlock(&m_mutex);
#endif
}

bool CThreadRWMutex::RTryLock()
{
#ifdef _WIN32
	return m_r_mutex.TryLock();
#else
	return pthread_rwlock_tryrdlock(&m_mutex) == 0;
#endif
}

void CThreadRWMutex::RUnlock()
{
#ifdef _WIN32
	m_r_mutex.Unlock();
#else
	pthread_rwlock_unlock(&m_mutex);
#endif
}

void CThreadRWMutex::WLock()
{
#ifdef _WIN32
	m_w_mutex.Lock();
#else
	pthread_rwlock_wrlock(&m_mutex);
#endif
}

bool CThreadRWMutex::WTryLock()
{
#ifdef _WIN32
	return m_w_mutex.TryLock();
#else
	return pthread_rwlock_trywrlock(&m_mutex) == 0;
#endif
}

void CThreadRWMutex::WUnlock()
{
#ifdef _WIN32
	m_w_mutex.Unlock();
#else
	pthread_rwlock_unlock(&m_mutex);
#endif
}

//===========================================
//
//      CSharedMutex
//
//===========================================
CSharedMutex::CSharedMutex(const char* name) : m_sem(name, 1, true) {}

CSharedMutex::~CSharedMutex() {}

void CSharedMutex::Lock() { m_sem.Lock(); }

void CSharedMutex::Unlock() { m_sem.Unlock(); }

bool CSharedMutex::TryLock() { return m_sem.TryLock(); }

//===========================================
//
//      CConditionVariable
//
//===========================================

CThreadConditionVariable::CThreadConditionVariable()
{
#ifdef _WIN32
	InitializeCriticalSection(&m_critSection);
	InitializeConditionVariable(&m_condVar);
#else
	pthread_condattr_init(&m_attr);
	pthread_cond_init(&m_cond, &m_attr);
	pthread_mutexattr_init(&m_mutAttr);
	pthread_mutex_init(&m_mut, &m_mutAttr);
#endif
}

CThreadConditionVariable::~CThreadConditionVariable()
{
#ifdef _WIN32
	DeleteCriticalSection(&m_critSection);
	DeleteConditionVariable(&m_condVar);
#else
	pthread_cond_destroy(&m_cond);
	pthread_condattr_destroy(&m_attr);
	pthread_mutex_destroy(&m_mut);
	pthread_mutexattr_destroy(&m_mutAttr);
#endif
}

void CThreadConditionVariable::Wait(int max_time_ms)
{
#ifdef _WIN32
	EnterCriticalSection(&m_critSection);
	DWORD time = max_time_ms;
	if(max_time_ms == -1)
		time = INFINITE;
	SleepConditionVariableCS(&m_condVar, &m_critSection, time);
	LeaveCriticalSection(&m_critSection);
#else
	timespec timeSpec;
	timeSpec.tv_sec = (max_time_ms / 1000);
	timeSpec.tv_nsec = (max_time_ms * 1000000) - (timeSpec.tv_nsec * 1000000000);
	pthread_cond_timedwait(&m_cond, &m_mut, &timeSpec);
#endif
}

void CThreadConditionVariable::SignalOne()
{
#ifdef _WIN32
	WakeConditionVariable(&m_convVar);
#else
	pthread_cond_signal(&m_cond);
#endif
}

void CThreadConditionVariable::SignalAll()
{
#ifdef _WIN32
	WakeAllConditionVariable(&m_condVar);
#else
	pthread_cond_broadcast(&m_cond);
#endif
}
