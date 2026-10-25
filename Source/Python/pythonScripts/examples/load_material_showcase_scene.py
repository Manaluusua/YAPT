"""
Material showcase scene.

A tight grid of stone blocks with slightly different heights forms the ground. Every block in the middle carries
one material group, the blocks around them get random textured materials (concrete, tiles, wood, metals, ...) from
the sets Tools/generate_showcase_textures.py generates into the asset folder:
  - a group is a grid of objects, one row per material variation and one column per step of the varied
    parameter (roughness for most groups, going from smooth on the left to rough on the right)
  - the two "blend" groups have one unique material per object, combining several layers (coat, sheen,
    thin film, subsurface, transmission, ...)

The objects are spheres by default, set OBJECT_MESH to any other mesh and it is fitted onto the same spots.
The environment is a generated studio (sky gradient, floor and soft boxes) or a constant gray, see ENV_STYLE.
A few area lights add sharper highlights on top.
Run it via the "run script" action of the app on an empty scene; objects are added, nothing is cleared.
"""

import base64
import hashlib
import math
import os
import random

import numpy as np
from pygltflib import GLTF2, Accessor, Attributes, Buffer, BufferView, Primitive, DATA_URI_HEADER
from pygltflib import Mesh as GltfMesh, FLOAT, UNSIGNED_INT, SCALAR, VEC2, VEC3, VEC4

from py_yapt import Material, ResourceDimension, ResourceFormat, ResourceUsageBits, vec2, vec3, vec4
from yapt.random_shapes import q_axis_angle, q_mul, q_rotate
from yapt.settings import resolve_asset_path

# ---------------------------------------------------------------------------
# Knobs
# ---------------------------------------------------------------------------

OBJECT_MESH = "models/Sphere.glb"   # any mesh, it is scaled to fit its spot and stood on the block
OBJECT_FILL = 0.8                   # object width relative to the spacing of its spot
OBJECT_YAW = 0.0                    # degrees, for meshes that have a front

BLOCK_SIZE = 60.0                   # width of a block
BLOCK_GAP = 0.6
BLOCK_DEPTH = 40.0                  # how far the blocks reach down from their top
BLOCK_MARGIN = 3.0                  # empty border between the block edge and the outermost objects
BLOCK_ELEVATION = 3.0               # max random height difference between neighbouring blocks
BLOCK_ALBEDO = 0.18
BLOCK_ROUGHNESS = 0.7
FILLER_RINGS = 2                    # rings of plain blocks around the material groups
FILLER_OBJECTS = 2                  # the filler blocks carry FILLER_OBJECTS x FILLER_OBJECTS textured objects, 0 for none
FILLER_OBJECT_REPEAT = 2.0          # texture repeats around the object (half of it from top to bottom)
SEED = 7

# textured materials of the filler blocks, from the sets Tools/generate_showcase_textures.py writes to
# ASSET_PATH/textures/procedural. The blocks deal them from a shuffled deck holding each entry count times,
# then pick one value from each "vary" list.
# The ORM map scales roughness and metalness, so metalness 1 lets the map decide
TEXTURED = {"preset": "PLASTIC", "metalness": 1.0, "roughness": 1.0}
FILLER_MATERIALS = [
    (2, dict(TEXTURED, textures="concrete"),
     {"albedo": [(1.0, 1.0, 1.0), (0.8, 0.82, 0.85), (1.1, 1.0, 0.9)], "uv_repeat": [1.0, 2.0], "roughness": [0.9, 1.0]}),
    (2, dict(TEXTURED, textures="ceramic_tiles"),
     {"albedo": [(1.0, 1.0, 1.0), (0.95, 0.9, 0.85)], "uv_repeat": [1.0, 2.0], "roughness": [0.7, 1.0]}),
    (2, dict(TEXTURED, textures="wood_planks"),
     {"albedo": [(1.0, 1.0, 1.0), (0.7, 0.6, 0.55), (1.2, 1.1, 1.0)], "uv_repeat": [1.0, 1.5], "roughness": [0.6, 1.0]}),
    (1, dict(TEXTURED, textures="brushed_metal"),
     {"albedo": [(1.0, 1.0, 1.0), (1.0, 0.8, 0.55), (0.95, 0.65, 0.5)], "uv_repeat": [1.0, 2.0], "roughness": [0.6, 1.0]}),
    (1, dict(TEXTURED, textures="diamond_plate"),
     {"albedo": [(1.0, 1.0, 1.0), (0.9, 0.9, 0.95)], "uv_repeat": [1.0, 2.0], "roughness": [0.8, 1.0]}),
    (1, dict(TEXTURED, textures="rubber_dots"),
     {"albedo": [(1.0, 1.0, 1.0), (2.5, 0.6, 0.5), (0.5, 1.2, 2.5)], "uv_repeat": [1.0, 2.0], "roughness": [1.0]}),
    (1, dict(TEXTURED, textures="chipped_paint"),
     {"albedo": [(1.0, 1.0, 1.0)], "uv_repeat": [1.0, 1.5], "roughness": [1.0]}),
    (1, dict(TEXTURED, textures="circuit_board", emission=(3.0, 3.0, 3.0)),
     {"albedo": [(1.0, 1.0, 1.0)], "uv_repeat": [1.0, 1.5], "roughness": [1.0]}),
]

