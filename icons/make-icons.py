#!/usr/bin/env python3
"""Generate vole's application icon: one SVG and the PNG sizes a Linux desktop
installs.

Stdlib only, like the other tools in this project -- no ImageMagick, no
librsvg, no Pillow. It carries its own scanline rasterizer (about 80 lines
below) because the alternative was making everyone who touches the icon install
a toolchain to regenerate a 16x16 PNG.

The geometry lives HERE and nowhere else. The SVG is emitted from the same
shape list that gets rasterized, so the vector and the bitmaps cannot drift
apart -- edit this file, re-run it, commit what changes.

    python3 make-icons.py

THE DESIGN. A vole seen from above, which is already the shape of a computer
mouse: the body is the shell, the seam and wheel are the buttons, and the tail
reads as the cable. The pun is the point -- it ties the name to what the
program does without borrowing anyone's branding, which for this project is a
requirement and not a preference (see README).
"""

import math
import struct
import zlib
from pathlib import Path

# ----------------------------------------------------------------- palette ---

BODY   = "#8A6642"   # warm vole brown
DARK   = "#6E5136"   # ears, tail, button seam
ACCENT = "#35B0A6"   # the scroll wheel: the one "this is software" note
EYE    = "#241C14"

# ------------------------------------------------------------------ shapes ---
# Design space is 256x256. Order is paint order, back to front.
#
# `detail` shapes are dropped at small sizes, where they turn to mud rather
# than to detail: below about 32px the silhouette is the whole icon.

SHAPES = [
    # Ears sit behind the body, so only their outer arcs show as bumps. They
    # are the whole vole: without them this is a generic mouse icon.
    dict(kind="circle", cx=74,  cy=64, r=27, fill=DARK, small=dict(r=32)),
    dict(kind="circle", cx=182, cy=64, r=27, fill=DARK, small=dict(r=32)),

    # The tail, which is also the cable. One sweeping curve, emerging from
    # under the body and tapering away to the lower right.
    #
    # Painted BEHIND the body, like the ears: drawn on top, its root sits as a
    # visible blob on the body's edge and the whole thing reads as something
    # stuck on rather than something the animal has. It must also NOT curl back
    # on itself -- an earlier revision ended with an upturn, and at icon size
    # the hook closed against the body and read as a detached loop. And it has
    # to stop short of the edge, because half the stroke width hangs outside
    # the path and the bitmaps clip whatever crosses the viewBox.
    dict(kind="stroke", stroke=DARK, width=17, small=dict(width=30),
         d="M 126 196 C 134 238, 164 248, 198 238"),

    # Body: narrow at the nose, widest at the palm, rounded at the rear.
    dict(kind="path", fill=BODY, d=(
        "M 128 28 "
        "C 168 28, 204 72, 204 126 "
        "C 204 186, 180 220, 128 220 "
        "C 76 220, 52 186, 52 126 "
        "C 52 72, 88 28, 128 28 Z")),

    # Button seam, running back from the wheel exactly as a real mouse's does.
    dict(kind="stroke", stroke=DARK, width=7, detail=True,
         d="M 128 92 L 128 150"),

    # Scroll wheel, at the front edge where it belongs, and the one accent
    # colour: this is a configuration tool.
    dict(kind="rrect", x=119, y=46, w=18, h=40, rx=9, fill=ACCENT,
         small=dict(x=114, w=28, rx=14)),

    # No eyes. An earlier revision had them and the icon read as a bear's
    # face -- two dark dots above a seam become a nose and mouth, and the
    # top-down mouse disappears entirely.
]

VIEW = 256
DETAIL_MIN_PX = 32          # below this, drop `detail` shapes


# -------------------------------------------------------------- path parse ---

def parse_path(d):
    """Flatten an SVG path (M/L/C/Z, absolute only) into polylines."""
    toks = d.replace(",", " ").split()
    polys, cur, start, i = [], [], None, 0

    def flush():
        if len(cur) > 1:
            polys.append(list(cur))

    while i < len(toks):
        op = toks[i]; i += 1
        if op == "M":
            flush(); cur.clear()
            start = (float(toks[i]), float(toks[i + 1])); i += 2
            cur.append(start)
        elif op == "L":
            cur.append((float(toks[i]), float(toks[i + 1]))); i += 2
        elif op == "C":
            p0 = cur[-1]
            p1 = (float(toks[i]),     float(toks[i + 1]))
            p2 = (float(toks[i + 2]), float(toks[i + 3]))
            p3 = (float(toks[i + 4]), float(toks[i + 5]))
            i += 6
            # 24 segments is far finer than 512px needs and costs nothing here.
            for s in range(1, 25):
                t, u = s / 24.0, 1.0 - s / 24.0
                cur.append((
                    u*u*u*p0[0] + 3*u*u*t*p1[0] + 3*u*t*t*p2[0] + t*t*t*p3[0],
                    u*u*u*p0[1] + 3*u*u*t*p1[1] + 3*u*t*t*p2[1] + t*t*t*p3[1]))
        elif op == "Z":
            cur.append(start)
        else:
            raise ValueError("unsupported path op %r" % op)
    flush()
    return polys


