#include <Gui/RenderVarEntry.h>
#include <Gui/MutableNumberElement.h>
#include <Gui/RenderVarWrapper.h>

namespace YAPT
{
	RenderVarEntry::RenderVarEntry(QWidget* parent, GuiController* controller, const QString& name, RendererVariable* renderVar)
		:QWidget(parent),
		m_controller(controller),
		m_rendererVar(renderVar)
	{

		m_layout = new QHBoxLayout(this);
		m_layout->setAlignment(Qt::AlignLeft);

		QLabel* label = new QLabel(name, this);
		m_layout->addWidget(label);

		RendererVariableType type = m_rendererVar->getType();

		switch (type)
		{
		case YAPT::RendererVariableType::FLOAT:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<float, 1, float, YAPT::RendererVariableType::FLOAT>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::UINT:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<uint32_t, 1, uint32_t, YAPT::RendererVariableType::UINT>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::INT:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<int32_t, 1, int32_t, YAPT::RendererVariableType::INT>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::VEC2:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<float, 2, glm::vec2, YAPT::RendererVariableType::VEC2>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::VEC3:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<float, 3, glm::vec3, YAPT::RendererVariableType::VEC3>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::VEC4:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<float, 4, glm::vec4, YAPT::RendererVariableType::VEC4>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::IVEC2:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<int32_t, 2, glm::ivec2, YAPT::RendererVariableType::IVEC2>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::IVEC3:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<int32_t, 3, glm::ivec3, YAPT::RendererVariableType::IVEC3>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::IVEC4:
		{
			RenderVarWrapper* wrapper = new ConcreteRenderVarWrapper<int32_t, 4, glm::ivec4, YAPT::RendererVariableType::IVEC4>(this, renderVar);
			m_layout->addWidget(wrapper);
		}
		break;
		case YAPT::RendererVariableType::TEXTURE:
		{
			QLabel* l = new QLabel("Texture", this);
			m_layout->addWidget(l);
		}
		break;
		case YAPT::RendererVariableType::BUFFER:
		{
			QLabel* l = new QLabel("Buffer", this);
			m_layout->addWidget(l);
		}
		break;
		default:
			break;
		}








	}
	RenderVarEntry::~RenderVarEntry()
	{

	}
}