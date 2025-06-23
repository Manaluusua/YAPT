from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel

class ObjectsView(QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("ObjectsView"))