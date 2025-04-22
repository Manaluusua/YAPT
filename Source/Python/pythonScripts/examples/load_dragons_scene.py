
from py_yapt import Material, vec3

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

def createObject(name, meshPath, pos):
    global resources
    global renderer
    global scene

    mesh = resources.load_meshes_from_path(meshPath)[0]
    obj = scene.create_object(f"{name}_obj")
    mat = renderer.createMaterial(f"{name}_mat")
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setRoughness(0.5)
    
    obj.setMesh(mesh);
    obj.setMaterial(mat, 0);
    
    obj.getTransform().setScale(vec3(100));
    obj.getTransform().setTranslation(pos);
    
    return obj, mesh, mat

#envmap
env_map = resources.load_texture_from_path("D:/Random/3DSampleAssets/EnvMaps/env_room_studio_bgra8.dds")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

createObject("groundPlane", "D:/Random/3DSampleAssets/Plane/plane.glb", vec3([0, -50, 0]))

scene.get_main_camera().getTransform().setTranslation(vec3([0, -5, 55]));
