from PySide6.QtWidgets import QDialog, QWidget, QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QSizePolicy, QDoubleSpinBox, QPushButton, QComboBox
from PySide6.QtGui import QAction


class CompactDoubleSpinBox(QDoubleSpinBox):
    """QDoubleSpinBox that only shows the decimals a value needs (0.5 instead of 0.500000).
    decimals() still sets the precision, so more digits can be typed in when needed."""

    def textFromValue(self, value):
        locale = self.locale()
        text = locale.toString(float(value), 'f', self.decimals())
        if not self.isGroupSeparatorShown():
            text = text.replace(locale.groupSeparator(), "")

        decimal_point = locale.decimalPoint()
        if decimal_point in text:
            text = text.rstrip("0").rstrip(decimal_point)

        if text in ("", "-0"):
            text = "0"
        return text


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
            sb = CompactDoubleSpinBox()
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
