from PySide6.QtWidgets import QMainWindow, QMessageBox, QFileDialog
from PySide6.QtGui import QAction
from yapt.yapt_renderer_vars_window import RendererVarsWindow
import os
from pathlib import Path

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
        openPath = Path(os.path.abspath(__file__)).parent
        file_name, _ = QFileDialog.getOpenFileName(self, "Open Python Script", str(openPath), "All Files (*);;Text Files (*.py)")
        if(file_name):
            try:
                with open(file_name, 'r') as f:
                        script_content = f.read()
                locals_ = locals()
                locals_["yapt_instance"] = self._app
                exec(script_content, globals(), locals_)
            except FileNotFoundError:
                print("The script file was not found.")
            except Exception as e:
                print(f"An error occurred: {e}")
    
    def show_rvars(self):
        rvars_window = RendererVarsWindow(self, self._app.get_renderer())
        rvars_window.show()

   