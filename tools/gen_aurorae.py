#!/usr/bin/env python3
"""XP Luna Aurorae pencere dekorasyonunu üretir.

Çıktı: <out>/XP-Luna/{decoration,close,minimize,maximize,restore,help}.svg,
XP-Lunarc, metadata.desktop, metadata.json
"""
import json
import sys
from pathlib import Path

from PIL import Image, ImageEnhance

sys.path.insert(0, str(Path(__file__).parent))
from luna import (CAPTION_ACTIVE, CAPTION_INACTIVE, Sheet, draw, lingrad,  # noqa: E402
                  radgrad, rrect_path)

TOP = 29          # başlık + üst kenar
TOP_MAX = 25      # büyütülmüş pencerede başlık
BORDER = 4        # görünen kenar kalınlığı
SLICE = 8         # sol/sağ dilim (yuvarlak köşe için 8, iç 4 piksel istemci altında kalır)
BTN = 21
SCALE = 2         # 2x çizip gömüyoruz: HiDPI'da da keskin

THEME = "XP-Luna"


def frame_colors(active):
    if active:
        left = ["#0831D9", "#166AEE", "#0855DD", "#0855DD"]
        right = ["#00138C", "#001EA0", "#003BDA", "#0855DD"]
        return left, right, CAPTION_ACTIVE, ("#0831D9", "#001EA0")
    left = ["#7590DE", "#A0B6EC", "#8DA5E5", "#8DA5E5"]
    right = ["#5A72C4", "#6A84D2", "#7E98DF", "#8DA5E5"]
    return left, right, CAPTION_INACTIVE, ("#7590DE", "#6A84D2")


def decoration(active, maximized=False):
    W = 64
    top = TOP_MAX if maximized else TOP
    H = top + 16 + BORDER
    left, right, cap, edges = frame_colors(active)
    defs, grad = lingrad(cap, 0, 0, 0, 1)
    body = []
    r = 0 if maximized else 8
    # başlık
    body.append(f'<path d="{rrect_path(0, 0, W, top + 0.01, r, r, 0, 0)}" fill="{grad}"/>')
    if not maximized:
        # başlığın yan çizgileri (köşe yayını izleyen ince kenar)
        body.append(f'<path d="M0.5,{top} V8.5 A8,8 0 0 1 8.5,0.5" fill="none" '
                    f'stroke="{edges[0]}" stroke-opacity="0.55" stroke-width="1"/>')
        body.append(f'<path d="M{W - 0.5},{top} V8.5 A8,8 0 0 0 {W - 8.5},0.5" fill="none" '
                    f'stroke="{edges[1]}" stroke-opacity="0.65" stroke-width="1"/>')
    # yan ve alt kenarlar
    for i, c in enumerate(left):
        body.append(f'<rect x="{i}" y="{top}" width="1" height="{H - top}" fill="{c}"/>')
    for i, c in enumerate(right):
        body.append(f'<rect x="{W - 1 - i}" y="{top}" width="1" height="{H - top}" fill="{c}"/>')
    # iç kısım (istemci altında kalır) — düz renk
    body.append(f'<rect x="{BORDER}" y="{top}" width="{W - 2 * BORDER}" height="{H - top - BORDER}" '
                f'fill="{left[-1]}"/>')
    for i, c in enumerate(right):  # alt kenar: dıştan içe
        y = H - 1 - i
        body.append(f'<rect x="{i}" y="{y}" width="{W - 2 * i}" height="1" fill="{c}"/>')
    img = draw(W, H, "".join(body), defs, SCALE)
    return img, (SLICE, top, SLICE, BORDER)


# ------------------------------------------------------------- düğmeler ---
def glyph(kind):
    w = "#FFFFFF"
    if kind == "minimize":
        return f'<rect x="5" y="13" width="7" height="3" fill="{w}"/>'
    if kind == "maximize":
        return (f'<path fill-rule="evenodd" fill="{w}" '
                f'd="M5,4 h11 v12 h-11 z M6,7 h9 v8 h-9 z"/>')
    if kind == "restore":
        return (f'<path fill-rule="evenodd" fill="{w}" d="M4,8 h9 v9 h-9 z M5,10.5 h7 v5.5 h-7 z"/>'
                f'<path fill="{w}" d="M8,4 h9 v2.5 h-9 z M16,6.5 h1 v6.5 h-1 z M8,6.5 h1 v1.5 h-1 z M13,12 h3 v1 h-3 z"/>')
    if kind == "close":
        return (f'<path d="M6.5,6.5 L14.5,14.5 M14.5,6.5 L6.5,14.5" stroke="{w}" '
                f'stroke-width="2.2" stroke-linecap="square"/>')
    if kind == "help":
        return (f'<text x="10.5" y="16" text-anchor="middle" font-family="Tahoma" '
                f'font-weight="bold" font-size="14" fill="{w}">?</text>')
    raise ValueError(kind)


