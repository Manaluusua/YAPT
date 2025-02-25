from py_yapt import Renderer, RendererCache
from yapt.yapt_main_window import MainWindow
from PySide6.QtWidgets import QApplication
from PySide6.QtGui import QPalette, QColor
import sys

class Application:
  def __init__(self):
    self._app = QApplication(sys.argv)
    self.set_dark_theme()

    self._main_window = MainWindow(self)
    renderer_cache = RendererCache.getDefaultRendererCache()
    self._renderer = Renderer()
    self._renderer.init(renderer_cache, self._main_window.winId(), False)
    
  def set_dark_theme(self):
    lightColor = "#a2ff00"
    backgroundColor = QColor(30, 30, 30)
    baseColor = QColor(40, 40, 40)
    alternateBase = QColor(50, 50, 50)

    palette = QPalette()
    palette.setColor(QPalette.Window,backgroundColor)  
    palette.setColor(QPalette.WindowText, lightColor)  
    palette.setColor(QPalette.Base, QColor(40, 40, 40))  
    palette.setColor(QPalette.AlternateBase, alternateBase)
    palette.setColor(QPalette.ToolTipBase, lightColor)
    palette.setColor(QPalette.ToolTipText, backgroundColor)
    palette.setColor(QPalette.Text, lightColor)
    palette.setColor(QPalette.Button, alternateBase)
    palette.setColor(QPalette.ButtonText, lightColor)
    palette.setColor(QPalette.Highlight, lightColor) 
    palette.setColor(QPalette.HighlightedText, backgroundColor)
    self._app.setPalette(palette)
    self._app.setStyleSheet(f"""
            QMenu {{
                background-color: {baseColor.name()}; 
                color: {lightColor}; 
            }}
            QMenu::item:selected {{
                background-color: {lightColor}; 
                color: {baseColor.name()};
            }}
        """)
    def closeEvent(self, event):
        self.shutdown()
        super().closeEvent(event)
  def execute(self):
    self._app.exec()
    
  def shutdown(self):
    self._renderer.release()

  def get_renderer(self):
      return self._renderer
