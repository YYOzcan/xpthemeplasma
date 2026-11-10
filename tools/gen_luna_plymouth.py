#!/usr/bin/env python3
"""XP açılış ekranından Plymouth teması (betik eklentisi).

    python3 tools/gen_luna_plymouth.py <varlik-dizini> <çıktı: .../xpthemeplasma>

Plymouth'ta kırpma olmadığı için kayan bloklar, kutunun iç alanı kadar önceden
kırpılmış 19 kare olarak üretilir (XP: 8 piksellik adımlar).
"""
import shutil
import sys
from pathlib import Path

from PIL import Image

KUTU = (258, 354, 377, 362)        # 640x480'de ilerleme kutusunun içi


def main(varlik, out):
    v, out = Path(varlik), Path(out)
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
    src = Path(__file__).parent.parent / "plymouth"
    for f in ["xpthemeplasma.plymouth", "xpthemeplasma.script"]:
        shutil.copy2(src / f, out / f)
    print("plymouth ->", out)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
