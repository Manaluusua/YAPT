import imageio as iio
from pathlib import Path
from yapt.texture_helper import TextureHelper

alloc_pool = yapt_instance.get_renderer().createAllocationPool()
tex_helper = TextureHelper(alloc_pool)

#envmap
env_map_path = Path("D:/Random/3DSampleAssets/EnvMaps/env_room_studio_bgra8.dds")
env_map = iio.imread(str(env_map_path))
print(f"Image shape: {env_map.shape}")
print(f"Image data type: {env_map.dtype}")
tex_helper.add_texture(env_map)
#sphere

#scene setup

alloc_pool.allocate()
print(yapt_instance)