ENV_STYLE = "studio"                # "studio" (generated below) or "gray"
ENV_GRAY = 0.5
ENV_INTENSITY = 1.0
ENV_RESOLUTION = (2048, 1024)

# studio environment: sky gradient above the horizon, darker floor below it
ENV_ZENITH = (0.5, 0.5, 0.52)
ENV_HORIZON = (0.85, 0.78, 0.68)
ENV_FLOOR_HORIZON = (0.55, 0.48, 0.42)
ENV_FLOOR_NADIR = (0.12, 0.11, 0.1)

# large soft glows: azimuth, elevation, angular radius (degrees), color, intensity
ENV_GLOWS = [
    (180.0, -12.0, 25.0, (1.0, 0.68, 0.42), 0.9),     # warm glow on the backdrop behind the grid
    (180.0, 30.0, 35.0, (0.5, 0.65, 1.0), 0.4),       # cooler light above it
]

# soft boxes in the environment: azimuth (degrees from +z, the camera side, towards +x), elevation, half width,
# half height (degrees), color, intensity. Keep them large, the environment is not importance sampled
ENV_SOFTBOXES = [
    (-40.0, 35.0, 22.0, 14.0, (1.0, 0.86, 0.7), 5.0),      # warm key, front left
    (55.0, 20.0, 18.0, 18.0, (0.85, 0.9, 1.0), 2.5),       # cool fill, front right
    (0.0, 70.0, 18.0, 18.0, (1.0, 1.0, 1.0), 2.5),         # overhead panel
    (150.0, 15.0, 3.0, 22.0, (1.0, 0.95, 0.9), 8.0),       # rim strips behind the grid
    (-150.0, 15.0, 3.0, 22.0, (1.0, 0.95, 0.9), 8.0),
    (100.0, 5.0, 25.0, 2.5, (0.55, 0.7, 1.0), 2.5),        # coloured accent strips low on the sides
    (-100.0, 5.0, 25.0, 2.5, (1.0, 0.5, 0.25), 4.0),
]
ENV_SOFTBOX_EDGE = 0.08             # edge falloff, relative to the box size

# emissive panels: position, target, size (width along the horizontal edge, height), color, radiance
LIGHTS = [
    ((-160.0, 220.0, 120.0), (0.0, 0.0, 0.0), (70.0, 70.0), (1.0, 0.95, 0.88), 6.0),     # key
    ((200.0, 160.0, -60.0), (0.0, 0.0, 0.0), (50.0, 50.0), (1.0, 0.97, 0.92), 4.0),      # fill
    ((0.0, 110.0, -240.0), (0.0, 10.0, -40.0), (160.0, 10.0), (1.0, 0.75, 0.5), 14.0),   # warm rim strip behind the grid
    ((-240.0, 30.0, 0.0), (0.0, 0.0, 0.0), (200.0, 6.0), (0.35, 0.55, 1.0), 6.0),       # grazing blue strip, left
    ((240.0, 30.0, 0.0), (0.0, 0.0, 0.0), (200.0, 6.0), (1.0, 0.45, 0.15), 12.0),        # grazing orange strip, right
]

# glowing orbs on the filler blocks around the groups: block (row, column), radius, color, radiance
ORBS = [
    ((-1, -1), 4.0, (1.0, 0.55, 0.2), 40.0),
    ((-1, 3), 4.0, (0.2, 0.75, 1.0), 40.0),
    ((1, -1), 3.0, (1.0, 0.9, 0.8), 40.0),
    ((1, 3), 3.0, (0.75, 0.85, 1.0), 40.0),
    ((3, -1), 4.0, (1.0, 0.3, 0.65), 40.0),
    ((3, 3), 4.0, (1.0, 0.75, 0.3), 40.0),
]

CAMERA_POSITION = (0.0, 165.0, 150.0)
CAMERA_TARGET = (0.0, 0.0, 14.0)

ROUGHNESS = [0.0, 0.12, 0.25, 0.45, 0.7]
GLASS_ROUGHNESS = [0.0, 0.05, 0.12, 0.25, 0.4]
COAT_ROUGHNESS = [0.0, 0.08, 0.18, 0.32, 0.5]
SHEEN_ROUGHNESS = [0.1, 0.3, 0.5, 0.7, 0.9]
THIN_FILM_NM = [150.0, 250.0, 350.0, 450.0, 600.0]
SSS_DISTANCE = [0.01, 0.04, 0.12, 0.3, 0.8]     # scattering distance relative to the object width

# (A, B) Cauchy coefficients, B in um^2
CAUCHY = [
    (1.4580, 0.00354),   # fused silica
    (1.5046, 0.00420),   # BK7 crown
    (1.6700, 0.00743),   # BaF10 barium flint
    (1.7280, 0.01342),   # SF10 dense flint
    (2.3790, 0.01335),   # diamond
]

# ---------------------------------------------------------------------------
# Material groups
#
# A group is a list of rows, a row a list of parameter dicts (see apply_params for the keys). The colors are
# linear. "tint" is the color transmitted through the full width of the object, "sss_distance" the
# scattering distance relative to the object width, both are converted using the size the object ends up at.
# ---------------------------------------------------------------------------

