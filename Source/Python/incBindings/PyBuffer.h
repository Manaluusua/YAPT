#pragma once
#include <PyBindingsCommon.h>
#include <Renderer/Buffer.h>
#include <Common/RCObjectPtr.h>

namespace YAPT
{
	class PyBuffer
	{
	public:
		DECLARE_BINDING_CLASS(PyBuffer);
		PyBuffer(Buffer* buffer);
		~PyBuffer();

		void upload(size_t offsetInBytes, size_t sizeInBytes, const void* data);
		void memoryAllocated();
		
	private:
		RCObjectPtr<Buffer> m_buffer;
		bool m_memoryAllocated;

	};
}
