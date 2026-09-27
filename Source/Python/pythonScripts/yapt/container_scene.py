"""
Furniture showcase scenes: a piece of furniture (a "container": a table, a cabinet, ...) stands against the
back wall of a closed room and a set of objects is arranged onto its support surfaces.

Used by examples/load_table_showcase_scene.py and examples/load_cabinet_showcase_scene.py, which hold the
tweakable CONFIG dicts; see them for what each key does.

Units are metres (the converted assets are authored in metres), +Y is up and the container's front faces +Z,
towards the camera. The room floor is at y = 0, centred on x = 0, with the back wall at z = -depth / 2.

Adding a container: add a ContainerDef to CONTAINERS. Its support surfaces are rectangles in the container's own
glTF space: the height of the surface, its usable x / z extents (keep clear of edges, posts etc.) and the free
height above it (None = open).
Adding an object: add a PlaceableDef to PLACEABLES. With split_nodes=True every root node of the glTF is placed
on its own (e.g. a row of bottles), otherwise the whole file moves as one piece (e.g. a lamp + its bulb).
"""

import itertools
import math
import os
import random
from dataclasses import dataclass, field

import numpy as np
from py_yapt import Material, vec2, vec3, vec4
from yapt.random_shapes import q_axis_angle, q_mul, q_rotate, resolve_seed, to_linear, v3l, Q_IDENTITY
from yapt.scene_loader import SceneLoader

# ---------------------------------------------------------------------------
# Containers and placeable objects
# ---------------------------------------------------------------------------

@dataclass
class Surface:
    name: str
    y: float
    x: tuple                    # (min, max)
    z: tuple                    # (min, max), min is the back
    max_height: float = None    # free height above the surface, None = open
    spread: str = "even"        # "even": free width shared around the items, "justify": first / last item at the ends

@dataclass
class ContainerDef:
    path: str                   # relative to the config's "converted_root"
    surfaces: list
    material_overrides: dict = field(default_factory=dict)   # glTF material name -> {material setter: value}

@dataclass
class PlaceableDef:
    path: str
    split_nodes: bool = False

CONTAINERS = {
    "gallinera_table": ContainerDef(
        "gallinera_table_4k/gallinera_table_4k.gltf",
        [Surface("top", 0.4878, (-0.39, 0.39), (-0.235, 0.235))]),

    "vintage_cabinet": ContainerDef(
        "vintage_cabinet_01_4k/vintage_cabinet_01_4k.gltf",
        [
            # counter under the upper hutch, its carved valance hangs down to ~0.345 above the counter
            Surface("counter_niche", 0.9112, (-0.81, 0.81), (-0.24, 0.08), max_height=0.34),
            # counter in front of the hutch, open above
            Surface("counter_front", 0.9112, (-0.96, 0.96), (0.10, 0.335), spread="justify"),
            # shelves behind the glass doors
            Surface("upper_shelf_1", 1.370, (-0.93, 0.93), (-0.24, 0.08), max_height=0.20),
            Surface("upper_shelf_2", 1.600, (-0.93, 0.93), (-0.24, 0.07), max_height=0.19),
            Surface("upper_shelf_3", 1.820, (-0.76, 0.76), (-0.24, 0.07), max_height=0.15),
        ],
        material_overrides={
            # zero thickness panes authored as alpha blended dark glass, yapt has no alpha so make them clear
            # glass. IOR ~1 since a single refracting surface would bend the rays without bending them back.
            "vintage_cabinet_01_glass": {
                "setTransparency": 1.0, "setAlbedo": (1.0, 1.0, 1.0), "setMetalness": 0.0,
                "setRoughness": 0.0, "setDielectricIOR": 1.02, "setClearCoatAmount": 0.0,
            },
        }),
}