def vary(base, key, values):
    return [dict(base, **{key: v}) for v in values]

def film(roughness, coat_ior, thickness_nm=None):
    """Thin film on top of the base. The interference is only applied when the coat and the base are both
    smooth (roughness < ~0.045) or both rough, so they share the roughness."""
    params = {"roughness": roughness, "clearcoat": 1.0, "clearcoat_roughness": roughness, "clearcoat_ior": coat_ior}
    if thickness_nm is not None:
        params["thin_film_nm"] = thickness_nm
    return params

METAL = lambda preset: {"preset": preset}
GLASS = {"preset": "GLASS"}
PLASTIC = {"preset": "PLASTIC"}

GROUPS = {
    "metals": [
        vary(METAL("METAL_GOLD"), "roughness", ROUGHNESS),
        vary(METAL("METAL_COPPER"), "roughness", ROUGHNESS),
        vary(METAL("METAL_ALUMINIUM"), "roughness", ROUGHNESS),
        vary(METAL("METAL_SILVER"), "roughness", ROUGHNESS),
        vary(METAL("METAL_BRASS"), "roughness", ROUGHNESS),
    ],
    "glass": [
        vary(GLASS, "roughness", GLASS_ROUGHNESS),
        vary(dict(GLASS, tint=(0.25, 0.75, 0.3)), "roughness", GLASS_ROUGHNESS),
        vary(dict(GLASS, tint=(0.85, 0.45, 0.08)), "roughness", GLASS_ROUGHNESS),
        vary(dict(GLASS, tint=(0.1, 0.25, 0.85)), "roughness", GLASS_ROUGHNESS),
        vary(dict(GLASS, roughness=0.0), "dispersion", CAUCHY),
    ],
    "plastic": [
        vary(dict(PLASTIC, albedo=(0.8, 0.8, 0.8)), "roughness", ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.7, 0.04, 0.03)), "roughness", ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.03, 0.12, 0.6)), "roughness", ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.01, 0.01, 0.01)), "roughness", ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.8, 0.3, 0.02), anisotropy=0.85), "roughness", [max(r, 0.08) for r in ROUGHNESS]),
    ],
    "sheen": [
        vary(dict(PLASTIC, albedo=(0.3, 0.01, 0.03), specular_amount=0.0, sheen=1.0, sheen_tint=(1.0, 0.35, 0.4)), "sheen_roughness", SHEEN_ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.02, 0.05, 0.3), specular_amount=0.0, sheen=1.0, sheen_tint=(0.4, 0.55, 1.0)), "sheen_roughness", SHEEN_ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.01, 0.01, 0.01), specular_amount=0.0, sheen=1.0, sheen_tint=(0.9, 0.9, 0.9)), "sheen_roughness", SHEEN_ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.05, 0.2, 0.03), specular_amount=0.0, sheen=1.0, sheen_tint=(1.0, 0.85, 0.2)), "sheen_roughness", SHEEN_ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.7, 0.7, 0.7), roughness=0.15, sheen=1.0, sheen_tint=(0.6, 0.3, 1.0)), "sheen_roughness", SHEEN_ROUGHNESS),
    ],
    "clearcoat": [
        vary(dict(PLASTIC, albedo=(0.55, 0.01, 0.01), roughness=0.45, clearcoat=1.0), "clearcoat_roughness", COAT_ROUGHNESS),
        vary(dict(METAL("METAL_COPPER"), roughness=0.5, clearcoat=1.0), "clearcoat_roughness", COAT_ROUGHNESS),
        vary(dict(PLASTIC, albedo=(0.005, 0.005, 0.005), roughness=0.8, clearcoat=1.0), "clearcoat_roughness", COAT_ROUGHNESS),
        vary(dict(PLASTIC, metalness=1.0, albedo=(0.05, 0.4, 0.12), specular_tint=(0.5, 0.9, 0.6), roughness=0.4, clearcoat=1.0), "clearcoat_roughness", COAT_ROUGHNESS),
        vary(dict(METAL("METAL_ALUMINIUM"), roughness=0.35, anisotropy=0.9, clearcoat=1.0), "clearcoat_roughness", COAT_ROUGHNESS),
    ],
    "thin_film": [
        vary(dict(PLASTIC, albedo=(0.0, 0.0, 0.0), ior=1.3, **film(0.0, 2.0)), "thin_film_nm", THIN_FILM_NM),
        vary(dict(PLASTIC, metalness=1.0, albedo=(0.35, 0.33, 0.3), specular_tint=(0.7, 0.7, 0.7), **film(0.1, 2.4)), "thin_film_nm", THIN_FILM_NM),
        vary(dict(METAL("METAL_GOLD"), **film(0.0, 2.2)), "thin_film_nm", THIN_FILM_NM),
        vary(dict(PLASTIC, albedo=(0.8, 0.8, 0.8), **film(0.15, 2.0)), "thin_film_nm", THIN_FILM_NM),
        vary(dict(PLASTIC, albedo=(0.01, 0.03, 0.15), **film(0.3, 2.2)), "thin_film_nm", THIN_FILM_NM),
    ],
    "subsurface": [
        vary(dict(PLASTIC, albedo=(0.8, 0.45, 0.33), roughness=0.4, subsurface=1.0, subsurface_radius=(1.0, 0.35, 0.2)), "sss_distance", SSS_DISTANCE),
        vary(dict(PLASTIC, albedo=(0.85, 0.85, 0.82), roughness=0.3, subsurface=1.0, subsurface_radius=(1.0, 0.95, 0.85)), "sss_distance", SSS_DISTANCE),
        vary(dict(PLASTIC, albedo=(0.25, 0.65, 0.35), roughness=0.2, subsurface=1.0, subsurface_radius=(0.4, 1.0, 0.6)), "sss_distance", SSS_DISTANCE),
        vary(dict(PLASTIC, albedo=(0.9, 0.55, 0.15), roughness=0.35, subsurface=1.0, subsurface_radius=(1.0, 0.6, 0.25)), "sss_distance", SSS_DISTANCE),
        vary(dict(PLASTIC, albedo=(0.25, 0.4, 0.85), roughness=0.3, subsurface=1.0, subsurface_radius=(0.3, 0.6, 1.0)), "sss_distance", SSS_DISTANCE),
    ],
    "blend_a": [
        [
            #pearl
            dict(PLASTIC, albedo=(0.85, 0.82, 0.78), subsurface=1.0, sss_distance=0.08, subsurface_radius=(1.0, 0.9, 0.8), **film(0.12, 1.5, 450.0)),
            #gold under blue velvet
            dict(METAL("METAL_GOLD"), roughness=0.35, sheen=1.0, sheen_tint=(0.2, 0.4, 1.0), sheen_roughness=0.4),
            #opal
            dict(PLASTIC, albedo=(0.7, 0.8, 0.9), subsurface=1.0, sss_distance=0.2, subsurface_radius=(0.5, 0.8, 1.0), **film(0.0, 1.45, 320.0)),
        ],
        [
            #coated frosted teal glass
            dict(GLASS, roughness=0.35, tint=(0.2, 0.7, 0.65), clearcoat=1.0, clearcoat_roughness=0.0),
            #lacquered brushed copper
            dict(METAL("METAL_COPPER"), roughness=0.35, anisotropy=0.9, clearcoat=1.0, clearcoat_roughness=0.02),
            #iridescent dispersive crystal
            dict(GLASS, dispersion=CAUCHY[3], **film(0.0, 1.5, 600.0)),
        ],
        [
            #lacquered jade
            dict(PLASTIC, albedo=(0.15, 0.55, 0.25), roughness=0.4, subsurface=1.0, sss_distance=0.15, subsurface_radius=(0.3, 1.0, 0.5),
                 clearcoat=1.0, clearcoat_roughness=0.03),
            #peach
            dict(PLASTIC, albedo=(0.9, 0.45, 0.2), roughness=0.6, subsurface=1.0, sss_distance=0.05, subsurface_radius=(1.0, 0.5, 0.3),
                 sheen=1.0, sheen_tint=(1.0, 0.8, 0.7), sheen_roughness=0.7),
            #beetle shell
            dict(PLASTIC, metalness=1.0, albedo=(0.02, 0.12, 0.05), specular_tint=(0.3, 0.6, 0.4), **film(0.15, 1.7, 700.0)),
        ],
    ],
    "blend_b": [
        [
            #translucent red plastic
            dict(PLASTIC, albedo=(0.8, 0.05, 0.05), roughness=0.25, transparency=0.5),
            #amethyst
            dict(GLASS, roughness=0.0, tint=(0.55, 0.15, 0.75), dispersion=CAUCHY[4]),
            #pearlescent car paint
            dict(PLASTIC, metalness=1.0, albedo=(0.05, 0.15, 0.6), specular_tint=(0.5, 0.6, 1.0), **film(0.25, 1.6, 380.0)),
        ],
        [
            #satin aluminium
            dict(METAL("METAL_ALUMINIUM"), roughness=0.3, sheen=1.0, sheen_tint=(1.0, 1.0, 1.0), sheen_roughness=0.3),
            #half metal copper
            dict(METAL("METAL_COPPER"), metalness=0.5, roughness=0.2),
            #lacquered smoky glass
            dict(GLASS, roughness=0.25, tint=(0.15, 0.13, 0.12), clearcoat=1.0, clearcoat_roughness=0.0),
        ],
        [
            #holographic fabric
            dict(PLASTIC, albedo=(0.01, 0.01, 0.02), sheen=1.0, sheen_tint=(0.2, 1.0, 0.9), sheen_roughness=0.5, **film(0.2, 1.6, 500.0)),
            #candle wax
            dict(PLASTIC, albedo=(0.9, 0.85, 0.7), roughness=0.5, subsurface=1.0, sss_distance=0.25, subsurface_radius=(1.0, 0.8, 0.5),
                 sheen=0.5, sheen_tint=(1.0, 1.0, 0.9), sheen_roughness=0.8),
            #heat tinted brushed gold
            dict(METAL("METAL_GOLD"), anisotropy=0.7, **film(0.25, 1.8, 320.0)),
        ],
    ],
}

