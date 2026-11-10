#!/usr/bin/env python3
"""XP'nin varsayılan imleçlerinden (user32.dll) Xcursor teması.

    python3 tools/gen_luna_imlec.py <xp-dizini> <çıktı: .../icons/Windows XP>

XP'nin "işaretçi gölgesi" (varsayılan açık) yumuşak bir gölgeyle taklit edilir.
"""
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

sys.path.insert(0, str(Path(__file__).parent))
from msstyles import decode_dib  # noqa: E402
from peres import PEResources  # noqa: E402

# user32.dll imleç grubu -> X imleç adları
IMLECLER = {
    100: "default left_ptr arrow top_left_arrow x-cursor X_cursor right_ptr draft_large draft_small context-menu",
    101: "text xterm ibeam vertical-text",
    102: "wait watch",
    103: "crosshair cross tcross cross_reverse diamond_cross cell plus",
    104: "up-arrow center_ptr sb_up_arrow",
    105: "nwse-resize size_fdiag bd_double_arrow nw-resize se-resize top_left_corner bottom_right_corner c7f8f8f7f8f8f8f8f8f8f8f8f8f8f8f8 ul_angle lr_angle",
    106: "nesw-resize size_bdiag fd_double_arrow ne-resize sw-resize top_right_corner bottom_left_corner fcf1c3c7cd4491d801f1e1c78f100000 ur_angle ll_angle",
    107: "ew-resize size_hor sb_h_double_arrow h_double_arrow e-resize w-resize col-resize left_side right_side split_h 028006030e0e7ebffc7f7070c0600140 14fef782d02440884392942c11205230",
    108: "ns-resize size_ver sb_v_double_arrow v_double_arrow n-resize s-resize row-resize top_side bottom_side split_v 00008160000006810000408080010102 2870a09082c103050810ffdffffe0204",
    109: "move fleur size_all all-scroll grab grabbing openhand closedhand dnd-move dnd-none 4498f0e0c1937ffe01fd06f973665830 9081237383d90e509aa00f00170e968f",
    110: "not-allowed forbidden crossed_circle no-drop circle dnd-no-drop 03b6e0fcb3499374a867c041f52298f0",
    111: "progress left_ptr_watch half-busy 3ecb610c1bf2410f44200f48c40d3599 08e8e1c95fe2fc01f976f1e063a24ccd 00000000000000020006000e7e9ffc3f",
    112: "help question_arrow whats_this left_ptr_help 5c6cd98b3f3ebcb1f9c7f1c204630408 d9ce0ab605698f320427677b458ad60b",
    113: "pencil draft",
    114: "pointer hand1 hand2 pointing_hand hand e29285e634086352946a0e7090d73106 9d800788f1b08800ae810202380a0822",
}
ALIAS_DND = {"copy": 100, "alias": 100, "dnd-copy": 100, "dnd-link": 100, "link": 100, "dnd-ask": 100,
             "zoom-in": 103, "zoom-out": 103, "color-picker": 103}


def decode_cursor(blob):
    """CURSOR kaynağı (sıcak nokta öneki çıkarılmış) -> RGBA. Ters çevirme pikselleri siyah olur."""
    hsize, w, h2, planes, bpp = struct.unpack_from("<IiiHH", blob, 0)
    h = h2 // 2
    hdr = bytearray(blob[:hsize])
    struct.pack_into("<i", hdr, 8, h)
    img, _ = decode_dib(bytes(hdr) + blob[hsize:])
    a = np.asarray(img).copy()
    ncol = 0 if bpp > 8 else (struct.unpack_from("<I", blob, 32)[0] or (1 << bpp))
    xor_stride = ((w * bpp + 31) // 32) * 4
    off = hsize + ncol * 4 + xor_stride * h
    and_stride = ((w + 31) // 32) * 4
    m = np.frombuffer(blob[off:off + and_stride * h], np.uint8).reshape(h, and_stride)
    andbits = np.unpackbits(m, axis=1)[:, :w][::-1]
    if bpp == 32 and a[:, :, 3].any():
        return Image.fromarray(a, "RGBA")
    lum = a[:, :, :3].max(axis=2)
    out = a.copy()
    out[:, :, 3] = 255
    transparent = (andbits == 1) & (lum == 0)
    invert = (andbits == 1) & (lum > 0)
    out[transparent] = 0
    out[invert] = (0, 0, 0, 255)
    return Image.fromarray(out, "RGBA")


def with_shadow(img):
    """XP işaretçi gölgesi: 2 px sağ-alt, yumuşak, %35."""
    a = np.asarray(img)
    alpha = Image.fromarray(a[:, :, 3])
    sh = Image.new("L", img.size)
    sh.paste(alpha, (2, 2))
    sh = sh.filter(ImageFilter.GaussianBlur(1.2)).point(lambda v: int(v * 0.35))
    shadow = Image.merge("RGBA", [Image.new("L", img.size, 0)] * 3 + [sh])
    shadow.alpha_composite(img)
    return shadow


def xcursor(frames):
    """frames: [(boyut, RGBA, xhot, yhot, gecikme)] -> Xcursor baytları (ARGB, önçarpımlı)."""
    ntoc = len(frames)
    header = struct.pack("<4sIII", b"Xcur", 16, 0x10000, ntoc)
    pos = 16 + ntoc * 12
    toc, chunks = b"", b""
    for size, img, xh, yh, delay in frames:
        w, h = img.size
        a = np.asarray(img).astype(np.uint32)
        al = a[:, :, 3]
        r = a[:, :, 0] * al // 255
        g = a[:, :, 1] * al // 255
        b = a[:, :, 2] * al // 255
        argb = (al << 24) | (r << 16) | (g << 8) | b
        chunk = struct.pack("<IIIIIIIII", 36, 0xFFFD0002, size, 1, w, h, xh, yh, delay) + argb.astype("<u4").tobytes()
        toc += struct.pack("<III", 0xFFFD0002, size, pos)
        pos += len(chunk)
        chunks += chunk
    return header + toc + chunks


def main(xp, out):
    xp, out = Path(xp), Path(out)
    r = PEResources((xp / "ikon/user32.dll").read_bytes())
    singles = {n: d for t, n, l, d in r.items() if t == "CURSOR"}
    groups = {}
    for t, n, l, d in r.items():
        if t == "GROUP_CURSOR":
            rid = struct.unpack_from("<H", d, 6 + 12)[0]
            blob = singles[rid]
            hx, hy = struct.unpack_from("<HH", blob, 0)
            groups[n] = (decode_cursor(blob[4:]), hx, hy)
    cdir = out / "cursors"
    cdir.mkdir(parents=True, exist_ok=True)
    data = {}
    for gid, names in IMLECLER.items():
        img, hx, hy = groups[gid]
        img = with_shadow(img)
        frames = []
        for size in (24, 32, 48, 64):
            if size == 32:
                im, x, y = img, hx, hy
            else:
                im = img.resize((size, size), Image.LANCZOS if size < 32 else Image.NEAREST)
                x, y = hx * size // 32, hy * size // 32
            frames.append((size, im, x, y, 0))
        data[gid] = xcursor(frames)
        for name in names.split():
            (cdir / name).write_bytes(data[gid])
    for name, gid in ALIAS_DND.items():
        (cdir / name).write_bytes(data[gid])
    print("imleçler ->", cdir, f"({len(list(cdir.iterdir()))} ad)")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
