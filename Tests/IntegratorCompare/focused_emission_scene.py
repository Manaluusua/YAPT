# Test scene for the cosine power (focused) emission profile.
#
# A closed white diffuse box, so the environment contributes nothing and every integrator
# has to transport the light itself, lit by a single small emissive plane below the ceiling
# aimed straight down. Exposure is pinned to manual so brightness can be compared between
# renders - auto exposure would normalise away exactly the differences these tests look for.
#
# Loadable straight from the app (File -> Run Script) or driven by compare_integrators.py /
# focus_sweep.py, which reach in for the "light_mat" this script leaves in its namespace.

import os

from py_yapt import Material, vec4, vec3
from PySide6.QtGui import QQuaternion, QVector3D

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

ASSET_ROOT = os.environ.get("YAPT_ASSET_ROOT", "D:/Random/3DSampleAssets")
TWO_SIDED = os.environ.get("TWO_SIDED", "0") == "1"

EMISSION_FOCUS = 8.0
ROOM_SIZE = 40.0


def axis_angle(axis, degrees, pre = None):
	rot = QQuaternion.fromAxisAndAngle(QVector3D(*axis), degrees)
	if pre is not None:
		rot = rot * pre
	return rot


def to_quat(rot):
	return vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])


def add_plane(name, mat, rot, pos, scale):
	obj = scene.create_render_object(name)
	obj.setMesh(plane_mesh)
	obj.setMaterial(mat, 0)
	obj.getTransform().setOrientation(to_quat(rot))
	obj.getTransform().setTranslation(pos)
	obj.getTransform().setScale(vec3(scale))
	return obj


plane_meshes, prim_mapping = resources.load_meshes_from_path(f"{ASSET_ROOT}/Plane/plane.glb")[0]
plane_mesh = plane_meshes[0]

#plain white diffuse for the whole room, so the falloff of the beam is easy to read
room_mat = renderer.createMaterial("room_mat")
room_mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
room_mat.setAlbedo(vec3([0.7, 0.7, 0.7]))
room_mat.setRoughness(1.0)
room_mat.setSpecularAmount(0.0)

half = ROOM_SIZE * 0.5

#closed box: floor, ceiling and four walls, all facing inwards. the walls are rotated -90
#about X so their normals end up pointing into the room, +90 would face them outwards and
#the box would leak.
add_plane("floor_obj", room_mat, axis_angle((1, 0, 0), 0), vec3([0, 0, 0]), ROOM_SIZE)
add_plane("ceiling_obj", room_mat, axis_angle((1, 0, 0), 180), vec3([0, ROOM_SIZE, 0]), ROOM_SIZE)
for i in range(4):
	rot = axis_angle((0, 1, 0), i * 90, QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -90))
	pos = vec3([0, half, half]) if i == 0 else vec3([half, half, 0]) if i == 1 else vec3([0, half, -half]) if i == 2 else vec3([-half, half, 0])
	add_plane(f"wall_obj_{i}", room_mat, rot, pos, ROOM_SIZE)

#emitter, flipped over to face the floor. a two sided one is moved to the middle of the
#room instead, so both of its faces have something to light up.
light_mat = renderer.createMaterial("focusedLight_mat")
light_mat.setEmission(vec3([8, 8, 8]))
light_mat.setEmissionFocus(EMISSION_FOCUS)
light_mat.setTwoSided(TWO_SIDED)
light_y = half if TWO_SIDED else ROOM_SIZE - 2
add_plane("focusedLight_obj", light_mat, axis_angle((1, 0, 0), 180), vec3([0, light_y, 0]), 6)

cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(vec3([0, 22, half - 2]))
cam_transform.setOrientation(to_quat(QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -25)))

#fixed exposure, so brightness can be compared between renders
renderer.getRendererVariable("Tonemap.UseAutoExposure").setSelectedOption(0)
renderer.getRendererVariable("Tonemap.ManualExposure").setFromFloatArray([1.0])

print(f"  scene loaded: EmissionFocus = {light_mat.getEmissionFocus()}, two sided = {TWO_SIDED}")