# which block each group sits on, rows from the back to the front
GROUP_LAYOUT = [
    ["metals", "glass", "thin_film"],
    ["plastic", "clearcoat", "sheen"],
    ["subsurface", "blend_a", "blend_b"],
]

# ---------------------------------------------------------------------------

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

UP = np.array([0.0, 1.0, 0.0])

def normalize(v):
    v = np.asarray(v, dtype=np.float64)
    return v / np.linalg.norm(v)

def v3(t):
    return vec3([float(c) for c in t])

# --- materials ---------------------------------------------------------------

FLOAT_SETTERS = {
    "roughness": "setRoughness",
    "metalness": "setMetalness",
    "transparency": "setTransparency",
    "anisotropy": "setAnisotropy",
    "anisotropy_rotation": "setAnisotropyRotation",
    "ior": "setDielectricIOR",
    "specular_amount": "setSpecularAmount",
    "clearcoat": "setClearCoatAmount",
    "clearcoat_roughness": "setClearCoatRoughness",
    "clearcoat_ior": "setClearCoatIOR",
    "sheen": "setSheenAmount",
    "sheen_roughness": "setSheenRoughness",
    "thin_film_nm": "setThinFilmThicknessNM",
    "subsurface": "setSubsurfaceAmount",
}

COLOR_SETTERS = {
    "albedo": "setAlbedo",
    "specular_tint": "setSpecularTint",
    "sheen_tint": "setSheenTint",
    "subsurface_radius": "setSubsurfaceRadius",
    "emission": "setEmission",
}

