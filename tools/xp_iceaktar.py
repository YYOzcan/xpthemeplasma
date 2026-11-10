#!/usr/bin/env python3
"""Kendi Windows XP kopyandan gerçek varlıkları içe aktar.

    python3 tools/xp_iceaktar.py "/yol/Windows XP.iso"     # ISO
    python3 tools/xp_iceaktar.py /mnt/cdrom/I386           # CD'nin I386 klasörü
    python3 tools/xp_iceaktar.py /mnt/xp/WINDOWS           # kurulu XP'nin WINDOWS klasörü

Çıktı: ~/.local/share/xpthemeplasma/xp/{stil,ikon,ses,imlec,font,duvar,dil.txt}
Bu dosyalar Microsoft'a aittir; yalnızca yerelde kullanılır, depoya girmez.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HEDEF = Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local/share")) / "xpthemeplasma/xp"

STIL = ["luna.msstyles", "luna.theme"]
IKON = """shell32.dll explorer.exe user32.dll logonui.exe ntoskrnl.exe ntkrnlpa.exe notepad.exe calc.exe
mspaint.exe cmd.exe iexplore.exe wmplayer.exe wmploc.dll msimn.exe sol.exe winmine.exe freecell.exe
spider.exe mshearts.exe taskmgr.exe regedit.exe wordpad.exe write.exe msmsgs.exe hh.exe helpctr.exe
sndvol32.exe sndrec32.exe charmap.exe magnify.exe osk.exe moviemk.exe mmsys.cpl desk.cpl main.cpl
timedate.cpl netshell.dll hotplug.dll stobject.dll xpsp2res.dll wscui.cpl firewall.cpl powercfg.cpl
appwiz.cpl sysdm.cpl inetcpl.cpl access.cpl intl.cpl joy.cpl ncpa.cpl nusrmgr.cpl telephon.cpl
bthprops.cpl wuaucpl.cpl mstsc.exe rstrui.exe cleanmgr.exe zipfldr.dll moricons.dll progman.exe
winhlp32.exe mplayer2.exe dxdiag.exe msconfig.exe ieframe.dll shdocvw.dll mshtml.dll msinfo32.exe
sndrec32.exe narrator.exe utilman.exe accwiz.exe cdplayer.exe mspaint.exe shimgvw.dll fontext.dll
cabview.dll url.dll mydocs.dll syncui.dll dfrgres.dll compstui.dll ntbackup.exe migwiz.exe
wiashext.dll sti.dll mmcndmgr.dll comres.dll pifmgr.dll setupapi.dll mmc.exe eventvwr.exe
devmgr.dll themeui.dll printui.dll netplwiz.dll wlanui.dll xpsp1res.dll ole2.dll""".split()
FONT = """tahoma.ttf tahomabd.ttf trebuc.ttf trebucbd.ttf trebucit.ttf trebucbi.ttf framd.ttf framdit.ttf
lucon.ttf verdana.ttf verdanab.ttf verdanai.ttf verdanaz.ttf micross.ttf marlett.ttf arial.ttf
arialbd.ttf ariali.ttf arialbi.ttf ariblk.ttf times.ttf timesbd.ttf timesi.ttf timesbi.ttf cour.ttf
courbd.ttf couri.ttf courbi.ttf georgia.ttf georgiab.ttf georgiai.ttf georgiaz.ttf comic.ttf
comicbd.ttf impact.ttf pala.ttf palab.ttf palai.ttf palabi.ttf sylfaen.ttf l_10646.ttf lsans.ttf
lsansd.ttf lsansi.ttf lsansdi.ttf webdings.ttf wingding.ttf symbol.ttf estre.ttf mvboli.ttf
vga857.fon vgaoem.fon app857.fon vgasys.fon vgasyst.fon vgafix.fon vgafixt.fon sserife.fon
sseriffe.fon sserift.fon serife.fon smalle.fon smallet.fon coure.fon couret.fon modern.fon
roman.fon script.fon dosapp.fon""".split()
DUVAR = """ascent.jpg autumn.jpg azul.jpg bliss.jpg crystal.jpg follow.jpg friend.jpg home.jpg moon.jpg
peace.jpg power.jpg purplef.jpg radiance.jpg redmoon.jpg ripple.jpg stoneh.jpg tulips.jpg vortec.jpg
wind.jpg winxp.jpg""".split()

# 8.3 kısaltmasına uğrayan adlar (CAB içindeki ad -> gerçek ad)
KISA = {"luna.mst": "luna.msstyles", "luna.the": "luna.theme"}


def ac_iso(iso, tmp):
    print("ISO açılıyor…")
    subprocess.run(["7z", "x", "-y", f"-o{tmp}", str(iso), "I386/*", "i386/*"],
                   check=True, stdout=subprocess.DEVNULL)
    for d in Path(tmp).iterdir():
        if d.name.lower() == "i386":
            return d
    raise SystemExit("ISO içinde I386 klasörü yok.")


def dizin_haritasi(kaynak: Path):
    """Küçük harfli ad -> yol (alt klasörler dahil)."""
    harita = {}
    for p in kaynak.rglob("*"):
        if p.is_file():
            harita.setdefault(p.name.lower(), p)
    return harita


def bul_ve_cikar(ad, harita, cikti: Path, tmp: Path):
    """ad'ı düz ya da CAB sıkıştırılmış (ör. SHELL32.DL_) biçimde bul, cikti'ya yaz."""
    ad = ad.lower()
    if ad in harita:
        shutil.copy2(harita[ad], cikti / ad)
        return True
    kokler = [ad[:-1] + "_"]
    stem, _, ext = ad.rpartition(".")
    if len(stem) > 8 or len(ext) > 3:     # uzun adlar CAB'de 8.3 olarak saklanır
        kokler.append(f"{stem[:8]}.{ext[:2]}_")
    for k in kokler:
        if k in harita:
            d = tmp / "cab"
            shutil.rmtree(d, ignore_errors=True)
            d.mkdir()
            r = subprocess.run(["cabextract", "-q", "-d", str(d), str(harita[k])],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            dosyalar = list(d.iterdir())
            if r.returncode == 0 and dosyalar:
                shutil.move(str(dosyalar[0]), cikti / ad)
                return True
    return False


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    kaynak = Path(sys.argv[1]).expanduser()
    if not kaynak.exists():
        sys.exit(f"Bulunamadı: {kaynak}")
    for t in ["7z", "cabextract"]:
        if not shutil.which(t):
            sys.exit(f"Gerekli araç yok: {t}")
    with tempfile.TemporaryDirectory(prefix="xpaktar-") as tmp:
        tmp = Path(tmp)
        kok = ac_iso(kaynak, tmp) if kaynak.is_file() else kaynak
        harita = dizin_haritasi(kok)
        # 8.3 adlı CAB içerikleri için (luna.mst gibi) — doğrudan aç
        if HEDEF.exists():
            shutil.rmtree(HEDEF)
        gruplar = {"stil": STIL, "ikon": IKON, "font": FONT, "duvar": DUVAR}
        sayac = {}
        for grup, adlar in gruplar.items():
            out = HEDEF / grup
            out.mkdir(parents=True, exist_ok=True)
            n = 0
            for ad in dict.fromkeys(adlar):
                if bul_ve_cikar(ad, harita, out, tmp):
                    n += 1
            sayac[grup] = n
        # luna.msstyles CAB'de "luna.mst" olarak durur
        stil = HEDEF / "stil"
        if not (stil / "luna.msstyles").exists() and "luna.ms_" in harita:
            d = tmp / "cab2"
            d.mkdir()
            subprocess.run(["cabextract", "-q", "-d", str(d), str(harita["luna.ms_"])], check=True)
            for f in d.iterdir():
                shutil.move(str(f), stil / KISA.get(f.name.lower(), f.name.lower()))
        if not (stil / "luna.theme").exists() and "luna.th_" in harita:
            d = tmp / "cab3"
            d.mkdir()
            subprocess.run(["cabextract", "-q", "-d", str(d), str(harita["luna.th_"])], check=True)
            for f in d.iterdir():
                shutil.move(str(f), stil / KISA.get(f.name.lower(), f.name.lower()))
        # tüm sesler ve imleçler
        for grup, uzantilar in {"ses": (".wav",), "imlec": (".cur", ".ani")}.items():
            out = HEDEF / grup
            out.mkdir(parents=True, exist_ok=True)
            n = 0
            for ad, yol in harita.items():
                if ad.endswith(uzantilar):
                    shutil.copy2(yol, out / ad)
                    n += 1
                elif ad.endswith(tuple(u[:-1] + "_" for u in uzantilar)):
                    hedef_ad = ad[:-1] + {"wa_": "v", "cu_": "r", "an_": "i"}[ad[-3:]]
                    if bul_ve_cikar(hedef_ad, harita, out, tmp):
                        n += 1
            sayac[grup] = n
        # dil: shell32.dll'nin kaynak dil kimliği (1055 = Türkçe)
        dil = "en"
        try:
            sys.path.insert(0, str(Path(__file__).parent))
            from peres import PEResources
            langs = {l for t, n, l, d in PEResources((HEDEF / "ikon/shell32.dll").read_bytes()).items()
                     if t == "STRING"}
            dil = {1055: "tr", 1031: "de", 1036: "fr", 1034: "es", 3082: "es"}.get(
                next(iter(langs - {1033}), 1033), "en")
        except Exception as e:
            print("dil algılanamadı:", e)
        (HEDEF / "dil.txt").write_text(dil + "\n")
    if not (HEDEF / "stil/luna.msstyles").exists():
        sys.exit("luna.msstyles bulunamadı — bu bir Windows XP kaynağı mı?")
    print("İçe aktarıldı ->", HEDEF)
    for k, v in sayac.items():
        print(f"  {k:6s}: {v}")
    print("  dil   :", dil)


if __name__ == "__main__":
    main()
