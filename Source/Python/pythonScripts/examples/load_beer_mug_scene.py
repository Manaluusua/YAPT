"""
Beer mug caustic scene.

A dimpled one litre glass mug of lager with a foam head stands on a wooden table in front of a wall with a window.
Low sun comes in through the window from behind the mug, and the thick glass and the beer focus it into a golden
caustic on the table in front of the mug. Condensation droplets sit on the outside of the glass and bubbles rise
in the beer.

  - the sun is a small emissive quad far behind the wall with a very narrow emission lobe, so its light is nearly
    parallel and the caustic stays sharp. It is placed so the wall hides it from the camera
  - the beer is a closed volume a hair inside the glass cavity. The renderer tracks nested volumes on a stack
    without priorities, so the glass and the beer can't share a surface: the thin air gap keeps every interface
    entering or leaving exactly one volume. The bubbles are air volumes inside the beer
  - the garden seen through the window is a generated environment map
  - BDPT is selected, the caustic needs light paths traced from the sun through the glass

All geometry is generated here, the table and wall use textures/weathered_planks. Units are centimetres, the
table top is at y = 0.
Run it via the "run script" action of the app on an empty scene; objects are added, nothing is cleared.
"""

import base64
import hashlib
import math
import os

import numpy as np
from pygltflib import GLTF2, Accessor, Attributes, Buffer, BufferView, Primitive, DATA_URI_HEADER
from pygltflib import Mesh as GltfMesh, FLOAT, UNSIGNED_INT, SCALAR, VEC2, VEC3, VEC4

from py_yapt import Material, ResourceDimension, ResourceFormat, ResourceUsageBits, vec2, vec3, vec4
from yapt.random_shapes import q_axis_angle, q_mul
from yapt.settings import resolve_asset_path

# ---------------------------------------------------------------------------
# Knobs
# ---------------------------------------------------------------------------

SEED = 7

MUG_POSITION = (0.0, 0.0, 0.0)
MUG_YAW = -25.0                     # degrees, the handle points along +x at 0
MUG_SEGMENTS = 420
MUG_ROW_SPACING = 0.07              # resolution along the profile, the dimple edges need it fine
BEER_GAP = 0.01                     # air between the glass and the beer
BEER_LEVEL = 17.6
FOAM_OVERLAP = 0.6                  # how deep the foam reaches into the beer

# mug profile: (radius, height) from the bottom on the axis, up the outside, over the rim and down the inside back
# to the axis. The outside gets the dimples, the inside stays smooth
MUG = [(0.0, 0.0), (4.8, 0.0), (5.45, 0.12), (5.72, 0.6), (5.68, 1.4), (5.66, 3.0), (5.8, 10.0), (5.95, 16.0),
       (6.0, 18.6), (5.95, 19.3), (5.72, 19.55), (5.46, 19.3), (5.42, 18.6), (5.38, 16.0), (5.25, 10.0),
       (5.08, 3.6), (4.75, 2.35), (3.5, 2.0), (0.0, 1.95)]

# oval dimples in staggered rows: row heights, dimples per row, half width (arc length) and half height, depth.
# Every other row is rotated by half a step, the dimple under the handle is left out
DIMPLE_ROWS = [3.3, 6.3, 9.3, 12.3, 15.3]
DIMPLES_PER_ROW = 8
DIMPLE_SIZE = (1.9, 1.65)
DIMPLE_DEPTH = 0.3
DIMPLE_SHARPNESS = 0.75             # < 1 steepens the dimple walls into ridges

# handle centre line (distance from the axis, height) in the plane of the handle, ends sunk into the wall,
# and the half width / half depth of its rounded cross section
HANDLE_PATH = [(5.55, 17.9), (6.6, 18.15), (8.6, 17.7), (10.0, 15.9), (10.6, 13.0), (10.5, 10.0), (9.6, 7.6),
               (7.9, 6.4), (6.6, 6.2), (5.4, 6.3)]
HANDLE_SECTION = (0.95, 0.7)

# foam head above the rim: (radius, height) after it leaves the wall, ending on the axis
FOAM_DOME = [(5.5, 19.7), (5.78, 20.2), (5.55, 20.95), (4.3, 21.55), (2.2, 21.9), (0.0, 22.0)]
FOAM_LUMPS = 0.12

BUBBLE_STREAMS = 7
LOOSE_BUBBLES = 120
DROPLETS = 900
DROPLET_RADIUS = (0.05, 0.34)

GLASS = {"preset": "GLASS", "roughness": 0.0, "ior": 1.52, "tint": ((0.93, 0.98, 0.95), 6.0)}
BEER = {"preset": "GLASS", "roughness": 0.0, "ior": 1.345, "tint": ((0.9, 0.56, 0.1), 11.0)}
AIR = {"preset": "GLASS", "roughness": 0.0, "ior": 1.0}
WATER = {"preset": "GLASS", "roughness": 0.0, "ior": 1.333}
FOAM = {"preset": "PLASTIC", "albedo": (0.9, 0.84, 0.68), "roughness": 0.85, "subsurface": 1.0,
        "subsurface_radius": (1.0, 0.85, 0.55), "sss_distance": 0.4}
