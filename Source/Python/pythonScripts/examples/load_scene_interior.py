
from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
from yapt.scene_loader import SceneLoader
import math

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

sceneLoader = SceneLoader(resources)
sceneLoader.loadSceneGLTF("D:/Random/3DSampleAssets/SceneIndoorTable/indoor_coffee.glb")
