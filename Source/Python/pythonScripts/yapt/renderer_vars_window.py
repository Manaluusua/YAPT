from PySide6.QtWidgets import QDialog, QWidget, QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QSizePolicy, QDoubleSpinBox, QPushButton, QComboBox
from PySide6.QtGui import QAction
from py_yapt import RendererVariableType, vec2, vec3, vec4, ivec2, ivec3, ivec4
from yapt.ui_utility import NumericValuesUI

class RendererVariableUI(QWidget):
    def __init__(self, parent, name, renderervar):
        super().__init__(parent)
        self._r_var = renderervar
        layout = QHBoxLayout(self)
        layout.addWidget(QLabel(name))
        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )
        rvar = self._r_var

        needsApplyButton = True

        if(rvar.getType() == RendererVariableType.FLOAT):
            self._setup_float_var(layout)
        elif(rvar.getType() == RendererVariableType.INT):
            self._setup_int_var(layout)
        elif(rvar.getType() == RendererVariableType.TEXTURE):
            self._setup_tex_var(layout)
            needsApplyButton = False
        elif(rvar.getType() == RendererVariableType.BUFFER):
            self._setup_buff_var(layout)
            needsApplyButton = False
        elif(rvar.getType() == RendererVariableType.OPTIONS):
            self._setup_options_var(layout)
            needsApplyButton = False

        if needsApplyButton:
            button = QPushButton("\u2713")
            button.clicked.connect(self.apply)
            layout.addWidget(button)

        self.setLayout(layout)

    def apply(self):
        self._applyFunc()


    def _apply_float(self):
        rval = self._r_var
        rval.setFromFloatArray(self._numbers_ui.get_values())

    def _apply_int(self):
        rval = self._r_var
        rval.setFromIntArray(self._numbers_ui.get_values_int())
        
    def _setup_float_var(self, layout):
        
        rvar = self._r_var
        values = rvar.getAsFloatArray()
        limits = rvar.getLimitsFloat()

        self._numbers_ui = NumericValuesUI(self, len(values), 6)
        self._numbers_ui.set_limits(limits[0], limits[1])
        self._numbers_ui.set_values(values)
        layout.addWidget(self._numbers_ui)
        self._applyFunc = self._apply_float

    def _setup_int_var(self, layout):
        rvar = self._r_var
        values = rvar.getAsIntArray()
        limits = rvar.getLimitsInt()

        self._numbers_ui = NumericValuesUI(self, len(values), 0)
        self._numbers_ui.set_limits(limits[0], limits[1])
        self._numbers_ui.set_values(values)
        layout.addWidget(self._numbers_ui)
        self._applyFunc = self._apply_int

    def _setup_tex_var(self, layout):
        rvar = self._r_var
        tex = rvar.getTexture()
        name = "None"
        if(tex):
            name = tex.getName()
        self._res_name = QLabel(name)
        layout.addWidget(self._res_name)

    def _setup_buff_var(self, layout):
        rvar = self._r_var
        buff = rvar.getBuffer()
        name = "None"
        if(buff):
            name = buff.getName()
        self._res_name = QLabel(name)
        layout.addWidget(self._res_name)


    def _selection_index_changed(self, index):
        rval = self._r_var
        rval.setSelectedOption(index)

    def _setup_options_var(self, layout):
        rvar = self._r_var
        options = rvar.getOptions()
        selected_index = rvar.getSelectedOption()
        
        self._dropdown = QComboBox()
        self._dropdown.addItems(options) 
        self._dropdown.setCurrentIndex(selected_index)
        self._dropdown.currentIndexChanged.connect(self._selection_index_changed)
        layout.addWidget(self._dropdown)



class RendererVarsWindow(QDialog):
    def __init__(self, parent, renderer):
        super().__init__(parent)
        self._renderer = renderer

        self.setWindowTitle("Renderer Variables")
        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )

    def clear(self):
        QWidget().setLayout(self.layout())

    def setup(self):

        self.clear()

        dialogLayout = QVBoxLayout(self)
        self.setLayout(dialogLayout)

        all_renderer_var_names = self._renderer.getAllRendererVariableNames()

        categories = {}
        default_category = "Generic"

        for renderer_var_name in all_renderer_var_names:
            renderer_var = self._renderer.getRendererVariable(renderer_var_name)
            s = renderer_var_name.split('.', 1)

            if(len(s) == 1):
                entry = (s[0], renderer_var)
                if default_category in categories:
                    categories[default_category].append(entry)
                else:
                    categories[default_category] = [entry]
            else:
                entry = (s[1], renderer_var)
                if s[0] in categories:
                    categories[s[0]].append(entry)
                else:
                    categories[s[0]] = [entry]

        sorted_keys = sorted(categories.keys())

        for key in sorted_keys:
            groupBox = QGroupBox(key, self)
            groupBoxLayout = QVBoxLayout(groupBox)
            groupBox.setLayout(groupBoxLayout)

            for value in sorted(categories[key]):
                renderer_var_ui = RendererVariableUI(self, value[0], value[1])
                groupBoxLayout.addWidget(renderer_var_ui)
            dialogLayout.addWidget(groupBox)

            