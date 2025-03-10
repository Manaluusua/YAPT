#ifndef YAPT_BUFFER_H
#define YAPT_BUFFER_H

#include <Gfx/GfxBasicTypes.h>
#include <Common/RCObject.h>

namespace YAPT
{
	
	class Buffer : public RCObject
	{
	public:
		virtual void upload(size_t offsetInBytes, size_t sizeInBytes, const void* data) = 0;
		virtual const BufferDesc& getDesc() const = 0;
		virtual void setName(const char* name) = 0;
		virtual const char* getName() const = 0;
	protected:
		virtual ~Buffer() {}

		
	};
}

#endif