TABLE_WOOD = {"preset": "PLASTIC", "albedo": (0.95, 0.78, 0.56), "roughness": 0.55, "clearcoat": 0.35,
              "clearcoat_roughness": 0.18}
WALL_WOOD = {"preset": "PLASTIC", "albedo": (0.75, 0.6, 0.45), "roughness": 0.7}
FRAME_WOOD = {"preset": "PLASTIC", "albedo": (0.12, 0.075, 0.045), "roughness": 0.5, "clearcoat": 0.3,
              "clearcoat_roughness": 0.2}
BROWN_GLASS = {"preset": "GLASS", "roughness": 0.0, "ior": 1.52, "tint": ((0.45, 0.16, 0.03), 2.0)}
GREEN_GLASS = {"preset": "GLASS", "roughness": 0.0, "ior": 1.52, "tint": ((0.25, 0.55, 0.2), 2.0)}
CERAMIC = {"preset": "PLASTIC", "albedo": (0.78, 0.74, 0.66), "roughness": 0.3, "clearcoat": 1.0,
           "clearcoat_roughness": 0.05}

PLANKS = "textures/weathered_planks/weathered_planks"
PLANKS_SIZE = 100.0                 # cm one tile of the plank texture covers

TABLE_SIZE = (300.0, 4.0)           # width, thickness, it runs from the wall to TABLE_FRONT
TABLE_FRONT = 50.0
WALL_Z = -75.0
WALL_THICKNESS = 12.0
WALL_EXTENT = ((-260.0, 260.0), (-10.0, 220.0))
WINDOW = (76.0, 88.0)               # width, height
WINDOW_FRAME = (5.0, 4.0, 6.0)      # outer frame width, mullion width, depth

# sun: direction towards it (degrees, azimuth from -z towards -x, elevation), distance, quad size, emission lobe,
# and the irradiance it gives at the mug. The window is placed so the mug sees the sun through the middle of its
# upper left pane, clear of the mullions
SUN_AZIMUTH = 28.0
SUN_ELEVATION = 24.0
SUN_DISTANCE = 600.0
SUN_SIZE = 12.0
SUN_FOCUS = 200.0
SUN_IRRADIANCE = 15.0
SUN_COLOR = (1.0, 0.8, 0.55)

# things on the table by the wall: lathe profile, material, x
BOTTLE = [(0.0, 0.0), (3.0, 0.0), (3.2, 0.4), (3.2, 13.0), (2.8, 15.5), (1.4, 18.5), (1.2, 21.5), (1.45, 22.2),
          (1.3, 22.8), (0.0, 22.8)]
TALL_BOTTLE = [(0.0, 0.0), (3.6, 0.0), (3.8, 0.4), (3.8, 17.0), (3.3, 20.0), (1.5, 23.0), (1.3, 28.5), (1.55, 29.0),
               (1.35, 29.6), (0.0, 29.6)]
JUG = [(0.0, 0.0), (4.0, 0.0), (4.6, 1.0), (5.4, 6.0), (5.0, 11.0), (3.6, 14.0), (3.4, 17.0), (3.9, 18.2),
       (3.6, 18.4), (0.0, 18.2)]
BACK_ITEMS = [(JUG, CERAMIC, -46.0), (BOTTLE, BROWN_GLASS, -34.0), (TALL_BOTTLE, GREEN_GLASS, -25.0)]
BACK_ITEMS_Z = WALL_Z + 9.0

# environment: garden behind the window, a dim warm room everywhere else
ENV_RESOLUTION = (1024, 512)
ENV_GARDEN_INTENSITY = 1.0
ENV_ROOM = (0.025, 0.018, 0.011)
ENV_FOLIAGE_BLOBS = 90
EXPOSURE_COMPENSATION = 0.8                # EV added to the metered scene before auto exposure, keeps the room dim

CAMERA_POSITION = (-24.0, 22.0, 60.0)
CAMERA_TARGET = (-4.0, 7.0, 4.0)
CAMERA_FOV_Y = 36.0

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

def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)

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

# --- materials ---------------------------------------------------------------

FLOAT_SETTERS = {
    "roughness": "setRoughness",
    "ior": "setDielectricIOR",
    "clearcoat": "setClearCoatAmount",
    "clearcoat_roughness": "setClearCoatRoughness",
    "subsurface": "setSubsurfaceAmount",
    "sss_distance": "setSubsurfaceRadiusScale",
}

COLOR_SETTERS = {
    "albedo": "setAlbedo",
    "subsurface_radius": "setSubsurfaceRadius",
}

