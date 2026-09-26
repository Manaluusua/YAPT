from py_yapt import Camera, vec3, vec4
from PySide6.QtGui import QKeyEvent, QQuaternion, QVector3D, QVector2D
from PySide6.QtCore import Qt, QPoint
import math
import sys

class CameraController:
    MAX_PITCH = 89.9

    def __init__(self, cam):
        self._cam = cam
        self._rotation_speed = QVector2D(1, 1)
        self._movement_speed = 30
        self._active_movement = QVector3D(0, 0, 0)
        self._last_screen_delta = QVector2D(0, 0)
        self._last_mouse_pos = QPoint()


    def set_aspect(self, ratio):
        self._cam.setAspectRatio(ratio)

    def set_movement_speed(self, speed):
        self._movement_speed = speed

    def get_movement_speed(self):
        return self._movement_speed

    def _get_yaw_pitch(self):
        # read the angles back from the camera so rotating continues from wherever the camera
        # currently points (scripts, the transform view etc. can change it behind our back)
        o = self._cam.getTransform().getOrientation()
        q = QQuaternion(o.w, o.x, o.y, o.z)
        forward = q.rotatedVector(QVector3D(0, 0, -1))
        pitch = math.degrees(math.asin(max(-1.0, min(1.0, forward.y()))))

        if math.hypot(forward.x(), forward.z()) > 1e-4:
            yaw = math.degrees(math.atan2(-forward.x(), -forward.z()))
        else:
            # looking straight up/down: the up vector holds the heading instead
            up = q.rotatedVector(QVector3D(0, 1, 0))
            sign = 1.0 if forward.y() > 0.0 else -1.0
            yaw = math.degrees(math.atan2(sign * up.x(), sign * up.z()))

        return yaw, pitch

    def update(self, dt):
        cam = self._cam
        if not self._last_screen_delta.isNull():
            yaw, pitch = self._get_yaw_pitch()
            yaw += self._last_screen_delta.x() * self._rotation_speed.x()
            pitch += self._last_screen_delta.y() * self._rotation_speed.y()
            pitch = max(-self.MAX_PITCH, min(self.MAX_PITCH, pitch))
            roth = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), yaw)
            rotV = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), pitch)
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