def apply_params(mat, params, object_size):
    if "preset" in params:
        mat.setFromMaterialPreset(getattr(Material.MaterialPreset, params["preset"]))
    for key, value in params.items():
        if key in FLOAT_SETTERS:
            getattr(mat, FLOAT_SETTERS[key])(float(value))
        elif key in COLOR_SETTERS:
            getattr(mat, COLOR_SETTERS[key])(v3(value))
        elif key == "tint":
            mat.setAbsorption(v3([-math.log(max(c, 1e-4)) / object_size for c in value]))
        elif key == "sss_distance":
            mat.setSubsurfaceRadiusScale(float(value) * object_size)
        elif key == "dispersion":
            mat.setEnableDispersion(True)
            mat.setCauchysCoefficients(vec2([float(c) for c in value]))
        elif key == "textures":
            apply_texture_set(mat, value, params.get("uv_repeat", 1.0))
        elif key not in ("preset", "uv_repeat"):
            raise KeyError(f"unknown material parameter {key}")

TEXTURE_SET_FOLDER = "textures/procedural"
_texture_sets = {}

def texture_set(name):
    """Albedo, normal, ORM and the optional emissive map of a generated set, None for the ones that are missing."""
    if name not in _texture_sets:
        maps = {}
        for kind, fmt in (("albedo", "R8G8B8A8_SRGB"), ("normal", "R8G8B8A8_UNORM"), ("orm", "R8G8B8A8_UNORM"), ("emissive", "R8G8B8A8_SRGB")):
            path = f"{TEXTURE_SET_FOLDER}/{name}/{name}_{kind}.png"
            maps[kind] = resources.load_texture_2D(path, fmt) if os.path.exists(resolve_asset_path(path)) else None
        if maps["albedo"] is None:
            print(f"[material_showcase] texture set {name} not found, run Tools/generate_showcase_textures.py")
        _texture_sets[name] = maps
    return _texture_sets[name]

def apply_texture_set(mat, name, repeat):
    """repeat is one number or a (u, v) pair."""
    maps = texture_set(name)
    repeat_u, repeat_v = (repeat, repeat) if np.isscalar(repeat) else repeat
    scale = vec2([float(repeat_u), float(repeat_v)])
    setters = {"albedo": mat.setAlbedoTexture, "normal": mat.setNormalTexture, "orm": mat.setORMTexture, "emissive": mat.setEmissiveTexture}
    for kind, setter in setters.items():
        if maps[kind] is not None:
            setter(maps[kind], scale)

_filler_deck = []

def filler_params(rng):
    if not _filler_deck:
        _filler_deck.extend(entry for entry in FILLER_MATERIALS for _ in range(entry[0]))
        rng.shuffle(_filler_deck)
    _, base, variations = _filler_deck.pop()
    params = dict(base)
    for key, options in variations.items():
        params[key] = rng.choice(options)
    return params

def plain_material(name, albedo, roughness):
    mat = renderer.createMaterial(name)
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setAlbedo(v3(albedo))
    mat.setRoughness(roughness)
    return mat

# --- meshes ------------------------------------------------------------------

