from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QGroupBox
from yapt.transform_view import TransformView
from yapt.material_view import MaterialView
from yapt.light_view import LightView
from py_yapt import RenderObject, Light

class InspectorView(QWidget):
    def __init__(self, parent):
        super().__init__(parent)
        
        self._selected_obj = None
        

    def show_object(self, obj):
        self.clear()
        self._selected_obj = obj

        self_layout = self.layout()

        trans_group = QGroupBox("Transform")
        trans_group_layout = QVBoxLayout(trans_group)
        trans_group.setLayout(trans_group_layout)
        self_layout.addWidget(trans_group)

        obj = self._selected_obj
        if obj != None:
            self.setup_transform_view(trans_group_layout)

            if isinstance(obj, RenderObject):
                mesh_group = QGroupBox("Mesh")
                mesh_group_layout = QVBoxLayout(mesh_group)
                mesh_group.setLayout(mesh_group_layout)
                self.setup_mesh_view(mesh_group_layout)
                self_layout.addWidget(mesh_group)

                mat_group = QGroupBox("Materials")
                mat_group_layout = QVBoxLayout(mat_group)
                mat_group.setLayout(mat_group_layout)
                self.setup_material_view(mat_group_layout)
                self_layout.addWidget(mat_group)
            elif isinstance(obj, Light):
                light_group = QGroupBox("Light")
                light_group_layout = QVBoxLayout(light_group)
                light_group.setLayout(light_group_layout)
                light_group_layout.addWidget(LightView(self, obj))
                self_layout.addWidget(light_group)

        self_layout.addStretch()
        

    def clear(self):
        QWidget().setLayout(self.layout())

        self_layout = QVBoxLayout(self)
        self.setLayout(self_layout)

    def setup_transform_view(self, layout):
        obj = self._selected_obj
        transform_view = TransformView(self, obj.getTransform())
        layout.addWidget(transform_view)

    def setup_mesh_view(self, layout):
        text = "None"
        mesh = self._selected_obj.getMesh()
        if mesh != None:
            text = f"name: {mesh.getName()}, Submesh Count: {mesh.getSubmeshCount()}"
        w = QLabel(text)
        layout.addWidget(w)

    def setup_material_view(self, layout):
    
        #for now just list all materials: TODO better ux, this becomes horrible pretty fast
        materials = self._selected_obj.getMaterials()
        for mat in materials:
            material_view = MaterialView(self, mat)
            layout.addWidget(material_view)
        