def make_material(name, params):
    mat = renderer.createMaterial(name)
    mat.setFromMaterialPreset(getattr(Material.MaterialPreset, params["preset"]))
    for key, value in params.items():
        if key in FLOAT_SETTERS:
            getattr(mat, FLOAT_SETTERS[key])(float(value))
        elif key in COLOR_SETTERS:
            getattr(mat, COLOR_SETTERS[key])(v3(value))
        elif key == "tint":
            color, distance = value
            mat.setAbsorption(v3([-math.log(max(c, 1e-4)) / distance for c in color]))
        elif key != "preset":
            raise KeyError(f"unknown material parameter {key}")
    return mat

def planks_material(name, params):
    mat = make_material(name, params)
    for suffix, fmt, setter in (("diff_4k", "R8G8B8A8_SRGB", mat.setAlbedoTexture),
                                ("nor_gl_4k", "R8G8B8A8_UNORM", mat.setNormalTexture)):
        path = f"{PLANKS}_{suffix}.png"
        if os.path.exists(resolve_asset_path(path)):
            texture = resources.load_texture_2D(path, fmt)
            if texture is not None:
                setter(texture, vec2([1.0, 1.0]))
    return mat

def emissive_material(name, color, radiance, focus):
    mat = make_material(name, {"preset": "PLASTIC", "albedo": (0.0, 0.0, 0.0), "roughness": 1.0})
    mat.setEmission(v3([c * radiance for c in color]))
    mat.setEmissionFocus(float(focus))
    return mat

# --- meshes ------------------------------------------------------------------

def upload_mesh(name, positions, normals, tangents, indices, uvs):
    """Packs the arrays into an in-memory glTF and goes through the regular glTF mesh loader."""
    chunks = [
        np.asarray(indices, dtype=np.uint32).reshape(-1),
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

def grid_arrays(grid, pole_rows=()):
    """Triangulates a (rows, cols, 3) grid of points whose columns close into a ring (a duplicated seam column keeps
    u running from 0 to 1). The grid has to enclose a volume, the winding is picked so the normals point out of it.
    The rows in pole_rows collapse onto one point and share one normal."""
    rows, cols, _ = grid.shape
    along = np.roll(grid, -1, axis=1) - np.roll(grid, 1, axis=1)
    grid = np.concatenate([grid, grid[:, :1]], axis=1)
    along = np.concatenate([along, along[:, :1]], axis=1)
    cols += 1
    idx = np.arange(rows * cols).reshape(rows, cols)
    c0, c1 = idx[:, :-1], idx[:, 1:]
    a, b, c, d = c0[:-1], c1[:-1], c1[1:], c0[1:]
    tris = np.concatenate([np.stack([a, b, c], -1).reshape(-1, 3), np.stack([a, c, d], -1).reshape(-1, 3)])
    pos = grid.reshape(-1, 3)

    p0, p1, p2 = pos[tris[:, 0]], pos[tris[:, 1]], pos[tris[:, 2]]
    centre = pos.mean(axis=0)
    if np.einsum("ij,ij->", p0 - centre, np.cross(p1 - centre, p2 - centre)) < 0.0:
        tris = tris[:, ::-1]
    p0, p1, p2 = pos[tris[:, 0]], pos[tris[:, 1]], pos[tris[:, 2]]
    face_normals = np.cross(p1 - p0, p2 - p0)

    normals = np.zeros_like(pos)
    for k in range(3):
        np.add.at(normals, tris[:, k], face_normals)
    normals = normals.reshape(rows, cols, 3)
    seam = normals[:, 0] + normals[:, -1]
    normals[:, 0], normals[:, -1] = seam, seam
    for r in pole_rows:
        normals[r] = normals[r].sum(axis=0)
    normals /= np.maximum(np.linalg.norm(normals, axis=-1, keepdims=True), 1e-12)

    along -= normals * np.sum(along * normals, axis=-1, keepdims=True)
    fallback = np.cross(normals, [0.0, 0.0, 1.0])
    along = np.where(np.linalg.norm(along, axis=-1, keepdims=True) > 1e-8, along, fallback)
    along /= np.maximum(np.linalg.norm(along, axis=-1, keepdims=True), 1e-12)
    tangents = np.concatenate([along, np.ones((rows, cols, 1))], axis=-1)

    u, v = np.meshgrid(np.arange(cols) / (cols - 1), np.arange(rows) / (rows - 1))
    uvs = np.stack([u, v], axis=-1)
    return pos, normals.reshape(-1, 3), tangents.reshape(-1, 4), tris, uvs.reshape(-1, 2)

def merge_arrays(parts):
    positions, normals, tangents, indices, uvs = [], [], [], [], []
    base = 0
    for pos, nrm, tan, tris, uv in parts:
        positions.append(pos)
        normals.append(nrm)
        tangents.append(tan)
        indices.append(tris + base)
        uvs.append(uv)
        base += len(pos)
    return (np.concatenate(positions), np.concatenate(normals), np.concatenate(tangents), np.concatenate(indices),
            np.concatenate(uvs))

def grid_mesh(name, grid, pole_rows=()):
    return upload_mesh(name, *grid_arrays(grid, pole_rows))

def smooth_profile(points, steps=8):
    """Catmull-Rom curve through the profile points."""
    pts = np.asarray(points, dtype=np.float64)
    padded = np.vstack([2 * pts[0] - pts[1], pts, 2 * pts[-1] - pts[-2]])
    out = []
    for i in range(len(pts) - 1):
        p0, p1, p2, p3 = padded[i:i + 4]
        for t in np.arange(steps) / steps:
            out.append(0.5 * (2 * p1 + (p2 - p0) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t
                              + (3 * p1 - p0 - 3 * p2 + p3) * t ** 3))
    out.append(pts[-1])
    out = np.array(out)
    out[:, 0] = np.maximum(out[:, 0], 0.0)
    return out

def resample(points, spacing):
    """Polyline resampled at even arc length steps, and the arc length of every new point."""
    s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(points, axis=0), axis=1))])
    t = np.linspace(0.0, s[-1], max(2, int(math.ceil(s[-1] / spacing)) + 1))
    return np.stack([np.interp(t, s, points[:, 0]), np.interp(t, s, points[:, 1])], axis=-1), t

