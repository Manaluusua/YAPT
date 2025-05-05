#pragma once

#include <Renderer/RendererConfiguration.h>
#include <Common/Logger.h>
#include <unordered_map>
#include <string>
#include <type_traits>
namespace YAPT
{
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RenderVariableFloatInternal : public RenderVariableFloat
{
public:
	virtual const float* getInternal() const = 0;

protected:
	virtual ~RenderVariableFloatInternal() {}
};


class RenderVariableIntInternal : public RenderVariableInt
{
public:
	virtual const int32_t* getInternal() const = 0;

protected:
	virtual ~RenderVariableIntInternal() {}
};


class RenderVariableOptionsInternal : public RenderVariableOptions
{
public:
	virtual uint32_t getInternal() const = 0;


protected:
	virtual ~RenderVariableOptionsInternal() {}
};

class RenderVariableTextureInternal : public RenderVariableTexture
{
public:
	virtual Texture* getInternal() const = 0;
protected:
	virtual ~RenderVariableTextureInternal() {}
};

class RenderVariableBufferInternal : public RenderVariableBuffer
{
public:
	virtual Buffer* getInternal() const = 0;
protected:
	virtual ~RenderVariableBufferInternal() {}
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef void(*CommitRendererVarChange)(RendererVariable* rendererVar);
	class CRendererConfiguration : public RendererConfiguration
	{
	private:

		template<typename VecType>
		static VecType getNumericValueInternal(RenderVariableFloatInternal* v)
		{
			VecType val{};
			if (VecType::length() == v->getComponentCount())
			{
				const float* src = v->getInternal();
				VecType::value_type* dst = glm::value_ptr(val);
				for (size_t i = 0; i < VecType::length(); ++i)
				{
					dst[i] = (VecType::value_type)src[i];
				}
			}
			else
			{
				YAPT_LOG_FATAL_ERROR("Failed to get internal numeric rendervar value, incorrect component counts: %d, %d", VecType::length(), v->getComponentCount());
			}
			return val;
		}

		template<>
		static float getNumericValueInternal<float>(RenderVariableFloatInternal* v)
		{
			float val = 0;
			if (v->getComponentCount() == 1)
			{
				const float* src = v->getInternal();
				val = *src;
			}
			else
			{
				YAPT_LOG_FATAL_ERROR("Failed to get internal numeric scalar int value, incorrect component counts: %d", v->getComponentCount());
			}
			return val;
		}

		template<typename VecType>
		static VecType getNumericValueInternal(RenderVariableIntInternal* v)
		{
			VecType val{};
			if (VecType::length() == v->getComponentCount())
			{
				const int32_t* src = v->getInternal();
				VecType::value_type* dst = glm::value_ptr(val);
				for (size_t i = 0; i < VecType::length(); ++i)
				{
					dst[i] = (VecType::value_type)src[i];
				}
			}
			else
			{
				YAPT_LOG_FATAL_ERROR("Failed to get internal numeric int value, incorrect component counts: %d, %d", VecType::length(), v->getComponentCount());
			}
			return val;
		}

		template<>
		static int32_t getNumericValueInternal<int32_t>(RenderVariableIntInternal* v)
		{
			int32_t val = 0;
			if (v->getComponentCount() == 1)
			{
				const int32_t* src = v->getInternal();
				val = *src;
			}
			else
			{
				YAPT_LOG_FATAL_ERROR("Failed to get internal numeric scalar int value, incorrect component counts: %d", v->getComponentCount());
			}
			return val;
		}

		template<typename T, RendererVariableType Type>
		struct RenderVarValueGetter
		{
			static T get(RendererVariable* r)
			{
				YAPT_LOG_FATAL_ERROR("Trying to get wrong type from RenderVariable, default is returned!");
				return T();
			}
		};

		template<typename T, typename ValidType = void>
		struct NumericRenderValueGetter
		{
			static T get(RendererVariable* r)
			{
				YAPT_LOG_FATAL_ERROR("Trying to get wrong type from RenderVariable, default is returned!");
				return T();
			}
		};


