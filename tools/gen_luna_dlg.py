#!/usr/bin/env python3
"""XP iletişim kutusu, menü ve dize kaynaklarını JSON'a aktar (XP sistem programları için).

    python3 tools/gen_luna_dlg.py <xp-dizini> <çıktı: .../varlik/dlg>

Kendi XP kopyandaki taskmgr.exe, sysdm.cpl, timedate.cpl, themeui.dll, desk.cpl, shell32.dll…
dosyalarındaki DIALOG/DIALOGEX, MENU/MENUEX ve STRINGTABLE kaynaklarını <modül>.json olarak yazar;
iletişim kutularında "#n" ile anılan simge/bitmap'leri <modül>-ico-<n>.png / <modül>-bmp-<n>.png
olarak çıkarır. apps/ortak/xpdlg.h bu JSON'dan aynı yerleşimi Qt ile kurar (iletişim birimi → piksel).
Çıktı Microsoft'a ait metin ve görseller içerir; yalnızca yerelde üretilir, depoya girmez.
"""
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from peres import PEResources  # noqa: E402
from msstyles import decode_dib  # noqa: E402
import xpikon  # noqa: E402

SINIF = {0x80: "BUTTON", 0x81: "EDIT", 0x82: "STATIC", 0x83: "LISTBOX", 0x84: "SCROLLBAR", 0x85: "COMBOBOX"}

# modül -> (iletişim kutuları: None = hepsi, dizeler: None = hepsi, menüler, ek bitmap/simge)
MODULLER = {
    "taskmgr.exe": dict(dlg=None, str=None, menu=True, bmp=[], ico=None),
    "sysdm.cpl": dict(dlg=None, str=None, menu=False, bmp=None, ico=None),
    "timedate.cpl": dict(dlg=None, str=None, menu=False, bmp=None, ico=None),
    "themeui.dll": dict(dlg=None, str=None, menu=False, bmp=None, ico=None),
    "desk.cpl": dict(dlg=None, str=None, menu=False, bmp=None, ico=None),
    "shell32.dll": dict(dlg=[1003, 29952, 29953], str=list(range(8960, 8976)) + [60, 4097, 4098, 4099, 4100], menu=False,
                        bmp=[], ico=[160]),
    "winver.exe": dict(dlg=None, str=None, menu=False, bmp=[], ico=[]),
    "appwiz.cpl": dict(dlg=[], str=None, menu=False, bmp=[], ico=[]),
    "netshell.dll": dict(dlg=[1802, 1806, 1807], str=None, menu=False, bmp=[], ico=[210, 190, 191, 192, 193, 195, 196]),
}


def wstr(d, o):
    """sz_Or_Ord: 0 = boş, 0xFFFF = sıra numarası, yoksa sıfırla biten UTF-16."""
    w = struct.unpack_from("<H", d, o)[0]
    if w == 0:
        return "", o + 2
    if w == 0xFFFF:
        return struct.unpack_from("<H", d, o + 2)[0], o + 4
    e = o
    while struct.unpack_from("<H", d, e)[0]:
        e += 2
    return d[o:e].decode("utf-16-le"), e + 2


def hizala(o, n=4):
    return (o + n - 1) & ~(n - 1)


def dialog_coz(d):
    ver, imza = struct.unpack_from("<HH", d, 0)
    ex = imza == 0xFFFF
    if ex:
        helpid, exstyle, style, n, x, y, cx, cy = struct.unpack_from("<IIIHhhhh", d, 4)
        o = 26
    else:
        style, exstyle, n, x, y, cx, cy = struct.unpack_from("<IIHhhhh", d, 0)
        o = 18
    menu, o = wstr(d, o)
    sinif, o = wstr(d, o)
    baslik, o = wstr(d, o)
    font = None
    if style & 0x40:                        # DS_SETFONT
        if ex:
            pt, agirlik, italik, cs = struct.unpack_from("<HHBB", d, o)
            o += 6
        else:
            pt, agirlik, italik = struct.unpack_from("<H", d, o)[0], 400, 0
            o += 2
        yuz, o = wstr(d, o)
        font = {"face": yuz, "pt": pt, "weight": agirlik, "italic": bool(italik)}
    kontroller = []
    for _ in range(n):
        o = hizala(o)
        if ex:
            hid, cexs, cst, kx, ky, kcx, kcy, kid = struct.unpack_from("<IIIhhhhi", d, o)
            o += 24
        else:
            cst, cexs, kx, ky, kcx, kcy, kid = struct.unpack_from("<IIhhhhH", d, o)
            o += 18
            if kid == 0xFFFF:
                kid = -1
        c, o = wstr(d, o)
        t, o = wstr(d, o)
        ek = struct.unpack_from("<H", d, o)[0]
        o += 2 + ek
        if isinstance(c, int):
            c = SINIF.get(c, f"#{c}")
        kontroller.append({"class": c, "id": kid, "x": kx, "y": ky, "w": kcx, "h": kcy,
                           "style": cst, "exstyle": cexs,
                           "text": t if isinstance(t, str) else f"#{t}"})
    return {"x": x, "y": y, "w": cx, "h": cy, "style": style, "exstyle": exstyle,
            "title": baslik if isinstance(baslik, str) else "", "font": font,
            "menu": menu if isinstance(menu, int) else None, "controls": kontroller}