def lathe_grid(profile, segments):
    """Spins the (radius, height) profile around +y, phi = 0 is +x."""
    phi = np.arange(segments) / segments * 2.0 * math.pi
    r = profile[:, 0:1]
    y = np.broadcast_to(profile[:, 1:2], (len(profile), segments))
    return np.stack([r * np.cos(phi), y, r * np.sin(phi)], axis=-1), phi

def pole_rows(profile):
    return [i for i in (0, len(profile) - 1) if profile[i, 0] < 1e-6]

def lathe_mesh(name, profile, segments=160):
    prof = smooth_profile(profile)
    return grid_mesh(name, lathe_grid(prof, segments)[0], pole_rows(prof))

def sphere_grid(center, radius, rings=6, segments=8):
    theta = np.linspace(0.0, math.pi, rings + 1)[:, None]
    phi = np.arange(segments)[None, :] / segments * 2.0 * math.pi
    dirs = np.stack([np.sin(theta) * np.cos(phi), np.broadcast_to(np.cos(theta), (rings + 1, segments)),
                     np.sin(theta) * np.sin(phi)], axis=-1)
    return center + dirs * radius

def box_arrays(lo, hi, uv_axes=(0, 1), uv_scale=1.0):
    """Flat shaded box, the texture is projected along uv_axes (world axes giving u and v) on every face, faces
    parallel to one of them use the remaining axis instead."""
    lo, hi = np.asarray(lo, dtype=np.float64), np.asarray(hi, dtype=np.float64)
    positions, normals, tangents, indices, uvs = [], [], [], [], []
    for axis in range(3):
        j, k = (axis + 1) % 3, (axis + 2) % 3
        u_axis, v_axis = [a if a != axis else 3 - uv_axes[0] - uv_axes[1] for a in uv_axes]
        for side in (0, 1):
            normal = np.zeros(3)
            normal[axis] = 1.0 if side else -1.0
            corners = []
            for sj, sk in ((0, 0), (1, 0), (1, 1), (0, 1)):
                p = np.empty(3)
                p[axis] = hi[axis] if side else lo[axis]
                p[j] = hi[j] if sj else lo[j]
                p[k] = hi[k] if sk else lo[k]
                corners.append(p)
            if np.dot(np.cross(corners[1] - corners[0], corners[2] - corners[0]), normal) < 0:
                corners = corners[::-1]
            tangent = np.zeros(3)
            tangent[u_axis] = 1.0
            base = len(positions)
            for p in corners:
                positions.append(p)
                normals.append(normal)
                tangents.append([*tangent, 1.0])
                uvs.append([p[u_axis] / uv_scale, p[v_axis] / uv_scale])
            indices += [[base, base + 1, base + 2], [base, base + 2, base + 3]]
    return (np.array(positions), np.array(normals), np.array(tangents), np.array(indices, dtype=np.int64),
            np.array(uvs))

def place(name, mesh, material, position=(0.0, 0.0, 0.0), orientation=(0.0, 0.0, 0.0, 1.0)):
    obj = scene.create_render_object(name)
    obj.setMesh(mesh)
    obj.setMaterial(material, 0)
    obj.getTransform().setOrientation(vec4(list(orientation)))
    obj.getTransform().setTranslation(v3(position))
    return obj

# --- mug -----------------------------------------------------------------------

def dimple_centers():
    step = 2.0 * math.pi / DIMPLES_PER_ROW
    centers = []
    for row, y in enumerate(DIMPLE_ROWS):
        for i in range(DIMPLES_PER_ROW):
            phi = (i + (0.5 if row % 2 == 0 else 0.0)) * step
            if row % 2 == 1 and i == 0:
                continue
            centers.append((phi, y))
    return centers

DIMPLES = dimple_centers()

def dimple_depth(phi, y, radius):
    depth = np.zeros(np.broadcast(phi, y).shape)
    for phi_c, y_c in DIMPLES:
        dphi = (phi - phi_c + math.pi) % (2.0 * math.pi) - math.pi
        e2 = (dphi * radius / DIMPLE_SIZE[0]) ** 2 + ((y - y_c) / DIMPLE_SIZE[1]) ** 2
        depth = np.maximum(depth, DIMPLE_DEPTH * np.clip(1.0 - e2, 0.0, None) ** DIMPLE_SHARPNESS)
    return depth

