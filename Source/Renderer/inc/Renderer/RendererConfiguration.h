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

		virtual void set(const glm::vec2& val) = 0;
		virtual void set(const glm::vec3& val) = 0;
		virtual void set(const glm::vec4& val) = 0;

		virtual void set(const glm::ivec2& val) = 0;
		virtual void set(const glm::ivec3& val) = 0;
		virtual void set(const glm::ivec4& val) = 0;

		virtual void set(const RCPtr<Texture>& val) = 0;
		virtual void set(const RCPtr<Buffer>& val) = 0;


		virtual bool get(float& val) = 0;
		virtual bool get(uint32_t& val) = 0;
		virtual bool get(int32_t& val) = 0;
					 
		virtual bool get(glm::vec2& val) = 0;
		virtual bool get(glm::vec3& val) = 0;
		virtual bool get(glm::vec4& val) = 0;

		virtual bool get(glm::ivec2& val) = 0;
		virtual bool get(glm::ivec3& val) = 0;
		virtual bool get(glm::ivec4& val) = 0;
					 
		virtual bool get(RCPtr<Texture>& val) = 0;
		virtual bool get(RCPtr<Buffer>& val) = 0;

		virtual bool getLimits(float& min, float& max) = 0;
		virtual bool getLimits(uint32_t& min, uint32_t& max) = 0;
		virtual bool getLimits(int32_t& min, int32_t& max) = 0;

		virtual bool getLimits(glm::vec2& min, glm::vec2& max) = 0;
		virtual bool getLimits(glm::vec3& min, glm::vec3& max) = 0;
		virtual bool getLimits(glm::vec4& min, glm::vec4& max) = 0;
		
		virtual bool getLimits(glm::ivec2& min, glm::ivec2& max) = 0;
		virtual bool getLimits(glm::ivec3& min, glm::ivec3& max) = 0;
		virtual bool getLimits(glm::ivec4& min, glm::ivec4& max) = 0;

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