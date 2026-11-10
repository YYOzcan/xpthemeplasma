#!/usr/bin/env python3
"""XP Luna Plasma masaüstü temasını üretir (plasma/desktoptheme/XP-Luna)."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from luna import (BUTTON_FACE, BUTTON_PRESSED, TASKBAR, Sheet, draw, lingrad,  # noqa: E402
                  rrect_path)

SCALE = 2
NAME = "XP-Luna"


def rr(x, y, w, h, r):
    return rrect_path(x, y, w, h, r, r, r, r)


def im(w, h, body, defs=""):
    return draw(w, h, body, defs, SCALE)


def write(path: Path, sheet: Sheet):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(sheet.svg())


# ------------------------------------------------------------ görev çubuğu ---
def panel_background(out):
    s = Sheet(scale=SCALE, width=300)
    d, g = lingrad(TASKBAR)
    pimg = im(12, 30, f'<rect width="12" height="30" fill="{g}"/>', d)
    for prefix in ["south", "north", ""]:
        s.add_frame(prefix, pimg, 1, 1, 1, 1)
    d2, g2 = lingrad(TASKBAR, 0, 0, 1, 0)
    vimg = im(30, 12, f'<rect width="30" height="12" fill="{g2}"/>', d2)
    for prefix in ["west", "east"]:
        s.add_frame(prefix, vimg, 1, 1, 1, 1)
    # kenar boşluğu ipuçları: uygulamacıklar panelin tüm yüksekliğini kullansın
    for prefix in ["south", "north", "west", "east", ""]:
        p = f"{prefix}-" if prefix else ""
        for side in ["top", "bottom", "left", "right"]:
            s.add_rect(f"{p}hint-{side}-margin", 1, 1)
    s.newline()
    write(out / "widgets/panel-background.svg", s)


def task_button(kind, w=40, h=26):
    """XP görev düğmesi."""
    if kind in ("normal", "minimized"):
        d, g = lingrad([(0, "#5D9BFF"), (0.08, "#4B8DF8"), (0.5, "#3C81F3"), (1, "#3477EA")])
        body = (f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="{g}" stroke="#1C4DB8" stroke-opacity="0.85"/>'
                f'<path d="M2,{h - 3} V3 Q2,1.5 3.5,1.5 H{w - 3}" fill="none" stroke="#FFFFFF" stroke-opacity="0.35"/>')
    elif kind == "hover":
        d, g = lingrad([(0, "#77B3FF"), (0.08, "#66ACFF"), (0.5, "#53A3FF"), (1, "#4794F5")])
        body = (f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="{g}" stroke="#2A63CF" stroke-opacity="0.85"/>'
                f'<path d="M2,{h - 3} V3 Q2,1.5 3.5,1.5 H{w - 3}" fill="none" stroke="#FFFFFF" stroke-opacity="0.45"/>')
    elif kind == "focus":
        d, g = lingrad([(0, "#1A47A6"), (0.15, "#1E52B7"), (1, "#2259C4")])
        body = (f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="{g}" stroke="#10307A"/>'
                f'<path d="M1.5,{h - 2} V3 Q1.5,1.5 3,1.5 H{w - 2}" fill="none" stroke="#000000" stroke-opacity="0.45" stroke-width="1.2"/>')
    elif kind == "attention":
        d, g = lingrad([(0, "#FFC56E"), (0.1, "#FBA93F"), (0.6, "#F28A1B"), (1, "#E07B12")])
        body = (f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="{g}" stroke="#B85F08"/>'
                f'<path d="M2,{h - 3} V3 Q2,1.5 3.5,1.5 H{w - 3}" fill="none" stroke="#FFFFFF" stroke-opacity="0.4"/>')
    else:
        raise ValueError(kind)
    return im(w, h, body, d)


def tasks(out):
    s = Sheet(scale=SCALE, width=400)
    for kind in ["normal", "minimized", "hover", "focus", "attention"]:
        img = task_button(kind)
        s.add_frame(kind, img, 4, 4, 4, 4)
        for side in ["south", "north", "west", "east"]:
            s.add_frame(f"{side}-{kind}", img, 4, 4, 4, 4)
    write(out / "widgets/tasks.svg", s)


# ------------------------------------------------------------ açılır pencere ---
def dialog_bg(w=40, h=40):
    body = (f'<rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="#FFFFFF" stroke="#7A96DF"/>'
            f'<rect x="1.5" y="1.5" width="{w - 3}" height="{h - 3}" fill="none" stroke="#FFFFFF"/>')
    return im(w, h, body)


def shadow(w=40, h=40, depth=4):
    """Sağ/alt yumuşak XP gölgesi (Plasma dialog shadow-* öğeleri)."""
    body = ""
    for i in range(depth):
        a = 0.22 * (1 - i / depth)
        body += (f'<rect x="{depth + i}" y="{h - depth + i}" width="{w - depth * 2}" height="1" fill="#000" fill-opacity="{a:.3f}"/>'
                 f'<rect x="{w - depth + i}" y="{depth + i}" width="1" height="{h - depth * 2 + 1}" fill="#000" fill-opacity="{a:.3f}"/>')
    return im(w, h, body)


def dialogs(out):
    s = Sheet(scale=SCALE, width=400)
    s.add_frame("", dialog_bg(), 4, 4, 4, 4)
    s.add_frame("shadow", shadow(), 4, 4, 4, 4)
    for side in ["top", "bottom", "left", "right"]:
        s.add_rect(f"hint-{side}-margin", 6, 6)
    s.newline()
    write(out / "dialogs/background.svg", s)
    write(out / "solid/dialogs/background.svg", s)


def tooltip(out):
    s = Sheet(scale=SCALE, width=300)
    img = im(24, 24, '<rect x="0.5" y="0.5" width="23" height="23" fill="#FFFFE1" stroke="#000000"/>')
    s.add_frame("", img, 3, 3, 3, 3)
    s.add_frame("shadow", shadow(24, 24, 3), 3, 3, 3, 3)
    write(out / "widgets/tooltip.svg", s)


def viewitem(out):
    s = Sheet(scale=SCALE, width=300)
    hov = im(20, 20, '<rect x="0.5" y="0.5" width="19" height="19" fill="#C1D2EE" stroke="#316AC5"/>')
    sel = im(20, 20, '<rect x="0.5" y="0.5" width="19" height="19" fill="#316AC5" fill-opacity="0.35" stroke="#316AC5"/>')
    s.add_frame("hover", hov, 2, 2, 2, 2)
    s.add_frame("selected", sel, 2, 2, 2, 2)
    s.add_frame("selected+hover", sel, 2, 2, 2, 2)
    s.add_frame("normal", im(20, 20, ""), 2, 2, 2, 2)
    write(out / "widgets/viewitem.svg", s)


def button(out):
    s = Sheet(scale=SCALE, width=300)

    def btn(stops, ring=None):
        d, g = lingrad(stops)
        body = f'<path d="{rr(0.5, 0.5, 29, 22, 3)}" fill="{g}" stroke="#003C74"/>'
        if ring:
            d2, g2 = lingrad(ring)
            d += d2
            body += f'<path d="{rr(1.5, 1.5, 27, 20, 2)}" fill="none" stroke="{g2}" stroke-width="2"/>'
        return im(30, 23, body, d)

    s.add_frame("normal", btn(BUTTON_FACE), 3, 3, 3, 3)
    s.add_frame("pressed", btn(BUTTON_PRESSED), 3, 3, 3, 3)
    s.add_frame("hover", btn(BUTTON_FACE, [(0, "#FFF0CF"), (0.15, "#FDD889"), (0.5, "#FBC761"), (1, "#E5A01A")]), 3, 3, 3, 3)
    s.add_frame("focus", btn(BUTTON_FACE, [(0, "#CEE7FF"), (0.15, "#BCD4F6"), (0.5, "#98B8EA"), (1, "#89ADE4")]), 3, 3, 3, 3)
    write(out / "widgets/button.svg", s)


def background(out):
    """Masaüstü üzerindeki uygulamacıklar için arka plan."""
    s = Sheet(scale=SCALE, width=300)
    img = im(30, 30, f'<path d="{rr(0.5, 0.5, 29, 29, 4)}" fill="#FFFFFF" fill-opacity="0.92" stroke="#7A96DF"/>')
    s.add_frame("", img, 5, 5, 5, 5)
    s.add_frame("shadow", shadow(30, 30, 4), 4, 4, 4, 4)
    write(out / "widgets/background.svg", s)
    write(out / "widgets/translucentbackground.svg", s)


COLORS = """[Colors:Window]
BackgroundNormal=255,255,255
BackgroundAlternate=236,233,216
ForegroundNormal=0,0,0
ForegroundInactive=113,111,100
ForegroundActive=49,106,197
ForegroundLink=0,102,204
ForegroundVisited=128,0,128
ForegroundNegative=192,0,0
ForegroundNeutral=176,116,0
ForegroundPositive=33,161,33
DecorationFocus=49,106,197
DecorationHover=229,160,26

