#!/usr/bin/env python3
"""Gerçek luna.msstyles bitmap'lerinden XP-Luna Aurorae teması.

    python3 tools/gen_luna_aurorae.py <luna.msstyles> <çıktı dizini>
"""
import json
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from luna import Sheet  # noqa: E402
from msstyles import Msstyles  # noqa: E402

THEME = "XP-Luna"
B = 4            # görünen kenar kalınlığı (XP: 4)


def stretch9(img, W, H, l, r, t, b):
    """9 dilimli germe (uxtheme SizingType=stretch)."""
    w, h = img.size
    out = Image.new("RGBA", (W, H))
    xs = [(0, l, 0, l), (l, w - r, l, W - r), (w - r, w, W - r, W)]
    ys = [(0, t, 0, t), (t, h - b, t, H - b), (h - b, h, H - b, H)]
    for sx0, sx1, dx0, dx1 in xs:
        for sy0, sy1, dy0, dy1 in ys:
            if sx1 <= sx0 or sy1 <= sy0 or dx1 <= dx0 or dy1 <= dy0:
                continue
            part = img.crop((sx0, sy0, sx1, sy1)).resize((dx1 - dx0, dy1 - dy0), Image.NEAREST)
            out.alpha_composite(part, (dx0, dy0))
    return out


def thin(img, axis):
    """5 piksellik kenarı 4'e indir: ortadaki (gerilebilir) sütunu/satırı at."""
    w, h = img.size
    if axis == "x" and w == 5:
        return _drop(img, 2, "x")
    if axis == "y" and h == 5:
        return _drop(img, 2, "y")
    return img


def _drop(img, i, axis):
    w, h = img.size
    if axis == "x":
        out = Image.new("RGBA", (w - 1, h))
        out.paste(img.crop((0, 0, i, h)), (0, 0))
        out.paste(img.crop((i + 1, 0, w, h)), (i, 0))
    else:
        out = Image.new("RGBA", (w, h - 1))
        out.paste(img.crop((0, 0, w, i)), (0, 0))
        out.paste(img.crop((0, i + 1, w, h)), (0, i))
    return out


def decoration(ms, state, maximized=False):
    """state 0 = etkin, 1 = etkin değil. Döner: (görüntü, l, t, r, b)."""
    cap_cls = "Window.MaxCaption" if maximized else "Window.Caption"
    cap = ms.images(cap_cls)[state]
    cl, cr, ct, cb = ms.margins(cap_cls)
    top = cap.size[1]                      # 29
    if maximized:
        top = 25
    left = thin(ms.images("Window.FrameLeft")[state], "x")
    right = thin(ms.images("Window.FrameRight")[state], "x")
    bottom = thin(ms.images("Window.FrameBottom")[state], "y")
    W, H = 120, top + 24 + B
    L, R = cl, cr                          # köşe genişlikleri (28, 35)
    img = Image.new("RGBA", (W, H))
    if maximized:
        # büyütülmüş pencere: yuvarlak köşe yok; başlığın orta kısmını kullan
        mid = cap.crop((cl, 0, cap.size[0] - cr, cap.size[1]))
        strip = stretch9(cap, W + 2 * 6, cap.size[1], cl, cr, ct, cb).crop((6, 4, W + 6, 4 + top))
        img.alpha_composite(strip, (0, 0))
        fill = mid.getpixel((0, cap.size[1] - 2))
        img.paste(Image.new("RGBA", (W, H - top), fill), (0, top))
        return img, 1, top, 1, 1
    img.alpha_composite(stretch9(cap, W, top, cl, cr, ct, cb), (0, 0))
    # iç dolgu (istemci altında kalır)
    fill = left.getpixel((left.size[0] - 1, 5))
    img.paste(Image.new("RGBA", (W - 2 * B, H - top - B), fill), (B, top))
    img.alpha_composite(left.resize((B, H - top - B), Image.NEAREST), (0, top))
    img.alpha_composite(right.resize((B, H - top - B), Image.NEAREST), (W - B, top))
    img.alpha_composite(stretch9(bottom, W, B, 5, 5, 0, 0), (0, H - B))
    return img, L, top, R, B


