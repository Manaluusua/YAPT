
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
    obj = scene.create_object(f"{name}_obj")
    mat = renderer.createMaterial(f"{name}_mat")
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setRoughness(0.2)
    

    obj.setMesh(mesh);
    obj.setMaterial(mat, 0);
    
    obj.getTransform().setScale(vec3(scale));
    obj.getTransform().setTranslation(pos);
    
    return obj, mesh, mat

def createGround():
    obj, mesh, mat = createObject("groundPlane", "D:/Random/3DSampleAssets/Plane/plane.glb", vec3([0, -50, 0]), 2000)
    mat.setRoughness(0.1)


def createLight():
    obj, mesh, mat = createObject("monkey", "D:/Random/3DSampleAssets/Suzanne/suzanne.glb", vec3([0, 60, 0]), 60)
    mat.setEmission(vec3([100, 100, 100]))

def letThereBeDragons():

    center = vec3([0, 0, 0])
    radius = 200

    count = 4
    

    for i in range(count):
        angle = i * (math.radians(360) / count)
        s = math.sin(angle)
        c = math.cos(angle)

        x,y,z = center[0] + c * radius, center[1], center[2] + s * radius
        print(f"x:{x}, y:{y}, z:{z}")
        obj, mesh, mat = createObject("dragon", "D:/Random/3DSampleAssets/Models/xyzrgb_dragon_decimated.glb", vec3([x, y, z]), 1)

        rot = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), math.degrees(angle))
        quat = vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])

        obj.getTransform().lookAt(vec3([x, y, z]), center, vec3([0, 1, 0]))
        
        if i % 4 == 0:
            mat.setFromMaterialPreset(Material.MaterialPreset.METAL_GOLD)
        elif i % 4 == 1:
            mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
            mat.setEnableDispersion(True)
            mat.setCauchysCoefficients(vec2([1.7280, 0.01342]))
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
env_map = resources.load_texture_from_path("D:/Random/3DSampleAssets/EnvMaps/env_room_studio_bgra8.dds")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)


createGround()
createLight()
letThereBeDragons()

scene.get_main_camera().getTransform().setTranslation(vec3([0, -5, 250]));
