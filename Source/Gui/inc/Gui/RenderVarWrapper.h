#pragma once

#include <qlabel.h>
#include <qgridlayout.h>
#include <Gui/MutableNumberElement.h>
#include <Common/Logger.h>
#include <glm/gtc/type_ptr.hpp>

//add scalar type to pointer shenanignas to make the codepath unified 
namespace glm
{
	inline float* value_ptr(float& v)
	{
		return &(v);
	}

	inline uint32_t* value_ptr(uint32_t& v)
	{
		return &(v);
	}

	inline int32_t* value_ptr(int32_t& v)
	{
		return &(v);
	}
}


namespace YAPT
{
	class RenderVarWrapper : public QWidget
	{
		Q_OBJECT
	public:
		RenderVarWrapper(QWidget* parent)
			:QWidget(parent)
		{

		}
	};

	template<typename COMPONENT_TYPE, size_t NUM_ELEMENTS, typename VARIABLE_TYPE,  RendererVariableType RENDERVAR_TYPE>
	class ConcreteRenderVarWrapper : public RenderVarWrapper, public MutableNumericElementListener
	{
	public:
		ConcreteRenderVarWrapper(QWidget* parent, RendererVariable* renderVar)
			:RenderVarWrapper(parent)
		{
			m_layout = new QHBoxLayout(this);
			m_renderVar = renderVar;

			for (uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			{
				auto entry = new MutableNumericElement<COMPONENT_TYPE>(this, this);

				m_layout->addWidget(entry);
				m_elements.push_back(entry);
			}

			refreshUIFromRenderVariable();
		}

		virtual void numericElementValueChanged() override
		{
			refreshRenderVariableFromUI();
		}

		void refreshRenderVariableFromUI()
		{
			RendererVariableType t = m_renderVar->getType();
			assert(t == RENDERVAR_TYPE);
			updateRenderVarFromUI<VARIABLE_TYPE>();

		}

		void refreshUIFromRenderVariable()
		{
			RendererVariableType t = m_renderVar->getType();
			assert(t == RENDERVAR_TYPE);

			updateUIFromRenderVar<VARIABLE_TYPE>();
			setLimitsVector<VARIABLE_TYPE>();
		}

		template<typename U>
		void setLimitsVector()
		{
			U min, max;
			if (m_renderVar->getLimits(min, max))
			{
				setLimitsFromArray(glm::value_ptr(min), glm::value_ptr(max));
			}
		}

		template<typename U>
		void updateRenderVarFromUI()
		{
			U v;
			setElementValuesToArray(glm::value_ptr(v));
			m_renderVar->set(v);
		}

		template<typename U>
		void updateUIFromRenderVar()
		{
			U v;
			bool success = m_renderVar->get(v);
			assert(success);
			setElementValuesFromArray(glm::value_ptr(v));
		}

		void setElementValuesToArray(COMPONENT_TYPE* farray)
		{
			for (size_t i = 0; i < m_elements.size(); ++i)
			{
				farray[i] = m_elements[i]->getValue();
			}
		}

		void setElementValuesFromArray(const COMPONENT_TYPE* farray)
		{
			for (size_t i = 0; i < m_elements.size(); ++i)
			{
				m_elements[i]->setValue(farray[i]);
			}
		}

		void setLimitsFromArray(const COMPONENT_TYPE* minArray, const COMPONENT_TYPE* maxArray)
		{
			for (size_t i = 0; i < m_elements.size(); ++i)
			{
				m_elements[i]->setLimits(minArray[i], maxArray[i]);
			}
		}



		RendererVariable* m_renderVar;
		QHBoxLayout* m_layout;
		std::vector<MutableNumericElement<COMPONENT_TYPE>*> m_elements;
	};



}

