"""Windows XP .msstyles okuyucu (luna.msstyles).

    ms = Msstyles("luna.msstyles", variant="NORMALBLUE")
    ms.prop("Window.CloseButton", "ImageCount")      -> "8"
    ms.images("Window.CloseButton")                  -> [PIL.Image RGBA, ...]  (ImageCount kadar)
    ms.bitmap("Blue\\\\CloseButton.bmp")              -> PIL.Image RGBA
    ms.margins("Window.Caption")                     -> (sol, sağ, üst, alt)

Sınıf adı kalıtımı: "TaskBar::Toolbar.Button(Hot)" -> "TaskBar::Toolbar.Button" ->
"Toolbar.Button" -> "Toolbar" (uxtheme'in yaptığı gibi).
"""
import re
import struct

import numpy as np
from PIL import Image

from peres import PEResources


def decode_dib(dib: bytes) -> Image.Image:
    """BITMAP kaynağını (BITMAPINFOHEADER + piksel) RGBA görüntüye çevirir."""
    hsize, w, h, planes, bpp, comp = struct.unpack_from("<IiiHHI", dib, 0)
    clr_used = struct.unpack_from("<I", dib, 32)[0]
    bottom_up = h > 0
    h = abs(h)
    off = hsize
    palette = None
    if bpp <= 8:
        n = clr_used or (1 << bpp)
        pal = np.frombuffer(dib, np.uint8, n * 4, off).reshape(n, 4)
        palette = pal[:, [2, 1, 0]]
        off += n * 4
    elif comp == 3:  # BI_BITFIELDS
        off += 12
    stride = ((w * bpp + 31) // 32) * 4
    raw = np.frombuffer(dib, np.uint8, stride * h, off).reshape(h, stride)
    if bpp == 32:
        px = raw[:, :w * 4].reshape(h, w, 4)
        rgba = px[:, :, [2, 1, 0, 3]].copy()
    elif bpp == 24:
        px = raw[:, :w * 3].reshape(h, w, 3)
        rgba = np.dstack([px[:, :, [2, 1, 0]], np.full((h, w), 255, np.uint8)])
    elif bpp == 8:
        idx = raw[:, :w]
        rgba = np.dstack([palette[idx], np.full((h, w), 255, np.uint8)])
    elif bpp == 4:
        b = raw[:, :(w + 1) // 2]
        idx = np.empty((h, b.shape[1] * 2), np.uint8)
        idx[:, 0::2] = b >> 4
        idx[:, 1::2] = b & 15
        idx = idx[:, :w]
        rgba = np.dstack([palette[idx], np.full((h, w), 255, np.uint8)])
    elif bpp == 1:
        bits = np.unpackbits(raw, axis=1)[:, :w]
        rgba = np.dstack([palette[bits], np.full((h, w), 255, np.uint8)])
    else:
        raise ValueError(f"{bpp} bpp desteklenmiyor")
    if bottom_up:
        rgba = rgba[::-1]
    return Image.fromarray(np.ascontiguousarray(rgba), "RGBA"), bpp


def unpremultiply(img: Image.Image) -> Image.Image:
    a = np.asarray(img).astype(np.float32)
    alpha = a[:, :, 3:4]
    rgb = np.where(alpha > 0, np.clip(a[:, :, :3] * 255.0 / np.maximum(alpha, 1), 0, 255), 0)
    return Image.fromarray(np.dstack([rgb, alpha]).astype(np.uint8), "RGBA")


class Msstyles:
    def __init__(self, path, variant="NORMALBLUE"):
        self.res = PEResources(open(path, "rb").read())
        txt = self.res.get("TEXTFILE", f"{variant}_INI").decode("utf-16-le")
        self.ini = self._parse(txt)
        self._cache = {}

    @staticmethod
    def _parse(txt):
        out, cur = {}, None
        for line in txt.splitlines():
            line = line.split(";", 1)[0].strip()
            if not line:
                continue
            if line.startswith("["):
                cur = line[1:line.index("]")].lower()
                out.setdefault(cur, {})
            elif "=" in line and cur is not None:
                k, v = line.split("=", 1)
                out[cur][k.strip().lower()] = v.strip()
        return out

    def chain(self, cls):
        """Arama zinciri: tam ad -> durumsuz -> uygulama öneksiz -> üst parça."""
        cls = cls.lower()
        seen = []

        def add(c):
            if c not in seen:
                seen.append(c)

        state = re.search(r"\(.*\)$", cls)
        base = cls[:state.start()] if state else cls
        variants = [cls, base] if state else [base]
        for v in variants:
            add(v)
            if "::" in v:
                add(v.split("::", 1)[1])
        for v in list(seen):
            if "." in v and "(" not in v:
                add(v.split(".", 1)[0])
        return seen

    def prop(self, cls, key, default=None):
        key = key.lower()
        for c in self.chain(cls):
            if c in self.ini and key in self.ini[c]:
                return self.ini[c][key]
        return default

    def ints(self, cls, key, default=None):
        v = self.prop(cls, key)
        if v is None:
            return default
        return [int(x) for x in re.split(r"[,\s]+", v.strip()) if x]

    def color(self, cls, key, default=None):
        v = self.ints(cls, key)
        return tuple(v[:3]) if v else default

    def margins(self, cls, key="SizingMargins"):
        return tuple(self.ints(cls, key, [0, 0, 0, 0]))   # sol, sağ, üst, alt

    def bitmap(self, filename, transparent_color=None):
        key = (filename.lower(), transparent_color)
        if key in self._cache:
            return self._cache[key]
        rname = filename.replace("\\", "_").replace(".", "_").upper()
        dib = self.res.get("BITMAP", rname)
        if dib is None:
            raise KeyError(rname)
        img, bpp = decode_dib(dib)
        if bpp == 32:
            img = unpremultiply(img) if self._premultiplied(img) else img
        elif transparent_color is not None:
            a = np.asarray(img).copy()
            mask = np.all(a[:, :, :3] == np.array(transparent_color, np.uint8), axis=2)
            a[mask] = 0
            img = Image.fromarray(a, "RGBA")
        self._cache[key] = img
        return img

    @staticmethod
    def _premultiplied(img):
        a = np.asarray(img)
        return bool(np.all(a[:, :, :3].max(axis=2) <= a[:, :, 3]))

    def transparent_color(self, cls):
        tr = (self.prop(cls, "Transparent", "false").lower() == "true"
              or self.prop(cls, "GlyphTransparent", "false").lower() == "true")
        if not tr:
            return None
        return self.color(cls, "TransparentColor", (255, 0, 255))

    def images(self, cls, key="ImageFile"):
        """Bitmap'i ImageCount parçasına böler (ImageLayout'a göre)."""
        fn = self.prop(cls, key) or self.prop(cls, key + "1")
        if fn is None:
            raise KeyError(f"{cls}: {key} yok")
        tc = self.transparent_color(cls)
        if key.lower().startswith("glyph"):
            tc = self.color(cls, "GlyphTransparentColor", (255, 0, 255)) \
                if self.prop(cls, "GlyphTransparent", "false").lower() == "true" else None
        img = self.bitmap(fn, tc)
        n = int(self.prop(cls, "ImageCount", "1"))
        vertical = self.prop(cls, "ImageLayout", "vertical").lower() == "vertical"
        W, H = img.size
        out = []
        for i in range(n):
            if vertical:
                h = H // n
                out.append(img.crop((0, i * h, W, (i + 1) * h)))
            else:
                w = W // n
                out.append(img.crop((i * w, 0, (i + 1) * w, H)))
        return out