PLACEABLES = {
    "lantern": PlaceableDef("Lantern_01_4k/Lantern_01_4k.gltf"),
    "brass_goblets": PlaceableDef("brass_goblets_4k/brass_goblets_4k.gltf", split_nodes=True),
    "brass_vase": PlaceableDef("brass_vase_02_4k/brass_vase_02_4k.gltf"),
    "industrial_pipe_lamp": PlaceableDef("industrial_pipe_lamp_4k/industrial_pipe_lamp_4k.gltf"),
    "wine_bottles": PlaceableDef("wine_bottles_01_4k/wine_bottles_01_4k.gltf", split_nodes=True),
}

# ---------------------------------------------------------------------------
# glTF bounds
# ---------------------------------------------------------------------------

def _node_bounds(gltf, node_index):
    """AABB of a root node's mesh in glTF scene space ((min xyz), (max xyz)), None if it has no mesh.
    Children are ignored, like the SceneLoader does."""
    node = gltf.nodes[node_index]
    if node.mesh is None:
        return None
    scale = node.scale or (1.0, 1.0, 1.0)
    rotation = tuple(node.rotation) if node.rotation else Q_IDENTITY
    translation = node.translation or (0.0, 0.0, 0.0)
    lo, hi = [math.inf] * 3, [-math.inf] * 3
    for prim in gltf.meshes[node.mesh].primitives:
        accessor = gltf.accessors[prim.attributes.POSITION]
        if accessor.min is None or accessor.max is None:
            continue
        for corner in itertools.product(*zip(accessor.min, accessor.max)):
            p = q_rotate(rotation, [corner[i] * scale[i] for i in range(3)])
            for i in range(3):
                v = p[i] + translation[i]
                lo[i], hi[i] = min(lo[i], v), max(hi[i], v)
    return (tuple(lo), tuple(hi)) if lo[0] != math.inf else None

def _union_bounds(bounds):
    bounds = [b for b in bounds if b is not None]
    if not bounds:
        return None
    return (tuple(min(b[0][i] for b in bounds) for i in range(3)), tuple(max(b[1][i] for b in bounds) for i in range(3)))

def _node_xz_points(gltf, gltf_path, node_index):
    """Vertex positions of a root node projected to XZ (glTF scene space), thinned to a 2 mm grid.
    Only external .bin buffers with float VEC3 positions are read, anything else returns None."""
    node = gltf.nodes[node_index]
    if node.mesh is None:
        return None
    rotation = quat_matrix(tuple(node.rotation) if node.rotation else Q_IDENTITY)
    scale = np.array(node.scale or (1.0, 1.0, 1.0))
    translation = np.array(node.translation or (0.0, 0.0, 0.0))
    points = []
    for prim in gltf.meshes[node.mesh].primitives:
        accessor = gltf.accessors[prim.attributes.POSITION]
        view = gltf.bufferViews[accessor.bufferView] if accessor.bufferView is not None else None
        if view is None or accessor.componentType != 5126 or accessor.type != "VEC3":
            return None
        uri = gltf.buffers[view.buffer].uri
        if uri is None or uri.startswith("data:"):
            return None
        with open(os.path.join(os.path.dirname(gltf_path), uri), "rb") as f:
            f.seek((view.byteOffset or 0) + (accessor.byteOffset or 0))
            stride = view.byteStride or 12
            raw = np.frombuffer(f.read(stride * (accessor.count - 1) + 12), dtype=np.uint8)
        pos = np.lib.stride_tricks.as_strided(raw.view(np.float32), shape=(accessor.count, 3), strides=(stride, 4))
        points.append((pos * scale) @ rotation.T + translation)
    xz = np.concatenate(points)[:, [0, 2]]
    return np.unique(np.round(xz / 0.002), axis=0) * 0.002

def quat_matrix(q):
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])

