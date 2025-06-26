from PySide6.QtWidgets import QDialog, QWidget, QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QSizePolicy, QDoubleSpinBox, QPushButton, QComboBox
from PySide6.QtGui import QAction


class NumericValuesUI(QWidget):
    def __init__(self, parent, numbers_count, decimals_count, name = None, on_change_callback = None):
        super().__init__(parent)

        layout = QHBoxLayout(self)

        if name != None:
            layout.addWidget(QLabel(name))

        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )

        self._numbers_count = numbers_count
        self._spinBoxes = []

        for i in range(numbers_count):
            sb = QDoubleSpinBox()
            sb.setDecimals(decimals_count)
            sb.valueChanged.connect(self.value_changed)
            layout.addWidget(sb)
            self._spinBoxes.append(sb)

        self._on_change = on_change_callback
        self.setLayout(layout)

    def set_on_change_callback(self, cb):
        self._on_change = cb

    def set_limits(self, limitsMin, limitsMax):
        for i in range(self._numbers_count):
            sb = self._spinBoxes[i]
            sb.setMinimum(limitsMin[i]) 
            sb.setMaximum(limitsMax[i])  

            
    def set_values(self, values):
        for i in range(self._numbers_count):
            sb = self._spinBoxes[i]
            sb.setValue(values[i]) 

    def get_values(self):
        values = []
        for i in range(self._numbers_count):
            sb = self._spinBoxes[i]
            values.append(sb.value())
        return values

    def get_values_int(self):
        values = []
        for i in range(self._numbers_count):
            sb = self._spinBoxes[i]
            values.append(int(sb.value()))
        return values

    def get_numbers_count(self):
        return self._numbers_count

    def value_changed(self, val):
        if self._on_change != None:
           self._on_change(self.get_values())
