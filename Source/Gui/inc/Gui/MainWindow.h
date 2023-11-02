#pragma once

#include <QtWidgets/qmainwindow.h>
#include <qtimer.h>
#include <mutex>

namespace Ui {
	class MainWindow;
}

namespace YAPT
{
	class GuiController;
	class CameraController;
	class RenderVarsDialog;
	class MainWindow : public QMainWindow
	{
		Q_OBJECT
	public:
		MainWindow();
		void initialize(GuiController* controller);
		virtual ~MainWindow();

		WId GetHandleToRenderArea();

	protected:
		virtual void resizeEvent(QResizeEvent *ev) override;
		virtual void mouseMoveEvent(QMouseEvent* e) override;
		virtual void mousePressEvent(QMouseEvent* e) override;
		virtual void mouseReleaseEvent(QMouseEvent* e) override;

		virtual void keyPressEvent(QKeyEvent* e) override;
		virtual void keyReleaseEvent(QKeyEvent* e) override;
	private:
		bool refreshRendererSwapchain();
		void exitCalled();
		void renderCalled();

		void openRenderVarsDialog();

		qint64 m_startedTime;
		qint64 m_lastTime;
		QTimer* m_renderTimer;
		GuiController* m_controller;

		bool m_swapchainCreated;

		std::unique_ptr<RenderVarsDialog> m_renderVarsDialog;
		std::unique_ptr<CameraController> m_cameraController;
		Ui::MainWindow *m_ui;
		std::mutex m_rendererAccessMutex;
	};


	

}

