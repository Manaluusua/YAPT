#include <Gui/MainWindow.h>
#include <Gui/GuiController.h>
#include <Gui/CameraController.h>
#include "ui_mainwindow.h"
#include <qdebug.h>
#include <Renderer/Renderer.h>
#include <Scene/Scene.h>
#include <Scene/Camera.h>
#include <qdatetime.h>

#include <Gui/RenderVarsDialog.h>

namespace YAPT
{
	constexpr size_t renderTimerTickRate = 16;
	MainWindow::MainWindow()
		:m_ui(new Ui::MainWindow),
		m_controller(nullptr),
		m_swapchainCreated(false)
	{
		
		m_ui->setupUi(this);

	}

	void MainWindow::initialize(GuiController* controller)
	{
		assert(m_controller == nullptr);
		if (m_controller != nullptr) return;

		m_controller = controller;
		m_renderVarsDialog = std::make_unique<RenderVarsDialog>(this, controller);
		m_cameraController = std::make_unique<CameraController>(controller->getScene());
		
		connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::exitCalled);

		m_renderTimer = new QTimer(this);
		connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::renderCalled);
		m_renderTimer->start(renderTimerTickRate);

		m_startedTime = QDateTime::currentMSecsSinceEpoch();
		m_lastTime = m_startedTime;

		connect(m_ui->actionRenderer_Variables, &QAction::triggered, this, &MainWindow::openRenderVarsDialog);
	}

	void MainWindow::exitCalled()
	{
		m_renderTimer->stop();
		std::lock_guard<std::mutex> lock(m_rendererAccessMutex);
		m_controller->exit();
	}

	void MainWindow::renderCalled()
	{
		std::lock_guard<std::mutex> lock(m_rendererAccessMutex);

		if (!m_swapchainCreated)
		{
			m_swapchainCreated = refreshRendererSwapchain();
		}

		if (!m_swapchainCreated) return;

		quint64 currentTime = QDateTime::currentMSecsSinceEpoch();
		double fromStart = (currentTime - m_startedTime) / 1e3;
		float fromLastFrame = (currentTime - m_lastTime) / 1e3;

		m_cameraController->update(fromLastFrame);

		SceneUpdateParameters sceneUpdateParameters;
		sceneUpdateParameters.timeSinceLastFrameInSeconds = fromLastFrame;
		sceneUpdateParameters.overallElapsedTimeInSeconds = fromStart;

		m_controller->getScene()->update(sceneUpdateParameters);


		m_lastTime = currentTime;
	}

	WId MainWindow::GetHandleToRenderArea()
	{
		QWidget* widget = findChild<QWidget*>("renderSurface");
		WId handle = widget->winId();
		return handle;
	}

	bool MainWindow::refreshRendererSwapchain()
	{
		QWidget* widget = findChild<QWidget*>("renderSurface");
		WId handle = GetHandleToRenderArea();
		

		WindowSurfaceDefinition surfaceDef;
		surfaceDef.width = widget->width();
		surfaceDef.height = widget->height();
		surfaceDef.windowHandle = (HWND)handle;

		//check that the surface is actually of correct size
		RECT rect;
		if (GetWindowRect(surfaceDef.windowHandle, &rect))
		{
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;

			if (width != surfaceDef.width || height != surfaceDef.height)
			{
				return false;
			}
		}

		bool success = m_controller->getRenderer()->setRenderOutputToSurface(surfaceDef);

		//also change rendering resolution to match
		//setup camera and resolution
		ivec2p renderResolution = ivec2p(widget->width(), widget->height());
		m_controller->getRenderer()->getRendererConfiguration()->getRendererVariable("Generic.RenderResolution")->set(renderResolution);
		m_controller->getScene()->getMainCamera()->setAspectRatio(float(renderResolution.x) / renderResolution.y);

		m_swapchainCreated = true;

		return success;
	}


	void MainWindow::resizeEvent(QResizeEvent *ev)
	{
		if (!m_controller) return;
		m_swapchainCreated = false;
		
	}

	void MainWindow::openRenderVarsDialog()
	{
		m_renderVarsDialog->show();
	}


	MainWindow::~MainWindow()
	{
		
	}

	void MainWindow::mouseMoveEvent(QMouseEvent* e)
	{
		m_cameraController->mouseMoveEvent(e);
	}
	void MainWindow::mousePressEvent(QMouseEvent* e)
	{
		m_cameraController->mousePressEvent(e);
	}
	void MainWindow::mouseReleaseEvent(QMouseEvent* e)
	{
		m_cameraController->mouseReleaseEvent(e);
	}


	void MainWindow::keyPressEvent(QKeyEvent* e)
	{
		m_cameraController->keyPressEvent(e);
	}
	void MainWindow::keyReleaseEvent(QKeyEvent* e)
	{
		m_cameraController->keyReleaseEvent(e);
	}


}