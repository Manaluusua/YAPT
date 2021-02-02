#pragma once
#include <qgroupbox.h>
#include <Gui/RenderVarEntry.h>
#include <QtWidgets/QVBoxLayout>

namespace YAPT
{
    class GuiController;
    class RenderVarsGroup : public QGroupBox
    {
        Q_OBJECT

    public:
        explicit RenderVarsGroup(QWidget* parent, GuiController* controller, const QString& name);
        ~RenderVarsGroup();

        RenderVarsGroup* addGroup(const QString& name);
        RenderVarEntry* addEntry(const QString& name, RendererVariable* var);

        RenderVarsGroup* getGroup(const QString& name);
        RenderVarEntry* getEntry(const QString& name);


    private:

        QHash<QString, RenderVarsGroup*> m_groups;
        QHash<QString, RenderVarEntry*> m_entries;

        GuiController* m_controller;
        QVBoxLayout* m_layout;
    };
}

