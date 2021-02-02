#pragma once
#include <qlabel.h>
#include <qboxlayout.h>
#include <Renderer/Renderer.h>

namespace YAPT
{
    class GuiController;


    class RenderVarEntry : public QWidget
    {
        Q_OBJECT

    public:
        explicit RenderVarEntry(QWidget* parent, GuiController* controller, const QString& name, RendererVariable* renderVar);
        ~RenderVarEntry();


    private:
        QHBoxLayout* m_layout;
        RendererVariable* m_rendererVar;
        GuiController* m_controller;
    };
}

