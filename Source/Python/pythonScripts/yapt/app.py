from py_yapt import Renderer, Scene, RendererCache, ivec2
from yapt.main_window import MainWindow
from yapt.resources import Resources
from yapt.camera_controller import CameraController
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer, QDateTime
import sys
import gc

class Application:
    def __init__(self):
        self._qapp = QApplication(sys.argv)
        self._main_window = MainWindow(self)
        renderer_cache = RendererCache.getDefaultRendererCache()
        self._renderer = Renderer()
        self._renderer.init(
            renderer_cache, self._main_window.get_render_area_widget().winId(), False
        )
        self._scene = Scene()
        self._scene.init(self._renderer)

        self._resources = Resources(self._renderer)
        self._cam = CameraController(self._scene.getMainCamera())


        DEFAULT_TICK_RATE = 1000.0 / 60;
        self._timer = QTimer(self._main_window)
        self._timer.timeout.connect(self.update)
        self._timer.start(DEFAULT_TICK_RATE)
        self._last_update_ms = QDateTime.currentMSecsSinceEpoch()

        self._sc_ready = False

    def get_qt_app(self):
        return self._qapp

    def execute(self):
        self._qapp.exec()

    def shutdown(self):
        if(self._renderer == None):
            return

        self._timer.stop() #hammertime
        self._resources.clear()
        self._resources = None
        gc.collect()

        self._renderer.resetRenderOutput()
        self._scene.shutdown()
        self._renderer.shutdown()
        self._renderer = None
        

    def get_renderer(self):
        return self._renderer

    def get_resources(self):
        return self._resources

    def get_camera_controller(self):
        return self._cam

    def refresh_swapchain(self):
        
        render_widget = self._main_window.get_render_area_widget()
        render_res = [render_widget.width(), render_widget.height()];

        self._renderer.setRenderOutputToSurface(*render_res, render_widget.winId())
        self._renderer.getRendererVariable("Generic.RenderResolution").setFromIntArray(render_res)
        self._cam.set_aspect(float(render_res[0]) / render_res[1]);
        self._sc_ready = True


    def update(self):

        if self._main_window.sizeChanged == True:
            self.refresh_swapchain()
            self._main_window.sizeChanged = False

        if(not self._sc_ready):
            return

        current_time = QDateTime.currentMSecsSinceEpoch()
        dt = (current_time - self._last_update_ms) * 1e-3;

        self._cam.update(dt);
        self._scene.update(dt);
        self._last_update_ms = current_time;
