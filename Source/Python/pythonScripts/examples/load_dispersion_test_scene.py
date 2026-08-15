
from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
import math

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

def createObject(name, meshPath, pos, scale):
    global resources
    global renderer
    global scene

    mesh_array, prim_mapping = resources.load_meshes_from_path(meshPath)[0]
    obj = scene.create_render_object(f"{name}_obj")
    mat = renderer.createMaterial(f"{name}_mat")
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setRoughness(0.2)
    
    mesh = mesh_array[0]

    obj.setMesh(mesh_array[0]);
    obj.setMaterial(mat, 0);
    
    obj.getTransform().setScale(vec3(scale));
    obj.getTransform().setTranslation(pos);
    
    return obj, mesh, mat


def createSurrounding():

    global resources
    global renderer
    global scene

    createWalls = False
    createRoof = False
    wallDistance = 450
    roofDistance = 450
    wallScale = 1000 
    roofScale = 1000
    
    albedo_tex = resources.load_texture_2D("D:/Random/3DSampleAssets/Textures/weathered_planks/weathered_planks_diff_4k.png", "R8G8B8A8_SRGB", True)
    normal_tex = resources.load_texture_2D("D:/Random/3DSampleAssets/Textures/weathered_planks/weathered_planks_nor_gl_4k.png","R8G8B8A8_UNORM", True)

    obj, mesh, mat = createObject("groundPlane", "D:/Random/3DSampleAssets/Plane/plane.glb", v3(0, -50, 0), 2000)
    tex_scale = 45
    mat.setRoughness(0.5)
    #mat.setClearCoatAmount(1)
    mat.setAlbedoTexture(albedo_tex, v2(tex_scale, tex_scale))
    mat.setNormalTexture(normal_tex, v2(tex_scale, tex_scale))

    if(createWalls):
        for i in range(4):
            wall = scene.create_render_object(f"wallPlane_obj_{i}")
            wall.setMesh(mesh);
            wall.setMaterial(mat, 0);

            rot = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 90)
            rot =  QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), i * 90) * rot
            quat = vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])
            
            offset = wallDistance
            pos = None
            if(i < 2):
                offset = -offset;

            if(i % 2 == 1):
                pos = vec3([offset, 0, 0])
               
            else:
                 pos = vec3([0, 0, offset])

            wall.getTransform().setOrientation(quat);
            wall.getTransform().setTranslation(pos)
            wall.getTransform().setScale(vec3(wallScale));

    if(createRoof):
            roof = scene.create_render_object(f"roofPlane_obj_{i}")
            roof.setMesh(mesh);
            roof.setMaterial(mat, 0);

            rot = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 180)
            quat = vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])
            
            offset = roofDistance
            pos = v3(0, offset, 0)

            roof.getTransform().setOrientation(quat);
            roof.getTransform().setTranslation(pos)
            roof.getTransform().setScale(vec3(roofScale));

def createLight():

    global resources
    global renderer
    global scene

    lightScale = 2
    center = vec3([-30, 20, 0])
    radius = 30
    emission_val = 200
    
    obj, mesh, mat = createObject("lightPlane", "D:/Random/3DSampleAssets/Plane/plane.glb", center, lightScale)
    
    rot = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 115)
    rot =  QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), 90) * rot
    quat = vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])
    
    obj.getTransform().setOrientation(quat);
    
    mat.setEmission(v3(emission_val, emission_val, emission_val))


def loadObjects():

    center = vec3([0, -50, 0])

    angle = 0
    s = math.sin(angle)
    c = math.cos(angle)
    
    obj, mesh, mat = createObject("Goblet", "D:/Random/3DSampleAssets/Models/Goblet.gltf", center, 300)
    
    rot = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), math.degrees(angle))
    
    quat = v4(rot.x(), rot.y(), rot.z(), rot.scalar())
    
    obj.getTransform().setOrientation(quat)

    mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
    mat.setEnableDispersion(True)
    mat.setCauchysCoefficients(vec2([1.5046, 0.00420]))
    mat.setRoughness(0.0)

#envmap
env_map = resources.load_texture_2D("D:/Random/3DSampleAssets/EnvMapSources/photo_studio_loft_hall_4k.exr", "R16G16B16A16_SFLOAT")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

createSurrounding()
createLight()
loadObjects()
cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(vec3([40, 70, 80]));


cam_rot_q = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), 0) * QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -40)
quat = vec4([cam_rot_q.x(), cam_rot_q.y(), cam_rot_q.z(), cam_rot_q.scalar()])
cam_transform.setOrientation(quat)