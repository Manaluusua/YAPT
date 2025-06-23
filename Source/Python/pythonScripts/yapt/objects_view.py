from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QListWidget, QListWidgetItem

class ObjectListItem(QListWidgetItem):
    def __init__(self, parent, obj):
        super().__init__(parent)
        self._obj = obj
        self.setText(obj.getName())

    def get_object(self):
        return self._obj


class ObjectsView(QWidget):
    def __init__(self, parent, scene):
        super().__init__(parent)

        layout = QVBoxLayout(self)
        self._scene = scene
        self.list_widget = QListWidget()
        self.list_widget.currentItemChanged.connect(self.on_selection_change)
        layout.addWidget(self.list_widget)

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

        self.list_widget.clear()

        for obj in objects_list:
            list_item = ObjectListItem(self.list_widget, obj)
            self.list_widget.addItem(list_item)


    def on_selection_change(self, current, previous):
        if current != None:
            self.notify_selection_listeners(current.get_object())
        else:
            self.notify_selection_listeners(None)
        