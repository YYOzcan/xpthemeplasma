#!/usr/bin/env python3
"""Gerçek luna.msstyles / explorer.exe varlıklarından Plasma parçaları.

    python3 tools/gen_luna_plasma.py <xp-dizini> <desktoptheme-çıktı> <varlık-çıktı>

1) XP-Luna masaüstü temasında görev çubuğu arka planı ve görev düğmelerini gerçek
   bitmap'lerle değiştirir (stok görev yöneticisi bunları SVG'den çizer).
2) Başlat düğmesi, bildirim alanı ve Başlat paneli parçalarını PNG + kenar payı
   (varlik.json) olarak dışa aktarır; XP plasmoid'leri bunları BorderImage ile çizer.
"""
import json
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
import gen_desktoptheme as gd  # noqa: E402
from gen_luna_aurorae import stretch9  # noqa: E402
from luna import Sheet  # noqa: E402
from msstyles import Msstyles, decode_dib  # noqa: E402
from peres import PEResources  # noqa: E402

TASKBAR_H = 30


def up(img, s=2):
    return img.resize((img.size[0] * s, img.size[1] * s), Image.NEAREST)


def desktoptheme(ms, out):
    gd.main(out)                       # çizim tabanı (açılır pencereler, ipuçları, renkler)
    d = Path(out) / gd.NAME
    # ---- görev çubuğu arka planı
    bg = ms.images("TaskBar.BackgroundBottom")[0]
    l, r, t, b = ms.margins("TaskBar.BackgroundBottom")
    bar = stretch9(bg, 50, TASKBAR_H, l, r, t, b)
    s = Sheet(scale=2, width=300)
    for prefix in ["south", "north", ""]:
        s.add_frame(prefix, up(bar), 1, 1, 1, 1)
    vbar = bar.rotate(90, expand=True)
    for prefix in ["west", "east"]:
        s.add_frame(prefix, up(vbar), 1, 1, 1, 1)
    for prefix in ["south", "north", "west", "east", ""]:
        p = f"{prefix}-" if prefix else ""
        for side in ["top", "bottom", "left", "right"]:
            s.add_rect(f"{p}hint-{side}-margin", 1, 1)
    gd.write(d / "widgets/panel-background.svg", s)

    # ---- görev düğmeleri: Normal, Hot, Pressed, Disabled, Checked, HotChecked
    btn = ms.images("TaskBand::Toolbar.Button")
    l, r, t, b = ms.margins("TaskBand::Toolbar.Button")
    flash = ms.images("TaskBand.FlashButton")[0]

    def render(im, w=160, h=24, m=(l, r, t, b)):
        return stretch9(im, w, h, *m)

    states = {"normal": render(btn[0]), "minimized": render(btn[0]), "hover": render(btn[1]),
              "focus": render(btn[4]), "attention": render(flash, m=(4, 4, 4, 4))}
    s = Sheet(scale=2, width=500)
    for kind, im in states.items():
        s.add_frame(kind, up(im), 4, 4, 4, 4)
        for side in ["south", "north", "west", "east"]:
            s.add_frame(f"{side}-{kind}", up(im), 4, 4, 4, 4)
    gd.write(d / "widgets/tasks.svg", s)

    # ---- oklar: tepsi genişletici XP'nin yuvarlak "‹" düğmesi; diğerleri küçük siyah üçgen
    chev = ms.images("TrayNotifyHoriz::Button")[0]
    s = Sheet(scale=2, width=300)
    s.add_image("up-arrow", up(chev), chev.size[0], chev.size[1])
    tri = {"down-arrow": "M2,5 L14,5 L8,11 Z", "left-arrow": "M11,2 L11,14 L5,8 Z",
           "right-arrow": "M5,2 L5,14 L11,8 Z"}
    for eid, path in tri.items():
        s.add_image(eid, gd.im(16, 16, f'<path d="{path}" fill="#000000"/>'), 16, 16)
    gd.write(d / "widgets/arrows.svg", s)
    print("masaüstü teması (gerçek Luna) ->", d)


