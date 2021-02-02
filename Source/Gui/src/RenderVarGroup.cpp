#include <Gui/RenderVarGroup.h>

namespace YAPT
{
	RenderVarsGroup::RenderVarsGroup(QWidget* parent, GuiController* controller, const QString& name)
		:QGroupBox(name, parent),
		m_controller(controller)
	{
		m_layout = new QVBoxLayout(this);
		m_layout->setAlignment(Qt::AlignTop);
	}
	RenderVarsGroup::~RenderVarsGroup()
	{

	}

	RenderVarsGroup* RenderVarsGroup::addGroup(const QString& name)
	{
		RenderVarsGroup* grp = new RenderVarsGroup(this, m_controller, name);
		m_groups[name] = grp;
		m_layout->addWidget(grp);
		return grp;
	}
	RenderVarEntry* RenderVarsGroup::addEntry(const QString& name, RendererVariable* var)
	{
		RenderVarEntry* entry = new RenderVarEntry(this, m_controller, name, var);
		m_entries[name] = entry;
		m_layout->addWidget(entry);
		return entry;
	}


	RenderVarsGroup* RenderVarsGroup::getGroup(const QString& name)
	{
		auto iter = m_groups.find(name);
		if (iter == m_groups.end())
		{
			return nullptr;
		}
		return *iter;
	}
	RenderVarEntry* RenderVarsGroup::getEntry(const QString& name)
	{
		auto iter = m_entries.find(name);
		if (iter == m_entries.end())
		{
			return nullptr;
		}
		return *iter;
	}
}