from PySide6.QtWidgets import QDialog, QListWidget, QLabel, QHBoxLayout, QVBoxLayout, QWidget, QSizePolicy
from yapt.inspector_view import InspectorView
from yapt.objects_view import ObjectsView


class ObjectsDialog(QDialog):
    def __init__(self, parent, scene, renderer, resources):
        super().__init__(parent)
        self._renderer = renderer
        self._scene = scene
        self._resources = resources

        self.setWindowTitle("Objects")
        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )
        
        self._objects_view = ObjectsView(self)
        self._inspector_view = InspectorView(self)

        dialogLayout = QHBoxLayout(self)
        dialogLayout.addWidget(self._objects_view)
        dialogLayout.addWidget(self._inspector_view)
        self.setLayout(dialogLayout)

    def refresh(self):
        print("TODO: refresh")