def varliklar(ms, xp, out):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    meta = {}

    def save(name, img, margins=(0, 0, 0, 0), **extra):
        img.save(out / f"{name}.png")
        l, r, t, b = margins
        meta[name] = dict(width=img.size[0], height=img.size[1], left=l, right=r, top=t, bottom=b, **extra)

    # Başlat düğmesi (3 durum) — 30 px görev çubuğuna göre
    for name, im in zip(["start-normal", "start-hot", "start-pressed"], ms.images("Start::Button")):
        save(name, im, ms.margins("Start::Button"))
    f = ms.prop("Start::Button", "Font", "Franklin Gothic Medium, 14, italic")
    tsc = ms.color("Start::Button", "TextShadowColor", (69, 76, 16))
    meta["start-text"] = {"font": f, "shadow": "#%02x%02x%02x" % tsc,
                          "shadowOffset": ms.ints("Start::Button", "TextShadowOffset", [2, 2])}

    # Bildirim alanı + gizli simge oku
    save("tray", ms.images("TrayNotifyHoriz::TrayNotify.Background")[0],
         ms.margins("TrayNotifyHoriz::TrayNotify.Background"),
         content=ms.ints("TrayNotifyHoriz::TrayNotify.Background", "ContentMargins"))
    for name, im in zip(["tray-chevron-normal", "tray-chevron-hot", "tray-chevron-pressed"],
                        ms.images("TrayNotifyHoriz::Button")):
        save(name, im)

    # Gezgin / Denetim Masası görev bölmesi (ExplorerBar)
    for name, cls in [("eb-normal-head", "ExplorerBar.NormalGroupHead"), ("eb-normal-bg", "ExplorerBar.NormalGroupBackground"),
                      ("eb-special-head", "ExplorerBar.SpecialGroupHead"), ("eb-special-bg", "ExplorerBar.SpecialGroupBackground")]:
        save(name, ms.images(cls)[0], ms.margins(cls))
    for kind in ["Normal", "Special"]:
        for act in ["Collapse", "Expand"]:
            for i, im in enumerate(ms.images(f"ExplorerBar.{kind}Group{act}")):
                save(f"eb-{kind.lower()}-{act.lower()}-{i}", im)
    eb = {}
    for key, cls in [("pane", "ExplorerBar"), ("normalHead", "ExplorerBar.NormalGroupHead"),
                     ("normalLink", "ExplorerBar.NormalGroupBackground"),
                     ("specialHead", "ExplorerBar.SpecialGroupHead"),
                     ("specialLink", "ExplorerBar.SpecialGroupBackground")]:
        eb[key] = {k: ("#%02x%02x%02x" % c if c else None) for k, c in [
            ("text", ms.color(cls, "TextColor")), ("g1", ms.color(cls, "GradientColor1")),
            ("g2", ms.color(cls, "GradientColor2")), ("fill", ms.color(cls, "FillColor"))]}
    meta["explorerbar"] = eb
    # araç çubuğu şeritleri (Gezgin: shell32 214/215, IE: 204/205; 24 px)
    s32 = PEResources((Path(xp) / "ikon/shell32.dll").read_bytes())
    for bid, name in [(214, "tb-gezgin"), (215, "tb-gezgin-hot"), (204, "tb-ie"), (205, "tb-ie-hot"),
                      (216, "tb-gezgin-16")]:
        d = s32.get("BITMAP", bid)
        if d:
            im, _ = decode_dib(d)
            save(name, im)

    # balon (bildirim) kapatma düğmesi: normal, üzerinde, basılı
    for name, im in zip(["balloon-close-normal", "balloon-close-hot", "balloon-close-pressed"],
                        ms.images("Tooltip.Close")):
        save(name, im)

    # Başlat paneli
    for name, cls in [("panel-user", "StartPanel.UserPane"), ("panel-usertile", "StartPanel.UserPicture"),
                      ("panel-proglist", "StartPanel.ProgList"), ("panel-progsep", "StartPanel.ProgListSeparator"),
                      ("panel-more", "StartPanel.MorePrograms"), ("panel-more-arrow", "StartPanel.MoreProgramsArrow"),
                      ("panel-more-arrow-hot", "StartPanel.MoreProgramsArrow(hot)"),
                      ("panel-places", "StartPanel.PlacesList"), ("panel-placessep", "StartPanel.PlacesListSeparator"),
                      ("panel-logoff", "StartPanel.Logoff"), ("panel-logoffbuttons", "StartPanel.LogoffButtons"),
                      ("panel-logoffbuttons-hot", "StartPanel.LogoffButtons(hot)")]:
        save(name, ms.images(cls)[0], ms.margins(cls))
    for key, cls in [("user", "StartPanel.UserPane"), ("proglist", "StartPanel.ProgList"),
                     ("places", "StartPanel.PlacesList"), ("more", "StartPanel.MorePrograms"),
                     ("logoff", "StartPanel.Logoff")]:
        col = ms.color(cls, "TextColor")
        hot = ms.color(cls, "HotTracking")
        meta.setdefault("colors", {})[key] = {
            "text": "#%02x%02x%02x" % col if col else None,
            "hot": "#%02x%02x%02x" % hot if hot else None,
            "font": ms.prop(cls, "Font")}
    sh = ms.color("StartPanel.UserPane", "TextShadowColor")
    meta["colors"]["user"]["shadow"] = "#%02x%02x%02x" % sh

    # explorer.exe: başlat bayrağı (143) ve oturum kapat/kapat simgeleri (7010/7011)
    ex = PEResources((Path(xp) / "ikon/explorer.exe").read_bytes())
    flag, _ = decode_dib(ex.get("BITMAP", 143))
    save("start-flag", flag)
    for rid, name in [(7010, "logoff-icons"), (7011, "logoff-icons-hot")]:
        dib = ex.get("BITMAP", rid)
        if dib:
            im, _ = decode_dib(dib)
            # 24 bpp: macenta saydam
            import numpy as np
            a = np.asarray(im).copy()
            a[np.all(a[:, :, :3] == (255, 0, 255), axis=2)] = 0
            save(name, Image.fromarray(a))
    # ---- açılış ekranı (ntoskrnl): kaynakta palet siyah tutulur, XP çalışma anında yükler.
    # Bayrak/metin bölgelerinden çıkarılan XP açılış paleti:
    boot_pal = [(0, 0, 0), (32, 32, 32), (70, 70, 70), (32, 26, 21), (83, 101, 1), (178, 53, 5),
                (126, 126, 126), (137, 146, 0), (45, 62, 210), (247, 107, 32), (94, 127, 252),
                (142, 220, 4), (243, 188, 27), (188, 188, 188), (255, 255, 255), (252, 252, 252)]
    palb = b"".join(bytes([bb, gg, rr, 0]) for rr, gg, bb in boot_pal)
    nt = Path(xp) / "ikon/ntoskrnl.exe"
    if nt.exists():
        kr = PEResources(nt.read_bytes())
        for rid, name in [(1, "boot"), (8, "boot-blok")]:
            d = kr.get("BITMAP", rid)
            if d:
                im, _ = decode_dib(d[:40] + palb + d[104:])
                save(name, im)
        meta["boot"] = {"box": [255, 351, 380, 365], "inner": [258, 354, 377, 362]}

    # ---- karşılama ekranı (logonui.exe)
    lu = Path(xp) / "ikon/logonui.exe"
    if lu.exists():
        lr = PEResources(lu.read_bytes())
        for rid, name in [(100, "logon-glow"), (112, "logon-selected"), (102, "logon-password"),
                          (113, "logon-tile"), (119, "logon-tile-hot"), (114, "logon-userpic"),
                          (123, "logon-logo"), (103, "logon-go"), (104, "logon-go-hot"),
                          (105, "logon-help"), (106, "logon-help-hot"), (107, "logon-power"),
                          (121, "logon-power-hot"), (124, "logon-divider"), (125, "logon-topline"),
                          (126, "logon-bottomline")]:
            d = lr.get("BITMAP", rid)
            if d:
                im, _ = decode_dib(d)
                save(name, im)

    # ---- "Bilgisayarı Kapat" (msgina.dll) ve "Windows Oturumunu Kapat" (shell32.dll) pencereleri:
    # 313x198 arka plan, 48x40 bayrak, 32x32 düğme şeritleri (normal, basılı, üzerinde; mor = saydam)
    def mor_saydam(im):
        im = im.convert("RGBA")
        px = im.load()
        for y in range(im.height):
            for x in range(im.width):
                if px[x, y][:3] == (255, 0, 255):
                    px[x, y] = (0, 0, 0, 0)
        return im

    def serit(res, bid, ad, n):
        d = res.get("BITMAP", bid)
        if d:
            im = mor_saydam(decode_dib(d)[0])
            for i in range(n):
                save(f"{ad}-{i}", im.crop((0, i * 32, 32, i * 32 + 32)))

    mg = Path(xp) / "ikon/msgina.dll"
    if mg.exists():
        mr = PEResources(mg.read_bytes())
        for rid, name in [(20142, "kapat-bg"), (20143, "kapat-bayrak")]:
            d = mr.get("BITMAP", rid)
            if d:
                save(name, decode_dib(d)[0])
        serit(mr, 20150, "kapat-dugme", 10)
    for rid, name in [(14354, "oturum-bg"), (14355, "oturum-bayrak"), (131, "hakkinda-banner"),
                      (138, "hakkinda-cizgi")]:
        d = s32.get("BITMAP", rid)
        if d:
            save(name, decode_dib(d)[0])
    serit(s32, 14356, "oturum-dugme", 6)

    strings = ex.strings()
    meta["strings"] = {"start": strings.get(578, "start"), "startTip": strings.get(533, ""),
                       "showHidden": strings.get(543, ""), "hide": strings.get(542, "")}
    (out / "varlik.json").write_text(json.dumps(meta, indent=1, ensure_ascii=False))
    print("varlıklar ->", out, f"({len(meta)} kayıt)")


def main(xp, theme_out, varlik_out):
    import varyant
    gd.NAME = varyant.ad("XP-Luna", "-")
    ms = Msstyles(str(Path(xp) / "stil/luna.msstyles"), varyant.MS)
    desktoptheme(ms, theme_out)
    varyant.dizini_cevir(Path(theme_out) / gd.NAME, varyant.tablo(str(Path(xp) / "stil/luna.msstyles")))
    varliklar(ms, xp, varlik_out)


if __name__ == "__main__":
    main(*sys.argv[1:4])
