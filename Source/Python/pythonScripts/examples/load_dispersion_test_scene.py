
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
    
    albedo_tex = resources.load_texture_2D("textures/weathered_planks/weathered_planks_diff_4k.png", "R8G8B8A8_SRGB", True)
    normal_tex = resources.load_texture_2D("textures/weathered_planks/weathered_planks_nor_gl_4k.png","R8G8B8A8_UNORM", True)

    obj, mesh, mat = createObject("groundPlane", "models/plane.glb", v3(0, -50, 0), 2000)
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
    center = vec3([-60, 20, 0])
    radius = 30
    emission_val = 200000
    
    obj, mesh, mat = createObject("lightPlane", "models/plane.glb", center, lightScale)
    
    rot = QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 115)
    rot =  QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), 90) * rot
    quat = vec4([rot.x(), rot.y(), rot.z(), rot.scalar()])
    
    obj.getTransform().setOrientation(quat);
    
    mat.setEmission(v3(emission_val, emission_val, emission_val))


#absorption coefficient (per world unit) that leaves `color` after travelling `distance` through the glass
def absorptionForColor(color, distance):
    return v3(*[-math.log(max(c, 1e-4)) / distance for c in color])

def loadObjects():

    #grid of goblets: columns (z) go from smooth to rough, rows (x) come in pairs of no dispersion / dispersion,
    #one pair per glass tint. The camera looks along -x, so the first pair is the front one
    columns = 5
    columnGap = 50
    rowGap = 35                         #between the rows of a pair
    pairGap = 55                        #between pairs
    frontX = 215
    absorptionDistance = 10             #goblets are ~50 units tall and ~24 wide
    cauchy = [1.5046, 0.00420]
    #IOR of the dispersive glass at 589 nm, so the rows of a pair differ only in dispersion
    plainIOR = cauchy[0] + cauchy[1] / (0.589 * 0.589)

    tints = [
        ("clear", None),
        ("turquoise", (0.55, 0.92, 0.85)),   #color after absorptionDistance
        ("green", (0.12, 0.55, 0.10)),
    ]

    x = frontX
    for pairIndex, (tintName, tintColor) in enumerate(tints):
        for dispersion in (False, True):
            for i in range(columns):
                z = -columnGap * math.floor(columns / 2) + i * columnGap
                name = f"Goblet_{tintName}_{'dispersion' if dispersion else 'plain'}_{i}"
                obj, mesh, mat = createObject(name, "models/Goblet/Goblet.gltf", v3(x, -50, z), 300)

                r = i / (columns - 1) if columns > 1 else 1
                r = r * r

                mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
                mat.setRoughness(r)
                mat.setEnableDispersion(dispersion)
                if dispersion:
                    mat.setCauchysCoefficients(vec2(cauchy))
                else:
                    mat.setDielectricIOR(plainIOR)
                if tintColor is not None:
                    mat.setAbsorption(absorptionForColor(tintColor, absorptionDistance))

            x -= rowGap
        x += rowGap - pairGap

#envmap
env_map = resources.load_texture_2D("textures/envmaps/photo_studio_loft_hall_4k.exr", "R16G16B16A16_SFLOAT")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

createSurrounding()
createLight()
loadObjects()
#look at the goblet grid head on from +x (towards the light) so the columns span the frame, from above so the floor shows
cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(vec3([375, 90, 0]));


cam_rot_q = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), 90) * QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), -24)
quat = vec4([cam_rot_q.x(), cam_rot_q.y(), cam_rot_q.z(), cam_rot_q.scalar()])
cam_transform.setOrientation(quat)