#pragma once
#include <PyBindingsCommon.h>
#include <Renderer/Texture.h>
#include <Common/RCObjectPtr.h>

namespace YAPT
{

	class PyTexture
	{
	public:
		DECLARE_BINDING_CLASS(PyTexture);
		PyTexture(Texture* buffer);
		~PyTexture();

		void upload(uint32_t mipOffset, uint32_t arrayOffset, uint32_t mipCount, uint32_t arrayCount, size_t rowPitchInBytes, uintptr_t data);
		const char* getName() const;

		const RCObjectPtr<Texture>& getTexture() const { return m_texture; }
	private:
		RCObjectPtr<Texture> m_texture;


	};
}
