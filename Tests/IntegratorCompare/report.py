"""Difference images, montages and the printed comparison table."""

from itertools import combinations
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

#black -> blue -> cyan -> yellow -> red, so small differences stay dark and readable
HEATMAP_STOPS = [
    (0.00, (0, 0, 0)),
    (0.25, (40, 60, 190)),
    (0.50, (0, 190, 190)),
    (0.75, (240, 220, 60)),
    (1.00, (240, 50, 40)),
]

#signed map: blue where the second image is darker, red where it is brighter, near black
#where the two agree. magnitude alone cannot tell "lost light" from "gained light", which
#is the whole point when looking at how a lobe redistributes energy.
DIVERGING_STOPS = [
    (-1.00, (70, 130, 255)),
    (-0.35, (30, 50, 150)),
    (0.00, (8, 8, 10)),
    (0.35, (170, 60, 30)),
    (1.00, (255, 120, 60)),
]

LABEL_HEIGHT = 32
BACKGROUND = (18, 18, 18)
TEXT_COLOR = (232, 232, 232)
SUBTEXT_COLOR = (150, 150, 150)

#block size for the averaged difference image, matched to the 16x downsample the stats use
DIFF_BLOCK = 16


def apply_colormap(values, stops):
    """Map an array to RGB uint8 by interpolating between (position, rgb) stops."""
    low_bound, high_bound = stops[0][0], stops[-1][0]
    values = np.clip(values, low_bound, high_bound)
    out = np.zeros(values.shape + (3,), np.float64)

    for (low, low_rgb), (high, high_rgb) in zip(stops, stops[1:]):
        mask = (values >= low) & (values <= high)
        if not mask.any():
            continue
        t = ((values[mask] - low) / (high - low))[:, None]
        out[mask] = np.array(low_rgb) * (1 - t) + np.array(high_rgb) * t

    return out.astype(np.uint8)


def apply_heatmap(values01):
    """Map an array in [0,1] to an RGB uint8 heatmap."""
    return apply_colormap(values01, HEATMAP_STOPS)


def difference_stats(a, b, downsample=(15, 11)):
    """Absolute difference statistics between two captures.

    The downsampled figure is the one that matters: at full resolution two unbiased
    estimators still differ pixel by pixel through Monte Carlo noise, and block averaging
    is what separates noise from an actual disagreement between the integrators.
    """
    a = a.astype(np.float64)
    b = b.astype(np.float64)
    diff = np.abs(a - b)
    mid = (a.mean() + b.mean()) / 2

    small_a = np.asarray(Image.fromarray(a.astype(np.uint8)).resize(downsample, Image.BOX), np.float64)
    small_b = np.asarray(Image.fromarray(b.astype(np.uint8)).resize(downsample, Image.BOX), np.float64)
    small_diff = np.abs(small_a - small_b)
    small_mid = (small_a.mean() + small_b.mean()) / 2

    return {
        "mean_a": a.mean(),
        "mean_b": b.mean(),
        "mean_abs": diff.mean(),
        "rel_full": 100.0 * diff.mean() / mid if mid else 0.0,
        "rel_down": 100.0 * small_diff.mean() / small_mid if small_mid else 0.0,
        "mean_gap": 100.0 * (b.mean() - a.mean()) / a.mean() if a.mean() else 0.0,
        "p99": np.percentile(diff, 99),
        "max": diff.max(),
    }


def difference_image(a, b, amplify=6.0):
    """Per-pixel absolute difference as a heatmap, amplified so small errors are visible.

    Mostly shows Monte Carlo noise. Useful for spotting a difference with structure to it -
    a bright edge, a lit region that only one integrator finds - but for judging whether
    the integrators actually disagree, use blockwise_difference_image.
    """
    magnitude = np.abs(a.astype(np.float64) - b.astype(np.float64)).mean(axis=2)
    return apply_heatmap(magnitude * amplify / 255.0)


def _block_average(image, block):
    height, width = image.shape[:2]
    rows, cols = height // block, width // block
    cropped = image.astype(np.float64)[: rows * block, : cols * block]
    return cropped.reshape(rows, block, cols, block, 3).mean(axis=(1, 3))


def blockwise_difference_image(a, b, amplify=24.0, block=DIFF_BLOCK, signed=True):
    """Difference averaged over blocks first, which is what separates bias from noise.

    Averaging a block of pixels collapses zero mean noise but leaves any systematic
    disagreement standing, so whatever is still visible here is a real difference. Signed
    by default: blue means b is darker there, red means brighter, near black means they
    agree. Amplified harder than the per-pixel view since the values are much smaller once
    the noise is gone.
    """
    height, width = a.shape[:2]
    delta = (_block_average(b, block) - _block_average(a, block)).mean(axis=2)

    if signed:
        colored = apply_colormap(delta * amplify / 255.0, DIVERGING_STOPS)
    else:
        colored = apply_heatmap(np.abs(delta) * amplify / 255.0)

    return np.asarray(Image.fromarray(colored).resize((width, height), Image.NEAREST))