class MugProfile:
    """The smoothed and evenly resampled mug profile, split into its outside and the cavity."""

    def __init__(self):
        self.points, self.s = resample(smooth_profile(MUG), MUG_ROW_SPACING)
        self.rim = int(np.argmax(self.points[:, 1]))
        outer = self.points[:self.rim]
        outer = outer[outer[:, 1] > 1.0]
        self.outer_y, self.outer_r = outer[:, 1], outer[:, 0]
        #the cavity from the bottom on the axis up to the rim
        self.cavity = self.points[self.rim:][::-1]
        wall = self.cavity[self.cavity[:, 1] > 3.0]
        self.inner_y, self.inner_r = wall[:, 1], wall[:, 0]

    def outer_radius(self, y):
        return np.interp(y, self.outer_y, self.outer_r)

    def inner_radius(self, y):
        return np.interp(y, self.inner_y, self.inner_r)

    def outer_point(self, phi, y):
        r = self.outer_radius(y) - dimple_depth(phi, y, self.outer_radius(y))
        return np.array([r * math.cos(phi), y, r * math.sin(phi)])

    def outer_normal(self, phi, y):
        eps = 1e-3
        d_phi = self.outer_point(phi + eps, y) - self.outer_point(phi - eps, y)
        d_y = self.outer_point(phi, y + eps) - self.outer_point(phi, y - eps)
        n = normalize(np.cross(d_phi, d_y))
        return n if np.dot(n, [math.cos(phi), 0.0, math.sin(phi)]) > 0.0 else -n

    def cavity_offset(self, gap):
        """The cavity outline moved gap into the cavity."""
        c = self.cavity
        tangent = np.gradient(c, axis=0)
        tangent /= np.maximum(np.linalg.norm(tangent, axis=1, keepdims=True), 1e-12)
        out = c + np.stack([-tangent[:, 1], tangent[:, 0]], axis=1) * gap
        out[0, 0] = 0.0
        return out

def cut_at(profile, height):
    """The part of the profile below height, ending exactly at it."""
    above = int(np.argmax(profile[:, 1] >= height))
    a, b = profile[above - 1], profile[above]
    t = (height - a[1]) / (b[1] - a[1])
    return np.vstack([profile[:above], a + (b - a) * t])

def create_mug_glass(name, mug, material, position, orientation):
    grid, phi = lathe_grid(mug.points, MUG_SEGMENTS)
    outside = mug.s[:, None] < mug.s[mug.rim]
    r = mug.points[:, 0:1]
    depth = np.where(outside, dimple_depth(phi[None, :], mug.points[:, 1:2], np.maximum(r, 1e-6)), 0.0)
    scale = np.where(r > 1e-6, (r - depth) / np.maximum(r, 1e-6), 1.0)
    grid[..., 0] *= scale
    grid[..., 2] *= scale
    place(name, grid_mesh(name, grid, pole_rows(mug.points)), material, position, orientation)

def create_handle(name, material, position, orientation):
    path, _ = resample(smooth_profile(HANDLE_PATH, 12), 0.08)
    tangent = np.gradient(path, axis=0)
    tangent /= np.linalg.norm(tangent, axis=1, keepdims=True)
    in_plane = np.stack([-tangent[:, 1], tangent[:, 0]], axis=1)
    t = np.arange(48) / 48 * 2.0 * math.pi
    exponent = 2.0 / 3.0
    across = HANDLE_SECTION[0] * np.sign(np.cos(t)) * np.abs(np.cos(t)) ** exponent
    up = HANDLE_SECTION[1] * np.sign(np.sin(t)) * np.abs(np.sin(t)) ** exponent
    rings = np.empty((len(path), len(t), 3))
    rings[..., 0] = path[:, 0:1] + in_plane[:, 0:1] * up[None, :]
    rings[..., 1] = path[:, 1:2] + in_plane[:, 1:2] * up[None, :]
    rings[..., 2] = np.broadcast_to(across[None, :], (len(path), len(t)))
    ends = [np.broadcast_to([path[i, 0], path[i, 1], 0.0], (1, len(t), 3)) for i in (0, -1)]
    grid = np.concatenate([ends[0], rings, ends[1]])
    place(name, grid_mesh(name, grid, (0, len(grid) - 1)), material, position, orientation)

def create_beer(name, mug, material, position, orientation):
    beer = cut_at(mug.cavity_offset(BEER_GAP), BEER_LEVEL)
    beer = np.vstack([beer, [0.0, BEER_LEVEL]])
    place(name, grid_mesh(name, lathe_grid(beer, MUG_SEGMENTS)[0], pole_rows(beer)), material, position, orientation)

