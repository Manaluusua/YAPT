from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QGroupBox
from yapt.transform_view import TransformView
class InspectorView(QWidget):
    def __init__(self, parent):
        super().__init__(parent)
        
        self._selected_obj = None
        

    def show_object(self, obj):
        self.clear()
        self._selected_obj = obj
        
        if obj == None:
            return

        self_layout = self.layout()
        self.setup_transform_view(self_layout)
        self.setup_mesh_view(self_layout)
        self.setup_material_view(self_layout)

        

    def clear(self):
        QWidget().setLayout(self.layout())

        self_layout = QVBoxLayout(self)
        self.setLayout(self_layout)

    def setup_transform_view(self, layout):
        obj = self._selected_obj
        transform_view = TransformView(self, obj.getTransform())
        layout.addWidget(transform_view)

    def setup_mesh_view(self, layout):
        dummy = QLabel("TODO: MESH")
        layout.addWidget(dummy)

    def setup_material_view(self, layout):
        dummy = QLabel("TODO: MATERIAL")
        layout.addWidget(dummy)