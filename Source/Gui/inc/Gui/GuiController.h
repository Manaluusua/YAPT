#pragma once
#include "Gui.h"

class QApplication;

namespace YAPT
{
	class MainWindow;	
	class GuiController : public Gui
	{
	public:
		GuiController();

		//GUI impl
		virtual ~GuiController() override;
		virtual bool initialize(int argc, char** argv, size_t windowWidth, size_t windowHeight) override;
		virtual int execute(Scene* scene, Renderer* renderer) override;
		virtual YaptRenderSurfaceHandle getRenderSurfaceHandle() override;

		void exit();

		Renderer* getRenderer() const;
		Scene* getScene() const;

	private:
		QApplication* m_application;
		Scene* m_scene;
		Renderer* m_renderer;
		MainWindow* m_mainWindow;
	};




}


