
from py_yapt import Material, vec4, vec3, vec2
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

def createSurrounding():

    global resources
    global renderer
    global scene

    createWalls = True
    wallDistance = 450
    wallScale = 1000 
    
    obj, mesh, mat = createObject("groundPlane", "D:/Random/3DSampleAssets/Plane/plane.glb", vec3([0, -50, 0]), 2000)
    mat.setRoughness(0.1)

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

def createLight():
    obj, mesh, mat = createObject("monkey", "D:/Random/3DSampleAssets/Suzanne/suzanne.glb", vec3([0, 100, 0]), 10)
    mat.setEmission(vec3([1000, 1000, 1000]))

def letThereBeDragons():

    center = vec3([0, 0, 0])
    radius = 200

    count = 4
    

    for i in range(count):
        angle = i * (math.radians(360) / count)
        s = math.sin(angle)
        c = math.cos(angle)

        x,y,z = center[0] + c * radius, center[1], center[2] + s * radius

        obj, mesh, mat = createObject("dragon", "D:/Random/3DSampleAssets/Models/xyzrgb_dragon_decimated.glb", vec3([x, y, z]), 1)

        rot = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), math.degrees(angle))
        quat = vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])

        obj.getTransform().lookAt(vec3([x, y, z]), center, vec3([0, 1, 0]))
        
        if i % 4 == 0:
            mat.setFromMaterialPreset(Material.MaterialPreset.METAL_GOLD)
        elif i % 4 == 1:
            mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
            mat.setEnableDispersion(True)
            mat.setCauchysCoefficients(vec2([1.5046, 0.00420]))
            mat.setRoughness(0.03)
            mat.setAbsorption(vec3([0.02, 0.01, 0.03]))
            pass
        elif i % 4 == 2:
            mat.setSheenAmount(1)
            mat.setSheenTint( vec3([0.4, 0.4, 1.0]))
            pass
        elif i % 4 == 3:
            mat.setFromMaterialPreset(Material.MaterialPreset.METAL_COPPER)
            mat.setRoughness(0.0001)

    

#envmap
env_map = resources.load_texture_2D("D:/Random/3DSampleAssets/EnvMapSources/photo_studio_loft_hall_4k.exr", "R16G16B16A16_SFLOAT")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

createSurrounding()
createLight()
letThereBeDragons()
cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(vec3([300, 40, 300]));


cam_rot_q = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), 45) * QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -20)
quat = vec4([cam_rot_q.x(), cam_rot_q.y(), cam_rot_q.z(), cam_rot_q.scalar()])
cam_transform.setOrientation(quat)