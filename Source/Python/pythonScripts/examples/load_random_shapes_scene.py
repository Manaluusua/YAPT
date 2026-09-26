"""
Randomized shape playground.

Builds a scene out of the plane, sphere, torus and suzanne sample meshes:
  - a large plane as the floor
  - a heap of shapes piled on top of each other (dropped + "rolled" into place)
  - a few neat towers of alternating tori and spheres, topped with a suzanne or a ball
  - a showcase grid: one column per material family, rows of sphere / suzanne / upright torus
  - loose shapes scattered around the floor (with small sphere-in-torus combos)
  - sphere lights only: big key lights up high plus small glowing orbs down in the scene

Everything random goes through one seeded RNG, so:
  - SEED = None   -> new scene every run, the seed is printed to the console
  - SEED = 1234   -> reproduces that scene exactly
  - env var YAPT_SCENE_SEED overrides SEED (handy for generating batches)
Tweak CONFIG below for the ranges; (a, b) tuples are sampled per scene, ints -> randint, floats -> uniform.
The builder itself lives in yapt/random_shapes.py (shared with load_random_shapes_large_scene.py).

Run it via the "run script" action of the app on an empty scene; objects are added, nothing is cleared.
"""

from yapt.random_shapes import RandomShapesSceneBuilder, resolve_seed

# ---------------------------------------------------------------------------
# Knobs
# ---------------------------------------------------------------------------

SEED = None

ASSET_ROOT = "D:/Random/3DSampleAssets"

CONFIG = {
    "asset_root": ASSET_ROOT,

    # heap of shapes stacked on each other
    "pile_count": (30, 48),
    "pile_radius": (22.0, 34.0),          # spread of the drop positions
    "pile_scale": (5.0, 12.0),
    "pile_roll_steps": (8, 16),           # higher = objects roll further off each other -> flatter heap

    # neat towers next to the pile
    "tower_count": (1, 3),
    "tower_pieces": (3, 6),
    "tower_base_scale": (8.0, 12.0),

    # material showcase grid (columns = material families, rows = shapes)
    "showcase_columns": (5, 7),
    "showcase_scale": 8.0,
    "showcase_spacing": 2.9,              # multiples of showcase_scale

    # loose shapes around the floor
    "scatter_count": (16, 30),
    "scatter_radius": 210.0,
    "scatter_scale": (3.5, 12.0),
    "scatter_combo_chance": 0.2,          # sphere-in-torus mini stacks

    # sphere lights
    "key_light_count": (2, 4),
    "key_light_radius": (8.0, 20.0),
    "key_light_height": (160.0, 280.0),
    "key_light_irradiance": 12.0,         # total irradiance the key lights aim at the scene centre
    "orb_light_count": (2, 6),
    "orb_light_radius": (1.5, 4.0),
    "orb_light_emission": (25.0, 90.0),

    # environment
    "env_maps": [f"{ASSET_ROOT}/EnvMapSources/photo_studio_loft_hall_4k.exr"],
    "env_map_chance": 1.0,                # 0 -> black sky, the sphere lights are the only lighting
    "floor_styles": {"matte": 1.0, "glossy": 0.7, "dark_glossy": 0.6, "tinted": 0.5, "planks": 0.5},

    # camera
    "camera_distance": (200.0, 250.0),
    "camera_height": (85.0, 125.0),
    "camera_azimuth_jitter": 15.0,        # degrees

    # shape / material mix
    "shape_weights": {"sphere": 1.0, "torus": 1.0, "suzanne": 0.8},
    "material_weights": {
        "plastic": 1.0,
        "matte": 0.6,
        "car_paint": 0.8,
        "velvet": 0.5,
        "metal": 1.0,
        "brushed_metal": 0.4,
        "anodized": 0.5,
        "glass": 0.5,
        "tinted_glass": 0.6,
        "frosted_glass": 0.4,
        "dispersive_glass": 0.4,
        "iridescent": 0.5,
    },
    "palette_hues": (3, 5),               # per scene colour palette, keeps the colours coherent
    "palette_off_chance": 0.15,           # chance of a fully random hue instead
}

RandomShapesSceneBuilder(yapt_instance, CONFIG, resolve_seed(SEED)).build()
