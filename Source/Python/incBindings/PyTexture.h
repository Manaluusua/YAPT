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

	private:
		RCObjectPtr<Texture> m_texture;


	};
}
