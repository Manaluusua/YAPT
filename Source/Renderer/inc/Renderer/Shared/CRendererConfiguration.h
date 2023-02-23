#pragma once

#include <Renderer/RendererConfiguration.h>
#include <Common/Logger.h>
#include <unordered_map>
#include <string>

#define INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(TYPE)\
		virtual void set(const TYPE& val) final		\
		{											\
			setIfTypeMatches(val);					\
		}											\
		virtual void setLimits(const TYPE& min, const TYPE& max) final \
		{											\
			setLimitsIfTypeMatches(min, max);		\
		}											
#define INTERNAL_DEFINE_RENDERVARTYPED_SETTERS(TYPE)\
		virtual void set(const TYPE& val) final		\
		{											\
			setIfTypeMatches(val);					\
		}											


#define INTERNAL_DEFINE_RENDERVARTYPED_GETTERS(TYPE)			\
		virtual bool get(TYPE& val) final						\
		{														\
			return getIfTypeMatches(val);						\
		}														\
		virtual bool getInternal(TYPE& val) final				\
		{														\
			return getIfTypeMatchesInternal(val);				\
		}														


#define INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(TYPE)\
		virtual bool get(TYPE& val) final						\
		{														\
			return getIfTypeMatches(val);						\
		}														\
		virtual bool getInternal(TYPE& val) final				\
		{														\
			return getIfTypeMatchesInternal(val);				\
		}														\
		virtual bool getLimits(TYPE& min, TYPE& max)final\
		{														\
			return getLimitsIfTypeMatches(min, max);			\
		}


namespace YAPT
{
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class RendererVariableInternal : public RendererVariable
	{
	public:
		virtual bool getInternal(float& val) = 0;
		virtual bool getInternal(uint32_t& val) = 0;
		virtual bool getInternal(int32_t& val) = 0;
		virtual bool getInternal(vec2p& val) = 0;
		virtual bool getInternal(vec3p& val) = 0;
		virtual bool getInternal(vec4p& val) = 0;
		virtual bool getInternal(ivec2p& val) = 0;
		virtual bool getInternal(ivec3p& val) = 0;
		virtual bool getInternal(ivec4p& val) = 0;
		virtual bool getInternal(RCPtr<Texture>& val) = 0;
		virtual bool getInternal(RCPtr<Buffer>& val) = 0;

		virtual void setLimits(const float& min, const float& max) = 0;
		virtual void setLimits(const uint32_t& min, const uint32_t& max) = 0;
		virtual void setLimits(const int32_t& min, const int32_t& max) = 0;
				
		virtual void setLimits(const vec2p& min, const vec2p& max) = 0;
		virtual void setLimits(const vec3p& min, const vec3p& max) = 0;
		virtual void setLimits(const vec4p& min, const vec4p& max) = 0;
				
		virtual void setLimits(const ivec2p& min, const ivec2p& max) = 0;
		virtual void setLimits(const ivec3p& min, const ivec3p& max) = 0;
		virtual void setLimits(const ivec4p& min, const ivec4p& max) = 0;


	};



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef void(*CommitRendererVarChange)(RendererVariable* rendererVar);
	class CRendererConfiguration : public RendererConfiguration
	{
	public:
		struct CommitCallback
		{
			RendererVariable* rendererVar;
			CommitRendererVarChange callback;
		};

		CRendererConfiguration();
		~CRendererConfiguration();
		RendererVariable* registerVariable(const char* name, RendererVariableType type);

		virtual RendererVariable* getRendererVariable(const char* name) final;
		virtual size_t getNumberOfRendererVariables() const final;
		virtual void queryRendererVariableNamesList(const char** listOut, size_t maxNumberOfEntries) final;


		RendererVariableInternal* getCRendererVariableInternal(const char* name)
		{
			return static_cast<RendererVariableInternal*>(getRendererVariable(name));
		}

		template<typename T>
		T getRendererVarValueInternal(const char* name)
		{
			T t;
			bool success = getCRendererVariableInternal(name)->getInternal(t);
			assert(success);
			return t;
		}

		void clear();

		void commitChanges();

		void addDirtyVariable(CommitCallback& cb);
	private:
		std::vector<CommitCallback> m_dirtyVariables;


		std::unordered_map<std::string, RendererVariable*> m_vars;
	};




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T, RendererVariableType TYPE>
	class CRendererVariable : public RendererVariableInternal
	{
	public:

		CRendererVariable(CRendererConfiguration* mngr)
			:m_mngr(mngr),
			m_dirty(false),
			m_hasLimits(false)
		{

		}


