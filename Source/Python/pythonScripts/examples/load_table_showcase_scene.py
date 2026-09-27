"""
Still life on the gallinera table: the table stands against the back wall of a closed room and all the converted
sample objects are arranged on its top. The sky is black, the room is lit by two sphere lights (and the pipe lamp's bulb).

  - "objects" picks what goes on the table; plain names or dicts with extra options:
        {"name": "industrial_pipe_lamp", "yaw": 30.0, "surfaces": ["top"]}
    names come from PLACEABLES in yapt/container_scene.py (add new assets there)
  - objects are packed in rows from the back of the table, tallest first; what doesn't fit is skipped (see console)
  - SEED only drives the small random turn of each object (yaw_jitter); env var YAPT_SCENE_SEED overrides it

Run it via the "run script" action of the app on an empty scene; objects are added, nothing is cleared.
Units are metres.
"""

from yapt.container_scene import ContainerSceneBuilder

SEED = None

ASSET_ROOT = "D:/Random/3DSampleAssets"

CONFIG = {
    "asset_root": ASSET_ROOT,
    "converted_root": f"{ASSET_ROOT}/Converted",

    "container": "gallinera_table",
    "objects": [
        "brass_vase",
        "industrial_pipe_lamp",
        "wine_bottles",
        "lantern",
        "brass_goblets",
    ],

    "arrange": {
        "gap": 0.015,               # minimum space between objects
        "edge_margin": 0.01,        # keep away from the edges of the surface
        "yaw_jitter": 12.0,         # degrees, random turn per object
        "tall_to_back": True,
        "allow_rotate": True,       # turn objects 90 degrees if that makes them fit
    },

    "room": {
        "size": (3.6, 3.6, 2.7),    # width (x), depth (z), height
        "container_wall_gap": 0.03,
        "floor_albedo_texture": f"{ASSET_ROOT}/Textures/weathered_planks/weathered_planks_diff_4k.png",
        "floor_normal_texture": f"{ASSET_ROOT}/Textures/weathered_planks/weathered_planks_nor_gl_4k.png",
        "floor_texture_size": 2.0,  # metres per texture repeat
        "wall_color": (0.78, 0.75, 0.70),
        "ceiling_color": (0.85, 0.85, 0.85),
    },

    "env_intensity": 0.0,           # World.EnvIntensityScale, 0 = black sky

    # sphere lights, position in room space (floor centre = origin, back wall at z = -depth / 2)
    # irradiance is what each light alone gives at the camera focus point
    "lights": [
        {"position": (-0.9, 2.2, -0.5), "radius": 0.2, "irradiance": 4.0, "tint": (1.0, 0.9, 0.78)},
        {"position": (1.2, 1.8, 0.6), "radius": 0.25, "irradiance": 1.2, "tint": (0.85, 0.92, 1.0)},
    ],

    "camera": {
        "frame": "container",       # "container" = table + objects, "objects" = just the objects
        "elevation": 22.0,          # degrees above the focus point, i.e. looking slightly down
        "azimuth": 0.0,             # degrees around the focus point, 0 = straight in front
        "distance": None,           # None = fit the framed bounds
        "frame_margin": 1.1,
        "focus_height": 0.5,        # where to aim within the framed bounds, 0 = bottom, 1 = top
        "fov": 40.0,                # vertical, degrees
        "near": 0.01,
        "move_speed": 1.0,          # fly camera speed, metres / second
    },
}

ContainerSceneBuilder(yapt_instance, CONFIG, SEED, log_tag="table_showcase").build()
