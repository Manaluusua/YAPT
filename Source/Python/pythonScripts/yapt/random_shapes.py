"""
Randomized scenes built out of the plane, sphere, torus and suzanne sample meshes.

Shared by examples/load_random_shapes_scene.py (one pile + showcase) and
examples/load_random_shapes_large_scene.py (a big field with many piles). The example scripts
hold the tweakable CONFIG dicts; see them for what each key does.

Everything random goes through one seeded random.Random, so a seed always rebuilds the same scene.
"""

import colorsys
import math
import os
import random
import time

import numpy as np
from py_yapt import Material, vec2, vec3, vec4
from yapt.settings import resolve_asset_path

FLOOR_Y = 0.0
DEFAULT_FLOOR_SCALE = 2000.0

# ---------------------------------------------------------------------------
# Collision proxies: each shape is approximated by spheres (x, y, z, radius) in mesh space.
# Objects are "dropped" from above and come to rest on the first proxy they touch, which
# gives believable contacts for stacking without a physics engine.
# ---------------------------------------------------------------------------

def _torus_proxies(count=14):
    # torus.glb: major radius 1.0, tube radius 0.25, lying in the XZ plane
    return [(math.cos(2.0 * math.pi * i / count), 0.0, math.sin(2.0 * math.pi * i / count), 0.25) for i in range(count)]

def _mirror_x(proxies):
    out = []
    for x, y, z, r in proxies:
        out.append((x, y, z, r))
        if x != 0.0:
            out.append((-x, y, z, r))
    return out

SHAPE_PROXIES = {
    "sphere": [(0.0, 0.0, 0.0, 1.0)],
    "torus": _torus_proxies(),
    # suzanne.glb faces +Z, y in [-0.98, 0.98], ears at x = +-1.37
    "suzanne": _mirror_x([
        (0.0, -0.50, 0.42, 0.45),   # jaw
        (0.0, -0.10, -0.25, 0.60),  # back of the head
        (0.33, 0.28, 0.66, 0.30),   # brows / eyes
        (0.36, 0.42, 0.02, 0.55),   # top of the head
        (0.95, 0.18, -0.30, 0.40),  # ears
        (0.0, 0.30, -0.35, 0.55),   # crown
    ]),
}

# relative to ASSET_PATH (see yapt/settings.py)
MESH_PATHS = {
    "plane": "models/plane.glb",
    "sphere": "models/Sphere.glb",
    "torus": "models/torus.glb",
    "suzanne": "models/suzanne.glb",
}

PLANKS_ALBEDO = "textures/weathered_planks/weathered_planks_diff_4k.png"
PLANKS_NORMAL = "textures/weathered_planks/weathered_planks_nor_gl_4k.png"

# ---------------------------------------------------------------------------
# Small math helpers (quaternions are (x, y, z, w) like the vec4 the transform expects)
# ---------------------------------------------------------------------------

def q_axis_angle(axis, degrees):
    half = math.radians(degrees) * 0.5
    s = math.sin(half)
    length = math.sqrt(axis[0] ** 2 + axis[1] ** 2 + axis[2] ** 2)
    return (axis[0] / length * s, axis[1] / length * s, axis[2] / length * s, math.cos(half))

def q_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)

def q_rotate(q, v):
    ux, uy, uz, w = q
    # t = 2 * cross(u, v); v' = v + w * t + cross(u, t)
    tx = 2.0 * (uy * v[2] - uz * v[1])
    ty = 2.0 * (uz * v[0] - ux * v[2])
    tz = 2.0 * (ux * v[1] - uy * v[0])
    return (v[0] + w * tx + (uy * tz - uz * ty),
            v[1] + w * ty + (uz * tx - ux * tz),
            v[2] + w * tz + (ux * ty - uy * tx))

Q_IDENTITY = (0.0, 0.0, 0.0, 1.0)

def to_linear(rgb):
    return tuple(c ** 2.2 for c in rgb)

def v3l(t):
    return vec3([float(t[0]), float(t[1]), float(t[2])])

def pick(rng, value):
    """(a, b) tuples are sampled: ints -> randint, floats -> uniform. Anything else is returned as is."""
    if isinstance(value, tuple):
        lo, hi = value
        if isinstance(lo, int) and isinstance(hi, int):
            return rng.randint(lo, hi)
        return rng.uniform(lo, hi)
    return value

def weighted_choice(rng, weights):
    keys = list(weights.keys())
    return rng.choices(keys, weights=[weights[k] for k in keys], k=1)[0]

def resolve_seed(seed):
    """YAPT_SCENE_SEED env var > given seed > fresh random seed."""
    env_seed = os.environ.get("YAPT_SCENE_SEED")
    if env_seed:
        return int(env_seed)
    if seed is not None:
        return seed
    return random.SystemRandom().randrange(1, 1_000_000)

# ---------------------------------------------------------------------------
# Spatial bucketing so big scenes don't test everything against everything
# ---------------------------------------------------------------------------

class _FloorGrid:
    def __init__(self, cell_size):
        self._cell_size = cell_size
        self._cells = {}

    def key(self, x, z):
        return (math.floor(x / self._cell_size), math.floor(z / self._cell_size))

    def keys_around(self, x, z, radius):
        x0, z0 = self.key(x - radius, z - radius)
        x1, z1 = self.key(x + radius, z + radius)
        for ix in range(x0, x1 + 1):
            for iz in range(z0, z1 + 1):
                yield (ix, iz)

    def cell(self, key):
        return self._cells.get(key)

    def cell_for_insert(self, key, factory):
        cell = self._cells.get(key)
        if cell is None:
            cell = factory()
            self._cells[key] = cell
        return cell