def _label_strip(width, text, sub_text=None):
    strip = Image.new("RGB", (width, LABEL_HEIGHT), BACKGROUND)
    draw = ImageDraw.Draw(strip)
    draw.text((6, 4), text, fill=TEXT_COLOR)
    if sub_text:
        draw.text((6, 18), sub_text, fill=SUBTEXT_COLOR)
    return strip


def build_montage(rows, scale=0.5, path=None):
    """rows is a list of lists of (rgb_array, title, subtitle) tuples."""
    tile_w = int(rows[0][0][0].shape[1] * scale)
    tile_h = int(rows[0][0][0].shape[0] * scale)
    columns = max(len(row) for row in rows)

    canvas = Image.new("RGB", (tile_w * columns, (tile_h + LABEL_HEIGHT) * len(rows)), BACKGROUND)
    for row_index, row in enumerate(rows):
        for col_index, (image, title, subtitle) in enumerate(row):
            x = col_index * tile_w
            y = row_index * (tile_h + LABEL_HEIGHT)
            canvas.paste(_label_strip(tile_w, title, subtitle), (x, y))
            canvas.paste(Image.fromarray(image).resize((tile_w, tile_h)), (x, y + LABEL_HEIGHT))

    if path is not None:
        canvas.save(path)
    return canvas


def write_comparison(captures, output_dir, amplify=6.0, montage_name="comparison.png"):
    """Print the pairwise table, write per-pair difference images and a montage.

    captures is an ordered dict-like of {label: rgb_array}. Returns the stats per pair.
    """
    output_dir = Path(output_dir)
    labels = list(captures)
    pairs = list(combinations(labels, 2))
    stats = {}

    print()
    print(f"  {'pair':<24s} {'mean A':>8s} {'mean B':>8s} {'gap':>8s} "
          f"{'rel|d| full':>12s} {'rel|d| 16x':>11s} {'p99':>6s} {'max':>5s}")
    print("  " + "-" * 88)
    for a, b in pairs:
        s = difference_stats(captures[a], captures[b])
        stats[(a, b)] = s
        print(f"  {a + ' vs ' + b:<24s} {s['mean_a']:8.3f} {s['mean_b']:8.3f} "
              f"{s['mean_gap']:+7.2f}% {s['rel_full']:11.2f}% {s['rel_down']:10.2f}% "
              f"{s['p99']:6.1f} {s['max']:5.0f}")
    print()
    print("  rel|d| full includes per-pixel Monte Carlo noise; rel|d| 16x is block averaged")
    print("  and is the number to judge agreement by. Compare against a focus 0 run, which")
    print("  exercises the unchanged lambertian path and gives this scene's noise floor.")

    per_pixel = {}
    blockwise = {}
    for a, b in pairs:
        per_pixel[(a, b)] = difference_image(captures[a], captures[b], amplify)
        blockwise[(a, b)] = blockwise_difference_image(captures[a], captures[b])
        Image.fromarray(per_pixel[(a, b)]).save(output_dir / f"diff_{a}_vs_{b}.png")
        Image.fromarray(blockwise[(a, b)]).save(output_dir / f"diff_{a}_vs_{b}_blocks.png")

    rows = [
        [(captures[label], label, f"mean {captures[label].mean():.2f}") for label in labels],
        [(per_pixel[(a, b)], f"{a} vs {b}  per pixel (x{amplify:g})",
          f"rel|d| full = {stats[(a, b)]['rel_full']:.2f}%   mostly noise") for a, b in pairs],
        [(blockwise[(a, b)], f"{a} vs {b}  {DIFF_BLOCK}px blocks, signed (x24)",
          f"rel|d| 16x = {stats[(a, b)]['rel_down']:.2f}%   gap {stats[(a, b)]['mean_gap']:+.2f}%"
          f"   blue = {b} darker, red = brighter")
         for a, b in pairs],
    ]
    montage_path = output_dir / montage_name
    build_montage(rows, scale=0.45, path=montage_path)

    print()
    print(f"  wrote {montage_path}")
    print(f"        renders on top, per pixel difference in the middle (noise), block")
    print(f"        averaged difference at the bottom (what is left is a real disagreement)")

    return stats
