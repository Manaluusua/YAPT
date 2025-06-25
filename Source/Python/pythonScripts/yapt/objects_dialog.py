from PySide6.QtWidgets import QDialog, QListWidget, QLabel, QHBoxLayout, QVBoxLayout, QWidget, QSizePolicy, QGroupBox, QPushButton
from yapt.inspector_view import InspectorView
from yapt.objects_view import ObjectsView
import gc

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
        
        #Objects view setup
        grp_box_objects = QGroupBox("Objects", self)
        grp_box_objects_layout = QVBoxLayout(grp_box_objects)
        grp_box_objects.setLayout(grp_box_objects_layout)

        self._objects_view = ObjectsView(self, self._scene)
        grp_box_objects_layout.addWidget(self._objects_view)

        add_button = QPushButton("+")
        add_button.clicked.connect(self.add_object)
        grp_box_objects_layout.addWidget(add_button)

        delete_button = QPushButton("-")
        delete_button.clicked.connect(self.delete_selected_objects)
        grp_box_objects_layout.addWidget(delete_button)
        
        #inspector setup
        grp_box_inspector = QGroupBox("Inspector", self)
        grp_box_inspector_layout = QVBoxLayout(grp_box_inspector)
        grp_box_inspector.setLayout(grp_box_inspector_layout)

        self._inspector_view = InspectorView(self)
        grp_box_inspector_layout.addWidget(self._inspector_view)

        #compose dialog
        dialogLayout = QHBoxLayout(self)
        dialogLayout.addWidget(grp_box_objects)
        dialogLayout.addWidget(grp_box_inspector)
        self.setLayout(dialogLayout)

        self._objects_view.add_selection_listener(self.object_selected)

        self._currently_selected_objects = None

    def add_object(self):
        print("TODO: Add Object")

    def delete_selected_objects(self):
        if self._currently_selected_objects != None:
            for obj in self._currently_selected_objects:
                if obj != None:
                    self._scene.destroy_object(obj)

            self._currently_selected_objects = None
            self.refresh()

    def refresh(self):
        self._inspector_view.show_object(None)
        self._objects_view.refresh()
        gc.collect()

    def object_selected(self, selected_obj):
        self._currently_selected_objects = [selected_obj]
        self._inspector_view.show_object(selected_obj)