def menu_coz(d):
    ver, boyut = struct.unpack_from("<HH", d, 0)
    if ver == 1:                            # MENUEX
        o = 4 + boyut

        def ex_oge(o):
            ogeler = []
            while True:
                tip, durum, kimlik, bayrak = struct.unpack_from("<IIIH", d, o)
                o += 14
                metin, o = wstr(d, o)
                o = hizala(o)
                oge = {"id": kimlik, "text": metin, "sep": bool(tip & 0x800),
                       "grayed": bool(durum & 0x3), "checked": bool(durum & 0x8),
                       "radio": bool(tip & 0x200)}
                if bayrak & 0x01:
                    o += 4                  # dwHelpId
                    oge["items"], o = ex_oge(o)
                ogeler.append(oge)
                if bayrak & 0x80:
                    return ogeler, o
        return ex_oge(o)[0]

    def oge(o):
        ogeler = []
        while True:
            bayrak = struct.unpack_from("<H", d, o)[0]
            o += 2
            if bayrak & 0x10:               # MF_POPUP
                metin, o = wstr(d, o)
                alt, o = oge(o)
                ogeler.append({"text": metin, "items": alt, "grayed": bool(bayrak & 0x3)})
            else:
                kimlik = struct.unpack_from("<H", d, o)[0]
                o += 2
                metin, o = wstr(d, o)
                ogeler.append({"id": kimlik, "text": metin, "sep": kimlik == 0 and not metin,
                               "grayed": bool(bayrak & 0x3), "checked": bool(bayrak & 0x8)})
            if bayrak & 0x80:               # MF_END
                return ogeler, o
    return oge(4 + boyut)[0]