class FootprintSet:
    """Circles (x, z, radius) on the floor that other objects should keep away from."""

    def __init__(self, cell_size=64.0):
        self._grid = _FloorGrid(cell_size)
        self._max_radius = 0.0

    def add(self, x, z, radius):
        self._grid.cell_for_insert(self._grid.key(x, z), list).append((x, z, radius))
        self._max_radius = max(self._max_radius, radius)

    def is_free(self, x, z, radius, margin=2.0):
        for key in self._grid.keys_around(x, z, radius + margin + self._max_radius):
            cell = self._grid.cell(key)
            if cell is None:
                continue
            for rx, rz, rr in cell:
                if (x - rx) ** 2 + (z - rz) ** 2 < (radius + rr + margin) ** 2:
                    return False
        return True


class ProxyWorld:
    """All placed collision proxies, bucketed by floor cell. Rows are (x, y, z, radius, insertion index)."""

    def __init__(self, cell_size=48.0):
        self._grid = _FloorGrid(cell_size)
        self._max_radius = 0.0
        self._next_index = 0

    def add(self, world_proxies):
        rows_per_cell = {}
        for row in world_proxies:
            key = self._grid.key(row[0], row[2])
            rows_per_cell.setdefault(key, []).append((row[0], row[1], row[2], row[3], self._next_index))
            self._next_index += 1
            self._max_radius = max(self._max_radius, float(row[3]))
        for key, rows in rows_per_cell.items():
            cell = self._grid.cell_for_insert(key, lambda: [np.zeros((0, 5))])
            cell[0] = np.concatenate([cell[0], np.array(rows)])

    def _gather(self, x, z, reach):
        arrays = []
        for key in self._grid.keys_around(x, z, reach + self._max_radius):
            cell = self._grid.cell(key)
            if cell is not None:
                arrays.append(cell[0])
        if not arrays:
            return None
        gathered = np.concatenate(arrays)
        # keep insertion order so ties resolve the same way regardless of the bucketing
        return gathered[np.argsort(gathered[:, 4], kind="stable")]

    @staticmethod
    def orient(local_proxies, scale, quat):
        out = np.empty((len(local_proxies), 4))
        for i, (x, y, z, r) in enumerate(local_proxies):
            px, py, pz = q_rotate(quat, (x * scale, y * scale, z * scale))
            out[i] = (px, py, pz, r * scale)
        return out

    def rest_height(self, oriented, x, z):
        """Lowest object-centre height at (x, z) where no proxy penetrates the floor or the world.
        Returns (y, support) where support is the horizontal push-away direction from the touching
        proxy, or None when resting on the floor."""
        oy = oriented[:, 1]
        r = oriented[:, 3]
        best = float(np.max(FLOOR_Y + r - oy))
        support = None

        reach = float(np.max(np.hypot(oriented[:, 0], oriented[:, 2]) + r))
        w = self._gather(x, z, reach)
        if w is None or len(w) == 0:
            return best, support

        dx = (x + oriented[:, 0])[:, None] - w[None, :, 0]
        dz = (z + oriented[:, 2])[:, None] - w[None, :, 2]
        s = r[:, None] + w[None, :, 3]
        h2 = s * s - (dx * dx + dz * dz)
        if not np.any(h2 > 0.0):
            return best, support

        heights = np.where(h2 > 0.0, w[None, :, 1] - oy[:, None] + np.sqrt(np.maximum(h2, 0.0)), -np.inf)
        flat = int(np.argmax(heights))
        j, i = np.unravel_index(flat, heights.shape)
        if heights[j, i] > best:
            best = float(heights[j, i])
            support = (float(dx[j, i]), float(dz[j, i]))
        return best, support

    def drop(self, oriented, x, z, rng, roll_steps=0, roll_step_size=1.0):
        """Drops the object at (x, z), then rolls it off whatever it landed on while that lowers it."""
        y, support = self.rest_height(oriented, x, z)
        step = roll_step_size
        for _ in range(roll_steps):
            if support is None or step < roll_step_size * 0.05:
                break
            length = math.hypot(support[0], support[1])
            if length < 1e-4:
                angle = rng.uniform(0.0, 2.0 * math.pi)
                dir_x, dir_z = math.cos(angle), math.sin(angle)
            else:
                dir_x, dir_z = support[0] / length, support[1] / length
            nx, nz = x + dir_x * step, z + dir_z * step
            ny, nsupport = self.rest_height(oriented, nx, nz)
            if ny < y - 1e-3:
                x, y, z, support = nx, ny, nz, nsupport
            else:
                step *= 0.5

        placed = oriented.copy()
        placed[:, 0] += x
        placed[:, 1] += y
        placed[:, 2] += z
        self.add(placed)
        return x, y, z

# ---------------------------------------------------------------------------
# Scene builder
# ---------------------------------------------------------------------------

class ShapeAsset:
    def __init__(self, name, path, resources):
        mesh_array, prim_map = resources.load_meshes_from_path(path)[0]
        self.name = name
        self.meshes = mesh_array
        self.prim_map = prim_map
        self.part_count = len(prim_map)
        self.proxies = SHAPE_PROXIES.get(name, [(0.0, 0.0, 0.0, 1.0)])
        self.bound_radius = max(math.sqrt(x * x + y * y + z * z) + r for x, y, z, r in self.proxies)


