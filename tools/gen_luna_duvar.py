#!/usr/bin/env python3
"""XP duvar kağıtlarını Plasma paketlerine çevirir.

    python3 tools/gen_luna_duvar.py <xp-dizini> <çıktı (wallpapers) dizini>

Varsayılan paket "XPThemePlasma" Bliss olur; diğerleri "XP-<Ad>" adıyla seçilebilir.
"""
import json
import shutil
import sys
from pathlib import Path

from PIL import Image

ADLAR = {"bliss": "Bliss", "ascent": "Ascent", "autumn": "Autumn", "azul": "Azul", "crystal": "Crystal",
         "follow": "Follow", "friend": "Friend", "home": "Home", "moon": "Moon flower", "peace": "Peace",
         "power": "Power", "purplef": "Purple flower", "radiance": "Radiance", "redmoon": "Red moon desert",
         "ripple": "Ripple", "stoneh": "Stonehenge", "tulips": "Tulips", "vortec": "Vortec space",
         "wind": "Wind", "winxp": "Windows XP"}


def paket(src: Path, dst: Path, pid, name):
    shutil.rmtree(dst, ignore_errors=True)
    img = Image.open(src)
    (dst / "contents/images").mkdir(parents=True)
    img.save(dst / f"contents/images/{img.size[0]}x{img.size[1]}.jpg", quality=95)
    img.resize((400, 300)).save(dst / "contents/screenshot.png")
    (dst / "metadata.json").write_text(json.dumps({"KPlugin": {
        "Id": pid, "Name": name, "Description": "Windows XP duvar kağıdı (kendi XP kopyandan)",
        "Authors": [{"Name": "Microsoft (XP kopyandan içe aktarıldı)"}]}}, indent=2, ensure_ascii=False))


def main(xp, out):
    xp, out = Path(xp), Path(out)
    for stem, name in ADLAR.items():
        src = xp / "duvar" / f"{stem}.jpg"
        if src.exists():
            paket(src, out / f"XP-{name.replace(' ', '')}", f"XP-{name.replace(' ', '')}", name)
    if (xp / "duvar/bliss.jpg").exists():
        paket(xp / "duvar/bliss.jpg", out / "XPThemePlasma", "XPThemePlasma", "Bliss")
    print("duvar kağıtları ->", out)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