def upload_mesh(name, positions, normals, tangents, indices, uvs):
    """Packs the arrays into an in-memory glTF and goes through the regular glTF mesh loader."""
    chunks = [
        np.asarray(indices, dtype=np.uint32),
        np.asarray(positions, dtype=np.float32),
        np.asarray(normals, dtype=np.float32),
        np.asarray(tangents, dtype=np.float32),
        np.asarray(uvs, dtype=np.float32),
    ]
    blob = b""
    views = []
    for arr in chunks:
        data = arr.tobytes()
        views.append(BufferView(buffer=0, byteOffset=len(blob), byteLength=len(data)))
        blob += data + b"\0" * (-len(data) % 4)

    pos = chunks[1]
    accessors = [
        Accessor(bufferView=0, componentType=UNSIGNED_INT, count=len(chunks[0]), type=SCALAR),
        Accessor(bufferView=1, componentType=FLOAT, count=len(pos), type=VEC3,
                 min=pos.min(axis=0).tolist(), max=pos.max(axis=0).tolist()),
        Accessor(bufferView=2, componentType=FLOAT, count=len(chunks[2]), type=VEC3),
        Accessor(bufferView=3, componentType=FLOAT, count=len(chunks[3]), type=VEC4),
        Accessor(bufferView=4, componentType=FLOAT, count=len(chunks[4]), type=VEC2),
    ]
    primitive = Primitive(attributes=Attributes(POSITION=1, NORMAL=2, TANGENT=3, TEXCOORD_0=4), indices=0)
    gltf = GLTF2(
        meshes=[GltfMesh(name=name, primitives=[primitive])],
        accessors=accessors,
        bufferViews=views,
        buffers=[Buffer(byteLength=len(blob), uri=DATA_URI_HEADER + base64.b64encode(blob).decode("ascii"))],
    )
    cache_key = f"procedural/{name}/{hashlib.sha1(blob).hexdigest()[:12]}"
    meshes, _ = resources.load_meshes_from_gltf(gltf, cache_key)[0]
    return meshes[0]

