from PySide6.QtWidgets import (QDialog, QWidget, QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QSizePolicy,
                               QDoubleSpinBox, QPushButton, QLineEdit, QPlainTextEdit, QFileDialog)


class ScreenshotDialog(QDialog):
    MAX_LOG_LINES = 200

    def __init__(self, parent, screenshot_controller):
        super().__init__(parent)
        self._controller = screenshot_controller

        self.setWindowTitle("Screenshot")
        self.setSizePolicy(
            QSizePolicy.MinimumExpanding,
            QSizePolicy.MinimumExpanding
        )

        dialogLayout = QVBoxLayout(self)
        dialogLayout.addWidget(self._create_output_group())
        dialogLayout.addWidget(self._create_single_group())
        dialogLayout.addWidget(self._create_continuous_group())
        dialogLayout.addWidget(self._create_log_group())
        self.setLayout(dialogLayout)

        self._controller.add_status_listener(self.on_status)
        self.refresh()

    #UI
    def _create_output_group(self):
        grp_box = QGroupBox("Output", self)
        grp_box_layout = QVBoxLayout(grp_box)

        directory_layout = QHBoxLayout()
        directory_layout.addWidget(QLabel("Directory"))
        self._directory_edit = QLineEdit(self._controller.get_output_directory())
        self._directory_edit.editingFinished.connect(self._directory_changed)
        directory_layout.addWidget(self._directory_edit)
        browse_button = QPushButton("...")
        browse_button.clicked.connect(self.browse_output_directory)
        directory_layout.addWidget(browse_button)
        grp_box_layout.addLayout(directory_layout)

        prefix_layout = QHBoxLayout()
        prefix_layout.addWidget(QLabel("Name prefix"))
        self._prefix_edit = QLineEdit(self._controller.get_name_prefix())
        self._prefix_edit.editingFinished.connect(self._prefix_changed)
        prefix_layout.addWidget(self._prefix_edit)
        grp_box_layout.addLayout(prefix_layout)

        grp_box.setLayout(grp_box_layout)
        return grp_box

    def _create_single_group(self):
        grp_box = QGroupBox("Single", self)
        grp_box_layout = QHBoxLayout(grp_box)

        take_button = QPushButton("Take Screenshot")
        take_button.clicked.connect(self.take_screenshot)
        grp_box_layout.addWidget(take_button)

        grp_box.setLayout(grp_box_layout)
        return grp_box

    def _create_continuous_group(self):
        grp_box = QGroupBox("Continuous", self)
        grp_box_layout = QVBoxLayout(grp_box)

        interval_layout = QHBoxLayout()
        interval_layout.addWidget(QLabel("Interval (s)"))
        self._interval_spinbox = QDoubleSpinBox()
        self._interval_spinbox.setDecimals(3)
        self._interval_spinbox.setMinimum(0.01)
        self._interval_spinbox.setMaximum(3600.0)
        self._interval_spinbox.setValue(self._controller.get_interval())
        self._interval_spinbox.valueChanged.connect(self._interval_changed)
        interval_layout.addWidget(self._interval_spinbox)
        grp_box_layout.addLayout(interval_layout)

        self._continuous_button = QPushButton("Start")
        self._continuous_button.clicked.connect(self.toggle_continuous)
        grp_box_layout.addWidget(self._continuous_button)

        self._status_label = QLabel("")
        grp_box_layout.addWidget(self._status_label)

        grp_box.setLayout(grp_box_layout)
        return grp_box

    def _create_log_group(self):
        grp_box = QGroupBox("Log", self)
        grp_box_layout = QVBoxLayout(grp_box)

        self._log_view = QPlainTextEdit()
        self._log_view.setReadOnly(True)
        self._log_view.setMaximumBlockCount(self.MAX_LOG_LINES)
        grp_box_layout.addWidget(self._log_view)

        grp_box.setLayout(grp_box_layout)
        return grp_box

    #actions
    def take_screenshot(self):
        self._controller.take_screenshot()
        self.refresh()

    def toggle_continuous(self):
        if self._controller.is_continuous_running():
            self._controller.stop_continuous()
        else:
            self._controller.start_continuous()
        self.refresh()

    def browse_output_directory(self):
        directory = QFileDialog.getExistingDirectory(self, "Screenshot Directory", self._controller.get_output_directory())
        if directory:
            self._controller.set_output_directory(directory)
            self._directory_edit.setText(directory)

    def refresh(self):
        running = self._controller.is_continuous_running()
        self._continuous_button.setText("Stop" if running else "Start")
        self._interval_spinbox.setEnabled(not running)
        self._status_label.setText(
            f"{'running' if running else 'stopped'}, {self._controller.get_pending_count()} readback(s) pending"
        )

    def on_status(self, message):
        self._log_view.appendPlainText(message)
        self.refresh()

    def _interval_changed(self, value):
        self._controller.set_interval(value)

    def _directory_changed(self):
        self._controller.set_output_directory(self._directory_edit.text())

    def _prefix_changed(self):
        self._controller.set_name_prefix(self._prefix_edit.text())
