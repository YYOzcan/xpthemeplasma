#!/usr/bin/env python3
"""XP oturumuna özel çeviri katmanı: KDE'nin Türkçe kataloglarındaki birkaç menü metnini XP'nin
terimleriyle değiştirip <çıktı>/tr/LC_MESSAGES altına yazar.

    python3 tools/xp_ceviri.py <çıktı: ~/.local/share/xpthemeplasma/data/locale>

Katman yalnızca XP oturumunun XDG_DATA_DIRS'inde öndedir; ATP ve diğer oturumlar sistemdeki asıl
çevirileri görür. Örnek: masaüstüne sağ tıklama "Masaüstü ve Duvar Kâğıdı" → "Özellikler".
"""
import struct
import sys
from pathlib import Path

SISTEM = Path("/usr/share/locale/tr/LC_MESSAGES")

# katalog -> [(bağlam ya da None = hepsi, msgid, XP metni)]
DEGISIKLIK = {
    "plasma_applet_org.kde.desktopcontainment": [
        ("@action:inmenu opens config dialog", "Desktop and Wallpaper", "Ö&zellikler"),
        (None, "&Refresh Desktop", "Y&enile"),
        ("@item:inmenu opens submenu with view options like sorting", "Icons", "Simgeleri &Yerleştir"),
        (None, "&Empty Trash", "Geri Dönüşüm Kutusunu &Boşalt"),
        (None, "&Move to Trash", "&Sil"),
        (None, "&Properties", "Ö&zellikler"),
    ],
    "kio6": [
        (None, "Create New", "Ye&ni"),
    ],
}


def mo_oku(veri: bytes):
    sihir = struct.unpack_from("<I", veri, 0)[0]
    son = "<" if sihir == 0x950412DE else ">"
    _, n, ko, ct = struct.unpack_from(son + "4I", veri, 4)
    kayit = []
    for i in range(n):
        ol, oo = struct.unpack_from(son + "2I", veri, ko + 8 * i)
        tl, to = struct.unpack_from(son + "2I", veri, ct + 8 * i)
        kayit.append((veri[oo:oo + ol], veri[to:to + tl]))
    return kayit


def mo_yaz(kayit):
    kayit = sorted(kayit, key=lambda k: k[0])
    n = len(kayit)
    ko, ct = 28, 28 + 8 * n
    veri_bas = 28 + 16 * n
    tablo_o, tablo_t, govde = b"", b"", b""
    for asil, _ in kayit:
        tablo_o += struct.pack("<2I", len(asil), veri_bas + len(govde))
        govde += asil + b"\0"
    for _, cev in kayit:
        tablo_t += struct.pack("<2I", len(cev), veri_bas + len(govde))
        govde += cev + b"\0"
    return struct.pack("<7I", 0x950412DE, 0, n, ko, ct, 0, 0) + tablo_o + tablo_t + govde


def main(cikti):
    hedef = Path(cikti) / "tr/LC_MESSAGES"
    hedef.mkdir(parents=True, exist_ok=True)
    for katalog, liste in DEGISIKLIK.items():
        kaynak = SISTEM / f"{katalog}.mo"
        if not kaynak.exists():
            continue
        kayit = mo_oku(kaynak.read_bytes())
        sayi = 0
        yeni = []
        for asil, cev in kayit:
            baglam, _, kimlik = asil.rpartition(b"\x04")
            for b, m, xp in liste:
                if kimlik == m.encode() and (b is None or baglam == b.encode()) and b"\0" not in kimlik:
                    cev = xp.encode()
                    sayi += 1
            yeni.append((asil, cev))
        (hedef / f"{katalog}.mo").write_bytes(mo_yaz(yeni))
        print(f"  {katalog}: {sayi} metin XP'ye çevrildi")


if __name__ == "__main__":
    main(sys.argv[1])