[Colors:View]
BackgroundNormal=255,255,255
BackgroundAlternate=247,247,242
ForegroundNormal=0,0,0
ForegroundInactive=113,111,100
ForegroundActive=49,106,197
ForegroundLink=0,102,204
ForegroundVisited=128,0,128
ForegroundNegative=192,0,0
ForegroundNeutral=176,116,0
ForegroundPositive=33,161,33
DecorationFocus=49,106,197
DecorationHover=229,160,26

[Colors:Button]
BackgroundNormal=236,233,216
BackgroundAlternate=241,239,226
ForegroundNormal=0,0,0
ForegroundInactive=113,111,100
ForegroundActive=49,106,197
ForegroundLink=0,102,204
ForegroundVisited=128,0,128
ForegroundNegative=192,0,0
ForegroundNeutral=176,116,0
ForegroundPositive=33,161,33
DecorationFocus=49,106,197
DecorationHover=229,160,26

[Colors:Selection]
BackgroundNormal=49,106,197
BackgroundAlternate=60,129,243
ForegroundNormal=255,255,255
ForegroundInactive=216,228,248
ForegroundActive=255,255,255
ForegroundLink=255,255,255
ForegroundVisited=255,255,255
ForegroundNegative=255,200,200
ForegroundNeutral=255,230,160
ForegroundPositive=200,255,200
DecorationFocus=49,106,197
DecorationHover=229,160,26

