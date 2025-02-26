from py_yapt import Renderer, Scene, RendererCache
from yapt.main_window import MainWindow
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer, QDateTime
import sys


class Application:
    def __init__(self):
        self._app = QApplication(sys.argv)
        self._main_window = MainWindow(self)
        renderer_cache = RendererCache.getDefaultRendererCache()
        self._renderer = Renderer()
        self._renderer.init(
            renderer_cache, self._main_window.get_render_area_widget().winId(), False
        )
        self._scene = Scene()
        self._scene.init(self._renderer)

        DEFAULT_TICK_RATE = 1000.0 / 60;
        self._timer = QTimer(self._main_window)
        self._timer.timeout.connect(self.update)
        self._timer.start(DEFAULT_TICK_RATE)
        self._last_update_ms = QDateTime.currentMSecsSinceEpoch()

        self._sc_ready = False

    def get_qt_app(self):
        return self._app

    def closeEvent(self, event):
        self.shutdown()
        super().closeEvent(event)

    def execute(self):
        self._app.exec()

    def shutdown(self):
        self._timer.stop() #hammertime
        self._scene.shutdown()
        self._renderer.shutdown()

    def get_renderer(self):
        return self._renderer

    def refresh_swapchain(self):
        print("swapchain refresh")
        self._sc_ready = False



    def update(self):
        if self._main_window.sizeChanged == True:
            self.refresh_swapchain()
            self._main_window.sizeChanged = False

        if(not self._sc_ready):
            return

        current_time = QDateTime.currentMSecsSinceEpoch()
        dt = (current_time - self._last_update_ms);

        #m_cameraController->update(fromLastFrame);
        
        self._scene.update(dt);
        
        
        self._last_update_ms = current_time;