def create_foam(name, mug, material, position, orientation, rng):
    wall = mug.cavity_offset(BEER_GAP * 3.0)
    wall = wall[wall[:, 1] >= BEER_LEVEL - FOAM_OVERLAP]
    wall = wall[wall[:, 1] <= FOAM_DOME[0][1] - 0.5]
    bottom = np.array([[0.0, BEER_LEVEL - FOAM_OVERLAP], [wall[0, 0] * 0.5, BEER_LEVEL - FOAM_OVERLAP]])
    dome, _ = resample(smooth_profile(np.vstack([wall[-1:], FOAM_DOME]), 12), 0.08)
    profile = np.vstack([bottom, wall, dome[1:]])
    grid, phi = lathe_grid(profile, MUG_SEGMENTS)

    #lumps on the head, fading in above the rim
    x, z = grid[..., 0], grid[..., 2]
    lumps = np.zeros(x.shape)
    for _ in range(14):
        k = rng.uniform(0.6, 2.5)
        a = rng.uniform(0.0, 2.0 * math.pi)
        lumps += np.sin(k * (x * math.cos(a) + z * math.sin(a)) + rng.uniform(0.0, 2.0 * math.pi)) / k
    lumps *= FOAM_LUMPS / 3.0 * smoothstep(FOAM_DOME[0][1], FOAM_DOME[1][1], grid[..., 1])
    grid[..., 1] += lumps
    grid[..., 0] *= 1.0 + 0.3 * lumps / np.maximum(profile[:, 0:1], 1.0)
    grid[..., 2] *= 1.0 + 0.3 * lumps / np.maximum(profile[:, 0:1], 1.0)
    place(name, grid_mesh(name, grid, pole_rows(profile)), material, position, orientation)

def create_bubbles(name, mug, material, position, orientation, rng):
    bottom = mug.cavity[0, 1] + BEER_GAP
    top = BEER_LEVEL - FOAM_OVERLAP - 0.15
    bubbles = []
    for _ in range(BUBBLE_STREAMS):
        r, a = 3.8 * math.sqrt(rng.uniform()), rng.uniform(0.0, 2.0 * math.pi)
        x, z = r * math.cos(a), r * math.sin(a)
        y = bottom + 0.1
        while y < top:
            t = (y - bottom) / (top - bottom)
            radius = 0.025 + 0.04 * t
            bubbles.append((np.array([x + 0.05 * math.sin(y * 3.0), y, z + 0.05 * math.cos(y * 2.3)]), radius))
            y += 0.15 + 0.9 * t
    for _ in range(LOOSE_BUBBLES):
        y = rng.uniform(mug.inner_y[0] + 0.3, top)
        r = (mug.inner_radius(y) - 0.25) * math.sqrt(rng.uniform())
        a = rng.uniform(0.0, 2.0 * math.pi)
        bubbles.append((np.array([r * math.cos(a), y, r * math.sin(a)]), rng.uniform(0.02, 0.06)))
    parts = [grid_arrays(sphere_grid(c, radius), (0, 6)) for c, radius in bubbles]
    place(name, upload_mesh(name, *merge_arrays(parts)), material, position, orientation)

def create_droplets(name, mug, material, position, orientation, rng):
    """Flattened water caps on the outside, kept a hair off the glass."""
    placed = []
    attempts = 0
    while len(placed) < DROPLETS and attempts < DROPLETS * 20:
        attempts += 1
        phi = rng.uniform(0.3, 2.0 * math.pi - 0.3)
        y = 1.2 + 17.4 * rng.uniform() ** 0.8
        if y > BEER_LEVEL and rng.uniform() < 0.6:
            continue
        a = DROPLET_RADIUS[0] + (DROPLET_RADIUS[1] - DROPLET_RADIUS[0]) * rng.uniform() ** 3
        p = mug.outer_point(phi, y)
        if any(np.linalg.norm(p - q) < a + b + 0.03 for q, b in placed):
            continue
        placed.append((p, a))

    parts = []
    rings, segments = 5, 10
    for p, a in placed:
        phi = math.atan2(p[2], p[0])
        n = mug.outer_normal(phi, p[1])
        t1 = normalize(np.cross(UP, n))
        t2 = np.cross(n, t1)
        h = 0.5 * a
        rho = (a * a + h * h) / (2.0 * h)
        base = p + n * 0.02
        sphere_center = base + n * (h - rho)
        alpha = np.linspace(0.0, math.asin(a / rho), rings + 1)[:, None]
        beta = np.arange(segments)[None, :] / segments * 2.0 * math.pi
        around = np.cos(beta)[..., None] * t1 + np.sin(beta)[..., None] * t2
        cap = sphere_center + rho * (np.sin(alpha)[..., None] * around + np.cos(alpha)[..., None] * n)
        grid = np.concatenate([cap, cap[-1:], np.broadcast_to(base, (1, segments, 3))])
        parts.append(grid_arrays(grid, (0, len(grid) - 1)))
    place(name, upload_mesh(name, *merge_arrays(parts)), material, position, orientation)
    return len(placed)

