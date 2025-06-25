import sys
from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QGroupBox
from PySide6.QtGui import QQuaternion
from yapt.ui_utility import NumericValuesUI
from py_yapt import vec3, vec4

class TransformView(QWidget):
    def __init__(self, parent, transform):
        super().__init__(parent)
        self._transform = transform

        max_float = sys.float_info.max
        limits_min = [-max_float, -max_float, -max_float]
        limits_max = [max_float, max_float, max_float]

        self.silence_notify = True

        self._translation_ui = NumericValuesUI(self, 3, 6, "Translation: ", self.translation_changed)
        self._translation_ui.set_limits(limits_min, limits_max)
        self._translation_ui.set_values(self._transform.getTranslation())

        self._orientation_ui = NumericValuesUI(self, 3, 6, "Orientation: ", self.orientation_changed)
        self._orientation_ui.set_limits(limits_min, limits_max)
        self._orientation_ui.set_values(self.orientation_to_euler(self._transform.getOrientation()))

        self._scale_ui = NumericValuesUI(self, 3, 6, "Scale: ", self.scale_changed)
        self._scale_ui.set_limits(limits_min, limits_max)
        self._scale_ui.set_values(self._transform.getScale())

        layout = QVBoxLayout(self)
        layout.addWidget(self._translation_ui)
        layout.addWidget(self._orientation_ui)
        layout.addWidget(self._scale_ui)
        self.setLayout(layout)

        self.silence_notify = False

    def orientation_to_euler(self, orientation):
        q = QQuaternion(orientation.w, orientation.x, orientation.y, orientation.z)
        qvec = q.toEulerAngles()
        return [qvec.x(), qvec.y(), qvec.z()]

    def euler_to_orientation(self, euler):
        q = QQuaternion.fromEulerAngles(euler[0], euler[1], euler[2])
        return vec4([q.x(), q.y(), q.z(), q.scalar()])

    def translation_changed(self, values):
        if(self.silence_notify == True):
            return
        self._transform.setTranslation(vec3(values))

    def orientation_changed(self, values):
        if(self.silence_notify == True):
            return
        self._transform.setOrientation(self.euler_to_orientation(values))

    def scale_changed(self, values):
        if(self.silence_notify == True):
            return
        self._transform.setScale(vec3(values))