def saat_dilimi_haritasi(r, out):
    """timedate.cpl bitmap 50: palet sıralı saat dilimi haritası. 2k = k. dilimin denizi, 2k+1 karası
    (k=1 GMT, 2..13 = +1..+12, 14..24 = -11..-1); 64+ yarım saatlik dilimlerin karaları. Çıktı her piksel için
    R = (fark + 14) * 4, G = 255 kara / 0 deniz; XP bunu yeşil kara, lacivert deniz, seçili dilim açık renk çizerdi."""
    import numpy as np
    from PIL import Image
    from msstyles import _rle
    d = r.get("BITMAP", 50)
    if not d:
        return
    hs, w, h, _, bpp, comp = struct.unpack_from("<IiiHHI", d, 0)
    clr = struct.unpack_from("<I", d, 32)[0] or 256
    veri = d[hs + 4 * clr:]
    if comp == 1:
        idx = _rle(veri, w, abs(h), 8)
    else:
        stride = ((w * 8 + 31) // 32) * 4
        idx = np.frombuffer(veri[:stride * abs(h)], np.uint8).reshape(abs(h), stride)[:, :w]
    if h > 0:
        idx = idx[::-1]
    fark = np.full(idx.shape, -99.0)
    kara = (idx % 2 == 1)
    for i in np.unique(idx):
        i = int(i)
        k = i // 2
        if i < 64:
            if k == 0:
                continue
            fark[idx == i] = k - 1 if k <= 13 else k - 25
        else:                                   # ek bölgeler (kayıt defterindeki MapID'ler): konumdan kestir
            ys, xs = np.nonzero(idx == i)
            f = (xs.mean() - 178) / 14.83       # GMT şeridi x≈178, dilim başına ≈14.8 piksel
            fark[idx == i] = round(f * 2) / 2 if abs(abs(f % 1) - 0.5) < 0.1 else round(f)
            kara[idx == i] = not (ys.min() == 0 or ys.max() == idx.shape[0] - 1)   # kenara değen = deniz
    rgba = np.zeros(idx.shape + (4,), np.uint8)
    rgba[..., 0] = np.where(fark > -50, np.clip((fark + 14) * 4, 0, 254), 255).astype(np.uint8)
    rgba[..., 1] = np.where(kara, 255, 0)
    rgba[..., 3] = 255
    Image.fromarray(rgba, "RGBA").save(out / "timedate-harita.png")


def main(xp, out):
    xp, out = Path(xp), Path(out)
    out.mkdir(parents=True, exist_ok=True)
    for ad, sec in MODULLER.items():
        yol = xp / "ikon" / ad
        if not yol.exists():
            print("  yok:", ad)
            continue
        veri = yol.read_bytes()
        r = PEResources(veri)
        mod = {"dialogs": {}, "menus": {}, "strings": {}}
        ref_ico, ref_bmp = set(sec["ico"] or []), set(sec["bmp"] or [])
        for t, n, l, d in r.items():
            if t == "DIALOG" and isinstance(n, int) and (sec["dlg"] is None or n in sec["dlg"]):
                try:
                    dlg = dialog_coz(d)
                except Exception as e:      # bozuk/özel kaynak
                    print(f"  {ad} iletişim {n}: {e}")
                    continue
                mod["dialogs"][str(n)] = dlg
                for k in dlg["controls"]:
                    if k["class"] == "STATIC" and k["text"].startswith("#"):
                        tur = k["style"] & 0x1F
                        (ref_ico if tur == 0x3 else ref_bmp if tur == 0xE else set()).add(int(k["text"][1:]))
            elif t == "MENU" and sec["menu"] and isinstance(n, int):
                try:
                    mod["menus"][str(n)] = menu_coz(d)
                except Exception as e:
                    print(f"  {ad} menü {n}: {e}")
        dizeler = r.strings()
        mod["strings"] = {str(k): v.rstrip("\x00") for k, v in dizeler.items() if sec["str"] is None or k in sec["str"]}
        stem = ad.replace(".", "-")
        (out / f"{stem}.json").write_text(json.dumps(mod, ensure_ascii=False, separators=(",", ":")))
        # simgeler: istenenler (None = hepsi), 32 ve 16 piksel
        gruplar = xpikon.icon_images(veri)
        for n, imgs in gruplar.items():
            if not isinstance(n, int) or (sec["ico"] is not None and n not in ref_ico):
                continue
            xpikon.at_size(imgs, 32).save(out / f"{stem}-ico-{n}.png")
            xpikon.at_size(imgs, 16).save(out / f"{stem}-ico16-{n}.png")
        for t, n, l, d in r.items():
            if t == "BITMAP" and isinstance(n, int) and (sec["bmp"] is None or n in ref_bmp):
                try:
                    im = decode_dib(d)[0].convert("RGBA")
                    r0, g0, b0 = im.getpixel((0, 0))[:3]
                    if r0 > 248 and g0 < 8 and b0 > 248:          # mor köşe = saydam renk (ör. 255,1,255)
                        px = im.load()
                        for y in range(im.height):
                            for x in range(im.width):
                                if px[x, y][:3] == (r0, g0, b0):
                                    px[x, y] = (0, 0, 0, 0)
                    im.save(out / f"{stem}-bmp-{n}.png")
                except Exception as e:
                    print(f"  {ad} bitmap {n}: {e}")
        if ad == "timedate.cpl":
            saat_dilimi_haritasi(r, out)
        if ad == "appwiz.cpl":                  # Program Ekle veya Kaldır: HTA'nın büyük düğme görselleri
            import io
            from PIL import Image
            for t, n, l, d in r.items():
                if t == "HTML" and isinstance(n, str) and n.upper().endswith(".GIF"):
                    Image.open(io.BytesIO(d)).convert("RGBA").save(out / f"appwiz-{n.lower()[:-4]}.png")
        print(f"  {ad}: {len(mod['dialogs'])} iletişim kutusu, {len(mod['menus'])} menü, {len(mod['strings'])} dize")
    print("iletişim kutuları ->", out)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
