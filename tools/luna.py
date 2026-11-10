"""XPThemePlasma — Luna çizim çekirdeği.

Bütün görseller burada vektör (SVG) olarak sıfırdan çizilir, rsvg-convert ile
rasterlanır, 9 parçaya bölünür ve Kvantum / Plasma / Aurorae'nin beklediği
eleman kimlikleriyle tek bir SVG "sayfası"na gömülür.

Hiçbir Microsoft dosyası kullanılmaz; renkler Luna (Mavi) paletinden alınmıştır.
"""
from __future__ import annotations

import base64
import io
import subprocess
from dataclasses import dataclass, field

from PIL import Image

# ---------------------------------------------------------------- palet ---
LUNA = {
    "face": "#ECE9D8",          # 3B yüz / pencere zemini
    "window": "#FFFFFF",
    "highlight": "#316AC5",
    "highlight_text": "#FFFFFF",
    "text": "#000000",
    "gray_text": "#ACA899",
    "border": "#003C74",         # buton kenarı
    "input_border": "#7F9DB9",   # metin kutusu kenarı
    "menu_border": "#ACA899",
    "hot": "#E5A01A",            # turuncu üzerine gelme
    "hot_light": "#FDD889",
    "focus": "#98B8EA",
    "frame_dark": "#0831D9",
    "caption_text": "#FFFFFF",
    "caption_text_inactive": "#D8E4F8",
    "link": "#0066CC",
    "tooltip": "#FFFFE1",
    "green": "#21A121",
    "tab_border": "#919B9C",
    "group_border": "#D0D0BF",
    "group_title": "#0046D5",
}

# Başlık çubuğu degradesi (yukarıdan aşağı, konum 0..1)
CAPTION_ACTIVE = [
    (0.00, "#0997FF"), (0.08, "#0053EE"), (0.40, "#0050EE"),
    (0.88, "#0066FF"), (0.93, "#0066FF"), (0.95, "#005BFF"),
    (0.96, "#003DD7"), (1.00, "#003DD7"),
]
CAPTION_INACTIVE = [
    (0.00, "#A6BFF2"), (0.06, "#94AFE8"), (0.12, "#82A5E4"),
    (0.30, "#7996DE"), (0.60, "#7B99E1"), (0.85, "#82A9E9"),
    (0.94, "#7B96E1"), (1.00, "#7A93DF"),
]
TASKBAR = [
    (0.00, "#1F2F86"), (0.03, "#3165C4"), (0.06, "#3682E5"), (0.10, "#4490E6"),
    (0.12, "#3883E5"), (0.15, "#2B71E0"), (0.18, "#2663DA"), (0.20, "#235BD6"),
    (0.23, "#2258D5"), (0.38, "#2157D6"), (0.54, "#245DDB"), (0.86, "#2562DF"),
    (0.89, "#245FDC"), (0.92, "#2158D4"), (0.95, "#1D4EC0"), (1.00, "#1941A5"),
]
TRAY = [
    (0.00, "#0C59B9"), (0.06, "#139EE9"), (0.10, "#18B5F2"), (0.14, "#139BEB"),
    (0.19, "#1290E8"), (0.63, "#0D8DEA"), (0.81, "#0D9FF1"), (0.88, "#0F9EED"),
    (0.91, "#119BE9"), (0.94, "#1392E2"), (0.97, "#137ED7"), (1.00, "#095BC9"),
]
START_GREEN = [
    (0.00, "#3C9E3C"), (0.10, "#58B858"), (0.20, "#41A941"), (0.50, "#2F8F2F"),
    (0.80, "#2C8A2C"), (0.92, "#307F30"), (1.00, "#1F5F1F"),
]
BUTTON_FACE = [(0.0, "#FFFFFF"), (0.86, "#ECEBE6"), (1.0, "#D6D0C5")]
BUTTON_PRESSED = [(0.0, "#CDCAC3"), (0.08, "#E3E3DB"), (0.94, "#E5E5DE"), (1.0, "#F2F2F1")]
BUTTON_DISABLED = [(0.0, "#F5F4EA"), (1.0, "#F5F4EA")]


# ------------------------------------------------------------ SVG yardımcı ---
_gid = 0


def _new_id(prefix="g"):
    global _gid
    _gid += 1
    return f"{prefix}{_gid}"


def lingrad(stops, x1=0, y1=0, x2=0, y2=1, gid=None):
    """(defs_xml, url) döndürür. Konumlar objectBoundingBox biriminde."""
    gid = gid or _new_id("lg")
    s = "".join(
        f'<stop offset="{o}" stop-color="{c[:7]}"'
        + (f' stop-opacity="{int(c[7:9], 16) / 255:.3f}"' if len(c) == 9 else "")
        + "/>"
        for o, c in stops
    )
    return (f'<linearGradient id="{gid}" x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}">{s}</linearGradient>',
            f"url(#{gid})")


def radgrad(stops, cx=0.5, cy=0.5, r=0.5, fx=None, fy=None, gid=None):
    gid = gid or _new_id("rg")
    fx = cx if fx is None else fx
    fy = cy if fy is None else fy
    s = "".join(
        f'<stop offset="{o}" stop-color="{c[:7]}"'
        + (f' stop-opacity="{int(c[7:9], 16) / 255:.3f}"' if len(c) == 9 else "")
        + "/>"
        for o, c in stops
    )
    return (f'<radialGradient id="{gid}" cx="{cx}" cy="{cy}" r="{r}" fx="{fx}" fy="{fy}">{s}</radialGradient>',
            f"url(#{gid})")


