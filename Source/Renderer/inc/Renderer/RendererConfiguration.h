#pragma once

#include <Common/RCObjectPtr.h>
#include <Math/Math.h>
#include <Renderer/Texture.h>
#include <Renderer/Buffer.h>
#include <Renderer/CommonDefines.h>

namespace YAPT
{
	enum class RendererVariableType
	{
		FLOAT,
		UINT,
		INT,

		VEC2,
		VEC3,
		VEC4,
		IVEC2,
		IVEC3,
		IVEC4,

		TEXTURE,
		BUFFER
	};
	class RendererVariable
	{
	public:
		virtual void set(const float& val) = 0;
		virtual void set(const uint32_t& val) = 0;
		virtual void set(const int32_t& val) = 0;

		virtual void set(const vec2p& val) = 0;
		virtual void set(const vec3p& val) = 0;
		virtual void set(const vec4p& val) = 0;

		virtual void set(const ivec2p& val) = 0;
		virtual void set(const ivec3p& val) = 0;
		virtual void set(const ivec4p& val) = 0;

		virtual void set(const RCPtr<Texture>& val) = 0;
		virtual void set(const RCPtr<Buffer>& val) = 0;


		virtual bool get(float& val) = 0;
		virtual bool get(uint32_t& val) = 0;
		virtual bool get(int32_t& val) = 0;
					 
		virtual bool get(vec2p& val) = 0;
		virtual bool get(vec3p& val) = 0;
		virtual bool get(vec4p& val) = 0;

		virtual bool get(ivec2p& val) = 0;
		virtual bool get(ivec3p& val) = 0;
		virtual bool get(ivec4p& val) = 0;
					 
		virtual bool get(RCPtr<Texture>& val) = 0;
		virtual bool get(RCPtr<Buffer>& val) = 0;

		virtual bool getLimits(float& min, float& max) = 0;
		virtual bool getLimits(uint32_t& min, uint32_t& max) = 0;
		virtual bool getLimits(int32_t& min, int32_t& max) = 0;

		virtual bool getLimits(vec2p& min, vec2p& max) = 0;
		virtual bool getLimits(vec3p& min, vec3p& max) = 0;
		virtual bool getLimits(vec4p& min, vec4p& max) = 0;
		
		virtual bool getLimits(ivec2p& min, ivec2p& max) = 0;
		virtual bool getLimits(ivec3p& min, ivec3p& max) = 0;
		virtual bool getLimits(ivec4p& min, ivec4p& max) = 0;

		virtual RendererVariableType getType() const = 0;

		virtual ~RendererVariable(){}

	};

	class RendererConfiguration
	{
	public:
		virtual RendererVariable* getRendererVariable(const char* name) = 0;
		virtual size_t getNumberOfRendererVariables() const = 0;
		virtual void queryRendererVariableNamesList(const char** listOut, size_t maxNumberOfEntries) = 0;

		virtual ~RendererConfiguration() {}
		
	};
}