		template<typename T>
		struct NumericRenderValueGetter<T, std::void_t<typename T::value_type>>
		{
			typedef typename T::value_type ValueType;

			template<typename U>
			static T getVec(RendererVariable* r)
			{
				YAPT_LOG_FATAL_ERROR("Tying to get unsupported vector type from RenderVariable, default is returned!");
				return T();
			}

			template<>
			static T getVec<int32_t>(RendererVariable* r)
			{
				return getNumericValueInternal<T>(static_cast<RenderVariableIntInternal*>(r));
			}

			template<>
			static T getVec<float>(RendererVariable* r)
			{
				return getNumericValueInternal<T>(static_cast<RenderVariableFloatInternal*>(r));
			}


			static T get(RendererVariable* r)
			{
				return getVec<typename ValueType>(r);
			}

			
		};

		template<>
		struct NumericRenderValueGetter<float>
		{
			static float get(RendererVariable* r)
			{
				return getNumericValueInternal<float>(static_cast<RenderVariableFloatInternal*>(r));
			}
		};

		template<>
		struct NumericRenderValueGetter<int32_t>
		{
			static int32_t get(RendererVariable* r)
			{
				return getNumericValueInternal<int32_t>(static_cast<RenderVariableIntInternal*>(r));
			}
		};


		template<typename T>
		struct RenderVarValueGetter<T, RendererVariableType::FLOAT>
		{
			static T get(RendererVariable* r)
			{
				return NumericRenderValueGetter<typename T>::get(r);
			}
		};

		template<typename T>
		struct RenderVarValueGetter<T, RendererVariableType::INT>
		{
			static T get(RendererVariable* r)
			{
				return NumericRenderValueGetter<typename T>::get(r);
			}
		};

		template<>
		struct RenderVarValueGetter<int32_t, RendererVariableType::OPTIONS>
		{
			static int32_t get(RendererVariable* r)
			{
				return static_cast<RenderVariableOptionsInternal*>(r)->getInternal();
			}
		};

		template<>
		struct RenderVarValueGetter<Texture*, RendererVariableType::TEXTURE>
		{
			static Texture* get(RendererVariable* r)
			{
				return static_cast<RenderVariableTextureInternal*>(r)->getInternal();
			}
		};

		template<>
		struct RenderVarValueGetter<Buffer*, RendererVariableType::BUFFER>
		{
			static Buffer* get(RendererVariable* r)
			{
				return static_cast<RenderVariableBufferInternal*>(r)->getInternal();
			}
		};


	public:
		struct CommitCallback
		{
			RendererVariable* rendererVar;
			CommitRendererVarChange callback;
		};

		CRendererConfiguration();
		~CRendererConfiguration();
		void registerVariable(const char* name, RendererVariable* var);

		virtual RendererVariable* getRendererVariable(const char* name) final;
		virtual size_t getNumberOfRendererVariables() const final;
		virtual void queryRendererVariableNamesList(const char** listOut, size_t maxNumberOfEntries) final;

		

		template<typename T>
		T getRendererVarValueInternal(const char* name)
		{
			RendererVariable* v = getRendererVariable(name);
			if (v != nullptr)
			{
				RendererVariableType type = v->getType();

				switch (type)
				{
				case RendererVariableType::FLOAT:
					return RenderVarValueGetter<typename T, RendererVariableType::FLOAT>::get(v);
					break;
				case RendererVariableType::INT:
					return RenderVarValueGetter<typename T, RendererVariableType::INT>::get(v);
					break;
				case RendererVariableType::OPTIONS:
					return RenderVarValueGetter<typename T, RendererVariableType::OPTIONS>::get(v);
				case RendererVariableType::TEXTURE:
					return RenderVarValueGetter<typename T, RendererVariableType::TEXTURE>::get(v);
				case RendererVariableType::BUFFER:
					return RenderVarValueGetter<typename T, RendererVariableType::BUFFER>::get(v);
				default:
					break;
				}
			}

			return T();
		}

