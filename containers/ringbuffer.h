/**
 *
 * Quick ring buffer class
 *
 */
#pragma once

#include <stdint.h>

#include "crtlib.h"
#include "threadtools.h"

template<class T, class MutexT = CFakeMutex, class I = unsigned int>
class RingBuffer final
{
private:
	T* m_data;
	typedef I IndexType;
	MutexT m_mutex;

	size_t m_size;
	std::atomic<I> m_readIndex;
	std::atomic<I> m_writeIndex;
public:

	RingBuffer()
	{
		m_size = 0;
		m_readIndex.store(0);
		m_writeIndex.store(0);
		m_data = nullptr;
	}

	explicit RingBuffer(size_t size) :
		m_size(size)
	{
		m_readIndex.store(0);
		m_writeIndex.store(0);
		m_data = Q_malloc(sizeof(T) * size);
	}

	~RingBuffer()
	{
		if(m_data) Q_free(m_data);
	}


	size_t size() const
	{
		return m_size;
	}

	/* SIZE is in number of elements */
	void resize(size_t newsize)
	{
		auto lck = m_mutex.RAIILock();
		if(newsize <= 0) return;

		T* tmp = (T*)Q_malloc(sizeof(T) * newsize);

		/* Copy in the new stuff, taking into account the new size */
		memcpy(tmp, m_data, (m_size > newsize ? newsize : m_size) * sizeof(T));

		m_size = newsize;
		m_data = tmp;
	}

	void lock()
	{
		m_mutex.Lock();
	}

	void unlock()
	{
		m_mutex.Unlock();
	}

	const T* data() const
	{
		return m_data;
	}

	IndexType read_index() const
	{
		return m_readIndex.load();
	}

	IndexType write_index() const
	{
		return m_writeIndex.load();
	}

	void set_read_index(IndexType i)
	{
		m_readIndex.store(i);
	}

	void set_writeIndex(IndexType i)
	{
		m_writeIndex.store(i);
	}

	T read()
	{
		auto lck = m_mutex.RAIILock();
		IndexType index = m_readIndex.load();

		const T* t = &m_data[index];
		if(index >= m_size) m_readIndex.store(0);
		else m_readIndex.store(index+1);
		return *t;
	}

	void write(const T& elem)
	{
		auto lck = m_mutex.RAIILock();
		IndexType index = m_writeIndex.load();

		m_data[index] = elem;
		if(index >= m_size) m_writeIndex.store(0);
		else m_writeIndex.store(index+1);
	}

	RingBuffer& operator=(const RingBuffer& buf)
	{
		buf.m_mutex.Lock();
		this->m_mutex.Lock();

		if(m_data) Q_free(m_data);
		m_data = Q_malloc(buf.m_size * sizeof(T));
		memcpy(m_data, buf.m_data, sizeof(T) * buf.m_size);
		m_writeIndex = buf.m_writeIndex;
		m_readIndex = buf.m_readIndex;
		m_size = buf.m_size;

		buf.m_mutex.Unlock();
		this->m_mutex.Unlock();

		return *this;
	}

	RingBuffer& operator=(RingBuffer&& buf) noexcept
	{
		buf.m_mutex.Lock();
		m_mutex.Lock();

		m_data = buf.m_data;
		m_readIndex = buf.m_readIndex;
		m_writeIndex = buf.m_writeIndex;
		m_size = buf.m_size;

		m_mutex.Unlock();
		buf.m_mutex.Unlock();
	}

	T operator[](IndexType index) const
	{
		auto lck = m_mutex.RAIILock();

		return m_data[index];
	}
};

template<class T, class I>
using RingBufferTS = RingBuffer<T, CThreadSpinlock, I>;
