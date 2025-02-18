from py_yapt import Renderer
from yapt.yapt_mainwindow import MainWindow
from PySide6.QtWidgets import QApplication
import sys

class Application:
  def __init__(self):
    self._renderer = Renderer()
    self._renderer.init();
    self._app = QApplication(sys.argv)
    self._mainWindow = MainWindow()
    
  def execute(self):
    self._app.exec()
    
  def shutdown(self):
    self._renderer.release();