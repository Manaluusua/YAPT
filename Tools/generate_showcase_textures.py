"""
Generates the tileable texture sets used by examples/load_material_showcase_scene.py into ASSET_PATH/textures/procedural/.

Every set gets <name>_albedo.png (sRGB), <name>_normal.png and <name>_orm.png (linear), some also <name>_emissive.png (sRGB).
Conventions follow the material shader:
  - normal maps are tangent space, red along the tangent (+u), green along cross(tangent, normal), which the
    showcase blocks point along increasing image rows (+v)
  - ORM is glTF style: red occlusion, green roughness, blue metalness. Green and blue scale the material values

Only numpy and PIL are needed. Usage: python Tools/generate_showcase_textures.py [--size 1024] [--out <folder>]
"""

import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Source" / "Python" / "pythonScripts"))
from yapt.settings import ASSET_PATH

# ---------------------------------------------------------------------------
# helpers, all of them wrap around the edges so the results tile
# ---------------------------------------------------------------------------

def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)

def lerp(a, b, t):
    return a + (b - a) * t

def value_noise(size, cells_x, cells_y, rng):
    grid = rng.random((cells_y, cells_x))

    def axis(cells):
        coords = np.arange(size) * cells / size
        i0 = np.floor(coords).astype(int)
        f = coords - i0
        return i0 % cells, (i0 + 1) % cells, f * f * (3.0 - 2.0 * f)

    x0, x1, fx = axis(cells_x)
    y0, y1, fy = axis(cells_y)
    top = lerp(grid[y0[:, None], x0[None, :]], grid[y0[:, None], x1[None, :]], fx[None, :])
    bottom = lerp(grid[y1[:, None], x0[None, :]], grid[y1[:, None], x1[None, :]], fx[None, :])
    return lerp(top, bottom, fy[:, None])

def fbm(size, cells, octaves, rng, gain=0.5, stretch=(1, 1)):
    """Fractal noise in [0, 1], stretch multiplies the cell counts along x and y (anisotropic noise)."""
    total = np.zeros((size, size))
    amplitude, norm = 1.0, 0.0
    for octave in range(octaves):
        c = cells * 2 ** octave
        cx, cy = min(size, c * stretch[0]), min(size, c * stretch[1])
        total += value_noise(size, cx, cy, rng) * amplitude
        norm += amplitude
        amplitude *= gain
    return total / norm

def blur(img, radius):
    out = img.astype(np.float64)
    for axis in (0, 1):
        acc = np.zeros_like(out)
        for offset in range(-radius, radius + 1):
            acc += np.roll(out, offset, axis=axis)
        out = acc / (2 * radius + 1)
    return out

def cavity_ao(height, radius, strength):
    """Darkens texels that sit lower than their surroundings."""
    return np.clip(1.0 - (blur(height, radius) - height) * strength, 0.0, 1.0)