def svg_doc(w, h, body, defs=""):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
            f'width="{w}" height="{h}" viewBox="0 0 {w} {h}"><defs>{defs}</defs>{body}</svg>')


def rrect_path(x, y, w, h, tl=0, tr=0, br=0, bl=0):
    """Köşeleri ayrı ayrı yuvarlatılmış dikdörtgen yolu."""
    return (f"M{x + tl},{y} H{x + w - tr} "
            + (f"A{tr},{tr} 0 0 1 {x + w},{y + tr} " if tr else "")
            + f"V{y + h - br} "
            + (f"A{br},{br} 0 0 1 {x + w - br},{y + h} " if br else "")
            + f"H{x + bl} "
            + (f"A{bl},{bl} 0 0 1 {x},{y + h - bl} " if bl else "")
            + f"V{y + tl} "
            + (f"A{tl},{tl} 0 0 1 {x + tl},{y} " if tl else "")
            + "Z")


def render(svg: str, scale: float = 1.0) -> Image.Image:
    """SVG metnini RGBA PIL görüntüsüne çevirir (rsvg-convert)."""
    out = subprocess.run(["rsvg-convert", "-z", str(scale), "-f", "png"],
                         input=svg.encode(), capture_output=True, check=True).stdout
    return Image.open(io.BytesIO(out)).convert("RGBA")


def png_b64(img: Image.Image) -> str:
    buf = io.BytesIO()
    img.save(buf, "PNG", optimize=True)
    return base64.b64encode(buf.getvalue()).decode()


# ------------------------------------------------------------ 9 dilim ---
PARTS = ["topleft", "top", "topright", "left", "center", "right",
         "bottomleft", "bottom", "bottomright"]


def nine_slice(img: Image.Image, l, t, r, b, scale=1):
    """Görüntüyü (piksel cinsinden, ölçek uygulanmış) 9 parçaya böler.
    Dönüş: {parça: (PIL görüntü, mantıksal genişlik, mantıksal yükseklik)}"""
    W, H = img.size
    L, T, R, B = (int(round(v * scale)) for v in (l, t, r, b))
    xs = [0, L, W - R, W]
    ys = [0, T, H - B, H]
    out = {}
    i = 0
    for yi in range(3):
        for xi in range(3):
            name = PARTS[i]
            i += 1
            x0, x1, y0, y1 = xs[xi], xs[xi + 1], ys[yi], ys[yi + 1]
            if x1 <= x0 or y1 <= y0:
                continue
            crop = img.crop((x0, y0, x1, y1))
            out[name] = (crop, (x1 - x0) / scale, (y1 - y0) / scale)
    return out


# ------------------------------------------------------------ SVG sayfası ---
@dataclass
class Sheet:
    """Kimlikli raster/vektör elemanları yan yana dizen basit SVG sayfası."""
    scale: float = 1.0
    width: float = 1200
    items: list = field(default_factory=list)
    _x: float = 0
    _y: float = 0
    _row_h: float = 0
    gap: float = 4

    def _place(self, w, h):
        if self._x + w > self.width and self._x > 0:
            self._x = 0
            self._y += self._row_h + self.gap
            self._row_h = 0
        pos = (self._x, self._y)
        self._x += w + self.gap
        self._row_h = max(self._row_h, h)
        return pos

    def newline(self):
        self._x = 0
        self._y += self._row_h + self.gap * 2
        self._row_h = 0

    def add_image(self, eid, img: Image.Image, w=None, h=None):
        w = w if w is not None else img.size[0] / self.scale
        h = h if h is not None else img.size[1] / self.scale
        x, y = self._place(w, h)
        self.items.append(
            f'<image id="{eid}" x="{x}" y="{y}" width="{w}" height="{h}" '
            f'preserveAspectRatio="none" xlink:href="data:image/png;base64,{png_b64(img)}"/>')
        return x, y

    def add_rect(self, eid, w, h, fill="#000000", opacity=0.0):
        """Görünmez/ipucu elemanları (hint-*) için düz dikdörtgen."""
        x, y = self._place(w, h)
        self.items.append(f'<rect id="{eid}" x="{x}" y="{y}" width="{w}" height="{h}" '
                          f'fill="{fill}" fill-opacity="{opacity}"/>')

    def add_frame(self, prefix, img, l, t, r, b, sep="-", center_name="center",
                  skip=()):
        """9 dilimi `prefix-topleft` ... `prefix-center` adlarıyla ekler.
        prefix boşsa adlar `topleft`, `center` olur."""
        parts = nine_slice(img, l, t, r, b, self.scale)
        x0 = None
        for name in PARTS:
            if name not in parts or name in skip:
                continue
            crop, w, h = parts[name]
            n = center_name if name == "center" else name
            eid = f"{prefix}{sep}{n}" if prefix else n
            if eid.endswith(sep) and prefix:
                eid = prefix
            self.add_image(eid, crop, w, h)
        self.newline()

    def add_kv_frame(self, el, state, img, l, t, r, b):
        """Kvantum adlandırması: el-state-top ..., iç kısım: el-state"""
        parts = nine_slice(img, l, t, r, b, self.scale)
        for name in PARTS:
            if name not in parts:
                continue
            crop, w, h = parts[name]
            eid = f"{el}-{state}" if name == "center" else f"{el}-{state}-{name}"
            self.add_image(eid, crop, w, h)
        self.newline()

    def svg(self):
        H = self._y + self._row_h + self.gap
        return svg_doc(self.width, H, "\n".join(self.items))


def draw(w, h, body, defs="", scale=1.0) -> Image.Image:
    return render(svg_doc(w, h, body, defs), scale)
