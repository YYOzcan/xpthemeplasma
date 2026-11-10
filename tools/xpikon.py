"""PE dosyalarından XP simgelerini en yüksek renk derinliğiyle çıkarır.

    groups = icon_images(pe_bytes)   -> {grup_id: {boyut: RGBA görüntü}}
"""
import io
import struct

import numpy as np
from PIL import Image

from msstyles import decode_dib
from peres import PEResources


def _decode_entry(blob: bytes) -> Image.Image:
    if blob[:8] == b"\x89PNG\r\n\x1a\n":
        return Image.open(io.BytesIO(blob)).convert("RGBA")
    hsize, w, h2, planes, bpp = struct.unpack_from("<IiiHH", blob, 0)
    h = h2 // 2
    # XOR görüntüsü: yüksekliği yarıya indirilmiş bir DIB gibi çöz
    hdr = bytearray(blob[:hsize])
    struct.pack_into("<i", hdr, 8, h)
    xor_img, _ = decode_dib(bytes(hdr) + blob[hsize:])
    a = np.asarray(xor_img).copy()
    # AND maskesi XOR verisinden sonra gelir (1 bpp, satırlar 4 bayta hizalı, alttan üste)
    ncol = 0 if bpp > 8 else (struct.unpack_from("<I", blob, 32)[0] or (1 << bpp))
    xor_stride = ((w * bpp + 31) // 32) * 4
    off = hsize + ncol * 4 + xor_stride * h
    and_stride = ((w + 31) // 32) * 4
    mask_bytes = blob[off:off + and_stride * h]
    use_alpha = bpp == 32 and a[:, :, 3].any()
    if not use_alpha and len(mask_bytes) == and_stride * h:
        m = np.frombuffer(mask_bytes, np.uint8).reshape(h, and_stride)
        bits = np.unpackbits(m, axis=1)[:, :w][::-1]
        a[:, :, 3] = np.where(bits == 1, 0, 255)
    elif not use_alpha:
        a[:, :, 3] = 255
    return Image.fromarray(a, "RGBA")


def icon_images(pe: bytes):
    r = PEResources(pe)
    singles = {n: d for t, n, l, d in r.items() if t == "ICON"}
    out = {}
    for t, n, l, d in r.items():
        if t != "GROUP_ICON" or n in out:
            continue
        cnt = struct.unpack_from("<H", d, 4)[0]
        best = {}
        for i in range(cnt):
            w, h, cc, res, planes, bpp, size, rid = struct.unpack_from("<BBBBHHIH", d, 6 + 14 * i)
            w = w or 256
            blob = singles.get(rid)
            if blob is None:
                continue
            if blob[:8] != b"\x89PNG\r\n\x1a\n":
                bpp = struct.unpack_from("<H", blob, 14)[0]
            if w not in best or bpp > best[w][0]:
                best[w] = (bpp, blob)
        imgs = {}
        for w, (bpp, blob) in best.items():
            try:
                imgs[w] = _decode_entry(blob)
            except Exception:
                pass
        if imgs:
            out[n] = imgs
    return out


def at_size(imgs, size):
    """İstenen boyut yoksa en yakın büyüğü küçült (yoksa en büyüğü büyüt)."""
    if size in imgs:
        return imgs[size]
    bigger = sorted(s for s in imgs if s > size)
    src = imgs[bigger[0]] if bigger else imgs[max(imgs)]
    return src.resize((size, size), Image.LANCZOS)