[Colors:Tooltip]
BackgroundNormal=255,255,225
BackgroundAlternate=255,255,225
ForegroundNormal=0,0,0
ForegroundInactive=113,111,100
ForegroundActive=49,106,197
ForegroundLink=0,102,204
ForegroundVisited=128,0,128
ForegroundNegative=192,0,0
ForegroundNeutral=176,116,0
ForegroundPositive=33,161,33
DecorationFocus=49,106,197
DecorationHover=229,160,26

[Colors:Complementary]
BackgroundNormal=36,93,219
BackgroundAlternate=13,141,234
ForegroundNormal=255,255,255
ForegroundInactive=216,228,248
ForegroundActive=255,255,255
ForegroundLink=255,255,255
ForegroundVisited=230,230,255
ForegroundNegative=255,170,160
ForegroundNeutral=255,220,120
ForegroundPositive=160,255,160
DecorationFocus=255,255,255
DecorationHover=255,255,255

[Colors:Header]
BackgroundNormal=236,233,216
BackgroundAlternate=241,239,226
ForegroundNormal=0,0,0
ForegroundInactive=113,111,100
ForegroundActive=49,106,197
ForegroundLink=0,102,204
ForegroundVisited=128,0,128
ForegroundNegative=192,0,0
ForegroundNeutral=176,116,0
ForegroundPositive=33,161,33
DecorationFocus=49,106,197
DecorationHover=229,160,26
"""


def main(out):
    d = Path(out) / NAME
    d.mkdir(parents=True, exist_ok=True)
    panel_background(d)
    tasks(d)
    dialogs(d)
    tooltip(d)
    viewitem(d)
    button(d)
    background(d)
    (d / "colors").write_text(COLORS)
    (d / "metadata.json").write_text(json.dumps({
        "KPackageStructure": "Plasma/Theme",
        "KPlugin": {
            "Id": NAME, "Name": "XP Luna", "Name[tr]": "XP Luna",
            "Description": "Windows XP Luna (Mavi) biçemli Plasma teması",
            "Description[tr]": "Windows XP Luna (Mavi) biçemli Plasma teması",
            "License": "GPL-3.0-or-later", "Version": "1.0",
            "Authors": [{"Name": "XPThemePlasma"}],
            "Website": "https://github.com/YYOzcan/xpthemeplasma",
        },
        "X-Plasma-API": "5.0",
        "X-Plasma-Theme-Fallback": "default",
    }, indent=2, ensure_ascii=False))
    (d / "plasmarc").write_text("[ContrastEffect]\nenabled=false\n[BlurBehindEffect]\nenabled=false\n[AdaptiveTransparency]\nenabled=false\n")
    print("desktoptheme ->", d)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "build/desktoptheme")
