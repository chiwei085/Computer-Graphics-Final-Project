#!/usr/bin/env python3
"""Generate 6 procedural CC0 textures for assets/textures/.

All images are generated in-house (no downloads) and are released as CC0.
"""

from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "assets" / "textures"
SIZE = 512


def _save(img: Image.Image, name: str) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / name
    img.save(path)
    print(f"  wrote {path.relative_to(REPO_ROOT)}")


def gen_wood() -> None:
    """Warm wood grain: parallel streaks with sine wiggle."""
    rng = random.Random(42)
    img = Image.new("RGB", (SIZE, SIZE))
    px = img.load()
    assert px is not None
    for y in range(SIZE):
        for x in range(SIZE):
            phase = rng.uniform(0.0, math.pi * 2)
            ring = math.sin((y / SIZE) * math.pi * 18 + math.sin(x * 0.05 + phase) * 4)
            t = (ring + 1.0) * 0.5
            r = int(120 + t * 80)
            g = int(72 + t * 48)
            b = int(30 + t * 24)
            px[x, y] = (r, g, b)  # type: ignore[index]
    img = img.filter(ImageFilter.GaussianBlur(radius=0.6))
    _save(img, "wood.png")


def gen_cloth() -> None:
    """Woven fabric: alternating horizontal and vertical weave threads."""
    img = Image.new("RGB", (SIZE, SIZE))
    px = img.load()
    assert px is not None
    period = 8
    for y in range(SIZE):
        for x in range(SIZE):
            hv = (x // period + y // period) % 2
            sub_x = x % period
            sub_y = y % period
            mid = period // 2
            if hv == 0:
                t = max(0.0, 1.0 - abs(sub_y - mid) / (mid + 1)) * 0.6 + 0.4
            else:
                t = max(0.0, 1.0 - abs(sub_x - mid) / (mid + 1)) * 0.6 + 0.4
            r = int(200 * t)
            g = int(185 * t)
            b = int(160 * t)
            px[x, y] = (r, g, b)  # type: ignore[index]
    _save(img, "cloth.png")


def gen_circuit() -> None:
    """Bright green PCB with dark trace lines and solder pads.

    Background is kept bright so GL_MODULATE with EmissiveGreen stays vivid.
    Dark traces provide the circuit pattern detail.
    """
    rng = random.Random(7)
    bg = (160, 230, 160)  # bright green base — stays luminous under GL_MODULATE
    img = Image.new("RGB", (SIZE, SIZE), bg)
    draw = ImageDraw.Draw(img)

    # Dark trace lines (circuit routes)
    for _ in range(50):
        x0 = rng.randint(0, SIZE)
        y0 = rng.randint(0, SIZE)
        length = rng.randint(20, 100)
        w = rng.choice([2, 3])
        dark = (30, 60, 30)
        if rng.random() < 0.5:
            draw.line([(x0, y0), (x0 + length, y0)], fill=dark, width=w)
            draw.line([(x0 + length, y0),
                       (x0 + length, y0 + rng.randint(10, 50))],
                      fill=dark, width=w)
        else:
            draw.line([(x0, y0), (x0, y0 + length)], fill=dark, width=w)
            draw.line([(x0, y0 + length),
                       (x0 + rng.randint(10, 50), y0 + length)],
                      fill=dark, width=w)

    # Solder pads (bright white-yellow highlights)
    for _ in range(40):
        cx = rng.randint(0, SIZE)
        cy = rng.randint(0, SIZE)
        r = rng.randint(3, 7)
        draw.ellipse([(cx - r, cy - r), (cx + r, cy + r)],
                     fill=(230, 255, 180))

    _save(img, "circuit.png")


def gen_marble() -> None:
    """White marble with sinuous grey veins."""
    rng = random.Random(3)
    img = Image.new("RGB", (SIZE, SIZE))
    px = img.load()
    assert px is not None
    for y in range(SIZE):
        for x in range(SIZE):
            fx = x / SIZE
            fy = y / SIZE
            noise = math.sin(fx * 10 + math.sin(fy * 8 + rng.uniform(-0.2, 0.2)) * 3)
            t = abs(noise)
            base = 235
            vein = int(base - t * 90)
            px[x, y] = (vein, vein, vein)  # type: ignore[index]
    img = img.filter(ImageFilter.GaussianBlur(radius=0.8))
    _save(img, "marble.png")


def gen_metal() -> None:
    """Brushed metal: horizontal light streaks with slight variation."""
    rng = random.Random(11)
    img = Image.new("RGB", (SIZE, SIZE))
    px = img.load()
    assert px is not None
    row_vals: list[int] = []
    for y in range(SIZE):
        base = int(160 + rng.gauss(0, 18))
        base = max(110, min(210, base))
        row_vals.append(base)

    for y in range(SIZE):
        for x in range(SIZE):
            # Column micro-variation
            col_delta = int(rng.gauss(0, 5))
            v = max(100, min(220, row_vals[y] + col_delta))
            px[x, y] = (v, v + 4, v + 8)  # type: ignore[index]
    img = img.filter(ImageFilter.GaussianBlur(radius=0.3))
    _save(img, "metal.png")


def gen_paper() -> None:
    """Off-white paper with subtle fibrous texture."""
    rng = random.Random(99)
    img = Image.new("RGB", (SIZE, SIZE))
    px = img.load()
    assert px is not None
    for y in range(SIZE):
        for x in range(SIZE):
            noise = rng.gauss(0, 8)
            v = int(230 + noise)
            v = max(200, min(250, v))
            r = min(255, v + 5)
            g = min(255, v + 3)
            b = min(255, v - 4)
            px[x, y] = (r, g, b)  # type: ignore[index]
    img = img.filter(ImageFilter.GaussianBlur(radius=0.4))
    _save(img, "paper.png")


def main() -> None:
    print("Generating textures...")
    gen_wood()
    gen_cloth()
    gen_circuit()
    gen_marble()
    gen_metal()
    gen_paper()
    print("Done — 6 textures written to assets/textures/")


if __name__ == "__main__":
    main()
