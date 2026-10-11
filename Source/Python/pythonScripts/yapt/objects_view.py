from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QTreeWidget, QTreeWidgetItem, QSizePolicy

class ObjectTreeItem(QTreeWidgetItem):
    def __init__(self, parent, obj):
        super().__init__(parent)
        self._obj = obj
        self.setText(0, obj.getName())

    def get_object(self):
        return self._obj


class ObjectsView(QWidget):
    def __init__(self, parent, scene):
        super().__init__(parent)

        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )

        layout = QVBoxLayout(self)
        self._scene = scene
        self.tree_widget = QTreeWidget()
        self.tree_widget.setHeaderHidden(True)
        self.tree_widget.currentItemChanged.connect(self.on_selection_change)
        layout.addWidget(self.tree_widget)

        self._selection_listeners = set()

    def add_selection_listener(self, l):
        self._selection_listeners.add(l)

    def remove_selection_listener(self, l):
        self._selection_listeners.remove(l)

    def notify_selection_listeners(self, selected_obj):
        for l in self._selection_listeners:
            l(selected_obj)

    def refresh(self):

        objects_list = self._scene.get_all_objects_list()

        #group objects under their transform parent, objects whose parent isn't a listed object become roots
        objects_by_id = {obj.getTransform().getId(): obj for obj in objects_list}
        children = {}
        roots = []
        for obj in objects_list:
            parent_id = obj.getTransform().getParentId()
            if parent_id in objects_by_id:
                children.setdefault(parent_id, []).append(obj)
            else:
                roots.append(obj)

        sort_key = lambda obj : obj.getName().lower()

        def add_items(parent_item, objs):
            for obj in sorted(objs, key = sort_key):
                item = ObjectTreeItem(parent_item, obj)
                add_items(item, children.get(obj.getTransform().getId(), []))

        self.tree_widget.clear()
        add_items(self.tree_widget, roots)


    def on_selection_change(self, current, previous):
        if current != None:
            self.notify_selection_listeners(current.get_object())
        else:
            self.notify_selection_listeners(None)