def button_images(ms, cls, glyph_cls=None):
    """8 durum: etkin {normal, hot, pressed, disabled}, etkin değil {aynı 4}."""
    bgs = ms.images(cls)
    glyph_file = ms.prop(cls, "ImageFile3") or ms.prop(cls, "GlyphImageFile")
    tc = (255, 0, 255) if ms.prop(cls, "GlyphTransparent", "false").lower() == "true" else None
    g = ms.bitmap(glyph_file, tc)
    n = len(bgs)
    gh = g.size[1] // n
    glyphs = [g.crop((0, i * gh, g.size[0], (i + 1) * gh)) for i in range(n)]
    out = []
    for bg, gl in zip(bgs, glyphs):
        im = bg.copy()
        x = (im.size[0] - gl.size[0]) // 2
        y = (im.size[1] - gl.size[1]) // 2
        im.alpha_composite(gl, (x, y))
        out.append(im)
    return out


def button_svg(imgs):
    s = Sheet(scale=1, width=200)
    names = ["active", "hover", "pressed", "deactivated",
             "inactive", "hover-inactive", "pressed-inactive", "deactivated-inactive"]
    for name, im in zip(names, imgs):
        s.add_image(f"{name}-center", im, im.size[0], im.size[1])
        s.newline()
    return s.svg()


def main(msstyles, out):
    ms = Msstyles(msstyles)
    d = Path(out) / THEME
    d.mkdir(parents=True, exist_ok=True)
    s = Sheet(scale=1, width=600)
    for prefix, state, maxi in [("decoration", 0, False), ("decoration-inactive", 1, False),
                                ("decoration-maximized", 0, True), ("decoration-maximized-inactive", 1, True)]:
        img, l, t, r, b = decoration(ms, state, maxi)
        s.add_frame(prefix, img, l, t, r, b)
    (d / "decoration.svg").write_text(s.svg())

    for kind, cls in [("close", "Window.CloseButton"), ("minimize", "Window.MinButton"),
                      ("maximize", "Window.MaxButton"), ("restore", "Window.RestoreButton"),
                      ("help", "Window.HelpButton")]:
        (d / f"{kind}.svg").write_text(button_svg(button_images(ms, cls)))

    act = ms.color("Window.Caption", "TextShadowColor", (10, 24, 131))
    (d / f"{THEME}rc").write_text(f"""[General]
ActiveTextColor=255,255,255
InactiveTextColor=216,228,248
UseTextShadow=true
ActiveTextShadowColor={act[0]},{act[1]},{act[2]}
InactiveTextShadowColor=0,0,0,0
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
BorderLeft={B}
BorderRight={B}
BorderBottom={B}
TitleEdgeTop=5
TitleEdgeBottom=3
TitleEdgeLeft=5
TitleEdgeRight=4
TitleEdgeTopMaximized=2
TitleEdgeBottomMaximized=2
TitleEdgeLeftMaximized=2
TitleEdgeRightMaximized=2
TitleBorderLeft=5
TitleBorderRight=4
TitleHeight=21
ButtonWidth=21
ButtonHeight=21
ButtonSpacing=2
ButtonMarginTop=0
ButtonMarginTopMaximized=0
ExplicitButtonSpacer=6
PaddingTop=0
PaddingBottom=0
PaddingLeft=0
PaddingRight=0
""")
    (d / "metadata.desktop").write_text(f"""[Desktop Entry]
Name=XP Luna
Comment=Windows XP Luna (luna.msstyles'tan)
X-KDE-PluginInfo-Name={THEME}
X-KDE-PluginInfo-Author=XPThemePlasma
X-KDE-PluginInfo-Version=2.0
X-KDE-PluginInfo-License=GPL-3.0-or-later
""")
    (d / "metadata.json").write_text(json.dumps({
        "KPlugin": {"Id": THEME, "Name": "XP Luna", "Version": "2.0",
                    "Description": "Windows XP Luna (luna.msstyles'tan)",
                    "Authors": [{"Name": "XPThemePlasma"}]},
        "KPackageStructure": "KWin/Aurorae"}, indent=2, ensure_ascii=False))
    print("aurorae (gerçek Luna) ->", d)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "build/aurorae")
