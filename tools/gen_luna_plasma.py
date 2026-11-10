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

    strings = ex.strings()
    meta["strings"] = {"start": strings.get(578, "start"), "startTip": strings.get(533, ""),
                       "showHidden": strings.get(543, ""), "hide": strings.get(542, "")}
    (out / "varlik.json").write_text(json.dumps(meta, indent=1, ensure_ascii=False))
    print("varlıklar ->", out, f"({len(meta)} kayıt)")


def main(xp, theme_out, varlik_out):
    ms = Msstyles(str(Path(xp) / "stil/luna.msstyles"))
    desktoptheme(ms, theme_out)
    varliklar(ms, xp, varlik_out)


if __name__ == "__main__":
    main(*sys.argv[1:4])