		virtual ~CRendererVariable()
		{

		}
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(float)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(uint32_t)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(int32_t)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(vec2p)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(vec3p)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(vec4p)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(ivec2p)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(ivec3p)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS_WITH_LIMITS(ivec4p)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS(RCPtr<Texture>)
		INTERNAL_DEFINE_RENDERVARTYPED_SETTERS(RCPtr<Buffer>)

		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(float)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(uint32_t)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(int32_t)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(vec2p)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(vec3p)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(vec4p)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(ivec2p)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(ivec3p)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS_WITH_LIMITS(ivec4p)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS(RCPtr<Texture>)
		INTERNAL_DEFINE_RENDERVARTYPED_GETTERS(RCPtr<Buffer>)

		virtual RendererVariableType getType() const final
		{
			return TYPE;
		}

		static void commit(RendererVariable* rendererVar)
		{
			CRendererVariable<T, TYPE>* rv = static_cast<CRendererVariable<T, TYPE>*>(rendererVar);

			rv->m_val = rv->m_newVal;
			rv->m_dirty = false;
		}

	private:
		template<typename U>
		struct BasetypeHelper
		{

		};

		enum class BasetypeScalar {};
		enum class BasetypeVector {};
		enum class BasetypeResource {};

#define CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(TYPE, ARCHETYPE)  \
		template<>													\
		struct BasetypeHelper<TYPE>								\
		{															\
			typedef ARCHETYPE Basetype;								\
		};

		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(float, BasetypeScalar)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(uint32_t, BasetypeScalar)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(int32_t, BasetypeScalar)

		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(vec2p, BasetypeVector)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(vec3p, BasetypeVector)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(vec4p, BasetypeVector)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(ivec2p, BasetypeVector)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(ivec3p, BasetypeVector)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(ivec4p, BasetypeVector)

		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(RCPtr<Texture>, BasetypeResource)
		CRENDERVARIABLE_INTERNAL_DEFINE_ARCHETYPE(RCPtr<Buffer>, BasetypeResource)
		


		template<typename U>
		void setIfTypeMatches(const U& val)
		{
			YAPT_LOG_ERROR("Tried to set renderer variable but the type was incorrect");
		}

		template<>
		void setIfTypeMatches<T>(const T& val)
		{
			m_newVal = val;
			if (m_hasLimits)
			{
				clampValueToLimits<BasetypeHelper<T>::Basetype>();
			}

			notifyChanged();
		}

		template<typename U>
		void setLimitsIfTypeMatches(const U& minimum, const U& maximum)
		{
			YAPT_LOG_ERROR("Tried to set renderer variable limits but the type was incorrect");
		}

		template<>
		void setLimitsIfTypeMatches<T>(const T& minimum, const T& maximum)
		{
			m_min = minimum;
			m_max = maximum;
			m_hasLimits = true;
		}

		template<typename U>
		bool getIfTypeMatches(U& val)
		{
			return false;
		}
		template<>
		bool getIfTypeMatches<T>(T& val)
		{
			val = m_newVal;
			return true;
		}

		template<typename U>
		bool getLimitsIfTypeMatches(U& min, U& max)
		{
			return false;
		}
		template<>
		bool getLimitsIfTypeMatches<T>(T& min, T& max)
		{
			min = m_min;
			max = m_max;
			return m_hasLimits;

		}

		template<typename U>
		bool getIfTypeMatchesInternal(U& val)
		{

			return false;
		}
		template<>
		bool getIfTypeMatchesInternal<T>(T& val)
		{

			val = m_val;
			return true;

		}

		template<typename BASETYPE>
		void clampValueToLimits()
		{

		}

		template<>
		void clampValueToLimits<BasetypeScalar>()
		{
			if (m_newVal < m_min)
			{
				m_newVal = m_min;
			}
			if (m_newVal > m_max)
			{
				m_newVal = m_max;
			}
		}

		template<>
		void clampValueToLimits<BasetypeVector>()
		{
			glm::clamp(m_newVal, m_min, m_max);
		}

		template<>
		void clampValueToLimits<BasetypeResource>()
		{
			//nothing to do
		}
	

		void notifyChanged()
		{
			if (!m_dirty)
			{
				CRendererConfiguration::CommitCallback cb;
				cb.rendererVar = this;
				cb.callback = CRendererVariable<T, TYPE>::commit;
				m_mngr->addDirtyVariable(cb);
			}

			m_dirty = true;
		}
		CRendererConfiguration* m_mngr;
		T m_newVal;
		T m_val;
		T m_min;
		T m_max;
		bool m_hasLimits;
		bool m_dirty;

	};


}
