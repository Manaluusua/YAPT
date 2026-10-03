"""
Still life on the vintage cabinet: the cabinet stands against the back wall of a closed room and all the converted
sample objects are arranged on its counter. The sky is black, the room is lit by two sphere lights (and the pipe lamp's bulb and the lantern's flame).

  - "objects" picks what goes on the cabinet; plain names or dicts with extra options:
        {"name": "brass_vase", "yaw": 30.0, "surfaces": ["counter_front"]}
    names come from PLACEABLES in yapt/container_scene.py (add new assets there)
  - the cabinet's surfaces are filled in order: "counter_niche" (under the upper hutch, max 0.34 m tall objects),
    "counter_front" (in front of the hutch, open above) and the glass door shelves "upper_shelf_1..3"
    (only ~0.2 m of room, none of the current objects fit); "arrange" -> "surfaces" limits which ones are used
  - objects are packed in rows from the back, tallest first; what doesn't fit is skipped (see console)
  - SEED only drives the small random turn of each object (yaw_jitter); env var YAPT_SCENE_SEED overrides it

Run it via the "run script" action of the app on an empty scene; objects are added, nothing is cleared.
Asset paths are relative to ASSET_PATH ([paths] asset_path in settings.cfg / settings_local.cfg), absolute paths work too.
Units are metres.
"""

from yapt.container_scene import ContainerSceneBuilder

SEED = None

CONFIG = {
    "container": "vintage_cabinet",
    "objects": [
        "brass_vase",
        "industrial_pipe_lamp",
        "wine_bottles",
        "lantern",
        "brass_goblets",
    ],

    "arrange": {
        "surfaces": ["counter_niche", "counter_front"],
        "gap": 0.03,                # minimum space between objects
        "edge_margin": 0.01,        # keep away from the edges of the surface
        "yaw_jitter": 12.0,         # degrees, random turn per object
        "tall_to_back": True,
        "allow_rotate": True,       # turn objects 90 degrees if that makes them fit
    },

    "room": {
        "size": (4.2, 4.2, 2.9),    # width (x), depth (z), height
        "container_wall_gap": 0.03,
        "floor_albedo_texture": "textures/weathered_planks/weathered_planks_diff_4k.png",
        "floor_normal_texture": "textures/weathered_planks/weathered_planks_nor_gl_4k.png",
        "floor_texture_size": 2.0,  # metres per texture repeat
        "wall_color": (0.78, 0.75, 0.70),
        "ceiling_color": (0.85, 0.85, 0.85),
    },

    "env_intensity": 0.0,           # World.EnvIntensityScale, 0 = black sky

    # sphere lights, position in room space (floor centre = origin, back wall at z = -depth / 2)
    # irradiance is what each light alone gives at the camera focus point
    "lights": [
        {"position": (-1.2, 2.5, -0.6), "radius": 0.2, "irradiance": 4.0, "tint": (1.0, 0.9, 0.78)},
        {"position": (1.4, 2.2, 0.8), "radius": 0.25, "irradiance": 1.2, "tint": (0.85, 0.92, 1.0)},
    ],

    "camera": {
        "frame": "objects",         # "objects" = the counter still life, "container" = the whole cabinet
        "elevation": 18.0,          # degrees above the focus point, i.e. looking slightly down
        "azimuth": 0.0,             # degrees around the focus point, 0 = straight in front
        "distance": None,           # None = fit the framed bounds
        "frame_margin": 1.2,
        "focus_height": 0.4,        # where to aim within the framed bounds, 0 = bottom, 1 = top
        "fov": 40.0,                # vertical, degrees
        "near": 0.01,
        "move_speed": 1.0,          # fly camera speed, metres / second
    },
}

ContainerSceneBuilder(yapt_instance, CONFIG, SEED, log_tag="cabinet_showcase").build()
