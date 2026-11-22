# Test scene for punctual (point / spot / directional) lights.
#
# An open floor with a back wall and a few diffuse and glossy shapes, lit only by punctual lights.
# The environment is scaled to zero, so all light comes from the lights under test. Every
# integrator reaches a punctual light only through explicit connections, and BDPT also starts light
# subpaths from it, so its pdfs and MIS terms are exercised on both ends.
#
# LIGHT_TYPE selects the lights: point, spot, directional, or mixed (all three plus an emissive
# sphere, which checks that mesh-light MIS still uses the right pick probability with punctual lights
# in the light list). mesh_control swaps the point light for a small emissive sphere, giving the
# baseline disagreement of the scene without any punctual light. env has no lights at all, the scene is lit by the
# (procedural) environment only, which BDPT then samples as its light source.

import os

from py_yapt import Material, Light, vec4, vec3
from PySide6.QtGui import QQuaternion, QVector3D

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

LIGHT_TYPE = os.environ.get("LIGHT_TYPE", "mixed")


def to_quat(rot):
	return vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])


def add_object(name, mesh, mat, pos, scale, rot = None):
	obj = scene.create_render_object(name)
	obj.setMesh(mesh)
	obj.setMaterial(mat, 0)
	if rot is not None:
		obj.getTransform().setOrientation(to_quat(rot))
	obj.getTransform().setTranslation(pos)
	obj.getTransform().setScale(vec3(scale))
	return obj


plane_mesh = resources.load_meshes_from_path("models/plane.glb")[0][0][0]
sphere_mesh = resources.load_meshes_from_path("models/Sphere.glb")[0][0][0]

diffuse_mat = renderer.createMaterial("diffuse_mat")
diffuse_mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
diffuse_mat.setAlbedo(vec3([0.7, 0.7, 0.7]))
diffuse_mat.setRoughness(1.0)
diffuse_mat.setSpecularAmount(0.0)

glossy_mat = renderer.createMaterial("glossy_mat")
glossy_mat.setFromMaterialPreset(Material.MaterialPreset.METAL_COPPER)
glossy_mat.setRoughness(0.35)

plastic_mat = renderer.createMaterial("plastic_mat")
plastic_mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
plastic_mat.setAlbedo(vec3([0.2, 0.4, 0.8]))
plastic_mat.setRoughness(0.25)

add_object("floor_obj", plane_mesh, diffuse_mat, vec3([0, 0, 0]), 40)
add_object("wall_obj", plane_mesh, diffuse_mat, vec3([0, 20, -20]), 40, QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 90))
add_object("sphere_diffuse", sphere_mesh, diffuse_mat, vec3([-6, 3, 0]), 3)
add_object("sphere_glossy", sphere_mesh, glossy_mat, vec3([2, 3, -4]), 3)
add_object("sphere_plastic", sphere_mesh, plastic_mat, vec3([7, 2, 3]), 2)


def add_light(name, light_type, position, target, color, intensity):
	light = scene.create_light(name, light_type)
	light.getTransform().lookAt(vec3(position), vec3(target), vec3([0, 1, 0]))
	light.setColor(vec3(color))
	light.setIntensity(intensity)
	return light


lights = []
if LIGHT_TYPE in ("point", "mixed"):
	lights.append(add_light("point_light", Light.LightType.POINT, [-2, 12, 6], [0, 0, 0], [1.0, 0.9, 0.8], 150.0))
if LIGHT_TYPE in ("spot", "mixed"):
	spot = add_light("spot_light", Light.LightType.SPOT, [10, 16, 10], [2, 0, -2], [0.8, 0.9, 1.0], 600.0)
	spot.setSpotConeAngles(0.3, 0.5)
	lights.append(spot)
if LIGHT_TYPE in ("directional", "mixed"):
	lights.append(add_light("sun_light", Light.LightType.DIRECTIONAL, [-6, 12, 8], [0, 0, 0], [1.0, 1.0, 1.0], 1.0))
if LIGHT_TYPE == "mesh_control":
	#small emissive sphere where the point light would be: the same scene lit only through the pre-existing mesh light path
	control_mat = renderer.createMaterial("control_emitter_mat")
	control_mat.setEmission(vec3([150, 135, 120]))
	add_object("control_emitter_obj", sphere_mesh, control_mat, vec3([-2, 12, 6]), 0.5)
if LIGHT_TYPE == "mixed":
	emitter_mat = renderer.createMaterial("emitter_mat")
	emitter_mat.setEmission(vec3([6, 3, 1]))
	add_object("emitter_obj", sphere_mesh, emitter_mat, vec3([-10, 6, -10]), 1.5)

cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(vec3([0, 14, 26]))
cam_transform.setOrientation(to_quat(QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -25)))

#no environment light (except in the env variant) and fixed exposure, so brightness can be compared between renders
renderer.getRendererVariable("World.EnvIntensityScale").setFromFloatArray([0.5 if LIGHT_TYPE == "env" else 0.0])
renderer.getRendererVariable("Tonemap.UseAutoExposure").setSelectedOption(0)
renderer.getRendererVariable("Tonemap.ManualExposure").setFromFloatArray([1.0])

print(f"  scene loaded: LIGHT_TYPE = {LIGHT_TYPE}, {len(lights)} punctual lights")
