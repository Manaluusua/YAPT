"""
Dispersion beams test scene.

Narrow light beams hit simple glass shapes standing on a white floor, and the dispersed light lands on the floor
as rainbow streaks (the classic "prism on a sheet of paper" setup):
  - each beam is a small emissive slit with a very high emission focus (cos^n profile), so it is nearly
    collimated. The slit sits in front of a dark housing so it reads as a little projector
  - the beams come in low (BEAM_ELEVATION degrees downwards) so the light coming out of the glass skims over
    the floor and draws long streaks, like in a photo
  - every object uses a different real world glass, Cauchy coefficients n(lambda) = A + B / lambda^2 with
    lambda in micrometres (same convention as the shader)
  - prisms are rotated so that their beam goes through at minimum deviation (at 589 nm) and the spectrum
    leaves towards +z, i.e. towards the camera

All geometry is generated here, the only asset used is the (optional) environment map.
Run it via the "run script" action of the app on an empty scene; objects are added, nothing is cleared.
The caustics only show up with the bidirectional integrator: light has to be traced from the slits through the glass.
"""

import base64
import hashlib
import math
import os

import numpy as np
from pygltflib import GLTF2, Accessor, Attributes, Buffer, BufferView, Primitive, DATA_URI_HEADER
from pygltflib import Mesh as GltfMesh, FLOAT, UNSIGNED_INT, SCALAR, VEC3, VEC4

from py_yapt import Material, vec2, vec3, vec4
from yapt.random_shapes import q_axis_angle, q_mul
from yapt.settings import resolve_asset_path

# ---------------------------------------------------------------------------
# Knobs
# ---------------------------------------------------------------------------

BEAM_RADIANCE = 80.0            # emitted power of a beam is pi * radiance * slit area
BEAM_FOCUS = 50000.0            # cos^n exponent of the emission, 50000 -> ~0.3 deg half angle at half power
BEAM_ELEVATION = 6.0            # degrees the beams point downwards
BEAM_DISTANCE = 45.0            # from the slit to the point it aims at
HOUSING_DEPTH = 8.0
HOUSING_MARGIN = 1.5            # how much the housing sticks out around the slit

ENV_MAP = "textures/envmaps/photo_studio_loft_hall_4k.exr"   # None -> no environment map
ENV_INTENSITY = 0.05            # keep the room dim so the beams stand out

FLOOR_SIZE = 2000.0
FLOOR_ALBEDO = 0.8
GLASS_LIFT = 0.02               # keeps the glass bottoms from being coplanar with the floor

CAMERA_POSITION = (0.0, 210.0, 290.0)
CAMERA_FOCUS = (0.0, 0.0, 55.0)

# (A, B) Cauchy coefficients, B in um^2
GLASSES = {
    "fused_silica":       (1.4580, 0.00354),
    "bk7_crown":          (1.5046, 0.00420),
    "baf10_barium_flint": (1.6700, 0.00743),
    "sf10_dense_flint":   (1.7280, 0.01342),
    "diamond":            (2.3790, 0.01335),   # fitted to n_D = 2.4175, n_F - n_C = 0.0255
}

# exit_yaw / incoming_yaw: horizontal direction in degrees, 0 = +z (towards the camera), 90 = +x
OBJECTS = [
    # equilateral prisms, prism axis vertical so the spectrum fans out across the floor
    {"shape": "prism", "glass": "bk7_crown", "x": -160.0, "apex": 60.0, "side": 18.0, "height": 16.0,
     "aim_height": 8.0, "beam": (0.6, 10.0), "exit_yaw": 0.0},
    {"shape": "prism", "glass": "baf10_barium_flint", "x": -80.0, "apex": 60.0, "side": 18.0, "height": 16.0,
     "aim_height": 8.0, "beam": (0.6, 10.0), "exit_yaw": 0.0},
    # a 60 degree diamond prism would trap the light by total internal reflection (critical angle ~24 deg),
    # the apex has to stay under twice that
    {"shape": "prism", "glass": "diamond", "x": 0.0, "apex": 40.0, "side": 20.0, "height": 16.0,
     "aim_height": 8.0, "beam": (0.6, 10.0), "exit_yaw": 0.0},
    # rectangular block (size = x, y, z) with chamfered vertical edges. Light leaving through the face opposite
    # the entry face comes out parallel again (no fan, only a thin coloured fringe), and leaving through a
    # perpendicular face is impossible for n > sqrt(2), it reflects inside instead. So the beam is aimed to
    # leave through the -x/+z chamfer, which works like a 45 degree prism. aim_offset is x on the -z face,
    # move it to positive values to send the beam through the parallel faces instead
    {"shape": "block", "glass": "sf10_dense_flint", "x": 80.0, "size": (18.0, 24.0, 10.0), "chamfer": 4.0,
     "aim_offset": -3.5, "aim_height": 12.0, "beam": (0.6, 8.0), "incoming_yaw": -45.0},
    # ball lens: focuses the wider beam, the colours separate around the focus (chromatic aberration)
    {"shape": "ball", "glass": "fused_silica", "x": 160.0, "radius": 8.0,
     "beam": (6.0, 6.0), "incoming_yaw": -30.0},
]

