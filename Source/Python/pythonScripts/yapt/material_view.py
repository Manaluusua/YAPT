import sys
from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QGroupBox, QCheckBox 
from yapt.ui_utility import NumericValuesUI
from py_yapt import vec2, vec3, vec4

class MaterialView(QWidget):
    def __init__(self, parent, mat):
        super().__init__(parent)
        self._mat = mat

        layout = QVBoxLayout(self)

        max_float = sys.float_info.max

        layout.addWidget(QLabel(self._mat.getName()))
        
        layout.addWidget(QLabel("Basic"))
        layout.addWidget(self.create_material_property_float("Albedo", 3))
        layout.addWidget(self.create_material_property_float("SpecularTint", 3))
        layout.addWidget(self.create_material_property_float("Metalness", 1))
        layout.addWidget(self.create_material_property_float("SpecularAmount", 1))
        layout.addWidget(self.create_material_property_float("DielectricIOR", 1, limit_max = 6))
        layout.addWidget(self.create_material_property_float("Roughness", 1))
        layout.addWidget(self.create_material_property_float("Anisotropy", 1))
        layout.addWidget(self.create_material_property_float("AnisotropyRotation", 1))


        layout.addWidget(self.create_material_property_float("Transparency", 1))
        layout.addWidget(self.create_material_property_float("Absorption", 3))

        layout.addWidget(self.create_material_property_float("Emission", 3, limit_max = max_float))
        layout.addWidget(self.create_material_property_float("EmissionFocus", 1, limit_max = 256))

        layout.addWidget(QLabel("Clearcoat"))
        layout.addWidget(self.create_material_property_float("ClearCoatAmount", 1))
        layout.addWidget(self.create_material_property_float("ClearCoatRoughness", 1))
        layout.addWidget(self.create_material_property_float("ClearCoatIOR", 1, limit_max = 6))

        layout.addWidget(QLabel("Sheen"))
        layout.addWidget(self.create_material_property_float("SheenAmount", 1))
        layout.addWidget(self.create_material_property_float("SheenRoughness", 1))
        layout.addWidget(self.create_material_property_float("SheenTint", 3))

        layout.addWidget(QLabel("Misc"))
        layout.addWidget(self.create_material_property_bool("TwoSided"))
        layout.addWidget(self.create_material_property_bool("EnableDispersion"))
        layout.addWidget(self.create_material_property_float("CauchysCoefficients", 2, limit_max = 6))

        

        self.setLayout(layout)



    def create_material_property_float(self, prop_name, comp_count, getter_name = None, setter_name = None, limit_min = 0, limit_max = 1):

        min_arr = [limit_min] * comp_count
        max_arr = [limit_max] * comp_count

        if(getter_name == None):
            getter_name = f"get{prop_name}"
        if(setter_name == None):
            setter_name = f"set{prop_name}"

        getter = getattr(self._mat, getter_name)
        setter = getattr(self._mat, setter_name)

        cb = lambda value : self.prop_value_changed(setter, value)

        ui = NumericValuesUI(self, comp_count, 6, f"{prop_name}: ")
        ui.set_limits(min_arr, max_arr)
        if comp_count == 1:
            ui.set_values([getter()])
        else:
            ui.set_values(getter())
        
        ui.set_on_change_callback(cb)
        return ui
        
    def create_material_property_bool(self, prop_name, getter_name = None, setter_name = None):
        if(getter_name == None):
            getter_name = f"get{prop_name}"
        if(setter_name == None):
            setter_name = f"set{prop_name}"

        getter = getattr(self._mat, getter_name)
        setter = getattr(self._mat, setter_name)
        cb = lambda state : setter(state == 2)

        ui = QCheckBox(f"{prop_name}: ", self)
        ui.setChecked(getter())
        ui.stateChanged.connect(cb)
        return ui

    def float_arr_to_vec_type(values):
        comp_count = len(values)

        if comp_count == 1:
            return values[0]
        elif comp_count == 2:
            return vec2(values)
        elif comp_count == 3:
            return vec3(values)
        elif comp_count == 4:
            return vec4(values)

        return None

    def prop_value_changed(self, setter, values):
        setter(MaterialView.float_arr_to_vec_type(values))