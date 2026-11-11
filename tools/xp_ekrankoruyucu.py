#!/usr/bin/env python3
"""XP ekran koruyucuları: ad listesi ve logon.scr için yerel kopya düzeltmesi.

    python3 tools/xp_ekrankoruyucu.py <xp/ekrankoruyucu dizini>

Her .scr'nin adı (dize kaynağı 1, ör. "3B Borular") liste.json'a yazılır; Görüntü Özellikleri ve
xp-ekrankoruyucu bunu kullanır.

logon.scr gösterilecek logoyu işletim sistemi sürümüne göre seçer (Professional, Home, Whistler
Server/Advanced Server/Data Center). Wine'da bu denetim sunucu dalına düştüğü için "Whistler Data Center"
şeridi çıkar. Kaynak dizinindeki 200, 202, 203 ve 204 numaralı BITMAP girdileri 201'in
("Windows XP Professional") verisine yönlendirilir; kod ve diğer veriler değişmez.
Yalnızca kendi XP kopyandan içe aktarılan dosyaya uygulanır, depoya girmez.
"""
import struct
import sys
from pathlib import Path


def bolumler(d):
    e = struct.unpack_from("<I", d, 0x3C)[0]
    n = struct.unpack_from("<H", d, e + 6)[0]
    osz = struct.unpack_from("<H", d, e + 20)[0]
    opt = e + 24
    magic = struct.unpack_from("<H", d, opt)[0]
    dd = opt + (96 if magic == 0x10B else 112)
    rva = struct.unpack_from("<I", d, dd + 16)[0]
    out = []
    for i in range(n):
        o = opt + osz + i * 40
        vs, va, rs, rp = struct.unpack_from("<IIII", d, o + 8)
        out.append((va, max(vs, rs), rp))
    for va, s, rp in out:
        if va <= rva < va + s:
            return rva - va + rp
    raise ValueError("kaynak bölümü yok")


def girdiler(d, taban, off):
    named, ids = struct.unpack_from("<HH", d, taban + off + 12)
    for i in range(named + ids):
        ad, ptr = struct.unpack_from("<II", d, taban + off + 16 + i * 8)
        yield ad, ptr


def ad_listesi(dizin):
    import json
    sys.path.insert(0, str(Path(__file__).parent))
    from peres import PEResources
    liste = {}
    for f in sorted(Path(dizin).glob("*.scr")):
        try:
            liste[f.name] = PEResources(f.read_bytes()).strings().get(1, f.stem).rstrip("\x00")
        except Exception:
            liste[f.name] = f.stem
    (Path(dizin) / "liste.json").write_text(json.dumps(liste, ensure_ascii=False, indent=1))
    print(f"ekran koruyucuları: {len(liste)}")


def yama(yol):
    p = Path(yol)
    if not p.exists():
        return
    d = bytearray(p.read_bytes())
    taban = bolumler(d)
    veri = {}                                     # bitmap kimliği -> veri girdisinin dosya konumu
    for tur, tptr in girdiler(d, taban, 0):
        if tur != 2 or not tptr & 0x80000000:     # RT_BITMAP
            continue
        for kimlik, kptr in girdiler(d, taban, tptr & 0x7FFFFFFF):
            if kimlik & 0x80000000 or not kptr & 0x80000000:
                continue
            for _dil, dptr in girdiler(d, taban, kptr & 0x7FFFFFFF):
                if not dptr & 0x80000000:
                    veri[kimlik] = taban + dptr
    if 201 not in veri:
        print("logon.scr: 201 numaralı bitmap yok; değişiklik yapılmadı")
        return
    rva, boy = struct.unpack_from("<II", d, veri[201])
    sayi = 0
    for k in (200, 202, 203, 204):
        if k in veri and struct.unpack_from("<II", d, veri[k]) != (rva, boy):
            struct.pack_into("<II", d, veri[k], rva, boy)
            sayi += 1
    p.write_bytes(bytes(d))
    print(f"logon.scr: {sayi} logo girdisi 'Windows XP Professional'a yönlendirildi")


def onizleme(dizin):
    """Görüntü Özellikleri'ndeki küçük monitör için "Windows XP" logosu (bitmap 201)."""
    sys.path.insert(0, str(Path(__file__).parent))
    from peres import PEResources
    from msstyles import decode_dib
    p = Path(dizin) / "logon.scr"
    if p.exists():
        d = PEResources(p.read_bytes()).get("BITMAP", 201)
        if d:
            decode_dib(d)[0].save(Path(dizin) / "logon-onizleme.png")


def main(dizin):
    yama(Path(dizin) / "logon.scr")
    ad_listesi(dizin)
    onizleme(dizin)


if __name__ == "__main__":
    main(sys.argv[1])