# ---------------------------------------------------------------------------

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

UP = np.array([0.0, 1.0, 0.0])

def normalize(v):
    v = np.asarray(v, dtype=np.float64)
    return v / np.linalg.norm(v)

def yaw_dir(degrees):
    a = math.radians(degrees)
    return np.array([math.sin(a), 0.0, math.cos(a)])

def beam_dir(yaw, elevation):
    e = math.radians(elevation)
    return yaw_dir(yaw) * math.cos(e) - UP * math.sin(e)

def refractive_index(glass, wavelength_um):
    a, b = GLASSES[glass]
    return a + b / (wavelength_um * wavelength_um)

def min_deviation(n, apex):
    half = math.radians(apex) * 0.5
    return math.degrees(2.0 * math.asin(n * math.sin(half))) - apex

# --- meshes ------------------------------------------------------------------

def oriented_face(points, normal):
    """Returns the polygon wound counter clockwise around normal (front face, also the emitting side)."""
    points = [np.asarray(p, dtype=np.float64) for p in points]
    if np.dot(np.cross(points[1] - points[0], points[2] - points[0]), normal) < 0:
        points = points[::-1]
    return points, normalize(normal)

def flat_mesh_arrays(faces):
    """Flat shaded mesh out of convex polygons, each face gets its own vertices."""
    positions, normals, tangents, indices = [], [], [], []
    for points, normal in faces:
        base = len(positions)
        tangent = normalize(points[1] - points[0])
        for p in points:
            positions.append(p)
            normals.append(normal)
            tangents.append([tangent[0], tangent[1], tangent[2], 1.0])
        for i in range(1, len(points) - 1):
            indices += [base, base + i, base + i + 1]
    return positions, normals, tangents, indices