class RandomShapesSceneBuilder:
    """One pile with towers next to it, a material showcase, scattered shapes and sphere lights."""

    def __init__(self, app, config, seed, log_tag="random_shapes"):
        self._app = app
        self._resources = app.get_resources()
        self._renderer = app.get_renderer()
        self._scene = app.get_scene()
        self._cfg = config
        self._seed = seed
        self._log_tag = log_tag
        self._rng = random.Random(seed)
        self._world = ProxyWorld()
        self._reserved = FootprintSet()   # floor footprints the scatter should avoid
        self._mesh_paths = MESH_PATHS
        self._counter = 0
        self._stats = {}
        self._assets = {}
        self._palette = []
        self._material_pool = {}
        self._black_eye_material = None

    # --- basics -------------------------------------------------------------

    def _unique(self, name):
        self._counter += 1
        return f"rs_{name}_{self._counter}"

    def _count(self, key, amount=1):
        self._stats[key] = self._stats.get(key, 0) + amount

    def _asset(self, name):
        if name not in self._assets:
            self._assets[name] = ShapeAsset(name, self._mesh_paths[name], self._resources)
        return self._assets[name]

    def _create_instance(self, shape, materials, position, scale, quat):
        """Creates one render object per mesh of the shape and assigns a material per gltf primitive."""
        asset = self._asset(shape)
        objects = []
        for mesh in asset.meshes:
            obj = self._scene.create_render_object(self._unique(shape))
            obj.setMesh(mesh)
            transform = obj.getTransform()
            transform.setScale(vec3(float(scale)))
            transform.setOrientation(vec4([quat[0], quat[1], quat[2], quat[3]]))
            transform.setTranslation(v3l(position))
            objects.append(obj)

        for prim_index, mapping in enumerate(asset.prim_map):
            if mapping is None:
                continue
            mesh_index, submesh_index = mapping
            objects[mesh_index].setMaterial(materials[min(prim_index, len(materials) - 1)], submesh_index)

        self._count(shape)
        return objects

    def _place(self, shape, materials, scale, quat, x, z, roll_steps=0, reserve=True):
        asset = self._asset(shape)
        oriented = ProxyWorld.orient(asset.proxies, scale, quat)
        x, y, z = self._world.drop(oriented, x, z, self._rng, roll_steps, asset.bound_radius * scale * 0.4)
        self._create_instance(shape, materials, (x, y, z), scale, quat)
        if reserve:
            self._reserved.add(x, z, asset.bound_radius * scale)
        return x, y, z

    def _is_free(self, x, z, radius, margin=2.0):
        return self._reserved.is_free(x, z, radius, margin)

    # --- colours & materials -------------------------------------------------

    def _build_palette(self):
        rng = self._rng
        base = rng.random()
        count = pick(rng, self._cfg["palette_hues"])
        # a loose harmony: spread hues around the base with some jitter
        spread = rng.choice([0.08, 0.15, 0.33, 0.5])
        self._palette = [(base + i * spread + rng.uniform(-0.03, 0.03)) % 1.0 for i in range(count)]

    def _random_color(self, sat=(0.45, 0.9), val=(0.55, 0.95)):
        rng = self._rng
        if rng.random() < self._cfg["palette_off_chance"] or not self._palette:
            hue = rng.random()
        else:
            hue = (rng.choice(self._palette) + rng.uniform(-0.025, 0.025)) % 1.0
        rgb = colorsys.hsv_to_rgb(hue, rng.uniform(*sat), rng.uniform(*val))
        return to_linear(rgb)

    def _new_material(self, tag, preset):
        mat = self._renderer.createMaterial(self._unique(f"mat_{tag}"))
        mat.setFromMaterialPreset(preset)
        return mat

    def _random_family(self):
        return weighted_choice(self._rng, self._cfg["material_weights"])

    def _make_material(self, family=None, size=10.0, unique=False):
        """With config "material_pool_size" > 0 materials are shared between objects once the pool
        is full, which keeps big scenes from creating thousands of materials."""
        pool_size = self._cfg.get("material_pool_size", 0)
        if not pool_size or unique:
            return self._create_material(family, size)

        key = family or "*"
        pool = self._material_pool.setdefault(key, [])
        limit = pool_size if family is None else max(4, pool_size // 8)
        if len(pool) >= limit:
            return self._rng.choice(pool)
        mat = self._create_material(family, size)
        pool.append(mat)
        return mat

    def _create_material(self, family=None, size=10.0):
        rng = self._rng
        family = family or self._random_family()
        self._count(f"material:{family}")

        if family == "plastic":
            mat = self._new_material(family, Material.MaterialPreset.PLASTIC)
            mat.setAlbedo(v3l(self._random_color()))
            mat.setRoughness(rng.uniform(0.05, 0.5))
        elif family == "matte":
            mat = self._new_material(family, Material.MaterialPreset.PLASTIC)
            mat.setAlbedo(v3l(self._random_color(sat=(0.2, 0.6), val=(0.4, 0.9))))
            mat.setRoughness(rng.uniform(0.7, 1.0))
        elif family == "car_paint":
            mat = self._new_material(family, Material.MaterialPreset.PLASTIC)
            mat.setAlbedo(v3l(self._random_color(sat=(0.7, 1.0), val=(0.5, 0.9))))
            mat.setRoughness(rng.uniform(0.3, 0.5))
            mat.setClearCoatAmount(1.0)
            mat.setClearCoatRoughness(rng.uniform(0.0, 0.05))
            mat.setClearCoatIOR(1.5)
        elif family == "velvet":
            mat = self._new_material(family, Material.MaterialPreset.PLASTIC)
            color = self._random_color(sat=(0.5, 0.9), val=(0.3, 0.7))
            mat.setAlbedo(v3l(color))
            mat.setRoughness(rng.uniform(0.7, 0.95))
            mat.setSpecularAmount(0.0)
            mat.setSheenAmount(1.0)
            mat.setSheenTint(v3l(tuple(min(1.0, c * 1.6 + 0.15) for c in color)))
            mat.setSheenRoughness(rng.uniform(0.3, 0.8))
        elif family in ("metal", "brushed_metal"):
            preset = rng.choice([
                Material.MaterialPreset.METAL_GOLD,
                Material.MaterialPreset.METAL_SILVER,
                Material.MaterialPreset.METAL_COPPER,
                Material.MaterialPreset.METAL_BRASS,
                Material.MaterialPreset.METAL_ALUMINIUM,
            ])
            mat = self._new_material(family, preset)
            if family == "metal":
                mat.setRoughness(rng.choice([rng.uniform(0.0, 0.05), rng.uniform(0.05, 0.45)]))
            else:
                mat.setRoughness(rng.uniform(0.2, 0.4))
                mat.setAnisotropy(rng.choice([-1.0, 1.0]) * rng.uniform(0.6, 0.9))
        elif family == "anodized":
            mat = self._new_material(family, Material.MaterialPreset.PLASTIC)
            color = self._random_color(sat=(0.6, 1.0), val=(0.6, 1.0))
            mat.setMetalness(1.0)
            mat.setAlbedo(v3l(color))
            mat.setSpecularTint(v3l(tuple(min(1.0, c * 0.5 + 0.5) for c in color)))
            mat.setRoughness(rng.uniform(0.1, 0.35))
        elif family in ("glass", "tinted_glass", "frosted_glass", "dispersive_glass"):
            mat = self._new_material(family, Material.MaterialPreset.GLASS)
            mat.setDielectricIOR(rng.uniform(1.45, 1.6))
            mat.setRoughness(rng.uniform(0.0, 0.03))
            if family in ("tinted_glass", "frosted_glass") and (family == "tinted_glass" or rng.random() < 0.6):
                color = self._random_color(sat=(0.4, 0.9), val=(0.8, 1.0))
                density = rng.uniform(1.0, 3.0) / (2.0 * size)
                mat.setAbsorption(v3l(tuple((1.0 - c) * density for c in color)))
            if family == "frosted_glass":
                mat.setRoughness(rng.uniform(0.15, 0.4))
            if family == "dispersive_glass":
                mat.setEnableDispersion(True)
                mat.setCauchysCoefficients(vec2([rng.uniform(1.45, 1.6), rng.uniform(0.003, 0.012)]))
        elif family == "iridescent":
            mat = self._new_material(family, Material.MaterialPreset.PLASTIC)
            mat.setClearCoatAmount(1.0)
            mat.setClearCoatIOR(rng.uniform(1.4, 1.8))
            mat.setDielectricIOR(rng.uniform(1.3, 1.5))
            mat.setClearCoatRoughness(rng.uniform(0.0, 0.2))
            mat.setRoughness(rng.uniform(0.01, 0.3))
            mat.setThinFilmThicknessNM(rng.uniform(250.0, 900.0))
            if rng.random() < 0.5:
                mat.setMetalness(1.0)
                mat.setAlbedo(v3l(self._random_color(sat=(0.2, 0.6), val=(0.2, 0.5))))
            else:
                mat.setAlbedo(vec3(0.0))
        else:
            raise ValueError(f"unknown material family {family}")

        return mat

    def _make_eye_material(self, head_mat):
        rng = self._rng
        choice = rng.random()
        if choice < 0.35:
            if self._black_eye_material is not None:
                return self._black_eye_material
            mat = self._new_material("eyes", Material.MaterialPreset.PLASTIC)
            mat.setAlbedo(vec3(0.01))
            mat.setRoughness(0.05)
            if self._cfg.get("material_pool_size", 0):
                self._black_eye_material = mat
            return mat
        if choice < 0.5:
            return head_mat
        return self._make_material(rng.choice(["metal", "glass", "plastic", "car_paint", "iridescent"]), size=1.0)

    def _materials_for(self, shape, family=None, size=10.0, head_mat=None):
        mat = head_mat or self._make_material(family, size)
        if shape == "suzanne":
            return [mat, self._make_eye_material(mat)]
        return [mat]

    def _light_material(self, emission):
        mat = self._renderer.createMaterial(self._unique("mat_light"))
        mat.setEmission(v3l(emission))
        return mat

    # --- orientations ---------------------------------------------------------

    def _tilt(self, max_degrees):
        rng = self._rng
        angle = rng.uniform(0.0, 2.0 * math.pi)
        return q_axis_angle((math.cos(angle), 0.0, math.sin(angle)), rng.uniform(0.0, max_degrees))

    def _yaw(self, degrees=None):
        return q_axis_angle((0.0, 1.0, 0.0), self._rng.uniform(0.0, 360.0) if degrees is None else degrees)

    def _random_orientation(self, shape, loose=False):
        """loose = allow more chaotic poses (used in the piles)."""
        rng = self._rng
        if shape == "sphere":
            return q_mul(self._yaw(), self._tilt(180.0))
        if shape == "torus":
            if rng.random() < (0.25 if loose else 0.3):
                base = q_axis_angle((1.0, 0.0, 0.0), 90.0)   # standing on its edge
                return q_mul(self._yaw(), q_mul(self._tilt(10.0), base))
            return q_mul(self._yaw(), self._tilt(35.0 if loose else 6.0))
        # suzanne
        pose = rng.choices(["upright", "face_up", "side", "face_down"], weights=[3, 1.2, 1, 0.4 if loose else 0.1])[0]
        if pose == "upright":
            base = Q_IDENTITY
        elif pose == "face_up":
            base = q_axis_angle((1.0, 0.0, 0.0), -90.0)
        elif pose == "face_down":
            base = q_axis_angle((1.0, 0.0, 0.0), 90.0)
        else:
            base = q_axis_angle((0.0, 0.0, 1.0), rng.choice([-90.0, 90.0]))
        return q_mul(self._yaw(), q_mul(self._tilt(25.0 if loose else 10.0), base))

    def _random_shape(self):
        return weighted_choice(self._rng, self._cfg["shape_weights"])

    # --- building blocks -------------------------------------------------------

    def _build_environment(self, floor_scale=DEFAULT_FLOOR_SCALE):
        rng = self._rng
        env_maps = [p for p in self._cfg["env_maps"] if os.path.exists(resolve_asset_path(p))]
        if env_maps and rng.random() < self._cfg["env_map_chance"]:
            env_map = self._resources.load_texture_2D(rng.choice(env_maps), "R16G16B16A16_SFLOAT")
            if env_map is not None:
                self._renderer.getRendererVariable("World.Skycube").setTexture(env_map)
                self._count("env_map")

        style = weighted_choice(rng, self._cfg["floor_styles"])
        mat = self._new_material("floor", Material.MaterialPreset.PLASTIC)
        planks_albedo = PLANKS_ALBEDO
        planks_normal = PLANKS_NORMAL
        if style == "planks" and not os.path.exists(resolve_asset_path(planks_albedo)):
            style = "matte"

        if style == "matte":
            mat.setAlbedo(vec3(rng.uniform(0.35, 0.6)))
            mat.setRoughness(rng.uniform(0.6, 0.9))
        elif style == "glossy":
            mat.setAlbedo(vec3(rng.uniform(0.4, 0.7)))
            mat.setRoughness(rng.uniform(0.08, 0.25))
        elif style == "dark_glossy":
            mat.setAlbedo(vec3(rng.uniform(0.01, 0.05)))
            mat.setRoughness(rng.uniform(0.03, 0.15))
        elif style == "tinted":
            mat.setAlbedo(v3l(self._random_color(sat=(0.1, 0.3), val=(0.5, 0.8))))
            mat.setRoughness(rng.uniform(0.4, 0.8))
        else:
            # keep the planks the same size however big the floor is
            tex_scale = 45.0 * floor_scale / DEFAULT_FLOOR_SCALE
            mat.setRoughness(0.5)
            albedo_tex = self._resources.load_texture_2D(planks_albedo, "R8G8B8A8_SRGB")
            normal_tex = self._resources.load_texture_2D(planks_normal, "R8G8B8A8_UNORM")
            if albedo_tex is not None:
                mat.setAlbedoTexture(albedo_tex, vec2([tex_scale, tex_scale]))
            if normal_tex is not None:
                mat.setNormalTexture(normal_tex, vec2([tex_scale, tex_scale]))
        self._stats["floor"] = style

        self._create_instance("plane", [mat], (0.0, FLOOR_Y, 0.0), floor_scale, Q_IDENTITY)

    def _build_pile(self, cx, cz, count, radius, roll_steps, scale_range=None):
        """Drops count random shapes around (cx, cz). Returns the footprint radius reserved for it."""
        rng = self._rng
        scale_range = scale_range or self._cfg["pile_scale"]
        # bigger pieces first so they end up at the bottom
        scales = sorted((pick(rng, scale_range) for _ in range(count)), key=lambda s: -s * rng.uniform(0.7, 1.3))
        for scale in scales:
            shape = self._random_shape()
            quat = self._random_orientation(shape, loose=True)
            r = min(abs(rng.gauss(0.0, radius * 0.4)), radius)
            angle = rng.uniform(0.0, 2.0 * math.pi)
            self._place(shape, self._materials_for(shape, size=scale), scale, quat,
                        cx + math.cos(angle) * r, cz + math.sin(angle) * r, roll_steps, reserve=False)
        footprint = radius * 1.4
        self._reserved.add(cx, cz, footprint)
        self._count("pile_objects", count)
        return footprint

    def _build_towers_around(self, cx, cz, pile_footprint, count):
        rng = self._rng
        base_angle = rng.uniform(0.0, 2.0 * math.pi)

        for t in range(count):
            base_scale = pick(rng, self._cfg["tower_base_scale"])
            footprint = base_scale * 1.25
            # place around the pile, retry a few times if the spot is taken
            for _ in range(20):
                angle = base_angle + t * (2.0 * math.pi / count) + rng.uniform(-0.4, 0.4)
                dist = pile_footprint + footprint + rng.uniform(5.0, 25.0)
                x, z = cx + math.cos(angle) * dist, cz + math.sin(angle) * dist
                if self._is_free(x, z, footprint):
                    break
            else:
                continue
            self._build_tower(x, z, base_scale)
            self._reserved.add(x, z, footprint * 1.3)

    def _build_tower(self, x, z, base_scale):
        """Alternating torus / sphere stack, optionally crowned with a suzanne."""
        rng = self._rng
        pieces = pick(rng, self._cfg["tower_pieces"])
        # one shared material for the tower or a fresh one per piece
        shared = self._make_material(size=base_scale) if rng.random() < 0.35 else None
        torus_scale = base_scale
        sphere_radius = 0.0
        for i in range(pieces):
            is_top = i == pieces - 1
            jitter = base_scale * 0.03
            px, pz = x + rng.uniform(-jitter, jitter), z + rng.uniform(-jitter, jitter)
            if i % 2 == 0:
                if i > 0:
                    torus_scale = sphere_radius * rng.uniform(0.75, 0.95)
                quat = q_mul(self._yaw(), self._tilt(3.0))
                self._place("torus", self._materials_for("torus", size=torus_scale, head_mat=shared),
                            torus_scale, quat, px, pz, reserve=False)
                if is_top:
                    break
                if i + 2 >= pieces and rng.random() < 0.6:
                    # crown the tower with a suzanne sitting in the ring
                    scale = torus_scale * rng.uniform(0.7, 0.95)
                    quat = q_mul(self._yaw(), self._tilt(8.0))
                    self._place("suzanne", self._materials_for("suzanne", size=scale, head_mat=shared),
                                scale, quat, px, pz, reserve=False)
                    break
            else:
                sphere_radius = torus_scale * rng.uniform(0.82, 1.0)
                self._place("sphere", self._materials_for("sphere", size=sphere_radius, head_mat=shared),
                            sphere_radius, self._random_orientation("sphere"), px, pz, reserve=False)
        self._count("towers")

    def _showcase_half_extent(self, columns):
        spacing = self._cfg["showcase_scale"] * self._cfg["showcase_spacing"]
        return math.hypot(columns * 0.5 * spacing, 1.5 * spacing)

    def _build_showcase(self, cx, cz, face_x, face_z):
        """Grid with one column per material family and rows of sphere / suzanne / upright torus,
        facing the point (face_x, face_z)."""
        rng = self._rng
        cfg = self._cfg
        scale = cfg["showcase_scale"]
        spacing = scale * cfg["showcase_spacing"]
        columns = pick(rng, cfg["showcase_columns"])

        families = list(cfg["material_weights"].keys())
        rng.shuffle(families)
        families = families[:columns]

        # rotating +Z by yaw gives (sin yaw, 0, cos yaw)
        yaw = math.atan2(face_x - cx, face_z - cz)
        right = (math.cos(yaw), -math.sin(yaw))
        toward = (math.sin(yaw), math.cos(yaw))
        rows = [("sphere", 1.0), ("suzanne", 0.0), ("torus", -1.0)]   # front to back

        for column, family in enumerate(families):
            mat = self._make_material(family, size=scale, unique=True)
            u = (column - (columns - 1) * 0.5) * spacing
            for shape, row_offset in rows:
                v = row_offset * spacing
                x = cx + right[0] * u + toward[0] * v
                z = cz + right[1] * u + toward[1] * v
                facing = q_axis_angle((0.0, 1.0, 0.0), math.degrees(yaw) + rng.uniform(-20.0, 20.0))
                if shape == "torus":
                    quat = q_mul(facing, q_axis_angle((1.0, 0.0, 0.0), 90.0))
                elif shape == "suzanne":
                    quat = facing
                else:
                    quat = self._random_orientation("sphere")
                self._place(shape, self._materials_for(shape, size=scale, head_mat=mat), scale, quat, x, z)
        self._count("showcases")
        return families

    def _scatter(self, count, sample_xz, accept_xz=None):
        """Places up to count loose shapes (or sphere-in-torus combos) at free spots from sample_xz()."""
        rng = self._rng
        cfg = self._cfg
        placed = 0
        for _ in range(count * 30):
            if placed >= count:
                break
            x, z = sample_xz()
            scale = pick(rng, cfg["scatter_scale"])
            if accept_xz is not None and not accept_xz(x, z):
                continue

            if rng.random() < cfg["scatter_combo_chance"]:
                footprint = scale * 1.25
                if not self._is_free(x, z, footprint, margin=4.0):
                    continue
                self._place("torus", self._materials_for("torus", size=scale), scale,
                            q_mul(self._yaw(), self._tilt(3.0)), x, z, reserve=False)
                sphere_radius = scale * rng.uniform(0.5, 1.1)
                self._place("sphere", self._materials_for("sphere", size=sphere_radius), sphere_radius,
                            self._random_orientation("sphere"), x, z, reserve=False)
                self._reserved.add(x, z, footprint)
                self._count("scatter_combos")
            else:
                shape = self._random_shape()
                footprint = self._asset(shape).bound_radius * scale
                if not self._is_free(x, z, footprint, margin=4.0):
                    continue
                self._place(shape, self._materials_for(shape, size=scale), scale,
                            self._random_orientation(shape), x, z)
            placed += 1
        self._count("scatter_objects", placed)

    def _key_light(self, pos, radius, irradiance, target):
        """Sphere light whose emission gives roughly the wanted irradiance at the target point."""
        dist2 = (pos[0] - target[0]) ** 2 + (pos[1] - target[1]) ** 2 + (pos[2] - target[2]) ** 2
        radiance = irradiance * dist2 / (math.pi * radius * radius)
        tints = [(1.0, 0.82, 0.62), (1.0, 0.93, 0.85), (1.0, 1.0, 1.0), (0.8, 0.88, 1.0), (0.68, 0.8, 1.0)]
        tint = self._rng.choice(tints)
        luminance = 0.2126 * tint[0] + 0.7152 * tint[1] + 0.0722 * tint[2]
        emission = tuple(c / luminance * radiance for c in tint)
        self._create_instance("sphere", [self._light_material(emission)], pos, radius, Q_IDENTITY)
        self._count("key_lights")

    def _orb_params(self):
        rng = self._rng
        cfg = self._cfg
        radius = pick(rng, cfg["orb_light_radius"])
        hue = rng.random()
        color = colorsys.hsv_to_rgb(hue, rng.uniform(0.5, 1.0), 1.0)
        emission = tuple(c * pick(rng, cfg["orb_light_emission"]) for c in to_linear(color))
        return radius, [self._light_material(emission)]

    def _place_orb(self, mode, radius, mat, x, z):
        """mode: "floor" drops it on whatever is below, "float" hovers it above the ground/objects."""
        if mode == "floor":
            self._place("sphere", mat, radius, Q_IDENTITY, x, z)
        else:
            oriented = ProxyWorld.orient(SHAPE_PROXIES["sphere"], radius, Q_IDENTITY)
            rest, _ = self._world.rest_height(oriented, x, z)
            y = max(rest + radius, self._rng.uniform(12.0, 55.0))
            self._create_instance("sphere", mat, (x, y, z), radius, Q_IDENTITY)
        self._count("orb_lights")

    def _set_camera(self, pos, focus):
        cam = self._scene.get_main_camera()
        dx, dy, dz = focus[0] - pos[0], focus[1] - pos[1], focus[2] - pos[2]
        # camera looks down -Z: yaw about Y then pitch about X, same convention as the CameraController
        yaw = math.degrees(math.atan2(-dx, -dz))
        pitch = math.degrees(math.atan2(dy, math.hypot(dx, dz)))
        quat = q_mul(q_axis_angle((0.0, 1.0, 0.0), yaw), q_axis_angle((1.0, 0.0, 0.0), pitch))

        transform = cam.getTransform()
        transform.setTranslation(v3l(pos))
        transform.setOrientation(vec4([quat[0], quat[1], quat[2], quat[3]]))

        move_speed = self._cfg.get("camera_move_speed")
        controller = self._app.get_camera_controller()
        if move_speed is not None and hasattr(controller, "set_movement_speed"):
            controller.set_movement_speed(move_speed)

    def _print_summary(self, start):
        print(f"[{self._log_tag}] seed = {self._seed}  ({time.time() - start:.1f}s)")
        for key in sorted(self._stats):
            print(f"[{self._log_tag}]   {key}: {self._stats[key]}")

    # --- the scene ---------------------------------------------------------------

    def _build_layout(self):
        rng = self._rng
        side = rng.choice([-1.0, 1.0])   # mirror the layout left/right
        self._pile_center = (side * rng.uniform(-85.0, -50.0), rng.uniform(-45.0, -5.0))
        self._showcase_center = (side * rng.uniform(50.0, 80.0), rng.uniform(-5.0, 30.0))
        focus_x = (self._pile_center[0] + self._showcase_center[0]) * 0.5
        focus_z = (self._pile_center[1] + self._showcase_center[1]) * 0.5
        self._focus = (focus_x, 18.0, focus_z)

        azimuth = math.radians(rng.uniform(-1.0, 1.0) * self._cfg["camera_azimuth_jitter"])
        distance = pick(rng, self._cfg["camera_distance"])
        height = pick(rng, self._cfg["camera_height"])
        self._camera_pos = (focus_x + math.sin(azimuth) * distance, height, focus_z + math.cos(azimuth) * distance)

    def _build_lights(self):
        rng = self._rng
        cfg = self._cfg
        fx, fy, fz = self._focus

        # key lights: big spheres up high, emission chosen so each gives a share of the target irradiance
        key_count = pick(rng, cfg["key_light_count"])
        weights = [rng.uniform(0.5, 1.5) for _ in range(key_count)]
        weight_sum = sum(weights)
        for i in range(key_count):
            radius = pick(rng, cfg["key_light_radius"])
            angle = rng.uniform(0.0, 2.0 * math.pi)
            offset = rng.uniform(40.0, 220.0)
            pos = (fx + math.cos(angle) * offset, pick(rng, cfg["key_light_height"]), fz + math.sin(angle) * offset)
            self._key_light(pos, radius, cfg["key_light_irradiance"] * weights[i] / weight_sum, self._focus)

        # orbs: small saturated glowing spheres floating, lying on the floor or sitting on the pile
        orb_count = pick(rng, cfg["orb_light_count"])
        on_pile_used = False
        for _ in range(orb_count):
            radius, mat = self._orb_params()
            mode = rng.choices(["float", "floor", "pile"], weights=[0.5, 0.3, 0.0 if on_pile_used else 0.2])[0]
            if mode == "pile":
                on_pile_used = True
                cx, cz = self._pile_center
                self._place("sphere", mat, radius, Q_IDENTITY, cx + rng.uniform(-10, 10), cz + rng.uniform(-10, 10), reserve=False)
                self._count("orb_lights")
                continue

            for _ in range(30):
                r = cfg["scatter_radius"] * 0.8 * math.sqrt(rng.random())
                angle = rng.uniform(0.0, 2.0 * math.pi)
                x, z = fx + math.cos(angle) * r, fz + math.sin(angle) * r
                if mode == "float" or self._is_free(x, z, radius):
                    break
            self._place_orb(mode, radius, mat, x, z)

    def build(self):
        start = time.time()
        rng = self._rng
        cfg = self._cfg
        self._build_palette()
        self._build_layout()
        self._build_environment()

        count = pick(rng, cfg["pile_count"])
        radius = pick(rng, cfg["pile_radius"])
        roll_steps = pick(rng, cfg["pile_roll_steps"])
        pile_footprint = self._build_pile(self._pile_center[0], self._pile_center[1], count, radius, roll_steps)
        self._build_towers_around(self._pile_center[0], self._pile_center[1], pile_footprint, pick(rng, cfg["tower_count"]))

        families = self._build_showcase(self._showcase_center[0], self._showcase_center[1], self._camera_pos[0], self._camera_pos[2])
        self._stats["showcase_families"] = ", ".join(families)

        fx, fz = self._focus[0], self._focus[2]
        cam_x, cam_z = self._camera_pos[0], self._camera_pos[2]
        def sample_disk():
            r = cfg["scatter_radius"] * math.sqrt(rng.random())
            angle = rng.uniform(0.0, 2.0 * math.pi)
            return fx + math.cos(angle) * r, fz + math.sin(angle) * r
        def away_from_camera(x, z):
            return (x - cam_x) ** 2 + (z - cam_z) ** 2 >= 90.0 ** 2
        self._scatter(pick(rng, cfg["scatter_count"]), sample_disk, away_from_camera)

        self._build_lights()
        self._set_camera(self._camera_pos, self._focus)
        self._print_summary(start)


class LargeRandomShapesSceneBuilder(RandomShapesSceneBuilder):
    """A big square field: many piles of different sizes (some with towers), a few showcases,
    shapes scattered everywhere at a fixed density and a grid of key lights overhead."""

    def _area_sample(self, margin=0.0):
        half = self._cfg["area_half_size"] - margin
        return self._rng.uniform(-half, half), self._rng.uniform(-half, half)

    def _build_pile_sites(self):
        rng = self._rng
        cfg = self._cfg
        self._piles = []
        classes = cfg["pile_classes"]
        class_weights = {name: c["weight"] for name, c in classes.items()}

        for _ in range(pick(rng, cfg["pile_sites"])):
            pile_class = classes[weighted_choice(rng, class_weights)]
            count = pick(rng, pile_class["count"])
            radius = pick(rng, cfg["pile_radius_per_sqrt_count"]) * math.sqrt(count)
            footprint = radius * 1.4
            for _ in range(40):
                x, z = self._area_sample(margin=footprint + 20.0)
                if self._is_free(x, z, footprint, margin=cfg["site_spacing"]):
                    break
            else:
                continue

            roll_steps = pick(rng, cfg["pile_roll_steps"])
            footprint = self._build_pile(x, z, count, radius, roll_steps, pile_class.get("scale"))
            self._piles.append((x, z, count, footprint))
            if rng.random() < cfg["tower_chance"]:
                self._build_towers_around(x, z, footprint, pick(rng, cfg["towers_per_pile"]))
        self._stats["pile_sites"] = len(self._piles)

        # a few lone towers out in the open
        for _ in range(pick(rng, cfg["lone_towers"])):
            base_scale = pick(rng, cfg["tower_base_scale"])
            footprint = base_scale * 1.25
            for _ in range(20):
                x, z = self._area_sample(margin=footprint + 20.0)
                if self._is_free(x, z, footprint, margin=cfg["site_spacing"] * 0.5):
                    self._build_tower(x, z, base_scale)
                    self._reserved.add(x, z, footprint * 1.3)
                    break

    def _build_showcases(self):
        rng = self._rng
        cfg = self._cfg
        extent = self._showcase_half_extent(cfg["showcase_columns"][1])
        for _ in range(pick(rng, cfg["showcase_count"])):
            for _ in range(40):
                x, z = self._area_sample(margin=extent + 20.0)
                if self._is_free(x, z, extent, margin=cfg["site_spacing"]):
                    # face roughly towards the middle of the field, where people will be looking from
                    self._build_showcase(x, z, rng.uniform(-200.0, 200.0), rng.uniform(-200.0, 200.0))
                    break

    def _build_area_lights(self):
        rng = self._rng
        cfg = self._cfg
        half = cfg["area_half_size"]
        spacing = cfg["key_light_spacing"]
        cells = max(1, int(round(2.0 * half / spacing)))
        cell_size = 2.0 * half / cells

        # key lights: one per grid cell, jittered, each aimed at the ground right below it
        for ix in range(cells):
            for iz in range(cells):
                if rng.random() > cfg["key_light_fill"]:
                    continue
                x = -half + (ix + rng.uniform(0.15, 0.85)) * cell_size
                z = -half + (iz + rng.uniform(0.15, 0.85)) * cell_size
                pos = (x, pick(rng, cfg["key_light_height"]), z)
                self._key_light(pos, pick(rng, cfg["key_light_radius"]), pick(rng, cfg["key_light_irradiance"]), (x, FLOOR_Y, z))

        # orbs: density based, some sitting on top of piles
        area = (2.0 * half) ** 2
        for _ in range(int(cfg["orb_light_density"] * area)):
            radius, mat = self._orb_params()
            mode = rng.choices(["float", "floor", "pile"], weights=[0.5, 0.3, 0.2 if self._piles else 0.0])[0]
            if mode == "pile":
                px, pz, _, footprint = rng.choice(self._piles)
                spread = footprint * 0.3
                self._place("sphere", mat, radius, Q_IDENTITY, px + rng.uniform(-spread, spread), pz + rng.uniform(-spread, spread), reserve=False)
                self._count("orb_lights")
                continue

            for _ in range(30):
                x, z = self._area_sample()
                if mode == "float" or self._is_free(x, z, radius):
                    break
            self._place_orb(mode, radius, mat, x, z)

    def _setup_start_camera(self):
        """Start next to the biggest pile, looking at it with the rest of the field behind it."""
        rng = self._rng
        cfg = self._cfg
        if self._piles:
            px, pz, _, footprint = max(self._piles, key=lambda p: p[2])
        else:
            px, pz, footprint = 0.0, 0.0, 30.0

        if math.hypot(px, pz) > 1.0:
            azimuth = math.atan2(px, pz)   # from the field centre outwards, so we look back over the field
        else:
            azimuth = rng.uniform(0.0, 2.0 * math.pi)
        azimuth += math.radians(rng.uniform(-1.0, 1.0) * cfg["camera_azimuth_jitter"])
        distance = max(pick(rng, cfg["camera_distance"]), footprint * 3.0)
        height = pick(rng, cfg["camera_height"])
        pos = (px + math.sin(azimuth) * distance, height, pz + math.cos(azimuth) * distance)
        self._set_camera(pos, (px, 15.0, pz))

    def build(self):
        start = time.time()
        rng = self._rng
        cfg = self._cfg
        half = cfg["area_half_size"]

        self._build_palette()
        self._build_environment(cfg["floor_scale"])
        self._build_pile_sites()
        self._build_showcases()

        area = (2.0 * half) ** 2
        self._scatter(int(cfg["scatter_density"] * area), self._area_sample)

        self._build_area_lights()
        self._setup_start_camera()
        self._print_summary(start)