		void clear();

		void commitChanges();

		void addDirtyVariable(CommitCallback& cb);

	private:



		std::vector<CommitCallback> m_dirtyVariables;
		std::unordered_map<std::string, RendererVariable*> m_vars;
	};




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename NumericType, class BaseType, RendererVariableType Type, size_t ComponentCount>
	class CRendererVariableNumeric : public BaseType
	{
	public:

		CRendererVariableNumeric(CRendererConfiguration& configMngr, const NumericType* defaultValue, const NumericType* min, const NumericType* max)
			:m_mngr(configMngr),
			m_hasLimits(false),
			m_dirty(false)
		{
			if (min != nullptr && max != nullptr)
			{
				for (int i = 0; i < ComponentCount; ++i)
				{
					m_min[i] = min[i];
					m_max[i] = max[i];
				}
				m_hasLimits = true;
			}

			if (defaultValue != nullptr)
			{
				for (int i = 0; i < ComponentCount; ++i)
				{
					m_newVal[i] = defaultValue[i];
				}
			}

			setDirty();
		}


		virtual ~CRendererVariableNumeric()
		{

		}

		virtual void set(const NumericType* values) final
		{
			for (size_t i = 0; i < ComponentCount; ++i)
			{
				m_newVal[i] = values[i];
			}
			if (m_hasLimits)
			{
				clampValueToLimits();
			}

			setDirty();
		}
		virtual const NumericType* get() const final
		{
			return m_newVal;
		}
		virtual bool getLimits(const NumericType*& min, const NumericType*& max) const final
		{
			min = m_min;
			max = m_max;
			return true;
		}
		virtual size_t getComponentCount() const final
		{
			return ComponentCount;
		}


		virtual RendererVariableType getType() const final
		{
			return Type;
		}

		const NumericType* getInternal() const
		{
			return m_val;
		}

	private:

		void clampValueToLimits()
		{
			for (size_t i = 0; i < ComponentCount; ++i)
			{
				m_newVal[i] = glm::clamp(m_newVal[i], m_min[i], m_max[i]);
			}
		}

		static void commit(RendererVariable* rvar)
		{
			CRendererVariableNumeric<NumericType, BaseType, Type, ComponentCount>* inst = static_cast<CRendererVariableNumeric<NumericType, BaseType, Type, ComponentCount>*>(rvar);
			for (size_t i = 0; i < ComponentCount; ++i)
			{
				inst->m_val[i] = inst->m_newVal[i];
			}
			inst->m_dirty = false;
		}

		void setDirty()
		{
			if (!m_dirty)
			{
				CRendererConfiguration::CommitCallback cb;
				cb.rendererVar = this;
				cb.callback = CRendererVariableNumeric<NumericType, BaseType, Type, ComponentCount>::commit;
				m_mngr.addDirtyVariable(cb);
			}

			m_dirty = true;
		}
		
		CRendererConfiguration& m_mngr;
		NumericType m_val[ComponentCount];
		NumericType m_newVal[ComponentCount];
		NumericType m_min[ComponentCount];
		NumericType m_max[ComponentCount];
		bool m_hasLimits;
		bool m_dirty;
	};

	template<class BaseType, RendererVariableType Type, typename VecType>
	inline RendererVariable* constructNumericRenderVar(CRendererConfiguration& configMngr, const VecType& defaultValue, const VecType& min, const VecType& max)
	{
		return new CRendererVariableNumeric <VecType::value_type, BaseType, Type, VecType::length()>(configMngr, glm::value_ptr(defaultValue), glm::value_ptr(min), glm::value_ptr(max));
	}

	template<>
	inline RendererVariable* constructNumericRenderVar<RenderVariableIntInternal, RendererVariableType::INT>(CRendererConfiguration& configMngr, const int32_t& defaultValue, const int32_t& min, const int32_t& max)
	{
		return new CRendererVariableNumeric<int32_t, RenderVariableIntInternal, RendererVariableType::INT, 1>(configMngr, &defaultValue, &min, &max);
	}

