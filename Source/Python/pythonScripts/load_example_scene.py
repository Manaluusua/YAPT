import imageio as iio
from pathlib import Path
from yapt.texture_helper import TextureHelper

resources = yapt_instance.get_resources()


#envmap
env_map_path = Path("D:/Random/3DSampleAssets/EnvMaps/env_room_studio_bgra8.dds")
env_map = iio.imread(str(env_map_path))
print(f"Image shape: {env_map.shape}")
print(f"Image data type: {env_map.dtype}")

#sphere

#scene setup


print(yapt_instance)