def circle_poly(cx, cy, r, n=72):
    return [(cx + r*math.cos(2*math.pi*k/n), cy + r*math.sin(2*math.pi*k/n))
            for k in range(n + 1)]


def rrect_poly(x, y, w, h, rx):
    pts, n = [], 16
    for cx, cy, a0 in ((x+w-rx, y+rx, -math.pi/2), (x+w-rx, y+h-rx, 0.0),
                       (x+rx,   y+h-rx, math.pi/2), (x+rx,   y+rx, math.pi)):
        for k in range(n + 1):
            a = a0 + (math.pi/2) * k / n
            pts.append((cx + rx*math.cos(a), cy + rx*math.sin(a)))
    pts.append(pts[0])
    return pts


def stroke_polys(polylines, width):
    """A stroke as a union of quads and round joins/caps. Kept as separate
    polygons and combined with max() coverage, so overlaps do not darken."""
    r, out = width / 2.0, []
    for line in polylines:
        for (x0, y0), (x1, y1) in zip(line, line[1:]):
            dx, dy = x1 - x0, y1 - y0
            ln = math.hypot(dx, dy)
            if ln < 1e-9:
                continue
            nx, ny = -dy / ln * r, dx / ln * r
            out.append([(x0+nx, y0+ny), (x1+nx, y1+ny),
                        (x1-nx, y1-ny), (x0-nx, y0-ny), (x0+nx, y0+ny)])
        for px, py in line:                      # round joins and caps
            out.append(circle_poly(px, py, r, 24))
    return out


def shape_polys(sh):
    if sh["kind"] == "circle":
        return [circle_poly(sh["cx"], sh["cy"], sh["r"])]
    if sh["kind"] == "rrect":
        return [rrect_poly(sh["x"], sh["y"], sh["w"], sh["h"], sh["rx"])]
    if sh["kind"] == "path":
        return parse_path(sh["d"])
    if sh["kind"] == "stroke":
        return stroke_polys(parse_path(sh["d"]), sh["width"])
    raise ValueError(sh["kind"])


# -------------------------------------------------------------- rasterizer ---
# Scanline fill, nonzero winding, 4 sub-scanlines per row with analytic
# horizontal coverage. That is enough antialiasing that 16x16 stays legible.

SUB = 4


def fill_coverage(polys, size):
    """Per-pixel coverage 0..1 for a union of polygons, at `size` px square."""
    scale = size / float(VIEW)
    cov = [0.0] * (size * size)

    for poly in polys:
        pts = [(x * scale, y * scale) for x, y in poly]
        edges = [(x0, y0, x1, y1) for (x0, y0), (x1, y1) in zip(pts, pts[1:])
                 if y0 != y1]
        if not edges:
            continue
        ymin = max(0, int(min(min(e[1], e[3]) for e in edges)))
        ymax = min(size - 1, int(max(max(e[1], e[3]) for e in edges)) + 1)
        sub = [0.0] * size

        for row in range(ymin, ymax + 1):
            for k in range(size):
                sub[k] = 0.0
            hit = False
            for s in range(SUB):
                yc = row + (s + 0.5) / SUB
                xs = []
                for x0, y0, x1, y1 in edges:
                    if (y0 <= yc < y1) or (y1 <= yc < y0):
                        t = (yc - y0) / (y1 - y0)
                        xs.append((x0 + t * (x1 - x0), 1 if y1 > y0 else -1))
                if not xs:
                    continue
                xs.sort()
                wind = 0
                for j in range(len(xs) - 1):
                    wind += xs[j][1]
                    if wind == 0:
                        continue
                    a, b = xs[j][0], xs[j + 1][0]
                    if b <= 0 or a >= size:
                        continue
                    a, b = max(a, 0.0), min(b, float(size))
                    ia, ib = int(a), int(b)
                    hit = True
                    if ia == ib:
                        sub[ia] += (b - a) / SUB
                    else:
                        sub[ia] += (ia + 1 - a) / SUB
                        for px in range(ia + 1, min(ib, size)):
                            sub[px] += 1.0 / SUB
                        if ib < size:
                            sub[ib] += (b - ib) / SUB
            if hit:
                base = row * size
                for px in range(size):
                    v = sub[px]
                    if v > 0.0:
                        # max(), not +=: overlapping parts of one shape (a
                        # stroke's quads and joins) must not double-darken.
                        if v > cov[base + px]:
                            cov[base + px] = v if v < 1.0 else 1.0
    return cov