	template<>
	inline RendererVariable* constructNumericRenderVar<RenderVariableFloatInternal, RendererVariableType::FLOAT>(CRendererConfiguration& configMngr, const float& defaultValue, const float& min, const float& max)
	{
		return new CRendererVariableNumeric<float, RenderVariableFloatInternal, RendererVariableType::FLOAT, 1>(configMngr, &defaultValue, &min, &max);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename ResourceType, class BaseType, RendererVariableType Type>
	class CRendererVariableResource : public BaseType
	{
	public:

		CRendererVariableResource(CRendererConfiguration& configMngr)
			:m_mngr(configMngr),
			m_dirty(false)
		{

		}

		virtual ~CRendererVariableResource()
		{

		}

		virtual void set(const RCPtr<ResourceType> & val) final
		{
			m_newVal = val;
			setDirty();
		}
		virtual const RCPtr<ResourceType>& get() const final
		{
			return m_newVal;
		}

		virtual RendererVariableType getType() const final
		{
			return Type;
		}

		
		virtual ResourceType* getInternal() const final
		{
			return m_val.get();
		}

	private:

		static void commit(RendererVariable* rvar)
		{
			CRendererVariableResource<ResourceType, BaseType, Type>* inst = static_cast<CRendererVariableResource<ResourceType, BaseType, Type>*>(rvar);
			inst->m_val = inst->m_newVal;
			inst->m_dirty = false;
		}

		void setDirty()
		{
			if (!m_dirty)
			{
				CRendererConfiguration::CommitCallback cb;
				cb.rendererVar = this;
				cb.callback = CRendererVariableResource<ResourceType, BaseType, Type>::commit;
				m_mngr.addDirtyVariable(cb);
			}

			m_dirty = true;
		}

		CRendererConfiguration& m_mngr;
		RCPtr<ResourceType> m_newVal;
		RCPtr<ResourceType> m_val;
		bool m_dirty;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<size_t OptionsCount>
	class CRendererVariableOptions : public RenderVariableOptionsInternal
	{
	public:

		CRendererVariableOptions(CRendererConfiguration& configMngr, int32_t selectedOption, const char** options)
			:m_mngr(configMngr),
			m_dirty(false)
		{
			for (size_t i = 0; i < OptionsCount; ++i)
			{
				m_options[i] = options[i];
				m_optionsChar[i] = m_options[i].c_str();
			}
			set(selectedOption);
		}


		virtual ~CRendererVariableOptions()
		{

		}
		virtual void set(const uint32_t val) final
		{
			m_newSelection = val;
			setDirty();
		}
		virtual uint32_t get() const final
		{
			return m_newSelection;
		}

		virtual const char* const* getOptions() const final
		{
			return m_optionsChar;
		}
		virtual size_t getOptionsCount() const final
		{
			return OptionsCount;
		}


		virtual RendererVariableType getType() const final
		{
			return RendererVariableType::OPTIONS;
		}

		virtual uint32_t getInternal() const final
		{
			return m_selection;
		}


	private:

		static void commit(RendererVariable* rvar)
		{
			CRendererVariableOptions<OptionsCount>* inst = static_cast<CRendererVariableOptions<OptionsCount>*>(rvar);
			inst->m_selection = inst->m_newSelection;
			inst->m_dirty = false;
		}

		void setDirty()
		{
			if (!m_dirty)
			{
				CRendererConfiguration::CommitCallback cb;
				cb.rendererVar = this;
				cb.callback = CRendererVariableOptions<OptionsCount>::commit;
				m_mngr.addDirtyVariable(cb);
			}

			m_dirty = true;
		}

		CRendererConfiguration& m_mngr;
		uint32_t m_selection;
		uint32_t m_newSelection;
		std::string m_options[OptionsCount];
		const char* m_optionsChar[OptionsCount];
		bool m_dirty;
	};
}
