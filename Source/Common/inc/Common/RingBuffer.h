#pragma once
#include <atomic>
#include <vector>

namespace YAPT
{
    template <typename T>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue(size_t size);
        ~ThreadSafeQueue();

        void push(const T& val);
        T pop();

        bool tryPush(const T& val);

    private:
        std::vector<T> m_buffer;
        std::atomic<size_t> m_head;
        std::atomic<size_t> m_tail;
    };

    template <typename T>
    ThreadSafeQueue<T>::ThreadSafeQueue(size_t size)
    {
        m_buffer.resize(size);
        m_head = m_tail = 0;
    }

    template <typename T>
    ThreadSafeQueue<T>::~ThreadSafeQueue()
    {

    }

}
