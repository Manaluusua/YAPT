
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

    mesh = resources.load_meshes_from_path(meshPath)[0]
    obj = scene.create_render_object(f"{name}_obj")
    mat = renderer.createMaterial(f"{name}_mat")
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setRoughness(0.2)
    

    obj.setMesh(mesh);
    obj.setMaterial(mat, 0);
    
    obj.getTransform().setScale(vec3(scale));
    obj.getTransform().setTranslation(pos);
    
    return obj, mesh, mat

def setupMaterialBallMaterials(obj, matOuter, matInner):
    global resources
    global renderer
    global scene

    obj.setMaterial(matOuter, 0)
    obj.setMaterial(matInner, 1)
    obj.setMaterial(matOuter, 2)
    obj.setMaterial(matInner, 3)
    obj.setMaterial(matOuter, 4)

def createSurrounding():

    global resources
    global renderer
    global scene

    createWalls = False
    wallDistance = 450
    wallScale = 1000 
    
    albedo_tex = resources.load_texture_from_path("D:/Random/3DSampleAssets/Textures/weathered_planks_diff_4k.dds", False, True)
    normal_tex = resources.load_texture_from_path("D:/Random/3DSampleAssets/Textures/weathered_planks_nor_gl_4k.dds")

    obj, mesh, mat = createObject("groundPlane", "D:/Random/3DSampleAssets/Plane/plane.glb", v3(0, -50, 0), 2000)
    tex_scale = 45
    mat.setRoughness(0.5)
    #mat.setClearCoatAmount(1)
    mat.setAlbedoTexture(albedo_tex, v2(tex_scale, tex_scale))
    mat.setNormalTexture(normal_tex, v2(tex_scale, tex_scale))

    if(createWalls):
        for i in range(4):
            wall = scene.create_object(f"wallPlane_obj_{i}")
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

def createLight():
    obj, mesh, mat = createObject("monkey", "D:/Random/3DSampleAssets/Torus/torus.glb", v3(0, 10, 0), 30)

    #rot = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 240)
    #obj.getTransform().setOrientation(v4(rot.x(), rot.y(), rot.z(), rot.scalar()))
    emission_val = 200
    mat.setEmission(v3(emission_val, emission_val, emission_val))

def loadMaterialBalls():

    center = vec3([0, -50, 0])
    radius = 200

    count = 4
    

    for i in range(count):
        angle = math.radians(45) + i * (math.radians(360) / count)
        s = math.sin(angle)
        c = math.cos(angle)

        x,y,z = center[0] + c * radius, center[1], center[2] + s * radius

        obj, mesh, mat = createObject("materialBall", "D:/Random/3DSampleAssets/MaterialBall/materialball.glb", vec3([x, y, z]), 0.3)
        
        mat2 = renderer.createMaterial(f"materialBall_mat2")
        mat2.setFromMaterialPreset(Material.MaterialPreset.METAL_ALUMINIUM)
        mat2.setRoughness(0.2)
        mat2.setTwoSided(True)
        setupMaterialBallMaterials(obj, mat, mat2)


        roty = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), math.degrees(angle))
        rotx = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 90)
        rot = roty * rotx
        quat = v4(rot.x(), rot.y(), rot.z(), rot.scalar())

        obj.getTransform().setOrientation(quat)
        
        if i == 0:
            mat.setFromMaterialPreset(Material.MaterialPreset.METAL_GOLD)
            pass
        elif i == 1:
            mat.setSheenAmount(1)
            mat.setSheenTint(vec3([0.4, 0.4, 1.0]))
            pass
        elif i == 2:
            mat.setFromMaterialPreset(Material.MaterialPreset.METAL_COPPER)
            mat.setRoughness(0.0001)
        elif i == 3:
            mat.setClearCoatAmount(1)
            mat.setClearCoatIOR(1.5) #oil
            mat.setDielectricIOR(1.3) #water
            mat.setClearCoatRoughness(0.01)
            mat.setRoughness(0.01)
            mat.setThinFilmThicknessNM(351)
            mat.setAlbedo(v3(0.0, 0.0, 0.0))

def loadSimpleBalls():

    center = vec3([0, -20, 0])
    radius = 200

    count = 4
    

    for i in range(count):
        angle = i * (math.radians(360) / count)
        s = math.sin(angle)
        c = math.cos(angle)

        x,y,z = center[0] + c * radius, center[1], center[2] + s * radius

        obj, mesh, mat = createObject("sphere", "D:/Random/3DSampleAssets/Sphere/sphere.glb", vec3([x, y, z]), 30)

        rot = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), math.degrees(angle))

        quat = v4(rot.x(), rot.y(), rot.z(), rot.scalar())

        obj.getTransform().setOrientation(quat)
        
        if i == 0:
            mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
            mat.setRoughness(0.4)
            mat.setAbsorption(vec3([0.05, 0.05, 0.004]))
            pass
        elif i == 1:
            mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
            mat.setEnableDispersion(True)
            mat.setCauchysCoefficients(vec2([1.5046, 0.00420]))
            mat.setRoughness(0.0)
            pass
        elif i == 2:
            mat.setFromMaterialPreset(Material.MaterialPreset.METAL_COPPER)
            mat.setRoughness(0.4)
        elif i == 3:
            mat.setClearCoatAmount(1)
            mat.setClearCoatIOR(1.7) #oil
            mat.setDielectricIOR(1.4) #water
            mat.setClearCoatRoughness(0.2)
            mat.setRoughness(0.21)
            mat.setThinFilmThicknessNM(742)
            mat.setMetalness(1.0)
            mat.setAlbedo(v3(0.0, 0.3, 0.0))
            
    

#envmap
env_map = resources.load_texture_from_path("D:/Random/3DSampleAssets/EnvMaps/env_room_studio_bgra8.dds")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

createSurrounding()
createLight()
loadMaterialBalls()
loadSimpleBalls()
cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(vec3([300, 40, 300]));


cam_rot_q = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), 45) * QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -20)
quat = vec4([cam_rot_q.x(), cam_rot_q.y(), cam_rot_q.z(), cam_rot_q.scalar()])
cam_transform.setOrientation(quat)