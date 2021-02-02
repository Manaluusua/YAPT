#include "Common/RCObject.h"

namespace YAPT
{
    RCObject::RCObject(void)
        :m_referenceCount( 1 )
    {
    }


    RCObject::~RCObject(void)
    {
    }
	void RCObject::allReferencesReleased()
	{
		delete this;
	}
    
}