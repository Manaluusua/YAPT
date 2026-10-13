from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
import math

#Spheres with the same albedo and increasing subsurface amount. While the subsurface profile is still stubbed (exit point == entry point,
#lambertian exit lobe), all of them should look like the plain diffuse one, and the unidirectional and bidirectional integrators should agree.

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

def createObject(name, meshPath, pos, scale):
    mesh_array, prim_mapping = resources.load_meshes_from_path(meshPath)[0]
    obj = scene.create_render_object(f"{name}_obj")
    mat = renderer.createMaterial(f"{name}_mat")
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setRoughness(0.2)

    obj.setMesh(mesh_array[0])
    obj.setMaterial(mat, 0)

    obj.getTransform().setScale(vec3(scale))
    obj.getTransform().setTranslation(pos)

    return obj, mesh_array[0], mat

def createGround():
    obj, mesh, mat = createObject("groundPlane", "models/plane.glb", v3(0, -30, 0), 2000)
    mat.setRoughness(0.5)
    mat.setAlbedo(v3(0.5, 0.5, 0.5))

def createLight():
    obj, mesh, mat = createObject("lightPlane", "models/plane.glb", v3(0, 150, 100), 40)
    mat.setEmission(v3(20, 20, 20))

    #plane normal is +y, tilt it down towards the spheres
    rot = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -150)
    obj.getTransform().setOrientation(vec4([rot.x(), rot.y(), rot.z(), rot.scalar()]))

def createSpheres():
    amounts = [0.0, 0.5, 1.0]
    spacing = 80
    for i, amount in enumerate(amounts):
        x = (i - (len(amounts) - 1) * 0.5) * spacing
        obj, mesh, mat = createObject(f"sssSphere{i}", "models/Sphere.glb", v3(x, 0, 0), 30)
        mat.setAlbedo(v3(0.8, 0.35, 0.25))
        mat.setRoughness(0.3)
        mat.setSubsurfaceAmount(amount)
        mat.setSubsurfaceRadius(v3(1.0, 0.2, 0.1))
        mat.setSubsurfaceRadiusScale(5.0)

env_map = resources.load_texture_2D("textures/envmaps/photo_studio_loft_hall_4k.exr", "R16G16B16A16_SFLOAT")
if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

createGround()
createLight()
createSpheres()

cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(vec3([0, 40, 250]))
cam_rot_q = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -10)
cam_transform.setOrientation(vec4([cam_rot_q.x(), cam_rot_q.y(), cam_rot_q.z(), cam_rot_q.scalar()]))
