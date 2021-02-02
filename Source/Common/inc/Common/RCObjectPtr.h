#pragma once
namespace YAPT
{

    template< class T >
    class RCObjectPtr
    {
    public:
        RCObjectPtr( );

        RCObjectPtr( T* raw  );

        RCObjectPtr( const RCObjectPtr<T>& other );

        const RCObjectPtr<T>& operator=( const RCObjectPtr<T>& other );

        //implicit
        operator T*();

        //explicit
        T* get() const;

        T& operator*() const;

        T* operator->() const;

		T*const* operator&() const;
		T** operator&();

		//to make it easier to pass as void**
		void** asVoid()
		{
			return reinterpret_cast<void**>(&m_rawPtr);
		}


        ~RCObjectPtr();

    private:
        void initialize();

        T* m_rawPtr;
    };


    template< class T >
    inline RCObjectPtr<T>::RCObjectPtr()
        :m_rawPtr( 0 )
    {
        initialize();
    }

    template< class T >
    inline RCObjectPtr<T>::RCObjectPtr( T* raw  )
        :m_rawPtr( raw )
    {
        initialize();
    }

    template< class T >
    inline RCObjectPtr<T>::RCObjectPtr( const RCObjectPtr<T>& other )
        :m_rawPtr( other.m_rawPtr )
    {
        initialize();
    }

    template< class T >
    inline RCObjectPtr<T>::~RCObjectPtr()
    {
        if(m_rawPtr)
        {
            m_rawPtr->Release();
        }
    }

    template< class T >
    inline const RCObjectPtr<T>& RCObjectPtr<T>::operator=( const RCObjectPtr<T>& other )
    {
        if( m_rawPtr != other.m_rawPtr )
        {
            if( m_rawPtr )
            {
                m_rawPtr->Release();
            }
            m_rawPtr = other.m_rawPtr;
            initialize();
        }

        return *this;
    }

    template< class T >
    inline RCObjectPtr<T>::operator T*()
    {
        return m_rawPtr;
    }

    template< class T >
    inline T* RCObjectPtr<T>::get() const
    {
        return m_rawPtr;
    }

    template< class T >
    inline T& RCObjectPtr<T>::operator*() const
    {
        return *m_rawPtr;
    }

	template< class T >
	inline T*const* RCObjectPtr<T>::operator&() const
	{
		return &m_rawPtr;
	}

	template< class T >
	inline T** RCObjectPtr<T>::operator&()
	{
		return &m_rawPtr;
	}

    template< class T >
    inline T* RCObjectPtr<T>::operator->() const
    {
        return m_rawPtr;
    }
    
    template< class T >
    inline void RCObjectPtr<T>::initialize()
    {
        if( !m_rawPtr )
            return;

        m_rawPtr->AddRef();
    }


    template<typename T>
    using RCPtr = RCObjectPtr<T>;

}
