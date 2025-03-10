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

        void set(const float& val);
        void set(const uint32_t& val);
        void set(const int32_t& val);

        void set(const vec2p& val);
        void set(const vec3p& val);
        void set(const vec4p& val);

        void set(const ivec2p& val);
        void set(const ivec3p& val);
        void set(const ivec4p& val);

        void set(const PyTexture& val);
        void set(const PyBuffer& val);

        bool get(float& val);
        bool get(uint32_t& val);
        bool get(int32_t& val);

        bool get(vec2p& val);
        bool get(vec3p& val);
        bool get(vec4p& val);

        bool get(ivec2p& val);
        bool get(ivec3p& val);
        bool get(ivec4p& val);

        bool get(RCPtr<Texture>& val);
        bool get(RCPtr<Buffer>& val);

        bool getLimits(float& min, float& max);
        bool getLimits(uint32_t& min, uint32_t& max);
        bool getLimits(int32_t& min, int32_t& max);

        bool getLimits(vec2p& min, vec2p& max);
        bool getLimits(vec3p& min, vec3p& max);
        bool getLimits(vec4p& min, vec4p& max);

        bool getLimits(ivec2p& min, ivec2p& max);
        bool getLimits(ivec3p& min, ivec3p& max);
        bool getLimits(ivec4p& min, ivec4p& max);

        RendererVariableType getType() const;
		
	private:
		RendererVariable* m_rendererVar;


	};
}