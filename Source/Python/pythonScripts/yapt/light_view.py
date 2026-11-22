import math
import sys
from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox
from yapt.ui_utility import NumericValuesUI
from py_yapt import vec3, Light

LIGHT_TYPES = [
    ("Point", Light.LightType.POINT),
    ("Spot", Light.LightType.SPOT),
    ("Directional", Light.LightType.DIRECTIONAL),
]


class LightView(QWidget):
    def __init__(self, parent, light):
        super().__init__(parent)
        self._light = light

        layout = QVBoxLayout(self)

        type_row = QWidget(self)
        type_layout = QHBoxLayout(type_row)
        type_layout.addWidget(QLabel("Type: "))
        self._type_combo = QComboBox(type_row)
        for label, _ in LIGHT_TYPES:
            self._type_combo.addItem(label)
        self._type_combo.setCurrentIndex(self._type_index(light.getType()))
        self._type_combo.currentIndexChanged.connect(self._type_changed)
        type_layout.addWidget(self._type_combo)
        layout.addWidget(type_row)

        color_ui = NumericValuesUI(self, 3, 6, "Color: ")
        color_ui.set_limits([0] * 3, [1] * 3)
        color_ui.set_values(light.getColor())
        color_ui.set_on_change_callback(lambda v: self._light.setColor(vec3(v)))
        layout.addWidget(color_ui)

        intensity_ui = NumericValuesUI(self, 1, 6, "Intensity: ")
        intensity_ui.set_limits([0], [sys.float_info.max])
        intensity_ui.set_values([light.getIntensity()])
        intensity_ui.set_on_change_callback(lambda v: self._light.setIntensity(v[0]))
        layout.addWidget(intensity_ui)

        #cone half angles, edited in degrees
        self._cone_ui = NumericValuesUI(self, 2, 3, "Spot inner/outer angle: ")
        self._cone_ui.set_limits([0, 0], [90, 90])
        self._cone_ui.set_values([math.degrees(light.getSpotInnerAngle()), math.degrees(light.getSpotOuterAngle())])
        self._cone_ui.set_on_change_callback(self._cone_changed)
        layout.addWidget(self._cone_ui)

        self._update_visibility()
        self.setLayout(layout)

    def _type_index(self, light_type):
        for i, (_, t) in enumerate(LIGHT_TYPES):
            if t == light_type:
                return i
        return 0

    def _type_changed(self, index):
        self._light.setType(LIGHT_TYPES[index][1])
        self._update_visibility()

    def _cone_changed(self, values):
        self._light.setSpotConeAngles(math.radians(values[0]), math.radians(values[1]))

    def _update_visibility(self):
        self._cone_ui.setVisible(self._light.getType() == Light.LightType.SPOT)
