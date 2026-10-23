
from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
import math

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

ROW_CENTER = (0, -50, 0)       #on the floor
OBJECT_COUNT = 4
OBJECT_SPACING = 100           #goblets are ~27 units tall and ~14 wide
OBJECT_RADIUS = 17             #what the light cone has to cover
GOBLET_SCALE = 150             #goblet.glb is in meters, 0.18 tall
LIGHT_DISTANCE = 100           #from the goblet
LIGHT_ELEVATION = 50           #degrees above the floor, seen from the goblet
LIGHT_SIZE = 3                 #edge length of the square panels
LIGHT_EDGE_FALLOFF = 0.25      #emission at the edge of the goblet relative to the center of the cone
LIGHT_EMISSION = 8000
ENV_INTENSITY = 0.1
#Cauchy coefficients of SF10 dense flint, about 3x the dispersion of BK7
CAUCHY = [1.7280, 0.01342]

CAM_POS = (-178, -11, 54)
CAM_TARGET = (-122, -50, 5)

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

def createSpotLight(name, target, awayFromCamera):

    #small panel behind the goblet (seen from the camera) and above it, shining at it with a cone that barely covers it
    elevation = math.radians(LIGHT_ELEVATION)
    offset = awayFromCamera * (math.cos(elevation) * LIGHT_DISTANCE) + QVector3D(0, 1, 0) * (math.sin(elevation) * LIGHT_DISTANCE)
    pos = QVector3D(*target) + offset

    #plane.glb is 2x2 units with its front (emitting) side towards +y
    obj, mesh, mat = createObject(name, "models/plane.glb", v3(pos.x(), pos.y(), pos.z()), LIGHT_SIZE * 0.5)
    mat.setAlbedo(v3(0, 0, 0))
    mat.setEmission(v3(LIGHT_EMISSION, LIGHT_EMISSION, LIGHT_EMISSION))
    coneHalfAngle = math.atan(OBJECT_RADIUS / LIGHT_DISTANCE)
    mat.setEmissionFocus(math.log(LIGHT_EDGE_FALLOFF) / math.log(math.cos(coneHalfAngle)))

    rot = QQuaternion.rotationTo(QVector3D(0, 1, 0), -offset.normalized())
    obj.getTransform().setOrientation(vec4([rot.x(), rot.y(), rot.z(), rot.scalar()]))

def createGoblet(name, pos, glassMat, rimMat):

    #goblet.glb has the glass body as its first primitive and the gold rim as its second
    mesh_array, prim_mapping = resources.load_meshes_from_path("models/goblet.glb")[0]
    objects = []
    for mesh_index, mesh in enumerate(mesh_array):
        obj = scene.create_render_object(f"{name}_obj_{mesh_index}")
        obj.setMesh(mesh)
        obj.getTransform().setScale(vec3(GOBLET_SCALE))
        obj.getTransform().setTranslation(pos)
        objects.append(obj)

    for prim_index, mat in enumerate((glassMat, rimMat)):
        mesh_index, submesh_index = prim_mapping[prim_index]
        objects[mesh_index].setMaterial(mat, submesh_index)

def loadObjects():

    rimMat = renderer.createMaterial("GobletRim_mat")
    rimMat.setFromMaterialPreset(Material.MaterialPreset.METAL_GOLD)
    rimMat.setRoughness(0.15)

    #clear dispersive glass goblets in a row along x, roughness going from fully smooth to fully rough
    for i in range(OBJECT_COUNT):
        x = ROW_CENTER[0] + (i - (OBJECT_COUNT - 1) / 2) * OBJECT_SPACING
        pos = (x, ROW_CENTER[1], ROW_CENTER[2])

        r = i / (OBJECT_COUNT - 1) if OBJECT_COUNT > 1 else 1

        mat = renderer.createMaterial(f"Goblet_{i}_mat")
        mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
        mat.setRoughness(r)
        mat.setEnableDispersion(True)
        mat.setCauchysCoefficients(vec2(CAUCHY))
        createGoblet(f"Goblet_{i}", v3(*pos), mat, rimMat)

        toCamera = QVector3D(CAM_POS[0] - pos[0], 0, CAM_POS[2] - pos[2]).normalized()
        center = (pos[0], pos[1] + 0.09 * GOBLET_SCALE, pos[2])
        createSpotLight(f"Goblet_{i}_light", center, -toCamera)

#envmap
env_map = resources.load_texture_2D("textures/envmaps/photo_studio_loft_hall_4k.exr", "R16G16B16A16_SFLOAT")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

env_var = renderer.getRendererVariable("World.EnvIntensityScale")
if(env_var != None):
    env_var.setFromFloatArray([ENV_INTENSITY])

createSurrounding()
loadObjects()
dx, dy, dz = (CAM_TARGET[i] - CAM_POS[i] for i in range(3))
#camera looks down -z: yaw about y, then pitch about x
yaw = math.degrees(math.atan2(-dx, -dz))
pitch = math.degrees(math.atan2(dy, math.hypot(dx, dz)))

cam_transform = scene.get_main_camera().getTransform()
cam_transform.setTranslation(v3(*CAM_POS))

cam_rot_q = QQuaternion.fromAxisAndAngle(QVector3D(0, 1, 0), yaw) * QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), pitch)
quat = vec4([cam_rot_q.x(), cam_rot_q.y(), cam_rot_q.z(), cam_rot_q.scalar()])
cam_transform.setOrientation(quat)
