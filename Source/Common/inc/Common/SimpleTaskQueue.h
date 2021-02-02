#pragma once
#include <atomic>


namespace YAPT
{
    template <typename T, size_t SIZE>
    class SimpleTaskQueue
    {
    public:
        SimpleTaskQueue();
        ~SimpleTaskQueue();

        
        T* tryGetNextForWriting();
        T* getNextForWriting();

        bool commit();


        T* peekNextForReading();
        bool popNext();

    private:

        size_t nextIndex(size_t ind) const
        {
            return (ind + 1) % SIZE;
        }
        std::atomic<size_t> m_head;
        std::atomic<size_t> m_tail;

        T m_buffer[SIZE];
       
    };

    template <typename T, size_t SIZE>
    SimpleTaskQueue<T, SIZE>::SimpleTaskQueue()
    {
        m_head = 0;
        m_tail = 0;
    }

    template <typename T, size_t SIZE>
    SimpleTaskQueue<T, SIZE>::~SimpleTaskQueue()
    {

    }


    template <typename T, size_t SIZE>
    T* SimpleTaskQueue<T, SIZE>::tryGetNextForWriting()
    {
        size_t currentHead = m_head.load();
        size_t nextHead = nextIndex(currentHead);
        if (nextHead != m_tail.load())
        {
            return &m_buffer[currentHead];
        }

        return nullptr;
    }
    
    template <typename T, size_t SIZE>
    T* SimpleTaskQueue<T, SIZE>::getNextForWriting()
    {
        T* t;
        do
        {
            t = tryGetNextForWriting();
        } while (t == nullptr);

        return t;

    }
    template <typename T, size_t SIZE>
    bool SimpleTaskQueue<T, SIZE>::commit()
    {
        size_t currentHead = m_head.load();
        size_t nextHead = nextIndex(currentHead);
        if (nextHead != m_tail.load())
        {
            m_head.store(nextHead);
            return true;
        }
        return false;
        
    }

    template <typename T, size_t SIZE>
    T* SimpleTaskQueue<T, SIZE>::peekNextForReading()
    {
        size_t currentTail = m_tail;
        if (currentTail != m_head.load())
        {
            return &m_buffer[currentTail];

        }
        return nullptr;
    }

    template <typename T, size_t SIZE>
    bool SimpleTaskQueue<T, SIZE>::popNext()
    {
        size_t currentTail = m_tail;
        if (currentTail != m_head.load())
        {
            m_tail.store(nextIndex(currentTail));
            return true;
        }
        return false;
       
    }

}