def create_mug(rng):
    mug = MugProfile()
    position = MUG_POSITION
    orientation = q_axis_angle((0.0, 1.0, 0.0), MUG_YAW)
    glass = make_material("beer_mug_glass_mat", GLASS)
    create_mug_glass("beer_mug_glass", mug, glass, position, orientation)
    create_handle("beer_mug_handle", glass, position, orientation)
    create_beer("beer_mug_beer", mug, make_material("beer_mug_beer_mat", BEER), position, orientation)
    create_foam("beer_mug_foam", mug, make_material("beer_mug_foam_mat", FOAM), position, orientation, rng)
    create_bubbles("beer_mug_bubbles", mug, make_material("beer_mug_bubble_mat", AIR), position, orientation, rng)
    count = create_droplets("beer_mug_droplets", mug, make_material("beer_mug_droplet_mat", WATER), position,
                            orientation, rng)
    print(f"[beer_mug] {len(DIMPLES)} dimples, {count} droplets")

# --- room ----------------------------------------------------------------------

def sun_direction():
    a, e = math.radians(SUN_AZIMUTH), math.radians(SUN_ELEVATION)
    return np.array([-math.sin(a) * math.cos(e), math.sin(e), -math.cos(a) * math.cos(e)])

def mug_center():
    return np.asarray(MUG_POSITION) + UP * 9.0

def window_rect():
    """x and y extents of the window opening: the mug sees the sun through the middle of the upper left pane, the
    lower panes go down behind the table."""
    d = sun_direction()
    hit = mug_center() + d * ((WALL_Z - mug_center()[2]) / d[2])
    width, height = WINDOW
    x0, y0 = hit[0] + width * 0.25, hit[1] - height * 0.25
    return (x0 - width * 0.5, x0 + width * 0.5), (y0 - height * 0.5, y0 + height * 0.5)

def create_table():
    width, thickness = TABLE_SIZE
    arrays = box_arrays((-width * 0.5, -thickness, WALL_Z), (width * 0.5, 0.0, TABLE_FRONT), (2, 0), PLANKS_SIZE)
    place("beer_table", upload_mesh("beer_table", *arrays), planks_material("beer_table_mat", TABLE_WOOD))

def create_wall():
    (wx0, wx1), (wy0, wy1) = window_rect()
    (x0, x1), (y0, y1) = WALL_EXTENT
    z0, z1 = WALL_Z - WALL_THICKNESS, WALL_Z
    pieces = [((x0, y0), (wx0, y1)), ((wx1, y0), (x1, y1)), ((wx0, y0), (wx1, wy0)), ((wx0, wy1), (wx1, y1))]
    parts = [box_arrays((a[0], a[1], z0), (b[0], b[1], z1), (0, 1), PLANKS_SIZE) for a, b in pieces]
    place("beer_wall", upload_mesh("beer_wall", *merge_arrays(parts)), planks_material("beer_wall_mat", WALL_WOOD))

    frame, mullion, depth = WINDOW_FRAME
    zc = WALL_Z - WALL_THICKNESS * 0.5
    fz0, fz1 = zc - depth * 0.5, zc + depth * 0.5
    xm, ym = (wx0 + wx1) * 0.5, (wy0 + wy1) * 0.5
    boxes = [
        ((wx0, wy0, fz0), (wx0 + frame, wy1, fz1)), ((wx1 - frame, wy0, fz0), (wx1, wy1, fz1)),
        ((wx0, wy0, fz0), (wx1, wy0 + frame, fz1)), ((wx0, wy1 - frame, fz0), (wx1, wy1, fz1)),
        ((xm - mullion * 0.5, wy0, fz0), (xm + mullion * 0.5, wy1, fz1)),
        ((wx0, ym - mullion * 0.5, fz0), (wx1, ym + mullion * 0.5, fz1)),
    ]
    parts = [box_arrays(lo, hi) for lo, hi in boxes]
    place("beer_window_frame", upload_mesh("beer_window_frame", *merge_arrays(parts)),
          make_material("beer_window_frame_mat", FRAME_WOOD))

    for i, (profile, params, x) in enumerate(BACK_ITEMS):
        name = f"beer_back_item_{i}"
        place(name, lathe_mesh(name, profile, 96), make_material(f"{name}_mat", params), (x, 0.0, BACK_ITEMS_Z))

def create_sun():
    d = sun_direction()
    center = mug_center() + d * SUN_DISTANCE
    #plane.glb is 2x2 with its emitting side towards +y
    normal = -d
    side = normalize(np.cross(UP, normal))
    area = SUN_SIZE * SUN_SIZE
    radiance = SUN_IRRADIANCE * 2.0 * SUN_DISTANCE ** 2 / (area * (SUN_FOCUS + 2.0))
    meshes, _ = resources.load_meshes_from_path("models/plane.glb")[0]
    obj = place("beer_sun", meshes[0], emissive_material("beer_sun_mat", SUN_COLOR, radiance, SUN_FOCUS), center,
                quat_from_basis(side, normal, np.cross(side, normal)))
    obj.getTransform().setScale(v3((SUN_SIZE * 0.5, 1.0, SUN_SIZE * 0.5)))

    #the sun is only meant to be seen through the glass, check that the wall hides it from the camera
    cam = np.asarray(CAMERA_POSITION)
    hit = cam + (center - cam) * ((WALL_Z - cam[2]) / (center[2] - cam[2]))
    (wx0, wx1), (wy0, wy1) = window_rect()
    if wx0 < hit[0] < wx1 and wy0 < hit[1] < wy1:
        print("[beer_mug] warning: the sun quad is visible through the window from the camera")