def block_mesh():
    """Flat shaded box, BLOCK_SIZE wide and BLOCK_DEPTH tall around the origin. One texture repeat spans BLOCK_SIZE.
    The shader takes the bitangent as cross(tangent, normal), the tangent is picked so that it points along +v
    (down the image), which is how the generated normal maps are encoded. On the sides +v points down."""
    half = np.array([BLOCK_SIZE, BLOCK_DEPTH, BLOCK_SIZE]) * 0.5
    x, y, z = np.eye(3)
    faces = [(y, x), (-y, x), (z, x), (-z, -x), (x, -z), (-x, z)]   # (normal, u axis)
    positions, normals, tangents, indices, uvs = [], [], [], [], []
    for normal, u in faces:
        v = np.cross(u, normal)
        center = normal * np.dot(np.abs(normal), half)
        hu, hv = np.dot(np.abs(u), half), np.dot(np.abs(v), half)
        corners = [center + u * su * hu + v * sv * hv for su, sv in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        if np.dot(np.cross(corners[1] - corners[0], corners[2] - corners[0]), normal) < 0:
            corners = corners[::-1]
        base = len(positions)
        for p in corners:
            positions.append(p)
            normals.append(normal)
            tangents.append([u[0], u[1], u[2], 1.0])
            uvs.append([np.dot(p, u) / BLOCK_SIZE + 0.5, np.dot(p, v) / BLOCK_SIZE + 0.5])
        indices += [base, base + 1, base + 2, base, base + 2, base + 3]
    return upload_mesh("showcase_block", positions, normals, tangents, indices, uvs)

class ObjectShape:
    """The object mesh with its bounds, so it can be fitted onto a spot of any size."""

    def __init__(self, mesh_path):
        gltf = GLTF2().load(resolve_asset_path(mesh_path))
        bounds_min = np.full(3, np.inf)
        bounds_max = np.full(3, -np.inf)
        for gltf_mesh in gltf.meshes:
            for primitive in gltf_mesh.primitives:
                accessor = gltf.accessors[primitive.attributes.POSITION]
                bounds_min = np.minimum(bounds_min, accessor.min)
                bounds_max = np.maximum(bounds_max, accessor.max)
        self._min = bounds_min
        self._max = bounds_max
        self._entries = resources.load_meshes_from_path(mesh_path)

    def width(self):
        return max(self._max[0] - self._min[0], self._max[2] - self._min[2])

    def place(self, name, material, center_xz, floor_y, size):
        """Scales the shape to size wide, centres it on center_xz and stands it on floor_y."""
        scale = size / self.width()
        center = (self._min + self._max) * 0.5
        offset = np.array([-center[0], -self._min[1], -center[2]]) * scale
        yaw = q_axis_angle((0.0, 1.0, 0.0), OBJECT_YAW)
        position = np.array([center_xz[0], floor_y, center_xz[1]]) + np.array(q_rotate(yaw, offset))

        for entry_index, (meshes, prim_map) in enumerate(self._entries):
            objects = []
            for mesh_index, mesh in enumerate(meshes):
                obj = scene.create_render_object(f"{name}_{entry_index}_{mesh_index}")
                obj.setMesh(mesh)
                obj.getTransform().setScale(vec3(scale))
                obj.getTransform().setOrientation(vec4(list(yaw)))
                obj.getTransform().setTranslation(v3(position))
                objects.append(obj)
            for mapping in prim_map:
                if mapping is not None:
                    objects[mapping[0]].setMaterial(material, mapping[1])

# --- scene pieces -------------------------------------------------------------

def create_blocks():
    """Returns the top height of every block, keyed by its grid coordinate (0, 0 = first group)."""
    rng = random.Random(SEED)
    mesh = block_mesh()
    rows, cols = len(GROUP_LAYOUT), len(GROUP_LAYOUT[0])
    tops = {}
    for r in range(-FILLER_RINGS, rows + FILLER_RINGS):
        for c in range(-FILLER_RINGS, cols + FILLER_RINGS):
            top = rng.uniform(0.0, BLOCK_ELEVATION)
            name = f"showcase_block_{r}_{c}"
            if 0 <= r < rows and 0 <= c < cols:
                albedo = BLOCK_ALBEDO * rng.uniform(0.85, 1.15)
                mat = plain_material(f"{name}_mat", (albedo, albedo, albedo), BLOCK_ROUGHNESS)
            else:
                mat = renderer.createMaterial(f"{name}_mat")
                apply_params(mat, filler_params(rng), BLOCK_SIZE)
            obj = scene.create_render_object(name)
            obj.setMesh(mesh)
            obj.setMaterial(mat, 0)
            x, z = block_center(r, c)
            obj.getTransform().setTranslation(v3((x, top - BLOCK_DEPTH * 0.5, z)))
            tops[(r, c)] = top
    return tops

def block_center(r, c):
    rows, cols = len(GROUP_LAYOUT), len(GROUP_LAYOUT[0])
    pitch = BLOCK_SIZE + BLOCK_GAP
    x = (c - (cols - 1) * 0.5) * pitch
    z = (r - (rows - 1) * 0.5) * pitch
    return (x, z)

def create_group(group_name, block_xz, top, shape):
    rows = GROUPS[group_name]
    row_count = len(rows)
    col_count = max(len(row) for row in rows)
    inner = BLOCK_SIZE - 2.0 * BLOCK_MARGIN
    spacing = inner / max(row_count, col_count)
    size = spacing * OBJECT_FILL
    for i, row in enumerate(rows):
        for j, params in enumerate(row):
            x = block_xz[0] + (j - (len(row) - 1) * 0.5) * spacing
            z = block_xz[1] + (i - (row_count - 1) * 0.5) * spacing
            name = f"showcase_{group_name}_{i}_{j}"
            mat = renderer.createMaterial(f"{name}_mat")
            apply_params(mat, params, size)
            shape.place(name, mat, (x, z), top, size)

def create_filler_objects(tops, shape):
    """Textured objects on the filler blocks, materials dealt from the same deck as the blocks."""
    if FILLER_OBJECTS <= 0:
        return
    rng = random.Random(SEED + 1)
    rows, cols = len(GROUP_LAYOUT), len(GROUP_LAYOUT[0])
    spacing = (BLOCK_SIZE - 2.0 * BLOCK_MARGIN) / FILLER_OBJECTS
    size = spacing * OBJECT_FILL
    for (r, c), top in tops.items():
        if 0 <= r < rows and 0 <= c < cols:
            continue
        bx, bz = block_center(r, c)
        for i in range(FILLER_OBJECTS):
            for j in range(FILLER_OBJECTS):
                params = filler_params(rng)
                repeat = params.get("uv_repeat", 1.0) * FILLER_OBJECT_REPEAT
                params["uv_repeat"] = (repeat, repeat * 0.5)
                name = f"showcase_filler_{r}_{c}_{i}_{j}"
                mat = renderer.createMaterial(f"{name}_mat")
                apply_params(mat, params, size)
                x = bx + (j - (FILLER_OBJECTS - 1) * 0.5) * spacing
                z = bz + (i - (FILLER_OBJECTS - 1) * 0.5) * spacing
                shape.place(name, mat, (x, z), top, size)

def quat_from_basis(x, y, z):
    """Quaternion (x, y, z, w) of the rotation taking the local axes to the given orthonormal axes."""
    m = np.column_stack([x, y, z])
    w = math.sqrt(max(0.0, 1.0 + m[0, 0] + m[1, 1] + m[2, 2])) * 0.5
    if w > 1e-4:
        return ((m[2, 1] - m[1, 2]) / (4 * w), (m[0, 2] - m[2, 0]) / (4 * w), (m[1, 0] - m[0, 1]) / (4 * w), w)
    i = int(np.argmax(np.diag(m)))
    j, k = (i + 1) % 3, (i + 2) % 3
    s = math.sqrt(max(0.0, 1.0 + m[i, i] - m[j, j] - m[k, k])) * 2.0
    q = [0.0, 0.0, 0.0, (m[k, j] - m[j, k]) / s]
    q[i] = 0.25 * s
    q[j] = (m[j, i] + m[i, j]) / s
    q[k] = (m[k, i] + m[i, k]) / s
    return tuple(q)

def emissive_material(name, color, radiance):
    mat = plain_material(name, (0.0, 0.0, 0.0), 1.0)
    mat.setEmission(v3([c * radiance for c in color]))
    return mat

def create_panel_light(name, position, target, size, color, radiance):
    #plane.glb is 2x2 units with its front (emitting) side towards +y, the width goes along its x axis and is kept horizontal
    meshes, _ = resources.load_meshes_from_path("models/plane.glb")[0]
    obj = scene.create_render_object(name)
    obj.setMesh(meshes[0])
    obj.setMaterial(emissive_material(f"{name}_mat", color, radiance), 0)
    obj.getTransform().setScale(v3((size[0] * 0.5, 1.0, size[1] * 0.5)))
    obj.getTransform().setTranslation(v3(position))

    normal = normalize(np.asarray(target) - np.asarray(position))
    side = np.cross(UP, normal)
    side = normalize(side) if np.linalg.norm(side) > 1e-4 else np.array([1.0, 0.0, 0.0])
    obj.getTransform().setOrientation(vec4(list(quat_from_basis(side, normal, np.cross(side, normal)))))

def create_orb(name, block, radius, color, radiance, tops):
    meshes, _ = resources.load_meshes_from_path("models/Sphere.glb")[0]
    obj = scene.create_render_object(name)
    obj.setMesh(meshes[0])
    obj.setMaterial(emissive_material(f"{name}_mat", color, radiance), 0)
    obj.getTransform().setScale(vec3(radius))
    x, z = block_center(*block)
    obj.getTransform().setTranslation(v3((x, tops[block] + radius, z)))

def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)

