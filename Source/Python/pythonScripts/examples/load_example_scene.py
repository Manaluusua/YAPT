
from yapt.resources import Resources

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()

#envmap
env_map = resources.load_texture_from_path("D:/Random/3DSampleAssets/EnvMaps/env_room_studio_bgra8.dds")
print(env_map.getName())
if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

#meshes
plane_mesh = resources.load_meshes_from_path("D:/Random/3DSampleAssets/Plane/plane.glb")
sphere_mesh = resources.load_meshes_from_path("D:/Random/3DSampleAssets/Sphere/sphere.glb")
torus_mesh = resources.load_meshes_from_path("D:/Random/3DSampleAssets/Torus/torus.glb")


#scene setup


print(yapt_instance)