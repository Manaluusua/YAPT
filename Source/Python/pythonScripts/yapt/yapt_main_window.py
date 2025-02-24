from PySide6.QtWidgets import QMainWindow, QMessageBox
from PySide6.QtGui import QAction
from yapt.yapt_renderer_vars_window import RendererVarsWindow

class MainWindow(QMainWindow):
    def __init__(self, app):
        super().__init__()
        self._app = app
        self.setWindowTitle("YAPT")
        self.setGeometry(100, 100, 1920, 1080)
        
        self.setWindowTitle("YAPT")
        self.create_menu()
        self.show()

    def create_menu(self):
        menu_bar = self.menuBar()
        
        # File Menu
        file_menu = menu_bar.addMenu("File")
        run_script_action = QAction("Run Script", self)
        run_script_action.triggered.connect(self.run_script)
        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(run_script_action)
        file_menu.addAction(exit_action)
        
        # Options Menu
        options_menu = menu_bar.addMenu("Options")
        rvars_action = QAction("RenderVars", self)
        rvars_action.triggered.connect(self.show_rvars)
        options_menu.addAction(rvars_action)
    
    def run_script(self):
        QMessageBox.information(self, "Run Script", "Running script...")
    
    def show_rvars(self):
        rvars_window = RendererVarsWindow(self, self._app.get_renderer())
        rvars_window.show()

   