def _footprint(xz_points, yaw_degrees):
    """Extents of the points rotated about Y the same way the object is: (width, depth, (centre x, centre z))."""
    rotation = quat_matrix(q_axis_angle((0.0, 1.0, 0.0), yaw_degrees))
    rotated = xz_points @ rotation[np.ix_([0, 2], [0, 2])].T
    lo, hi = rotated.min(axis=0), rotated.max(axis=0)
    return hi[0] - lo[0], hi[1] - lo[1], ((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2)

# ---------------------------------------------------------------------------
# Arranging items onto surfaces
# ---------------------------------------------------------------------------

class _Item:
    def __init__(self, key, width, depth, height, surfaces=None):
        self.key = key
        self.width = width
        self.depth = depth
        self.height = height
        self.surfaces = surfaces        # allowed surface names, None = any
        self.rotated = False            # turned 90 degrees to fit
        self.surface = None
        self.position = None            # (x, z) centre in container space

class _Row:
    def __init__(self):
        self.items = []
        self.width = 0.0
        self.depth = 0.0

def arrange(items, surfaces, gap, edge_margin, height_margin, allow_rotate=True, tall_to_back=True):
    """Packs the items into rows on the surfaces, first fit in the given item and surface order, then lays the rows
    out from the back of each surface to the front (tallest row first with tall_to_back). Sets surface / position /
    rotated on the items that fit and returns the ones that didn't."""
    rows = {s.name: [] for s in surfaces}
    unplaced = []

    def used_depth(surface_rows, replaced=None, new_depth=0.0):
        depths = [new_depth if r is replaced else r.depth for r in surface_rows]
        return sum(depths) + gap * max(0, len(depths) - 1)

    def try_place(item, surface, rotated):
        width, depth = (item.depth, item.width) if rotated else (item.width, item.depth)
        usable_w = surface.x[1] - surface.x[0] - 2.0 * edge_margin
        usable_d = surface.z[1] - surface.z[0] - 2.0 * edge_margin
        if width > usable_w or depth > usable_d:
            return False
        surface_rows = rows[surface.name]
        for row in surface_rows:
            if row.width + gap + width <= usable_w and used_depth(surface_rows, row, max(row.depth, depth)) <= usable_d:
                row.items.append((item, width, depth))
                row.width += gap + width
                row.depth = max(row.depth, depth)
                return True
        if used_depth(surface_rows) + (gap if surface_rows else 0.0) + depth <= usable_d:
            row = _Row()
            row.items.append((item, width, depth))
            row.width, row.depth = width, depth
            surface_rows.append(row)
            return True
        return False

    for item in items:
        placed = False
        for surface in surfaces:
            if item.surfaces is not None and surface.name not in item.surfaces:
                continue
            if surface.max_height is not None and item.height > surface.max_height - height_margin:
                continue
            for rotated in ((False, True) if allow_rotate else (False,)):
                if try_place(item, surface, rotated):
                    item.surface, item.rotated, placed = surface, rotated, True
                    break
            if placed:
                break
        if not placed:
            unplaced.append(item)

    # spread the rows over the surface depth and the items over the row width
    for surface in surfaces:
        surface_rows = rows[surface.name]
        if not surface_rows:
            continue
        if tall_to_back:
            surface_rows.sort(key=lambda r: -max(item.height for item, _, _ in r.items))
        x0, x1 = surface.x[0] + edge_margin, surface.x[1] - edge_margin
        z0, z1 = surface.z[0] + edge_margin, surface.z[1] - edge_margin
        row_spacing = (z1 - z0 - sum(r.depth for r in surface_rows)) / (len(surface_rows) + 1)
        z = z0
        for row in surface_rows:
            z += row_spacing
            free = (x1 - x0) - sum(w for _, w, _ in row.items)
            count = len(row.items)
            if surface.spread == "justify" and count > 1:
                x, spacing = x0, free / (count - 1)
            else:
                spacing = free / (count + 1)
                x = x0 + spacing
            for item, width, _ in row.items:
                item.position = (x + width * 0.5, z + row.depth * 0.5)
                x += width + spacing
            z += row.depth
    return unplaced

# ---------------------------------------------------------------------------
# The scene
# ---------------------------------------------------------------------------

def _to_setter_value(value):
    if isinstance(value, (tuple, list)):
        return vec3([float(v) for v in value]) if len(value) == 3 else vec2([float(v) for v in value])
    return value

class ContainerSceneBuilder:
    def __init__(self, app, config, seed=None, log_tag="container_scene"):
        self._app = app
        self._resources = app.get_resources()
        self._renderer = app.get_renderer()
        self._scene = app.get_scene()
        self._cfg = config
        self._seed = resolve_seed(seed)
        self._rng = random.Random(self._seed)
        self._log_tag = log_tag
        self._counter = 0
        self._placed_bounds = []    # world AABBs of the arranged objects
        self.layout = {"objects": []}   # what was placed where, handy for debugging

    # --- helpers -------------------------------------------------------------

    def _unique(self, name):
        self._counter += 1
        return f"cs_{name}_{self._counter}"

    def _log(self, text):
        print(f"[{self._log_tag}] {text}")

    def _new_material(self, name, albedo, roughness):
        mat = self._renderer.createMaterial(self._unique(name))
        mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
        mat.setAlbedo(v3l(albedo))
        mat.setRoughness(roughness)
        mat.setTwoSided(True)
        return mat

    def _create_mesh_instance(self, path, material, position, scale, quat):
        mesh_array, prim_map = self._resources.load_meshes_from_path(path)[0]
        objects = []
        for mesh in mesh_array:
            obj = self._scene.create_render_object(self._unique(os.path.splitext(os.path.basename(path))[0]))
            obj.setMesh(mesh)
            transform = obj.getTransform()
            transform.setScale(v3l(scale))
            transform.setOrientation(vec4(list(quat)))
            transform.setTranslation(v3l(position))
            objects.append(obj)
        for mapping in prim_map:
            if mapping is not None:
                objects[mapping[0]].setMaterial(material, mapping[1])
        return objects

    def _load_gltf(self, path):
        loader = SceneLoader(self._resources, self._scene, self._renderer)
        roots = loader.load_scene_gltf(path)
        gltf = loader.get_gltf()
        root_nodes = gltf.scenes[gltf.scene or 0].nodes
        return loader, list(zip(root_nodes, roots))

    def _attach(self, objects, name, translation, quat, local_offset):
        """Parents the loaded root objects under a pivot at translation / quat; local_offset moves them inside it."""
        pivot = self._scene.create_scene_object(self._unique(f"{name}_pivot"))
        pivot.getTransform().setTranslation(v3l(translation))
        pivot.getTransform().setOrientation(vec4(list(quat)))
        offset = self._scene.create_scene_object(self._unique(f"{name}_offset"))
        offset.getTransform().setParent(pivot.getTransform())
        offset.getTransform().setTranslation(v3l(local_offset))
        for obj in objects:
            if obj is not None:
                obj.getTransform().setParent(offset.getTransform())
        return pivot

    # --- building blocks -----------------------------------------------------

    def _setup_environment(self):
        env_var = self._renderer.getRendererVariable("World.EnvIntensityScale")
        if env_var is not None:
            env_var.setFromFloatArray([float(self._cfg.get("env_intensity", 0.0))])
        else:
            self._log("renderer variable World.EnvIntensityScale not found")

    def _build_room(self):
        room = self._cfg["room"]
        w, d, h = room["size"]
        plane = f"{self._cfg['asset_root']}/Plane/plane.glb"   # 2 x 2 in XZ, facing +Y

        floor_mat = self._new_material("floor", (1.0, 1.0, 1.0), room.get("floor_roughness", 0.55))
        floor_albedo = room.get("floor_albedo_texture")
        floor_normal = room.get("floor_normal_texture")
        tile = room.get("floor_texture_size", 2.0)
        uv_scale = vec2([w / tile, d / tile])
        if floor_albedo and os.path.exists(floor_albedo):
            tex = self._resources.load_texture_2D(floor_albedo, "R8G8B8A8_SRGB")
            if tex is not None:
                floor_mat.setAlbedoTexture(tex, uv_scale)
        else:
            floor_mat.setAlbedo(v3l(to_linear(room.get("floor_color", (0.45, 0.4, 0.35)))))
        if floor_normal and os.path.exists(floor_normal):
            tex = self._resources.load_texture_2D(floor_normal, "R8G8B8A8_UNORM")
            if tex is not None:
                floor_mat.setNormalTexture(tex, uv_scale)

        wall_mat = self._new_material("wall", to_linear(room.get("wall_color", (0.78, 0.75, 0.7))), room.get("wall_roughness", 0.85))
        ceiling_mat = self._new_material("ceiling", to_linear(room.get("ceiling_color", (0.85, 0.85, 0.85))), 0.9)

        # (material, position, scale, orientation): plane normal +Y turned to face into the room
        panels = [
            (floor_mat, (0.0, 0.0, 0.0), (w / 2, 1.0, d / 2), Q_IDENTITY),
            (ceiling_mat, (0.0, h, 0.0), (w / 2, 1.0, d / 2), q_axis_angle((1.0, 0.0, 0.0), 180.0)),
            (wall_mat, (0.0, h / 2, -d / 2), (w / 2, 1.0, h / 2), q_axis_angle((1.0, 0.0, 0.0), 90.0)),
            (wall_mat, (0.0, h / 2, d / 2), (w / 2, 1.0, h / 2), q_axis_angle((1.0, 0.0, 0.0), -90.0)),
            (wall_mat, (-w / 2, h / 2, 0.0), (h / 2, 1.0, d / 2), q_axis_angle((0.0, 0.0, 1.0), -90.0)),
            (wall_mat, (w / 2, h / 2, 0.0), (h / 2, 1.0, d / 2), q_axis_angle((0.0, 0.0, 1.0), 90.0)),
        ]
        for mat, pos, scale, quat in panels:
            self._create_mesh_instance(plane, mat, pos, scale, quat)

    def _place_container(self):
        name = self._cfg["container"]
        definition = CONTAINERS[name]
        loader, roots = self._load_gltf(f"{self._cfg['converted_root']}/{definition.path}")
        gltf = loader.get_gltf()
        lo, hi = _union_bounds([_node_bounds(gltf, node) for node, _ in roots])

        # back of the container against the back wall, centred on x, standing on the floor
        w, d, h = self._cfg["room"]["size"]
        back_z = -d / 2 + self._cfg["room"].get("container_wall_gap", 0.03)
        self._container_offset = (-(lo[0] + hi[0]) / 2, -lo[1], back_z - lo[2])
        container = self._scene.create_scene_object(self._unique(f"{name}_root"))
        container.getTransform().setTranslation(v3l(self._container_offset))
        for _, obj in roots:
            if obj is not None:
                obj.getTransform().setParent(container.getTransform())

        for mat_name, setters in definition.material_overrides.items():
            mat = loader.get_material(mat_name)
            if mat is None:
                self._log(f"material override: no material {mat_name} in {definition.path}")
                continue
            for setter, value in setters.items():
                getattr(mat, setter)(_to_setter_value(value))

        self._container = definition
        self._container_bounds = (tuple(lo[i] + self._container_offset[i] for i in range(3)),
                                  tuple(hi[i] + self._container_offset[i] for i in range(3)))
        self.layout["container"] = {"name": name, "path": definition.path, "offset": self._container_offset}

    def _object_entries(self):
        entries = []
        for entry in self._cfg["objects"]:
            if isinstance(entry, str):
                entry = {"name": entry}
            entries.append(entry)
        return entries

    def _arrange_objects(self):
        cfg = self._cfg.get("arrange", {})
        jitter = cfg.get("yaw_jitter", 0.0)
        surfaces = [s for s in self._container.surfaces if s.name in cfg.get("surfaces", [s.name for s in self._container.surfaces])]

        # load everything first, the footprints come from the glTF geometry
        units = []
        for entry in self._object_entries():
            definition = PLACEABLES[entry["name"]]
            path = f"{self._cfg['converted_root']}/{definition.path}"
            loader, roots = self._load_gltf(path)
            gltf = loader.get_gltf()
            groups = [[root] for root in roots] if definition.split_nodes else [roots]
            for group in groups:
                bounds = _union_bounds([_node_bounds(gltf, node) for node, _ in group])
                if bounds is None:
                    continue
                lo, hi = bounds
                centre = np.array([(lo[0] + hi[0]) / 2, (lo[2] + hi[2]) / 2])
                points = [_node_xz_points(gltf, path, node) for node, _ in group]
                if any(p is None for p in points):
                    points = [np.array([[lo[0], lo[2]], [hi[0], lo[2]], [lo[0], hi[2]], [hi[0], hi[2]]])]
                points = np.concatenate(points) - centre

                yaw = entry.get("yaw", 0.0) + self._rng.uniform(-jitter, jitter)
                width, depth, _ = _footprint(points, yaw)
                label = gltf.nodes[group[0][0]].name if definition.split_nodes else entry["name"]
                item = _Item(len(units), width, depth, hi[1] - lo[1], entry.get("surfaces"))
                units.append({"item": item, "label": label, "objects": [obj for _, obj in group], "bounds": bounds,
                              "points": points, "yaw": yaw, "path": definition.path, "nodes": [node for node, _ in group]})

        # biggest footprints first packs tighter, the rows get sorted tallest to the back afterwards
        items = sorted((u["item"] for u in units), key=lambda it: -it.width * it.depth)
        unplaced = arrange(items, surfaces, cfg.get("gap", 0.02), cfg.get("edge_margin", 0.015), cfg.get("height_margin", 0.005),
                           cfg.get("allow_rotate", True), cfg.get("tall_to_back", True))

        for unit in units:
            item = unit["item"]
            if item.surface is None:
                for obj in unit["objects"]:
                    if obj is not None:
                        obj.getTransform().setTranslation(vec3([0.0, -100.0, 0.0]))   # parked out of sight
                continue
            lo, hi = unit["bounds"]
            yaw = unit["yaw"] + (90.0 if item.rotated else 0.0)
            width, depth, footprint_centre = _footprint(unit["points"], yaw)
            ox, oy, oz = self._container_offset
            translation = (item.position[0] - footprint_centre[0] + ox, item.surface.y + oy + 0.0005,
                           item.position[1] - footprint_centre[1] + oz)
            local_offset = (-(lo[0] + hi[0]) / 2, -lo[1], -(lo[2] + hi[2]) / 2)
            self._attach(unit["objects"], unit["label"], translation, q_axis_angle((0.0, 1.0, 0.0), yaw), local_offset)

            x, z = item.position[0] + ox, item.position[1] + oz
            self._placed_bounds.append(((x - width / 2, translation[1], z - depth / 2),
                                        (x + width / 2, translation[1] + item.height, z + depth / 2)))
            self.layout["objects"].append({"label": unit["label"], "path": unit["path"], "nodes": unit["nodes"],
                                           "surface": item.surface.name, "translation": translation, "yaw": yaw,
                                           "local_offset": local_offset, "footprint": ((x - width / 2, z - depth / 2), (x + width / 2, z + depth / 2))})
            self._log(f"  {unit['label']:28s} -> {item.surface.name} ({item.position[0]:+.2f}, {item.position[1]:+.2f}) yaw {yaw:.0f}")

        for item in unplaced:
            unit = units[item.key]
            self._log(f"  {unit['label']} ({item.width:.2f} x {item.depth:.2f} x {item.height:.2f} m) didn't fit on any surface, skipped")

    def _focus_bounds(self, frame):
        if frame == "container":
            return _union_bounds(self._placed_bounds + [self._container_bounds])
        if self._placed_bounds:
            return _union_bounds(self._placed_bounds)
        # nothing placed: frame the first surface
        s = self._container.surfaces[0]
        ox, oy, oz = self._container_offset
        return ((s.x[0] + ox, s.y + oy, s.z[0] + oz), (s.x[1] + ox, s.y + oy + 0.3, s.z[1] + oz))

    def _set_camera(self):
        cfg = self._cfg.get("camera", {})
        w, d, h = self._cfg["room"]["size"]
        cam = self._scene.get_main_camera()

        fov_y = math.radians(cfg.get("fov", 40.0))
        aspect = cam.getAspectRatio() if cam.getAspectRatio() > 0 else 16.0 / 9.0
        fov_x = 2.0 * math.atan(math.tan(fov_y / 2) * aspect)

        lo, hi = self._focus_bounds(cfg.get("frame", "objects"))
        focus = [(lo[i] + hi[i]) / 2 for i in range(3)]
        focus[1] = lo[1] + (hi[1] - lo[1]) * cfg.get("focus_height", 0.4)

        elevation = math.radians(cfg.get("elevation", 20.0))
        azimuth = math.radians(cfg.get("azimuth", 0.0))
        direction = (math.sin(azimuth) * math.cos(elevation), math.sin(elevation), math.cos(azimuth) * math.cos(elevation))

        distance = cfg.get("distance")
        if distance is None:
            # fit the arranged objects with some margin
            margin = cfg.get("frame_margin", 1.35)
            half_w = (hi[0] - lo[0]) / 2 * margin
            half_h = max(hi[1] - lo[1], 0.2) / 2 * margin
            distance = max(half_w / math.tan(fov_x / 2), half_h / math.tan(fov_y / 2)) + (hi[2] - lo[2]) / 2

        # stay inside the room
        inset = 0.15
        limits = [(-w / 2 + inset, w / 2 - inset), (inset, h - inset), (-d / 2 + inset, d / 2 - inset)]
        for i in range(3):
            if direction[i] > 1e-6:
                distance = min(distance, (limits[i][1] - focus[i]) / direction[i])
            elif direction[i] < -1e-6:
                distance = min(distance, (limits[i][0] - focus[i]) / direction[i])
        pos = [focus[i] + direction[i] * distance for i in range(3)]

        # camera looks down -Z: yaw about Y then pitch about X, same convention as the CameraController
        dx, dy, dz = focus[0] - pos[0], focus[1] - pos[1], focus[2] - pos[2]
        yaw = math.degrees(math.atan2(-dx, -dz))
        pitch = math.degrees(math.atan2(dy, math.hypot(dx, dz)))
        quat = q_mul(q_axis_angle((0.0, 1.0, 0.0), yaw), q_axis_angle((1.0, 0.0, 0.0), pitch))

        cam.setFOVY(fov_y)
        cam.setNearPlane(cfg.get("near", 0.01))
        cam.setFarPlane(cfg.get("far", 50.0))
        cam.getTransform().setTranslation(v3l(pos))
        cam.getTransform().setOrientation(vec4(list(quat)))

        controller = self._app.get_camera_controller()
        if controller is not None and "move_speed" in cfg:
            controller.set_movement_speed(cfg["move_speed"])

        self._focus = focus
        self.layout["camera"] = {"position": pos, "focus": focus, "fov_y": math.degrees(fov_y), "aspect": aspect}

    def _build_lights(self):
        """Sphere lights; emission is set so each gives roughly the wanted irradiance at the focus point."""
        sphere = f"{self._cfg['asset_root']}/Sphere/Sphere.glb"
        for light in self._cfg.get("lights", []):
            pos = light["position"]
            radius = light.get("radius", 0.15)
            tint = light.get("tint", (1.0, 1.0, 1.0))
            dist2 = sum((pos[i] - self._focus[i]) ** 2 for i in range(3))
            radiance = light.get("irradiance", 3.0) * dist2 / (math.pi * radius * radius)
            luminance = 0.2126 * tint[0] + 0.7152 * tint[1] + 0.0722 * tint[2]
            mat = self._renderer.createMaterial(self._unique("mat_light"))
            mat.setEmission(v3l(tuple(c / luminance * radiance for c in tint)))
            self._create_mesh_instance(sphere, mat, pos, (radius, radius, radius), Q_IDENTITY)
        self.layout["lights"] = self._cfg.get("lights", [])

    # --- the scene -------------------------------------------------------------

    def build(self):
        self._log(f"seed = {self._seed}, container = {self._cfg['container']}")
        self._setup_environment()
        self._build_room()
        self._place_container()
        self._arrange_objects()
        self._set_camera()
        self._build_lights()
        return self
