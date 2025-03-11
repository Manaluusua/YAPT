#pragma once
#include <PyBindingsCommon.h>
#include<Renderer/RendererConfiguration.h>

namespace YAPT
{
    class PyTexture;
    class PyBuffer;
	class PyRendererVar
	{
	public:
		DECLARE_BINDING_CLASS(PyRendererVar);

		PyRendererVar(RendererVariable* var);
		~PyRendererVar();

        void setFromFloatArray(std::vector<float> floats);
        void setFromIntArray(std::vector<int32_t> floats);

        void setTexture(const PyTexture& val);
        void setBuffer(const PyBuffer& val);

        std::vector<float> getAsFloatArray();
        std::vector<int32_t> getAsIntArray();

        PyTexture* getTexture();
        PyBuffer* getBuffer();

        std::array<std::vector<float>, 2> getLimitsFloat();
        std::array<std::vector<int>, 2> getLimitsInt();

        RendererVariableType getType() const;
		
	private:
		RendererVariable* m_rendererVar;


	};
}