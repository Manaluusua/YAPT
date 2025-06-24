from PySide6.QtWidgets import QMainWindow, QWidget, QFileDialog, QVBoxLayout
from PySide6.QtGui import QAction, QPalette, QColor
from yapt.renderer_vars_window import RendererVarsWindow
from yapt.objects_dialog import ObjectsDialog
import os
from pathlib import Path

class MainWindow(QMainWindow):
    def __init__(self, app):
        super().__init__()
        self._app = app
        self.setWindowTitle("YAPT")
        self.setGeometry(100, 100, 1920, 1080)
        self.setWindowTitle("YAPT")
        self.set_dark_theme()
        self.create_menu()
        self.setup_render_area()
        self.show()
        self.sizeChanged = True

        self._objects_dialog = None
        self._rvars_window = None
    #UI
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
        self._app.get_qt_app().setPalette(palette)
        self._app.get_qt_app().setStyleSheet(f"""
                QMenu {{
                    background-color: {baseColor.name()}; 
                    color: {lightColor}; 
                }}
                QMenu::item:selected {{
                    background-color: {lightColor}; 
                    color: {baseColor.name()};
                }}
            """)

    def create_menu(self):
        menu_bar = self.menuBar()
        
        # File Menu
        file_menu = menu_bar.addMenu("File")
        run_script_action = QAction("Run Script", self)
        run_script_action.triggered.connect(self.run_script)
        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.exit)
        file_menu.addAction(run_script_action)
        file_menu.addAction(exit_action)
        
        # Options Menu
        options_menu = menu_bar.addMenu("Options")
        rvars_action = QAction("RenderVars", self)
        rvars_action.triggered.connect(self.show_rvars)
        options_menu.addAction(rvars_action)
        objects_action = QAction("Objects", self)
        objects_action.triggered.connect(self.show_objects_dialog)
        options_menu.addAction(objects_action)
    
    def setup_render_area(self):
        self._renderAreaWidget = QWidget()
        self.setCentralWidget(self._renderAreaWidget)

    def get_render_area_widget(self):
        return self._renderAreaWidget

    def run_script(self):
        openPath = Path(os.path.abspath(__file__)).parent
        file_name, _ = QFileDialog.getOpenFileName(self, "Open Python Script", str(openPath), "All Files (*);;Text Files (*.py)")
        if(file_name):
            try:
                with open(file_name, 'r') as f:
                        script_content = f.read()
                context = { 
                    "yapt_instance" : self._app
                }
                exec(script_content, context, context)
                #locals_["yapt_instance"] = self._app
                #exec(script_content, globals(), locals_)
            except FileNotFoundError:
                print("The script file was not found.")
            except Exception as e:
                print(f"An error occurred: {e}")
    
    def show_rvars(self):
        if self._rvars_window == None:
            self._rvars_window = RendererVarsWindow(self, self._app.get_renderer())
        self._rvars_window.setup()
        self._rvars_window.show()

    def show_objects_dialog(self):

        if self._objects_dialog == None:
            self._objects_dialog = ObjectsDialog(self, self._app.get_scene(), self._app.get_renderer(), self._app.get_resources())

        self._objects_dialog.refresh()
        self._objects_dialog.show()

    def closeEvent(self, event):

        if self._rvars_window != None:
            self._rvars_window.setParent(None)

        if self._objects_dialog != None:
            self._objects_dialog.setParent(None)

        self._rvars_window = None
        self._objects_dialog = None

        self._app.shutdown()
        super().closeEvent(event)

    def exit(self):
        self.close()

    #events
    def resizeEvent(self, event):
        self.sizeChanged = True

    def mousePressEvent(self, event):
        self._app.get_camera_controller().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        self._app.get_camera_controller().mouseReleaseEvent(event)

    def mouseMoveEvent(self, event):
        self._app.get_camera_controller().mouseMoveEvent(event)

    def keyPressEvent(self, event):
        self._app.get_camera_controller().keyPressEvent(event)

    def keyReleaseEvent(self, event):
        self._app.get_camera_controller().keyReleaseEvent(event)
