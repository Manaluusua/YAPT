import sys
from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QGroupBox
from yapt.ui_utility import NumericValuesUI
from py_yapt import vec3, vec4

class MaterialView(QWidget):
    def __init__(self, parent, mat):
        super().__init__(parent)
        self._mat = mat

        layout = QVBoxLayout(self)

        self.silence_notify = True
        layout.addWidget(QLabel(self._mat.getName()))
        

        self.setLayout(layout)
        self.silence_notify = False

        

    