from py_yapt import Camera, vec3, vec4
from PySide6.QtGui import QKeyEvent, QQuaternion, QVector3D, QVector2D
from PySide6.QtCore import Qt, QPoint
import sys

class CameraController:
    def __init__(self, cam):
        self._cam = cam
        self._rotation_speed = QVector2D(1, 1)
        self._movement_speed = 30
        self._active_movement = QVector3D(0, 0, 0)
        self._current_rotation_angles = QVector2D(0, 0)
        self._last_screen_delta = QVector2D(0, 0)
        self._last_mouse_pos = QPoint()


    def set_aspect(self, ratio):
        self._cam.setAspectRatio(ratio)

    def update(self, dt):
        cam = self._cam
        if not self._last_screen_delta.isNull():
            self._current_rotation_angles.setX(self._current_rotation_angles.x() + self._last_screen_delta.x() + self._rotation_speed.x() * dt)
            self._current_rotation_angles.setY(self._current_rotation_angles.y() + self._last_screen_delta.y() + self._rotation_speed.y() * dt)
            roth = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), self._current_rotation_angles.x())
            rotV = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), self._current_rotation_angles.y())
            rot = roth * rotV
            quat = vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])
            cam.getTransform().setOrientation(quat);
            self._last_screen_delta = QVector2D(0, 0)


        if not self._active_movement.isNull():
            relativeMov = cam.getTransform().right() * self._active_movement.x() + cam.getTransform().up() * self._active_movement.y() + cam.getTransform().forward() * self._active_movement.z();
            cam.getTransform().addTranslation(relativeMov * self._movement_speed * dt)

    def mousePressEvent(self, event):
        self._last_mouse_pos = event.pos()

    def mouseReleaseEvent(self, event):
        pass

    def mouseMoveEvent(self, event):
        pos = event.pos()
        delta = pos - self._last_mouse_pos
        self._last_screen_delta = delta
        self._last_mouse_pos = pos;

    def keyPressEvent(self, event):
        if(event.key() == Qt.Key_W):
            self._active_movement.setZ(1)
        elif (event.key() == Qt.Key_S):
            self._active_movement.setZ(-1)
        if(event.key() == Qt.Key_A):
            self._active_movement.setX(-1)
        elif (event.key() == Qt.Key_D):
            self._active_movement.setX(1)
        if(event.key() == Qt.Key_Q):
            self._active_movement.setY(-1)
        elif (event.key() == Qt.Key_E):
            self._active_movement.setY(1)

    def keyReleaseEvent(self, event):
        if(event.key() == Qt.Key_W):
            self._active_movement.setZ(0)
        elif (event.key() == Qt.Key_S):
            self._active_movement.setZ(0)
        if(event.key() == Qt.Key_A):
            self._active_movement.setX(0)
        elif (event.key() == Qt.Key_D):
            self._active_movement.setX(0)
        if(event.key() == Qt.Key_Q):
            self._active_movement.setY(0)
        elif (event.key() == Qt.Key_E):
            self._active_movement.setY(0)