from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QGroupBox

class TransformView(QWidget):
    def __init__(self, parent, transform):
        super().__init__(parent)
        self._transform = transform