def button(kind, glyph_opacity=1.0):
    red = kind == "close"
    if red:
        stops = [(0, "#CC4600"), (0.55, "#DC6527"), (0.70, "#CD7546"), (0.90, "#FFCCB2"), (1, "#FFFFFF")]
        shade = "#DA4600"
    else:
        stops = [(0, "#0054E9"), (0.55, "#2263D5"), (0.70, "#4479E4"), (0.90, "#A3BBEC"), (1, "#FFFFFF")]
        shade = "#4646FF"
    d1, g = radgrad(stops, cx=0.9, cy=0.9, r=1.15)
    d2, sh = lingrad([(0, shade + "00"), (0.6, shade + "00"), (1, shade + "B0")], 0, 0, 0, 1)
    body = (f'<path d="{rrect_path(0.5, 0.5, 20, 20, 3, 3, 3, 3)}" fill="{g}" stroke="#FFFFFF" stroke-width="1"/>'
            f'<path d="{rrect_path(1.5, 1.5, 18, 18, 2, 2, 2, 2)}" fill="none" stroke="{sh}" stroke-width="1.5"/>'
            f'<g opacity="{glyph_opacity}">{glyph(kind)}</g>')
    return draw(BTN, BTN, body, d1 + d2, SCALE)


def brightness(img, f):
    rgb = ImageEnhance.Brightness(img.convert("RGB")).enhance(f)
    rgb.putalpha(img.getchannel("A"))
    return rgb


def fade(img, a):
    out = img.copy()
    out.putalpha(img.getchannel("A").point(lambda v: int(v * a)))
    return out


def button_svg(kind):
    s = Sheet(scale=SCALE, width=200)
    base = button(kind)
    dis = button(kind, glyph_opacity=0.35)
    variants = {
        "active": base,
        "hover": brightness(base, 1.2),
        "pressed": brightness(base, 0.82),
        "deactivated": fade(dis, 0.8),
        "inactive": fade(base, 0.62),
        "hover-inactive": brightness(base, 1.2),
        "pressed-inactive": brightness(base, 0.82),
        "deactivated-inactive": fade(dis, 0.5),
    }
    for name, img in variants.items():
        s.add_image(f"{name}-center", img, BTN, BTN)
        s.newline()
    return s.svg()


def main(out):
    d = Path(out) / THEME
    d.mkdir(parents=True, exist_ok=True)
    s = Sheet(scale=SCALE, width=400)
    for prefix, active, maxi in [("decoration", True, False), ("decoration-inactive", False, False),
                                 ("decoration-maximized", True, True),
                                 ("decoration-maximized-inactive", False, True)]:
        img, (l, t, r, b) = decoration(active, maxi)
        s.add_frame(prefix, img, l, t, r, b)
    (d / "decoration.svg").write_text(s.svg())
    for kind in ["close", "minimize", "maximize", "restore", "help"]:
        (d / f"{kind}.svg").write_text(button_svg(kind))

    (d / f"{THEME}rc").write_text(f"""[General]
ActiveTextColor=255,255,255
InactiveTextColor=216,228,248
UseTextShadow=true
ActiveTextShadowColor=10,24,131
InactiveTextShadowColor=110,135,210
TextShadowOffsetX=1
TextShadowOffsetY=1
TitleAlignment=Left
TitleVerticalAlignment=Center
Animation=0
DecorationPosition=0
Shadow=false
HaloActive=false
HaloInactive=false

[Layout]
BorderLeft={BORDER}
BorderRight={BORDER}
BorderBottom={BORDER}
TitleEdgeTop=5
TitleEdgeBottom=3
TitleEdgeLeft=5
TitleEdgeRight=5
TitleEdgeTopMaximized=2
TitleEdgeBottomMaximized=2
TitleEdgeLeftMaximized=2
TitleEdgeRightMaximized=2
TitleBorderLeft=4
TitleBorderRight=4
TitleHeight={BTN}
ButtonWidth={BTN}
ButtonHeight={BTN}
ButtonSpacing=2
ButtonMarginTop=0
ButtonMarginTopMaximized=0
ExplicitButtonSpacer=8
PaddingTop=0
PaddingBottom=0
PaddingLeft=0
PaddingRight=0
""")
    (d / "metadata.desktop").write_text(f"""[Desktop Entry]
Name=XP Luna
Name[tr]=XP Luna
Comment=Windows XP Luna (Mavi) biçemli pencere çerçevesi
X-KDE-PluginInfo-Name={THEME}
X-KDE-PluginInfo-Author=XPThemePlasma
X-KDE-PluginInfo-Version=1.0
X-KDE-PluginInfo-License=GPL-3.0-or-later
X-KDE-PluginInfo-Category=
""")
    (d / "metadata.json").write_text(json.dumps({
        "KPlugin": {"Id": THEME, "Name": "XP Luna", "Name[tr]": "XP Luna",
                    "Description": "Windows XP Luna (Mavi) biçemli pencere çerçevesi",
                    "License": "GPL-3.0-or-later", "Version": "1.0",
                    "Authors": [{"Name": "XPThemePlasma"}]},
        "KPackageStructure": "KWin/Aurorae"}, indent=2, ensure_ascii=False))
    print("aurorae ->", d)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "build/aurorae")
