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
		INT,
		OPTIONS,
		TEXTURE,
		BUFFER
	};
	class RendererVariable
	{
	public:
		virtual RendererVariableType getType() const = 0;
		virtual ~RendererVariable(){}

		template<typename T>
		T* as()
		{
			if (getType() == T::getTypeStatic())
			{
				return static_cast<T*>(this);
			}

			return nullptr;
		}

	};

	class RenderVariableFloat : public RendererVariable
	{
	public:
		virtual void set(const float* values) = 0;
		virtual const float* get() const = 0;
		virtual bool getLimits(const float*& min, const float*& max) const = 0;
		virtual size_t getComponentCount() const = 0;

		static RendererVariableType getTypeStatic() { return RendererVariableType::FLOAT; }

	protected:
		virtual ~RenderVariableFloat() {}
	};

	class RenderVariableInt : public RendererVariable
	{
	public:
		virtual void set(const int32_t* values) = 0;
		virtual const int32_t* get() const = 0;
		virtual bool getLimits(const int32_t*& min, const int32_t*& max) const = 0;
		virtual size_t getComponentCount() const = 0;

		static RendererVariableType getTypeStatic() { return RendererVariableType::INT; }

	protected:
		virtual ~RenderVariableInt() {}
	};

	class RenderVariableOptions : public RendererVariable
	{
	public:
		virtual void set(const uint32_t val) = 0;
		virtual uint32_t get() const = 0;
		virtual const char* const* getOptions() const = 0;
		virtual size_t getOptionsCount() const = 0;

		static RendererVariableType getTypeStatic() { return RendererVariableType::OPTIONS; }

	protected:
		virtual ~RenderVariableOptions() {}
	};

	class RenderVariableTexture : public RendererVariable
	{
	public:
		virtual void set(const RCPtr<Texture>& val) = 0;
		virtual const RCPtr<Texture>& get() const = 0;

		static RendererVariableType getTypeStatic() { return RendererVariableType::TEXTURE; }

	protected:
		virtual ~RenderVariableTexture() {}
	};

	class RenderVariableBuffer : public RendererVariable
	{
	public:
		virtual void set(const RCPtr<Buffer>& val) = 0;
		virtual const RCPtr<Buffer>& get() const = 0;

		static RendererVariableType getTypeStatic() { return RendererVariableType::BUFFER; }

	protected:
		virtual ~RenderVariableBuffer() {}
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