def normal_from_height(height, strength):
    """height in [0, 1], strength = height range in texels."""
    du = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 0.5 * strength
    dv = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * 0.5 * strength
    n = np.stack([-du, -dv, np.ones_like(height)], axis=-1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return n * 0.5 + 0.5

def wrap_coords(size):
    return np.meshgrid(np.arange(size), np.arange(size))

# ---------------------------------------------------------------------------
# texture sets, each returns a dict of linear float images
# ---------------------------------------------------------------------------

def ceramic_tiles(size, rng):
    tiles = 4
    tile = size // tiles
    grout, bevel = size / 170.0, size / 100.0
    x, y = wrap_coords(size)
    lx, ly = x % tile, y % tile
    edge = np.minimum(np.minimum(lx, tile - 1 - lx), np.minimum(ly, tile - 1 - ly))
    plateau = smoothstep(grout, grout + bevel, edge)

    palette = np.array([(0.80, 0.78, 0.72), (0.62, 0.72, 0.66), (0.20, 0.42, 0.45), (0.72, 0.40, 0.25), (0.85, 0.84, 0.80)])
    tile_ids = (y // tile) * tiles + (x // tile)
    choice = rng.integers(0, len(palette), tiles * tiles)
    brightness = rng.uniform(0.85, 1.1, tiles * tiles)
    tile_color = palette[choice][tile_ids] * brightness[tile_ids][..., None]
    mottle = fbm(size, 8, 5, rng)[..., None]
    glaze = tile_color * lerp(0.9, 1.08, mottle)
    grout_color = np.array([0.33, 0.31, 0.29]) * lerp(0.8, 1.1, fbm(size, 32, 3, rng))[..., None]
    albedo = lerp(grout_color, glaze, plateau[..., None])

    height = plateau * 0.8 + fbm(size, 16, 4, rng) * 0.03
    roughness = lerp(0.9, 0.12 + 0.1 * fbm(size, 16, 3, rng), plateau)
    return {"albedo": albedo, "height": height, "height_strength": size / 60.0,
            "ao": cavity_ao(height, 6, 2.0), "roughness": roughness, "metalness": np.zeros((size, size))}

def brushed_metal(size, rng):
    streaks = fbm(size, 2, 6, rng, gain=0.6, stretch=(1, 96))
    coarse = fbm(size, 4, 3, rng)
    scratches = (value_noise(size, 3, size // 2, rng) > 0.93) * 0.5
    height = streaks * 0.7 + scratches * 0.3
    albedo = np.repeat((0.78 + 0.08 * (streaks - 0.5) + 0.04 * (coarse - 0.5))[..., None], 3, axis=-1)
    roughness = np.clip(0.28 + 0.15 * (streaks - 0.5) + 0.1 * coarse, 0.05, 1.0)
    return {"albedo": albedo, "height": height, "height_strength": size / 300.0,
            "ao": np.ones((size, size)), "roughness": roughness, "metalness": np.ones((size, size))}

def diamond_plate(size, rng):
    cells = 8
    cell = size // cells
    x, y = wrap_coords(size)
    lx, ly = x % cell - cell * 0.5, y % cell - cell * 0.5
    flip = np.where(((x // cell) + (y // cell)) % 2 == 0, 1.0, -1.0)
    a = (lx + flip * ly) / math.sqrt(2.0)
    b = (lx - flip * ly) / math.sqrt(2.0)
    r = np.sqrt((a / (cell * 0.36)) ** 2 + (b / (cell * 0.08)) ** 2)
    bump = np.sqrt(np.clip(1.0 - r * r, 0.0, 1.0))
    wear = fbm(size, 8, 5, rng)
    height = bump * 0.9 + wear * 0.05
    dirt = (1.0 - smoothstep(0.0, 0.3, bump)) * smoothstep(0.55, 0.8, fbm(size, 12, 5, rng)) * 0.7
    albedo = lerp(np.array([0.72, 0.72, 0.74]), np.array([0.25, 0.22, 0.18]), dirt[..., None]) * lerp(0.9, 1.05, wear)[..., None]
    roughness = lerp(0.45, 0.2, smoothstep(0.2, 0.8, bump)) + dirt * 0.4
    metalness = 1.0 - dirt * 0.8
    return {"albedo": albedo, "height": height, "height_strength": size / 80.0,
            "ao": cavity_ao(height, 8, 1.5), "roughness": np.clip(roughness, 0.0, 1.0), "metalness": metalness}

def concrete(size, rng):
    base = fbm(size, 6, 7, rng)
    stains = fbm(size, 3, 4, rng)
    pits = smoothstep(0.78, 0.86, value_noise(size, 96, 96, rng) * fbm(size, 24, 2, rng) * 1.6)
    aggregate = smoothstep(0.7, 0.75, fbm(size, 48, 2, rng))
    height = base * 0.5 + aggregate * 0.15 - pits * 0.6
    gray = (0.42 + 0.18 * (base - 0.5) - 0.12 * smoothstep(0.55, 0.8, stains) + 0.08 * aggregate - 0.2 * pits)
    albedo = gray[..., None] * np.array([1.0, 0.97, 0.93])
    roughness = np.clip(0.8 + 0.15 * (base - 0.5) - 0.2 * smoothstep(0.6, 0.85, stains), 0.0, 1.0)
    return {"albedo": albedo, "height": height, "height_strength": size / 120.0,
            "ao": cavity_ao(height, 4, 3.0), "roughness": roughness, "metalness": np.zeros((size, size))}

def wood_planks(size, rng):
    rows = 5
    plank = size / rows
    x, y = wrap_coords(size)
    row = (y / plank).astype(int)
    seam_offsets = rng.uniform(0, size, rows)
    shifted = (x + seam_offsets[row]) % size
    segment = (shifted // (size / 2)).astype(int)
    seg_id = row * 2 + segment
    seg_seed = rng.uniform(0, 100, rows * 2)
    seg_tone = rng.uniform(0.8, 1.15, rows * 2)

    local_y = (y % plank) / plank
    warp = fbm(size, 3, 4, rng, stretch=(1, 4)) * 2.5
    rings = 0.5 + 0.5 * np.sin((local_y * 7.0 + warp + seg_seed[seg_id]) * 2.0 * math.pi)
    rings = rings ** 3
    grain = fbm(size, 2, 6, rng, stretch=(1, 48))
    light, dark = np.array([0.55, 0.34, 0.17]), np.array([0.26, 0.14, 0.06])
    albedo = lerp(light, dark, (rings * 0.7 + grain * 0.3)[..., None]) * seg_tone[seg_id][..., None]

    gap_v = np.minimum(y % plank, plank - 1 - (y % plank))
    gap_u = np.minimum(shifted % (size / 2), size / 2 - 1 - shifted % (size / 2))
    groove = smoothstep(1.0, 4.0, np.minimum(gap_v, gap_u))
    albedo = albedo * lerp(0.25, 1.0, groove)[..., None]
    height = groove * 0.8 + grain * 0.1 + rings * 0.05
    roughness = np.clip(0.45 + 0.2 * grain + 0.1 * rings, 0.0, 1.0) * lerp(1.4, 1.0, groove)
    return {"albedo": albedo, "height": height, "height_strength": size / 100.0,
            "ao": cavity_ao(height, 4, 2.0), "roughness": np.clip(roughness, 0.0, 1.0), "metalness": np.zeros((size, size))}

def rubber_dots(size, rng):
    nx, ny = 9, 5
    sx, sy = size / nx, size / ny
    x, y = wrap_coords(size)
    lx, ly = x % sx, y % sy
    #corners of the cell plus its centre approximate a hex lattice
    d = np.full((size, size), np.inf)
    for px, py in ((0, 0), (sx, 0), (0, sy), (sx, sy), (sx * 0.5, sy * 0.5)):
        d = np.minimum(d, np.hypot(lx - px, ly - py))
    radius = sx * 0.3
    dome = np.sqrt(np.clip(1.0 - (d / radius) ** 2, 0.0, 1.0)) * smoothstep(radius, radius * 0.92, d)
    dust = fbm(size, 8, 5, rng)
    height = dome * 0.7 + dust * 0.03
    albedo = np.repeat((0.05 + 0.03 * dust + 0.03 * smoothstep(0.6, 0.9, dust))[..., None], 3, axis=-1)
    roughness = lerp(0.85, 0.6, dome) + 0.05 * dust
    return {"albedo": albedo, "height": height, "height_strength": size / 70.0,
            "ao": cavity_ao(height, 6, 1.5), "roughness": np.clip(roughness, 0.0, 1.0), "metalness": np.zeros((size, size))}

def chipped_paint(size, rng):
    wear = fbm(size, 5, 7, rng)
    chips = smoothstep(0.6, 0.62, wear)
    rust_band = smoothstep(0.55, 0.6, wear) * (1.0 - chips)
    mottle = fbm(size, 16, 4, rng)
    paint = np.array([0.42, 0.06, 0.035]) * lerp(0.85, 1.1, mottle)[..., None]
    metal = np.array([0.62, 0.6, 0.57]) * lerp(0.85, 1.05, fbm(size, 64, 2, rng))[..., None]
    rust = np.array([0.32, 0.13, 0.04]) * lerp(0.7, 1.2, mottle)[..., None]
    albedo = lerp(lerp(paint, rust, rust_band[..., None] * 0.8), metal, chips[..., None])
    height = (1.0 - chips) * 0.6 + mottle * 0.03
    roughness = lerp(lerp(0.35, 0.9, rust_band), 0.3, chips)
    return {"albedo": albedo, "height": height, "height_strength": size / 200.0,
            "ao": cavity_ao(height, 3, 1.0), "roughness": roughness, "metalness": chips}

def circuit_board(size, rng):
    grid = size // 32
    traces = np.zeros((size, size))
    glow = np.zeros((size, size, 3))
    colors = [np.array([0.1, 0.9, 1.0]), np.array([1.0, 0.25, 0.7]), np.array([1.0, 0.65, 0.15])]
    width = max(2, size // 256)

    def draw(mask_value, x0, y0, x1, y1, color):
        xs = np.arange(min(x0, x1) - width, max(x0, x1) + width + 1) % size
        ys = np.arange(min(y0, y1) - width, max(y0, y1) + width + 1) % size
        traces[np.ix_(ys, xs)] = mask_value
        if color is not None:
            glow[np.ix_(ys, xs)] = color

    def pad(cx, cy, radius, color):
        r = int(radius) + 1
        xs = np.arange(cx - r, cx + r + 1)
        ys = np.arange(cy - r, cy + r + 1)
        disk = np.hypot(xs[None, :] - cx, ys[:, None] - cy) <= radius
        yy, xx = np.meshgrid(ys % size, xs % size, indexing="ij")
        traces[yy[disk], xx[disk]] = 1.0
        if color is not None:
            glow[yy[disk], xx[disk]] = color

    for _ in range(60):
        cx, cy = rng.integers(0, size // grid, 2) * grid
        color = colors[rng.integers(0, len(colors))] if rng.random() < 0.3 else None
        pad(cx, cy, width * 2.5, color)
        for _ in range(rng.integers(2, 7)):
            step = rng.integers(1, 6) * grid
            if rng.random() < 0.5:
                nx, ny = cx + step * rng.choice([-1, 1]), cy
            else:
                nx, ny = cx, cy + step * rng.choice([-1, 1])
            draw(1.0, cx, cy, nx, ny, color)
            cx, cy = nx, ny
        pad(cx, cy, width * 2.5, color)

    traces = blur(traces, 1)
    noise = fbm(size, 16, 4, rng)
    board = np.array([0.02, 0.07, 0.04]) * lerp(0.8, 1.2, noise)[..., None]
    copper = np.array([0.75, 0.55, 0.25])
    albedo = lerp(board, copper, traces[..., None])
    height = traces * 0.6 + noise * 0.02
    roughness = lerp(0.55, 0.25, traces)
    return {"albedo": albedo, "height": height, "height_strength": size / 300.0,
            "ao": cavity_ao(height, 3, 1.0), "roughness": roughness, "metalness": traces,
            "emissive": blur(glow, 2)}

TEXTURE_SETS = {
    "ceramic_tiles": ceramic_tiles,
    "brushed_metal": brushed_metal,
    "diamond_plate": diamond_plate,
    "concrete": concrete,
    "wood_planks": wood_planks,
    "rubber_dots": rubber_dots,
    "chipped_paint": chipped_paint,
    "circuit_board": circuit_board,
}

# ---------------------------------------------------------------------------

def to_srgb(linear):
    linear = np.clip(linear, 0.0, 1.0)
    return np.where(linear <= 0.0031308, linear * 12.92, 1.055 * np.power(linear, 1.0 / 2.4) - 0.055)

def save(path, image):
    Image.fromarray((np.clip(image, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8), "RGB").save(path)

def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--size", type=int, default=1024)
    parser.add_argument("--out", type=Path, default=ASSET_PATH / "textures" / "procedural")
    parser.add_argument("--seed", type=int, default=1234)
    parser.add_argument("sets", nargs="*", help="sets to generate, all by default")
    args = parser.parse_args()

    for index, name in enumerate(args.sets or TEXTURE_SETS):
        maps = TEXTURE_SETS[name](args.size, np.random.default_rng(args.seed + index))
        folder = args.out / name
        folder.mkdir(parents=True, exist_ok=True)
        save(folder / f"{name}_albedo.png", to_srgb(maps["albedo"]))
        save(folder / f"{name}_normal.png", normal_from_height(maps["height"], maps["height_strength"]))
        save(folder / f"{name}_orm.png", np.stack([maps["ao"], maps["roughness"], maps["metalness"]], axis=-1))
        if "emissive" in maps:
            save(folder / f"{name}_emissive.png", to_srgb(maps["emissive"]))
        print(f"wrote {folder}")

if __name__ == "__main__":
    main()