def rgb(h):
    return (int(h[1:3], 16), int(h[3:5], 16), int(h[5:7], 16))


def render(size):
    """Composite every shape into a straight-alpha RGBA buffer.

    Below DETAIL_MIN_PX the geometry is hinted rather than merely scaled:
    `detail` shapes are dropped and `small` overrides are applied. A 17-unit
    tail is under one pixel wide at 16px, so antialiasing renders it as a pale
    smudge; the ears disappear into the body for the same reason. Exaggerating
    them is what keeps the silhouette readable, and it is why this script
    renders each size from the geometry instead of downsampling one bitmap.
    """
    small = size < DETAIL_MIN_PX
    buf = [0.0] * (size * size * 4)            # premultiplied while compositing
    for sh in SHAPES:
        if sh.get("detail") and small:
            continue
        if small and "small" in sh:
            sh = {**sh, **sh["small"]}
        r, g, b = rgb(sh.get("fill") or sh["stroke"])
        cov = fill_coverage(shape_polys(sh), size)
        for i, a in enumerate(cov):
            if a <= 0.0:
                continue
            j = i * 4
            inv = 1.0 - a
            buf[j]     = buf[j]     * inv + r * a
            buf[j + 1] = buf[j + 1] * inv + g * a
            buf[j + 2] = buf[j + 2] * inv + b * a
            buf[j + 3] = buf[j + 3] * inv + 255.0 * a

    out = bytearray(size * size * 4)
    for i in range(size * size):
        j = i * 4
        a = buf[j + 3]
        if a <= 0.5:
            continue
        s = 255.0 / a                           # un-premultiply
        out[j]     = min(255, int(buf[j]     * s + 0.5))
        out[j + 1] = min(255, int(buf[j + 1] * s + 0.5))
        out[j + 2] = min(255, int(buf[j + 2] * s + 0.5))
        out[j + 3] = min(255, int(a + 0.5))
    return bytes(out)


def write_png(path, size, rgba):
    raw = b"".join(b"\x00" + rgba[y*size*4:(y+1)*size*4] for y in range(size))

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b""))


# --------------------------------------------------------------- svg output ---

def write_svg(path):
    body = []
    for sh in SHAPES:
        if sh["kind"] == "circle":
            body.append('  <circle cx="%g" cy="%g" r="%g" fill="%s"/>'
                        % (sh["cx"], sh["cy"], sh["r"], sh["fill"]))
        elif sh["kind"] == "rrect":
            body.append('  <rect x="%g" y="%g" width="%g" height="%g" rx="%g" fill="%s"/>'
                        % (sh["x"], sh["y"], sh["w"], sh["h"], sh["rx"], sh["fill"]))
        elif sh["kind"] == "path":
            body.append('  <path d="%s" fill="%s"/>' % (sh["d"], sh["fill"]))
        elif sh["kind"] == "stroke":
            body.append('  <path d="%s" fill="none" stroke="%s" stroke-width="%g"'
                        ' stroke-linecap="round" stroke-linejoin="round"/>'
                        % (sh["d"], sh["stroke"], sh["width"]))
    path.write_text(
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<!-- Generated by make-icons.py. Edit that, not this. -->\n'
        '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d"'
        ' viewBox="0 0 %d %d">\n%s\n</svg>\n'
        % (VIEW, VIEW, VIEW, VIEW, "\n".join(body)), encoding="utf-8")


SIZES = [16, 22, 24, 32, 48, 64, 128, 256, 512]

if __name__ == "__main__":
    here = Path(__file__).resolve().parent
    write_svg(here / "vole.svg")
    print("vole.svg")
    for size in SIZES:
        d = here / ("%dx%d" % (size, size))
        d.mkdir(exist_ok=True)
        write_png(d / "vole.png", size, render(size))
        print("%dx%d/vole.png" % (size, size))
