from py_yapt import Renderer, RendererCache, ivec2
from yapt.main_window import MainWindow
from yapt.resources import Resources
from yapt.scene_wrapper import SceneWrapper
from yapt.screenshot_controller import ScreenshotController
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer, QDateTime
import sys
import gc
import traceback

class Application:
    def __init__(self):
        self._qapp = QApplication(sys.argv)
        self._main_window = MainWindow(self)
        renderer_cache = RendererCache.getDefaultRendererCache()
        self._renderer = Renderer()
        self._renderer.init(
            renderer_cache, self._main_window.get_render_area_widget().winId(), False
        )
        self._scene = SceneWrapper(self._renderer)
        self._resources = Resources(self._renderer)

        self._tick_listeners = []
        self._screenshot_controller = ScreenshotController(self)

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
        self._screenshot_controller.shutdown()
        self._screenshot_controller = None
        self._tick_listeners.clear()
        self._resources.clear()
        self._resources = None
        self._scene.shutdown()
        self._scene = None
        gc.collect()

        self._renderer.resetRenderOutput()
        self._renderer.shutdown()
        self._renderer = None
        

    def get_renderer(self):
        return self._renderer

    def get_resources(self):
        return self._resources

    def get_scene(self):
        return self._scene

    def get_camera_controller(self):
        return self._scene.get_camera_controller()

    def get_screenshot_controller(self):
        return self._screenshot_controller

    #tick listeners are called once per update/render, with the delta time in seconds as their only argument
    def add_tick_listener(self, l):
        if l not in self._tick_listeners:
            self._tick_listeners.append(l)

    def remove_tick_listener(self, l):
        if l in self._tick_listeners:
            self._tick_listeners.remove(l)

    def notify_tick_listeners(self, dt):
        #iterate a copy so that a listener can unregister itself while being ticked
        for l in list(self._tick_listeners):
            try:
                l(dt)
            except Exception as e:
                print(f"An error occurred while ticking {l}: {e}")
                traceback.print_exc()

    def refresh_swapchain(self):
        
        render_widget = self._main_window.get_render_area_widget()
        render_res = [render_widget.width(), render_widget.height()];

        self._renderer.setRenderOutputToSurface(*render_res, render_widget.winId())
        self._renderer.getRendererVariable("Generic.RenderResolution").setFromIntArray(render_res)
        self._scene.get_camera_controller().set_aspect(float(render_res[0]) / render_res[1]);
        self._sc_ready = True


    def update(self):

        if self._main_window.sizeChanged == True:
            self.refresh_swapchain()
            self._main_window.sizeChanged = False

        if(not self._sc_ready):
            return

        current_time = QDateTime.currentMSecsSinceEpoch()
        dt = (current_time - self._last_update_ms) * 1e-3;
        self._scene.update(dt);
        self._last_update_ms = current_time;
        self.notify_tick_listeners(dt)
