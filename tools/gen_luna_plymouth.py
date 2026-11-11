#!/usr/bin/env python3
"""XP açılış ve kapanış ekranlarından Plymouth teması (betik eklentisi).

    python3 tools/gen_luna_plymouth.py <varlik-dizini> <çıktı: .../xpthemeplasma> [xp-dizini]

Açılış: Plymouth'ta kırpma olmadığı için kayan bloklar, kutunun iç alanı kadar önceden
kırpılmış 19 kare olarak üretilir (XP: 8 piksellik adımlar).

Kapanış: logonui.exe düzeni (UIFILE): üst panel rgb(0,48,156) + açık çizgi, alt panel
rgb(57,52,173)→rgb(0,48,156) yatay degrade + turuncu çizgi, orta rgb(90,126,220),
sol üstte parıltı, ortada ayraç; logo ayracın 20 px soluna, durum yazısı (Arial 14 pt,
rgb(239,247,255)) sağa yaslı olarak 40 px soluna. Yazı winlogon.exe'nin "Windows kapatılıyor...".
"""
import os
import shutil
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

KUTU = (258, 354, 377, 362)        # 640x480'de ilerleme kutusunun içi
DURUM = "Windows kapatılıyor..."


def yazi_tipi(xp: Path, ad, boyut):
    for aday in [xp / "font" / ad, Path("/usr/share/fonts/TTF") / ad,
                 Path("/usr/share/fonts/liberation/LiberationSans-Regular.ttf")]:
        if aday.exists():
            return ImageFont.truetype(str(aday), boyut)
    return ImageFont.load_default(boyut)


def kapanis(v: Path, out: Path, xp: Path):
    # üst bant: 800x80, alt kenarda 2 px açık çizgi
    ust = Image.new("RGBA", (800, 80), (0, 48, 156, 255))
    ust.paste(Image.open(v / "logon-topline.png").convert("RGBA").resize((800, 2)), (0, 78))
    ust.save(out / "kapat-ust.png")
    # alt bant: yatay degrade + üstte 3 px turuncu çizgi
    alt = Image.new("RGBA", (800, 88))
    d = ImageDraw.Draw(alt)
    for x in range(800):
        t = x / 799
        c = tuple(round(a + (b - a) * t) for a, b in zip((57, 52, 173), (0, 48, 156)))
        d.line([(x, 0), (x, 87)], fill=c + (255,))
    cizgi = Image.open(v / "logon-bottomline.png").convert("RGBA").resize((800, 3))
    alt.alpha_composite(cizgi, (0, 0))
    alt.save(out / "kapat-alt.png")
    shutil.copy2(v / "logon-glow.png", out / "kapat-parilti.png")
    shutil.copy2(v / "logon-divider.png", out / "kapat-ayrac.png")
    # logo + durum yazısı, 2x
    logo = Image.open(v / "logon-logo.png").convert("RGBA")
    logo = logo.resize((logo.width * 2, logo.height * 2), Image.LANCZOS)
    f = yazi_tipi(xp, "arial.ttf", 37)              # 14 pt @ 96 dpi = 18.7 px, 2x
    tw = round(f.getlength(DURUM))
    gen = max(logo.width + 40, tw + 80)
    yuk = logo.height + 40 + 50
    img = Image.new("RGBA", (gen, yuk), (90, 126, 220, 255))   # orta zeminle aynı, opak: ölçeklemede kenar izi kalmaz
    img.alpha_composite(logo, (gen - 40 - logo.width, 0))
    ImageDraw.Draw(img).text((gen - 80 - tw, logo.height + 40), DURUM, font=f, fill=(239, 247, 255, 255))
    img.save(out / "kapat-durum.png")


def main(varlik, out, xp=None):
    v, out = Path(varlik), Path(out)
    xp = Path(xp) if xp else Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local/share")) / "xpthemeplasma/xp"
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True)
    boot = Image.open(v / "boot.png").convert("RGBA")
    blok = Image.open(v / "boot-blok.png").convert("RGBA")
    boot.save(out / "boot.png")
    x0, y0, x1, y1 = KUTU
    w, h = x1 - x0 + 1, y1 - y0 + 1
    for n in range(19):
        kare = boot.crop((x0, y0, x1 + 1, y1 + 1)).copy()
        kare.alpha_composite(Image.new("RGBA", (w, h), (0, 0, 0, 255)))
        kare.paste(blok, (n * 8 - 22, 0), blok)
        kare.save(out / f"kare-{n:02d}.png")
    kapanis(v, out, xp)
    src = Path(__file__).parent.parent / "plymouth"
    for f in ["xpthemeplasma.plymouth", "xpthemeplasma.script"]:
        shutil.copy2(src / f, out / f)
    print("plymouth ->", out)


if __name__ == "__main__":
    main(*sys.argv[1:4])
