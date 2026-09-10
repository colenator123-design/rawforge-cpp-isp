"""Create a dependency-free PNG contact sheet from RawForge PPM previews."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    with path.open("rb") as stream:
        if stream.readline().strip() != b"P6":
            raise ValueError(f"{path} is not a binary PPM")
        width, height = map(int, stream.readline().split())
        if stream.readline().strip() != b"255":
            raise ValueError("Only 8-bit PPM files are supported")
        pixels = stream.read()
    if len(pixels) != width * height * 3:
        raise ValueError(f"Unexpected pixel count in {path}")
    return width, height, pixels


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(
        ">I", zlib.crc32(kind + payload) & 0xFFFFFFFF
    )


def write_png(path: Path, width: int, height: int, pixels: bytes) -> None:
    scanlines = b"".join(
        b"\x00" + pixels[row * width * 3 : (row + 1) * width * 3] for row in range(height)
    )
    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", header)
        + png_chunk(b"IDAT", zlib.compress(scanlines, level=9))
        + png_chunk(b"IEND", b"")
    )


def crop_and_scale(pixels: bytes, width: int, height: int) -> bytes:
    """Enlarge the same smooth/detail boundary crop for every comparison panel."""
    x0, y0 = int(width * 0.40), int(height * 0.38)
    crop_width, crop_height = int(width * 0.28), int(height * 0.34)
    output = bytearray(width * height * 3)
    for y in range(height):
        source_y = y0 + min(crop_height - 1, y * crop_height // height)
        for x in range(width):
            source_x = x0 + min(crop_width - 1, x * crop_width // width)
            source = (source_y * width + source_x) * 3
            destination = (y * width + x) * 3
            output[destination : destination + 3] = pixels[source : source + 3]
    return bytes(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=Path("outputs/demo"))
    parser.add_argument("--output", type=Path, default=Path("docs/images/comparison.png"))
    arguments = parser.parse_args()
    names = [
        "00_clean_scene.ppm",
        "01_noisy_quad_preview.ppm",
        "02_baseline_preview.ppm",
        "03_restored_preview.ppm",
    ]
    images = [read_ppm(arguments.input / name) for name in names]
    width, height = images[0][:2]
    if any(image[:2] != (width, height) for image in images):
        raise ValueError("All previews must have identical dimensions")
    gutter = 8
    output_width = width * len(images) + gutter * (len(images) - 1)
    rows: list[bytes] = []
    for image_row in (images, [(width, height, crop_and_scale(p, width, height)) for _, _, p in images]):
        if rows:
            rows.extend([b"\xff" * (output_width * 3)] * gutter)
        for y in range(height):
            row = bytearray()
            for index, (_, _, pixels) in enumerate(image_row):
                if index:
                    row.extend(b"\xff" * gutter * 3)
                row.extend(pixels[y * width * 3 : (y + 1) * width * 3])
            rows.append(bytes(row))
    write_png(arguments.output, output_width, height * 2 + gutter, b"".join(rows))
    print(f"Comparison: {arguments.output}")


if __name__ == "__main__":
    main()
