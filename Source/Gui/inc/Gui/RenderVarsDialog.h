#pragma once
#include <QDialog>

namespace Ui {
class RenderVarsDialog;
}
namespace YAPT
{
    class GuiController;
    class RenderVarsDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit RenderVarsDialog(QWidget* parent, GuiController* controller);
        ~RenderVarsDialog();



    private:

        void setup();

        Ui::RenderVarsDialog* m_ui;
        GuiController* m_controller;
    };
}

