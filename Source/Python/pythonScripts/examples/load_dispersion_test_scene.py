
from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
import math

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

LIGHT_POS = (0, 20, 0)
LIGHT_RADIUS = 2

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

    #small sphere light in the middle of the goblet groups, so it shines on every group the same way
    emission_val = 200000

    obj, mesh, mat = createObject("lightSphere", "models/sphere.glb", v3(*LIGHT_POS), LIGHT_RADIUS)
    mat.setEmission(v3(emission_val, emission_val, emission_val))


#absorption coefficient (per world unit) that leaves `color` after travelling `distance` through the glass
def absorptionForColor(color, distance):
    return v3(*[-math.log(max(c, 1e-4)) / distance for c in color])

def loadObjects():

    #one group per glass tint, each in its own direction around the light. A group is two arcs facing the light:
    #no dispersion on the inner arc, dispersion on the outer one, and along each arc the goblets go from smooth to rough.
    #The caustics fall away from the light, so the outer arc is shifted by half a step to put its goblets between
    #the caustics (and shadows) of the inner arc instead of on top of them
    columns = 5
    innerRadius = 130                   #distance of the no dispersion arc from the light
    outerRadius = 230                   #distance of the dispersion arc from the light
    groupGap = 20                       #degrees of empty floor between groups
    firstGroupAngle = 0                 #degrees, 0 = +x (towards the default camera), counter clockwise seen from above
    absorptionDistance = 10             #goblets are ~50 units tall and ~24 wide
    cauchy = [1.5046, 0.00420]
    #IOR of the dispersive glass at 589 nm, so the rows of a pair differ only in dispersion
    plainIOR = cauchy[0] + cauchy[1] / (0.589 * 0.589)

    tints = [
        ("clear", None),
        ("turquoise", (0.55, 0.92, 0.85)),   #color after absorptionDistance
        ("green", (0.12, 0.55, 0.10)),
    ]

    groupSpan = 360 / len(tints)
    columnStep = (groupSpan - groupGap) / columns     #degrees between the goblets of an arc

    for groupIndex, (tintName, tintColor) in enumerate(tints):
        groupCenter = firstGroupAngle + groupIndex * groupSpan
        for dispersion in (False, True):
            radius = outerRadius if dispersion else innerRadius
            stagger = 0.5 if dispersion else 0.0
            for i in range(columns):
                angle = math.radians(groupCenter + (i - (columns - 1) / 2 - 0.25 + stagger) * columnStep)
                x = LIGHT_POS[0] + math.cos(angle) * radius
                z = LIGHT_POS[2] - math.sin(angle) * radius
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

#envmap
env_map = resources.load_texture_2D("textures/envmaps/photo_studio_loft_hall_4k.exr", "R16G16B16A16_SFLOAT")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

createSurrounding()
createLight()
loadObjects()
#from high up on the +x side, steep enough that the caustics of the groups on the far side show between the goblets
cam_pos = (480, 480, 0)
cam_target = (0, -50, 0)
dx, dy, dz = (cam_target[i] - cam_pos[i] for i in range(3))
#camera looks down -z: yaw about y, then pitch about x
yaw = math.degrees(math.atan2(-dx, -dz))
pitch = math.degrees(math.atan2(dy, math.hypot(dx, dz)))

cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(v3(*cam_pos))

cam_rot_q = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), yaw) * QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), pitch)
quat = vec4([cam_rot_q.x(), cam_rot_q.y(), cam_rot_q.z(), cam_rot_q.scalar()])
cam_transform.setOrientation(quat)
