#!/usr/bin/env python3
"""XPLuna renk şemasının XP_VARYANT'a göre (Zeytin Yeşili / Gümüş) sürümünü üretir.

    XP_VARYANT=zeytin python3 tools/gen_luna_renk.py <luna.msstyles> <XPLuna.colors> <çıktı-dizini>

Mavi düzenin [SysMetrics] renkleri seçilen düzeninkilerle değiştirilir; Gümüş'te seçili metin XP'deki
gibi siyahtır (HighlightText 0 0 0).
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import varyant  # noqa: E402
from msstyles import Msstyles  # noqa: E402


def main(ms_yolu, taban, out):
    metin = varyant.cevir(Path(taban).read_text(), varyant.tablo(ms_yolu))
    ad = varyant.ad("XPLuna")
    satirlar, grup = [], None
    sm = Msstyles(ms_yolu, varyant.MS).ini.get("sysmetrics", {})
    ht = ",".join(sm.get("highlighttext", "255 255 255").split())
    for s in metin.splitlines():
        if s.startswith("["):
            grup = s
        elif s.startswith("ColorScheme="):
            s = "ColorScheme=" + ad
        elif s.startswith("Name"):
            s = s.split("=", 1)[0] + "=XP Luna" + varyant.AD
        elif grup == "[Colors:Selection]" and s.split("=", 1)[0] in ("ForegroundNormal", "ForegroundActive"):
            s = s.split("=", 1)[0] + "=" + ht
        satirlar.append(s)
    Path(out).mkdir(parents=True, exist_ok=True)
    (Path(out) / f"{ad}.colors").write_text("\n".join(satirlar) + "\n")
    print("renk şeması ->", Path(out) / f"{ad}.colors")


if __name__ == "__main__":
    main(*sys.argv[1:4])
