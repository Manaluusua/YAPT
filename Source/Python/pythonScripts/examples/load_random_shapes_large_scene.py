"""
Large randomized shape field: load_random_shapes_scene.py scaled up to ~10 x 10 times the floor area.

  - many piles of stacked shapes, from small heaps of a handful of objects to a few big ones,
    some with torus/sphere towers next to them, plus lone towers out in the open
  - a few material showcase grids
  - loose shapes and sphere-in-torus combos scattered over the whole field at a constant density
  - sphere lights only: a jittered grid of key lights overhead plus glowing orbs everywhere
  - the camera starts next to the biggest pile looking back over the field; movement speed is
    raised so the field can actually be crossed (WASD + QE)

Seeding works like the small scene: SEED = None for a new field every run (the seed is printed),
a number to reproduce one, env var YAPT_SCENE_SEED overrides both.
Materials are pooled (material_pool_size) so thousands of objects share a few hundred materials.

Run it via the "run script" action of the app on an empty scene; objects are added, nothing is cleared.
Asset paths are relative to ASSET_PATH ([paths] asset_path in settings.cfg / settings_local.cfg), absolute paths work too.
"""

from yapt.random_shapes import LargeRandomShapesSceneBuilder, resolve_seed

# ---------------------------------------------------------------------------
# Knobs
# ---------------------------------------------------------------------------

SEED = None

CONFIG = {
    # the field: a square of 2 * area_half_size (the small scene is ~420 units across)
    "area_half_size": 2100.0,
    "floor_scale": 8000.0,
    "site_spacing": 40.0,                 # minimum gap between piles / showcases

    # piles: each site picks a size class, the pile spreads with sqrt(count)
    "pile_sites": (30, 45),
    "pile_classes": {
        "small": {"weight": 0.45, "count": (4, 10), "scale": (4.0, 9.0)},
        "medium": {"weight": 0.38, "count": (10, 22), "scale": (4.5, 11.0)},
        "large": {"weight": 0.17, "count": (22, 42), "scale": (5.0, 12.0)},
    },
    "pile_scale": (5.0, 12.0),            # fallback when a class has no "scale"
    "pile_radius_per_sqrt_count": (3.8, 5.2),
    "pile_roll_steps": (8, 16),           # higher = objects roll further off each other -> flatter heap

    # towers
    "tower_chance": 0.35,                 # per pile
    "towers_per_pile": (1, 2),
    "lone_towers": (4, 10),
    "tower_pieces": (3, 6),
    "tower_base_scale": (8.0, 12.0),

    # material showcase grids (columns = material families, rows = shapes)
    "showcase_count": (3, 5),
    "showcase_columns": (5, 7),
    "showcase_scale": 8.0,
    "showcase_spacing": 2.9,              # multiples of showcase_scale

    # loose shapes, per square unit (the small scene has roughly 1.6e-4)
    "scatter_density": 1.3e-4,
    "scatter_scale": (3.5, 12.0),
    "scatter_combo_chance": 0.2,          # sphere-in-torus mini stacks

    # sphere lights
    "key_light_spacing": 600.0,           # one key light per grid cell of this size...
    "key_light_fill": 0.8,                # ...with this probability
    "key_light_radius": (10.0, 25.0),
    "key_light_height": (180.0, 320.0),
    "key_light_irradiance": (5.0, 10.0),  # irradiance on the ground right below each key light
    "orb_light_density": 1.2e-5,          # per square unit
    "orb_light_radius": (1.5, 4.0),
    "orb_light_emission": (25.0, 90.0),

    # environment
    "env_maps": ["textures/envmaps/photo_studio_loft_hall_4k.exr"],
    "env_map_chance": 1.0,                # 0 -> black sky, the sphere lights are the only lighting
    "floor_styles": {"matte": 1.0, "glossy": 0.7, "dark_glossy": 0.6, "tinted": 0.5, "planks": 0.5},

    # camera
    "camera_distance": (160.0, 220.0),
    "camera_height": (70.0, 110.0),
    "camera_azimuth_jitter": 25.0,        # degrees
    "camera_move_speed": 120.0,           # the controller default is 30

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
    "material_pool_size": 160,            # 0 = unique material per object
    "palette_hues": (3, 5),               # per scene colour palette, keeps the colours coherent
    "palette_off_chance": 0.15,           # chance of a fully random hue instead
}

LargeRandomShapesSceneBuilder(yapt_instance, CONFIG, resolve_seed(SEED), "random_shapes_large").build()