def direction_from_angles(azimuth, elevation):
    a, e = math.radians(azimuth), math.radians(elevation)
    return np.array([math.sin(a) * math.cos(e), math.sin(e), math.cos(a) * math.cos(e)])

def studio_environment(w, h):
    """Long-lat image in the layout the shader samples: u = atan2(z, x) / 2pi + 0.5, v = acos(y) / pi."""
    theta = ((np.arange(w) + 0.5) / w - 0.5) * 2.0 * math.pi
    phi = (np.arange(h) + 0.5) / h * math.pi
    theta, phi = np.meshgrid(theta, phi)
    dirs = np.stack([np.sin(phi) * np.cos(theta), np.cos(phi), np.sin(phi) * np.sin(theta)], axis=-1)
    y = dirs[..., 1:2]

    sky = np.array(ENV_HORIZON) + (np.array(ENV_ZENITH) - ENV_HORIZON) * np.sqrt(np.clip(y, 0.0, 1.0))
    floor = np.array(ENV_FLOOR_HORIZON) + (np.array(ENV_FLOOR_NADIR) - ENV_FLOOR_HORIZON) * np.clip(-y, 0.0, 1.0) ** 0.35
    color = floor + (sky - floor) * smoothstep(-0.02, 0.02, y)

    for azimuth, elevation, radius, glow_color, intensity in ENV_GLOWS:
        angle = np.degrees(np.arccos(np.clip(dirs @ direction_from_angles(azimuth, elevation), -1.0, 1.0)))
        color = color + np.exp(-0.5 * (angle / radius) ** 2)[..., None] * np.array(glow_color) * intensity

    for azimuth, elevation, half_w, half_h, box_color, intensity in ENV_SOFTBOXES:
        #flat rectangle facing the centre, so it reflects as a rectangle
        forward = direction_from_angles(azimuth, elevation)
        right = normalize(np.cross(UP, forward))
        up = np.cross(forward, right)
        facing = dirs @ forward
        with np.errstate(divide="ignore", invalid="ignore"):
            x = np.abs(dirs @ right / facing) / math.tan(math.radians(half_w))
            z = np.abs(dirs @ up / facing) / math.tan(math.radians(half_h))
        mask = (1.0 - smoothstep(1.0 - ENV_SOFTBOX_EDGE, 1.0, x)) * (1.0 - smoothstep(1.0 - ENV_SOFTBOX_EDGE, 1.0, z))
        mask = np.where(facing > 0.0, np.nan_to_num(mask), 0.0)
        color = color + mask[..., None] * np.array(box_color) * intensity

    return color

def setup_environment():
    if ENV_STYLE == "studio":
        w, h = ENV_RESOLUTION
        color = studio_environment(w, h)
    else:
        w, h = 16, 8
        color = np.full((h, w, 3), ENV_GRAY)
    pixels = np.ones((h, w, 4), dtype=np.float16)
    pixels[..., :3] = color
    env_map = renderer.createTexture(f"showcase_{ENV_STYLE}_env", ResourceDimension.TEXTURE_2D, ResourceFormat.RGBA16_SFLOAT,
                                     ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, w, h, 1, 1)
    env_map.upload(0, 0, 1, 1, w * 8, pixels.ctypes.data, 0)
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)
    env_var = renderer.getRendererVariable("World.EnvIntensityScale")
    if env_var is not None:
        env_var.setFromFloatArray([float(ENV_INTENSITY)])

def set_camera(pos, focus):
    dx, dy, dz = (focus[i] - pos[i] for i in range(3))
    #camera looks down -z: yaw about y then pitch about x, same as the CameraController
    yaw = math.degrees(math.atan2(-dx, -dz))
    pitch = math.degrees(math.atan2(dy, math.hypot(dx, dz)))
    quat = q_mul(q_axis_angle((0.0, 1.0, 0.0), yaw), q_axis_angle((1.0, 0.0, 0.0), pitch))
    transform = scene.get_main_camera().getTransform()
    transform.setTranslation(vec3(list(pos)))
    transform.setOrientation(vec4(list(quat)))

setup_environment()
tops = create_blocks()
shape = ObjectShape(OBJECT_MESH)
for r, row in enumerate(GROUP_LAYOUT):
    for c, group_name in enumerate(row):
        create_group(group_name, block_center(r, c), tops[(r, c)], shape)
create_filler_objects(tops, shape)
for i, light in enumerate(LIGHTS):
    create_panel_light(f"showcase_light_{i}", *light)
for i, (block, radius, color, radiance) in enumerate(ORBS):
    create_orb(f"showcase_orb_{i}", block, radius, color, radiance, tops)
set_camera(CAMERA_POSITION, CAMERA_TARGET)
