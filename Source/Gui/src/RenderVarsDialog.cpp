#include <Gui/RenderVarsDialog.h>
#include "ui_rendervarsdialog.h"
#include <Gui/GuiController.h>
#include <Renderer/Renderer.h>
#include <QLabel>
#include <Gui/RenderVarGroup.h>

namespace YAPT
{
    RenderVarsDialog::RenderVarsDialog(QWidget* parent, GuiController* controller) :
        QDialog(parent),
        m_ui(new Ui::RenderVarsDialog),
        m_controller(controller)
    {
        m_ui->setupUi(this);
        setup();
    }

    RenderVarsDialog::~RenderVarsDialog()
    {
        delete m_ui;
    }



    void RenderVarsDialog::setup()
    {
        RendererConfiguration* rConfig = m_controller->getRenderer()->getRendererConfiguration();
        size_t rendererVarsCount = rConfig->getNumberOfRendererVariables();

        std::vector<const char*> varNames;
        varNames.resize(rendererVarsCount);

        rConfig->queryRendererVariableNamesList(varNames.data(), rendererVarsCount);

        RenderVarsGroup* rootGroup = new RenderVarsGroup(this, m_controller, "");


        for (int i = 0; i < rendererVarsCount; ++i)
        {
            QString varName = varNames[i];
            QStringList strList = varName.split(".");

            
            RenderVarsGroup* group = rootGroup;

            for (int k = 0; k < strList.size() - 1; ++k)
            {
                RenderVarsGroup* group2 =  group->getGroup(strList[k]);
                if (group2 == nullptr)
                {
                    group2 = group->addGroup(strList[k]);
                }
                
                group = group2;
            }
            
            group->addEntry(strList.back(), rConfig->getRendererVariable(varNames[i]));

        }

        m_ui->RVarListLayout->addWidget(rootGroup);

    }
}