def quad_faces(half_u, half_v, axis_u, axis_v, normal):
    corners = [axis_u * su * half_u + axis_v * sv * half_v for su, sv in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    return [oriented_face(corners, normal)]

def box_faces(half_extents, axes):
    faces = []
    for i in range(3):
        j, k = (i + 1) % 3, (i + 2) % 3
        for sign in (-1.0, 1.0):
            center = axes[i] * sign * half_extents[i]
            corners = [center + axes[j] * sj * half_extents[j] + axes[k] * sk * half_extents[k]
                       for sj, sk in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            faces.append(oriented_face(corners, axes[i] * sign))
    return faces

def extruded_faces(outline_xz, height):
    """Convex outline on the floor extruded upwards from y = 0."""
    bottom = [np.array([x, 0.0, z]) for x, z in outline_xz]
    top = [p + UP * height for p in bottom]
    center = sum(bottom) / len(bottom)
    faces = [oriented_face(bottom, -UP), oriented_face(top, UP)]
    for i in range(len(bottom)):
        a, b = bottom[i], bottom[(i + 1) % len(bottom)]
        outward = np.cross(b - a, UP)
        if np.dot(outward, (a + b) * 0.5 - center) < 0:
            outward = -outward
        faces.append(oriented_face([a, b, top[(i + 1) % len(top)], top[i]], outward))
    return faces

def sphere_arrays(radius, rings=48, segments=96):
    positions, normals, tangents, indices = [], [], [], []
    for r in range(rings + 1):
        theta = math.pi * r / rings
        for s in range(segments + 1):
            phi = 2.0 * math.pi * s / segments
            n = np.array([math.sin(theta) * math.cos(phi), math.cos(theta), math.sin(theta) * math.sin(phi)])
            positions.append(n * radius)
            normals.append(n)
            tangents.append([-math.sin(phi), 0.0, math.cos(phi), 1.0])
    stride = segments + 1
    for r in range(rings):
        for s in range(segments):
            i0 = r * stride + s
            i1 = i0 + stride
            # counter clockwise seen from outside, skipping the zero area triangles at the poles
            if r > 0:
                indices += [i0, i0 + 1, i1]
            if r < rings - 1:
                indices += [i0 + 1, i1 + 1, i1]
    return positions, normals, tangents, indices

def upload_mesh(name, arrays):
    """Packs the arrays into an in-memory glTF and goes through the regular glTF mesh loader."""
    positions, normals, tangents, indices = arrays
    chunks = [
        np.asarray(indices, dtype=np.uint32),
        np.asarray(positions, dtype=np.float32),
        np.asarray(normals, dtype=np.float32),
        np.asarray(tangents, dtype=np.float32),
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
    ]
    primitive = Primitive(attributes=Attributes(POSITION=1, NORMAL=2, TANGENT=3), indices=0)
    gltf = GLTF2(
        meshes=[GltfMesh(name=name, primitives=[primitive])],
        accessors=accessors,
        bufferViews=views,
        buffers=[Buffer(byteLength=len(blob), uri=DATA_URI_HEADER + base64.b64encode(blob).decode("ascii"))],
    )
    #the geometry hash in the cache key makes re-running the script with changed knobs pick up the new shapes
    cache_key = f"procedural/{name}/{hashlib.sha1(blob).hexdigest()[:12]}"
    meshes, _ = resources.load_meshes_from_gltf(gltf, cache_key)[0]
    return meshes[0]

def add_object(name, arrays, material, position):
    obj = scene.create_render_object(name)
    obj.setMesh(upload_mesh(name, arrays))
    obj.setMaterial(material, 0)
    obj.getTransform().setTranslation(vec3([float(c) for c in position]))
    return obj

# --- materials ---------------------------------------------------------------

def matte_material(name, albedo, roughness=0.9):
    mat = renderer.createMaterial(name)
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setAlbedo(vec3(albedo))
    mat.setRoughness(roughness)
    return mat

def glass_material(name, glass):
    mat = renderer.createMaterial(name)
    mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
    mat.setRoughness(0.0)
    mat.setEnableDispersion(True)
    mat.setCauchysCoefficients(vec2(list(GLASSES[glass])))
    return mat

def beam_material(name):
    mat = renderer.createMaterial(name)
    mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
    mat.setAlbedo(vec3(0.0))
    mat.setEmission(vec3(BEAM_RADIANCE))
    mat.setEmissionFocus(BEAM_FOCUS)
    return mat

housing_mat = matte_material("dispersion_housing_mat", 0.02, 0.6)

# --- scene pieces -------------------------------------------------------------

def create_beam(name, aim, direction, size):
    """Emissive slit at BEAM_DISTANCE from aim, shining along direction, with a dark box behind it."""
    width_axis = normalize(np.cross(direction, UP))
    height_axis = normalize(np.cross(width_axis, direction))
    center = aim - direction * BEAM_DISTANCE
    half_w, half_h = size[0] * 0.5, size[1] * 0.5

    add_object(f"{name}_slit", flat_mesh_arrays(quad_faces(half_w, half_h, width_axis, height_axis, direction)),
               beam_material(f"{name}_slit_mat"), center)

    #front face of the housing sits just behind the slit, the slit only emits forwards
    housing_center = center - direction * (HOUSING_DEPTH * 0.5 + 0.05)
    half_extents = (half_w + HOUSING_MARGIN, half_h + HOUSING_MARGIN, HOUSING_DEPTH * 0.5)
    add_object(f"{name}_housing", flat_mesh_arrays(box_faces(half_extents, (width_axis, height_axis, direction))),
               housing_mat, housing_center)

def create_prism(name, cfg, mat, origin):
    """Upright prism, rotated so the beam goes through at minimum deviation and leaves along exit_yaw."""
    n = refractive_index(cfg["glass"], 0.589)
    apex = cfg["apex"]
    deviation = min_deviation(n, apex)
    exit_yaw = cfg["exit_yaw"]

    #at minimum deviation the ray inside runs parallel to the base, the ray bends away from the apex
    inside = yaw_dir(exit_yaw + deviation * 0.5)
    apex_dir = yaw_dir(exit_yaw + deviation * 0.5 + 90.0)

    #triangle in (apex_dir, inside) coordinates, centroid at the origin
    half = math.radians(apex) * 0.5
    h = cfg["side"] * math.cos(half)
    b = 2.0 * cfg["side"] * math.sin(half)
    to_xz = lambda u, v: tuple((apex_dir * u + inside * v)[[0, 2]])
    outline = [to_xz(2.0 * h / 3.0, 0.0), to_xz(-h / 3.0, -b * 0.5), to_xz(-h / 3.0, b * 0.5)]
    add_object(name, flat_mesh_arrays(extruded_faces(outline, cfg["height"])), mat, origin)

    #aim at the middle of the entry face (the one on the -inside side)
    entry = apex_dir * (h / 6.0) - inside * (b / 4.0)
    aim = origin + entry + UP * cfg["aim_height"]
    create_beam(name, aim, beam_dir(exit_yaw + deviation, BEAM_ELEVATION), cfg["beam"])
    return deviation

def create_block(name, cfg, mat, origin):
    sx, sy, sz = cfg["size"]
    hx, hz, c = sx / 2, sz / 2, cfg["chamfer"]
    outline = [(-hx + c, -hz), (hx - c, -hz), (hx, -hz + c), (hx, hz - c),
               (hx - c, hz), (-hx + c, hz), (-hx, hz - c), (-hx, -hz + c)]
    add_object(name, flat_mesh_arrays(extruded_faces(outline, sy)), mat, origin)

    #enters through the -z face
    aim = origin + np.array([cfg["aim_offset"], cfg["aim_height"], -sz / 2])
    create_beam(name, aim, beam_dir(cfg["incoming_yaw"], BEAM_ELEVATION), cfg["beam"])

def create_ball(name, cfg, mat, origin):
    r = cfg["radius"]
    center = origin + UP * r
    add_object(name, sphere_arrays(r), mat, center)
    create_beam(name, center, beam_dir(cfg["incoming_yaw"], BEAM_ELEVATION), cfg["beam"])

def create_floor():
    mat = matte_material("dispersion_floor_mat", FLOOR_ALBEDO)
    half = FLOOR_SIZE * 0.5
    add_object("dispersion_floor", flat_mesh_arrays(quad_faces(half, half, np.array([1.0, 0, 0]), np.array([0, 0, 1.0]), UP)),
               mat, (0.0, 0.0, 0.0))

def setup_environment():
    if ENV_MAP and os.path.exists(resolve_asset_path(ENV_MAP)):
        env_map = resources.load_texture_2D(ENV_MAP, "R16G16B16A16_SFLOAT")
        if env_map is not None:
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

def create_glass_objects():
    builders = {"prism": create_prism, "block": create_block, "ball": create_ball}
    print("[dispersion_beams] glass               n_C(656)  n_D(589)  n_F(486)  Abbe   min deviation")
    for i, cfg in enumerate(OBJECTS):
        name = f"dispersion_{cfg['shape']}_{cfg['glass']}_{i}"
        origin = np.array([cfg["x"], GLASS_LIFT, 0.0])
        deviation = builders[cfg["shape"]](name, cfg, glass_material(f"{name}_mat", cfg["glass"]), origin)

        n_c, n_d, n_f = (refractive_index(cfg["glass"], wl) for wl in (0.6563, 0.5893, 0.4861))
        abbe = (n_d - 1.0) / (n_f - n_c)
        dev = f"{deviation:.1f} deg (apex {cfg['apex']:.0f})" if deviation is not None else "-"
        print(f"[dispersion_beams] {cfg['glass']:<19} {n_c:.4f}    {n_d:.4f}    {n_f:.4f}    {abbe:5.1f}  {dev}")

setup_environment()
create_floor()
create_glass_objects()
set_camera(CAMERA_POSITION, CAMERA_FOCUS)
