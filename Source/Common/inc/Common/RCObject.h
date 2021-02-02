#pragma once
#include <atomic>

namespace YAPT
{

    class RCObject
    {
    public:
        RCObject();
        

        void AddRef();
        void Release();
	protected:
		virtual void allReferencesReleased();
		virtual ~RCObject();
		
    private:

        std::atomic<size_t> m_referenceCount;
    };

    inline void RCObject::AddRef()
    {
        m_referenceCount.fetch_add(1);
    }
    inline void RCObject::Release()
    {
        size_t old = m_referenceCount.fetch_sub(1);

        if( old == 1 )
        {
			allReferencesReleased();
        }
    }

}
