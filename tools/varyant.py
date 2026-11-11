"""XP renk düzeni seçimi: XP_VARYANT ortam değişkeni ("" = Varsayılan (mavi), "zeytin" = Zeytin Yeşili,
"gumus" = Gümüş). Üreteçler luna.msstyles'ın ilgili INI'sini (NormalBlue / NormalHomeStead /
NormalMetallic) okur, tema adlarına ek koyar ve çizim tabanındaki mavi renkleri o düzenin
[SysMetrics] renklerine çevirir."""
import os
import re

VARYANT = os.environ.get("XP_VARYANT", "")
MS = {"": "NORMALBLUE", "zeytin": "NORMALHOMESTEAD", "gumus": "NORMALMETALLIC"}[VARYANT]
EK = {"": "", "zeytin": "Zeytin", "gumus": "Gumus"}[VARYANT]
AD = {"": "", "zeytin": " (Zeytin Yeşili)", "gumus": " (Gümüş)"}[VARYANT]
WINE_RENK = {"": "NormalColor", "zeytin": "HomeStead", "gumus": "Metallic"}[VARYANT]


def ad(taban, ayrac=""):
    """XP-Luna -> XP-Luna-Zeytin; XPLuna -> XPLunaZeytin"""
    return taban + (ayrac + EK if EK else "")


ANAHTARLAR = ["highlight", "menuhilight", "activecaption", "gradientactivecaption", "inactivecaption",
              "gradientinactivecaption", "hottracking", "btnface", "menubar", "background", "highlighttext"]


def tablo(ms_yolu):
    """mavi RGB -> bu düzenin RGB'si"""
    from msstyles import Msstyles
    if not VARYANT:
        return {}
    mavi = Msstyles(ms_yolu, "NORMALBLUE").ini.get("sysmetrics", {})
    bu = Msstyles(ms_yolu, MS).ini.get("sysmetrics", {})
    out = {}
    for k in ANAHTARLAR:
        if k in mavi and k in bu and mavi[k] != bu[k]:
            a = tuple(int(x) for x in mavi[k].split())
            b = tuple(int(x) for x in bu[k].split())
            if a not in out:
                out[a] = b
    out.pop((255, 255, 255), None)          # beyaz yazı/zemin olduğu gibi kalsın
    return out


def cevir(metin, t):
    """Metindeki #rrggbb ve r,g,b biçimli renkleri tabloya göre değiştir."""
    if not t:
        return metin
    for (r, g, b), (r2, g2, b2) in t.items():
        metin = re.sub(f"#{r:02x}{g:02x}{b:02x}", f"#{r2:02x}{g2:02x}{b2:02x}", metin, flags=re.I)
        metin = re.sub(rf"(?<![\d.]){r}, ?{g}, ?{b}(?![\d.])", f"{r2},{g2},{b2}", metin)
    return metin


def dizini_cevir(kok, t, uzantilar=(".kvconfig", ".svg", "colors", "rc", ".colors", "metadata.json", ".desktop")):
    """Dizindeki metin dosyalarında renkleri çevir (svgz hariç)."""
    from pathlib import Path
    if not t:
        return
    for p in Path(kok).rglob("*"):
        if p.is_file() and (p.suffix in uzantilar or p.name in uzantilar):
            try:
                s = p.read_text()
            except UnicodeDecodeError:
                continue
            y = cevir(s, t)
            if y != s:
                p.write_text(y)