# --- environment and camera ------------------------------------------------------

def setup_environment(rng):
    """Long-lat image (u = atan2(z, x) / 2pi + 0.5, v = acos(y) / pi): an out of focus garden behind the wall,
    a dim warm room in front of it."""
    w, h = ENV_RESOLUTION
    theta = ((np.arange(w) + 0.5) / w - 0.5) * 2.0 * math.pi
    phi = (np.arange(h) + 0.5) / h * math.pi
    theta, phi = np.meshgrid(theta, phi)
    dirs = np.stack([np.sin(phi) * np.cos(theta), np.cos(phi), np.sin(phi) * np.sin(theta)], axis=-1)
    elevation = dirs[..., 1]

    sky = np.array([0.85, 0.92, 1.0]) * (0.9 + 0.6 * smoothstep(0.0, 0.8, elevation))[..., None]
    lawn = np.array([0.18, 0.24, 0.07])
    garden = np.where((elevation > 0.05)[..., None], sky, lawn)
    for _ in range(ENV_FOLIAGE_BLOBS):
        az = rng.uniform(-80.0, 80.0)
        el = rng.uniform(-5.0, 35.0)
        center = np.array([-math.sin(math.radians(az)) * math.cos(math.radians(el)), math.sin(math.radians(el)),
                           -math.cos(math.radians(az)) * math.cos(math.radians(el))])
        size = math.radians(rng.uniform(4.0, 14.0))
        angle = np.arccos(np.clip(dirs @ center, -1.0, 1.0))
        weight = np.exp(-(angle / size) ** 2)[..., None]
        lit = rng.uniform() ** 2
        color = np.array([0.12, 0.22, 0.05]) * (1.0 - lit) + np.array([0.75, 0.8, 0.3]) * lit
        garden = garden * (1.0 - weight * 0.85) + color * weight * 0.85

    #a glow around the sun, it lights the panes near it
    sun = sun_direction()
    glow = np.exp(-(np.arccos(np.clip(dirs @ sun, -1.0, 1.0)) / math.radians(18.0)) ** 2)[..., None]
    garden = garden + np.array([1.0, 0.85, 0.6]) * glow * 2.0

    behind = smoothstep(-0.1, -0.35, dirs[..., 2])[..., None]
    color = np.array(ENV_ROOM) * (1.0 - behind) + garden * ENV_GARDEN_INTENSITY * behind

    pixels = np.ones((h, w, 4), dtype=np.float16)
    pixels[..., :3] = color
    env_map = renderer.createTexture("beer_mug_env", ResourceDimension.TEXTURE_2D, ResourceFormat.RGBA16_SFLOAT,
                                     ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, w, h, 1, 1)
    env_map.upload(0, 0, 1, 1, w * 8, pixels.ctypes.data, 0)
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)
    env_var = renderer.getRendererVariable("World.EnvIntensityScale")
    if env_var is not None:
        env_var.setFromFloatArray([1.0])
    exposure_var = renderer.getRendererVariable("Tonemap.ExposureCompensation")
    if exposure_var is not None:
        exposure_var.setFromFloatArray([float(EXPOSURE_COMPENSATION)])

def select_bdpt():
    pipeline = renderer.getRendererVariable("RenderPipeline")
    options = list(pipeline.getOptions()) if pipeline is not None else []
    if "BDPT" in options:
        pipeline.setSelectedOption(options.index("BDPT"))
    else:
        print("[beer_mug] no BDPT pipeline, the caustic needs it")

def set_camera(pos, focus, fov_y):
    dx, dy, dz = (focus[i] - pos[i] for i in range(3))
    #camera looks down -z: yaw about y then pitch about x, same as the CameraController
    yaw = math.degrees(math.atan2(-dx, -dz))
    pitch = math.degrees(math.atan2(dy, math.hypot(dx, dz)))
    quat = q_mul(q_axis_angle((0.0, 1.0, 0.0), yaw), q_axis_angle((1.0, 0.0, 0.0), pitch))
    camera = scene.get_main_camera()
    camera.setFOVY(math.radians(fov_y))
    camera.getTransform().setTranslation(vec3(list(pos)))
    camera.getTransform().setOrientation(vec4(list(quat)))

# ---------------------------------------------------------------------------

rng = np.random.default_rng(SEED)

setup_environment(rng)
create_table()
create_wall()
create_sun()
create_mug(rng)
select_bdpt()
set_camera(CAMERA_POSITION, CAMERA_TARGET, CAMERA_FOV_Y)
