from py_yapt import ResourceDimension, ResourceFormat
from PySide6.QtGui import QImage
import numpy as np
import os

#the alpha of a tonemapped render target isn't meaningful as transparency, and writing it out tends to
#produce an image that every viewer shows as empty, so screenshots are written opaque
WRITE_OPAQUE = True

#how the texels of a readback are laid out: numpy type, channel count, channel order and value encoding
FORMAT_LAYOUTS = {
    ResourceFormat.RGBA8_UNORM:   (np.uint8,   4, "RGBA", "unorm8"),
    ResourceFormat.RGBA8_SRGB:    (np.uint8,   4, "RGBA", "unorm8"),
    ResourceFormat.BGRA8_UNORM:   (np.uint8,   4, "BGRA", "unorm8"),
    ResourceFormat.BGRA8_SRGB:    (np.uint8,   4, "BGRA", "unorm8"),
    ResourceFormat.RGB8_UNORM:    (np.uint8,   3, "RGB",  "unorm8"),
    ResourceFormat.RGB8_SRGB:     (np.uint8,   3, "RGB",  "unorm8"),
    ResourceFormat.BGR8_UNORM:    (np.uint8,   3, "BGR",  "unorm8"),
    ResourceFormat.BGR8_SRGB:     (np.uint8,   3, "BGR",  "unorm8"),
    ResourceFormat.R8_UNORM:      (np.uint8,   1, "R",    "unorm8"),
    ResourceFormat.RGBA16_UNORM:  (np.uint16,  4, "RGBA", "unorm16"),
    ResourceFormat.RGBA16_SFLOAT: (np.float16, 4, "RGBA", "float"),
    ResourceFormat.RGBA32_SFLOAT: (np.float32, 4, "RGBA", "float"),
    ResourceFormat.RGB32_SFLOAT:  (np.float32, 3, "RGB",  "float"),
    ResourceFormat.R16_SFLOAT:    (np.float16, 1, "R",    "float"),
    ResourceFormat.R32_SFLOAT:    (np.float32, 1, "R",    "float"),
}

SUPPORTED_DIMENSIONS = (ResourceDimension.TEXTURE_2D, ResourceDimension.TEXTURE_2D_ARRAY)


def write_readback_data(data, file_path):
    """
    Writes the contents of a ready readback out as an image file. The file type follows the extension of
    file_path (png, jpg, bmp, ...). Returns a short description of what was written, and raises on anything
    it cannot make sense of.
    """
    dimensions = data.getDimensions()
    if dimensions not in SUPPORTED_DIMENSIONS:
        raise ValueError(f"can only write 2d textures, got {dimensions}")

    resource_format = data.getFormat()
    layout = FORMAT_LAYOUTS.get(resource_format)
    if layout == None:
        raise ValueError(f"no image conversion for {resource_format}")

    pixels = _read_pixels(data, layout)
    rgba = _to_rgba8(pixels, layout)

    width = rgba.shape[1]
    height = rgba.shape[0]

    directory = os.path.dirname(file_path)
    if directory:
        os.makedirs(directory, exist_ok=True)

    #QImage does not own the buffer it is handed, so copy() before the numpy array goes out of scope
    image = QImage(rgba.tobytes(), width, height, width * 4, QImage.Format_RGBA8888).copy()
    if not image.save(file_path):
        raise RuntimeError(f"QImage failed to save '{file_path}'")

    return f"{width}x{height} {resource_format}"


def _read_pixels(data, layout):
    """Unpacks the raw readback bytes into a (height, width, channels) array of the texture's own type."""
    dtype, channels, _, _ = layout

    width = int(data.getWidth())
    height = int(data.getHeight())
    row_pitch = int(data.getRowPitchInBytes())
    texel_size = np.dtype(dtype).itemsize * channels

    if width == 0 or height == 0:
        raise ValueError(f"readback has no extent ({width}x{height})")

    if row_pitch < width * texel_size:
        raise ValueError(f"row pitch {row_pitch} is too small for {width} texels of {texel_size} bytes")

    raw = data.getBytes()
    expected_size = row_pitch * height
    if len(raw) < expected_size:
        raise ValueError(f"readback returned {len(raw)} bytes, expected {expected_size}")

    #index rows by the pitch and drop whatever padding follows them, instead of assuming the rows are back to back
    rows = np.frombuffer(raw, dtype=np.uint8, count=expected_size).reshape(height, row_pitch)
    rows = np.ascontiguousarray(rows[:, :width * texel_size])
    return rows.view(dtype).reshape(height, width, channels)


def _to_rgba8(pixels, layout):
    """Converts the unpacked texels into the 8 bit RGBA that QImage wants."""
    _, channels, order, encoding = layout

    if encoding == "unorm8":
        values = pixels
    elif encoding == "unorm16":
        values = (pixels >> 8).astype(np.uint8)
    elif encoding == "float":
        #the readback target is already tonemapped, so the values only need clamping to the displayable range
        values = np.rint(np.clip(pixels.astype(np.float32), 0.0, 1.0) * 255.0).astype(np.uint8)
    else:
        raise ValueError(f"unknown encoding '{encoding}'")

    height = values.shape[0]
    width = values.shape[1]
    rgba = np.empty((height, width, 4), dtype=np.uint8)

    if channels == 1:
        rgba[:, :, 0] = values[:, :, 0]
        rgba[:, :, 1] = values[:, :, 0]
        rgba[:, :, 2] = values[:, :, 0]
        rgba[:, :, 3] = 255
    else:
        red_index = order.index("R")
        green_index = order.index("G")
        blue_index = order.index("B")
        rgba[:, :, 0] = values[:, :, red_index]
        rgba[:, :, 1] = values[:, :, green_index]
        rgba[:, :, 2] = values[:, :, blue_index]
        if "A" in order and not WRITE_OPAQUE:
            rgba[:, :, 3] = values[:, :, order.index("A")]
        else:
            rgba[:, :, 3] = 255

    return rgba
