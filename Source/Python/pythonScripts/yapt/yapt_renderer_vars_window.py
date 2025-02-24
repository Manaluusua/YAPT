from PySide6.QtWidgets import QDialog, QWidget, QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QSizePolicy
from PySide6.QtGui import QAction

class RendererVariableUI(QWidget):
    def __init__(self, parent, name, renderervar):
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.addWidget(QLabel(name))
        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )


class RendererVarsWindow(QDialog):
    def __init__(self, parent, renderer):
        super().__init__(parent)
        self._renderer = renderer

        self.setWindowTitle("Renderer Variables")
        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )
        self.create_renderer_var_entries()

    def create_renderer_var_entries(self):
        all_renderer_var_names = self._renderer.getAllRendererVariableNames()
        groupBox = QGroupBox("Renderer Variables", self)
        groupBoxLayout = QVBoxLayout(groupBox)

        for renderer_var_name in all_renderer_var_names:
            renderer_var = self._renderer.getRendererVariable(renderer_var_name)
            renderer_var_ui = RendererVariableUI(self, renderer_var_name, renderer_var)
            groupBoxLayout.addWidget(renderer_var_ui)

        groupBox.setLayout(groupBoxLayout)

        dialogLayout = QVBoxLayout(self)
        dialogLayout.addWidget(groupBox)
        self.setLayout(dialogLayout)
            