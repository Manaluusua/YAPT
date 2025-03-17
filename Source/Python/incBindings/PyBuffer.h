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

		void upload(size_t offsetInBytes, size_t sizeInBytes, uintptr_t data, size_t dataPtrOffset);
		const char* getName() const;
		
		const RCObjectPtr<Buffer>& getBuffer() const { return m_buffer; }

	private:
		RCObjectPtr<Buffer> m_buffer;

	};
}
