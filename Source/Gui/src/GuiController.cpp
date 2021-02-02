#include "Gui/GuiController.h"
#include "Gui/MainWindow.h"
#include <QtWidgets/qapplication.h>
#include <qdatetime.h>
#include <Renderer/Renderer.h>

namespace YAPT
{
	GuiController::GuiController()
		:m_application(nullptr)
	{

	}

	GuiController::~GuiController()
	{
		if (m_application)
		{
			delete m_application;
			m_application = nullptr;
		}
		
	}

	bool GuiController::initialize(int argc, char** argv, size_t windowWidth, size_t windowHeight)
	{
		m_application = new QApplication(argc, argv);
		m_mainWindow = new MainWindow();
		m_mainWindow->resize((int)windowWidth, (int)windowHeight);
		return true;

	}

	YaptRenderSurfaceHandle GuiController::getRenderSurfaceHandle()
	{
		return (YaptRenderSurfaceHandle)m_mainWindow->GetHandleToRenderArea();
	}

	int GuiController::execute(Scene* scene, Renderer* renderer)
	{
		m_renderer = renderer;
		m_scene = scene;

		m_mainWindow->initialize(this);
		m_mainWindow->show();

		return (bool)m_application->exec();
	}

	Renderer* GuiController::getRenderer() const
	{
		return m_renderer;
	}
	Scene* GuiController::getScene() const
	{
		return m_scene;
	}

	void GuiController::exit()
	{
		m_renderer->resetRenderOutput();
		m_mainWindow->close();
		m_mainWindow = nullptr;
	}

}