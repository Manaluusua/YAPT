from PySide6.QtWidgets import QDialog, QWidget, QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QSizePolicy, QDoubleSpinBox, QPushButton, QComboBox
from PySide6.QtGui import QAction
from py_yapt import RendererVariableType, vec2, vec3, vec4, ivec2, ivec3, ivec4

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

    def apply(self):
        self._applyFunc()


    def _apply_float(self):
        float_arr = []
        rval = self._r_var
        for sb in self.spinBoxes:
            float_arr.append(float(sb.value()))

        rval.setFromFloatArray(float_arr)

    def _apply_int(self):
        int_arr = []
        rval = self._r_var
        for sb in self.spinBoxes:
            int_arr.append(int(sb.value()))

        rval.setFromIntArray(int_arr)
        
    def _setup_float_var(self, layout):
        
        rvar = self._r_var
        values = rvar.getAsFloatArray()
        limits = rvar.getLimitsFloat()

        self.spinBoxes = []
        for i in range(len(values)):
            sb = QDoubleSpinBox()
            sb.setMinimum(limits[0][i]) 
            sb.setMaximum(limits[1][i])  
            sb.setValue(values[i])  
            sb.setDecimals(6)
            layout.addWidget(sb)
            self.spinBoxes.append(sb)
        self._applyFunc = self._apply_float

    def _setup_int_var(self, layout):
        rvar = self._r_var
        values = rvar.getAsIntArray()
        limits = rvar.getLimitsInt()

        self.spinBoxes = []
        for i in range(len(values)):
            sb = QDoubleSpinBox()
            sb.setMinimum(limits[0][i]) 
            sb.setMaximum(limits[1][i])  
            sb.setValue(values[i])  
            sb.setDecimals(0)
            layout.addWidget(sb)
            self.spinBoxes.append(sb)

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
        self.create_renderer_var_entries()

    def create_renderer_var_entries(self):
        all_renderer_var_names = self._renderer.getAllRendererVariableNames()
        groupBox = QGroupBox("Renderer Variables", self)
        groupBoxLayout = QVBoxLayout(groupBox)

        for renderer_var_name in all_renderer_var_names:
            renderer_var = self._renderer.getRendererVariable(renderer_var_name)
            renderer_var_ui = RendererVariableUI(self, renderer_var_name, renderer_var)
            groupBoxLayout.addWidget(renderer_var_ui)

        groupBox.setLayout(groupBoxLayout)

        dialogLayout = QVBoxLayout(self)
        dialogLayout.addWidget(groupBox)
        self.setLayout(dialogLayout)
            