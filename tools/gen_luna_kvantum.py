#!/usr/bin/env python3
"""Gerçek luna.msstyles bitmap'lerinden XPLuna Kvantum teması.

    python3 tools/gen_luna_kvantum.py <luna.msstyles> <çıktı dizini>

Bitmap'i olmayan parçalar (metin kutusu kenarı, menüler, ipuçları, seçim) Luna'nın
INI renkleriyle gen_kvantum.py'deki çizimlerden gelir.
"""
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
import gen_kvantum as gk  # noqa: E402
from luna import Sheet  # noqa: E402
from msstyles import Msstyles  # noqa: E402

S = 2   # gk ile aynı ölçek: gerçek bitmap'ler 2x en yakın komşu ile büyütülür (geri küçültmede birebir)


def up(img):
    return img.resize((img.size[0] * S, img.size[1] * S), Image.NEAREST)


def compose(bg, glyph):
    im = bg.copy()
    im.alpha_composite(glyph, ((im.size[0] - glyph.size[0]) // 2, (im.size[1] - glyph.size[1]) // 2))
    return im


def fit(img, w, h):
    """uxtheme 'stretch' ile w×h'ye getir (5 px kenarları koruyarak)."""
    from gen_luna_aurorae import stretch9
    m = min(5, img.size[0] // 2 - 1, img.size[1] // 2 - 1)
    return stretch9(img, w, h, m, m, m, m)


def kv(sheet, el, state, img, l, t, r, b):
    sheet.add_kv_frame(el, state, up(img), l, t, r, b)


def ind(sheet, eid, img):
    sheet.add_image(eid, up(img), img.size[0], img.size[1])


def main(msstyles, out):
    import varyant
    gk.NAME = varyant.ad("XPLuna")
    ms = Msstyles(msstyles, varyant.MS)
    d = Path(out) / gk.NAME
    d.mkdir(parents=True, exist_ok=True)
    s = Sheet(scale=S, width=900)

    # ---- basma düğmesi: N, Hot, Pressed, Disabled, Defaulted
    n, h, p, dis, de = ms.images("button.pushbutton")
    for st, im in [("normal", n), ("focused", h), ("pressed", p), ("toggled", p), ("disabled", dis), ("default", de)]:
        kv(s, "btn", st, im, 3, 3, 3, 3)

    # ---- araç çubuğu düğmesi: N, Hot, Pressed, Disabled, Checked, HotChecked
    tb = ms.images("Toolbar.Button")
    for st, i in [("normal", 0), ("focused", 1), ("pressed", 2), ("disabled", 3), ("toggled", 4)]:
        kv(s, "tbaritem", st, tb[i], 3, 3, 3, 3)

    # ---- açılır kutu düğmesi (oku ile birlikte, 17x17)
    cb = ms.images("Combobox.DropDownButton")
    cg = ms.images("Combobox.DropDownButton", "GlyphImageFile")
    for st, i in [("normal", 0), ("focused", 1), ("pressed", 2), ("toggled", 2), ("disabled", 3)]:
        ind(s, f"carrow-{st}", compose(fit(cb[i], 17, 17), cg[i]))
    s.newline()

    # ---- kaydırma okları: Up N/H/P/D, Down, Left, Right
    ab = ms.images("ScrollBar.ArrowBtn")
    ag = ms.images("ScrollBar.ArrowBtn", "GlyphImageFile") if ms.prop("ScrollBar.ArrowBtn", "GlyphImageFile") else None
    if ag is None:
        g = ms.bitmap("Blue\\ScrollArrowGlyphs.bmp", (255, 0, 255))
        gh = g.size[1] // 16
        ag = [g.crop((0, i * gh, g.size[0], (i + 1) * gh)) for i in range(16)]
    for di, direction in enumerate(["up", "down", "left", "right"]):
        for si, st in enumerate(["normal", "focused", "pressed", "disabled"]):
            ind(s, f"scarrow-{direction}-{st}", compose(ab[di * 4 + si], ag[di * 4 + si]))
    s.newline()

    # ---- kaydırma başparmağı + tutamak + oluk (yatay taban; Kvantum dikeyde döndürür)
    th = ms.images("Scrollbar.ThumbBtnHorz")
    gr = ms.images("Scrollbar.GripperHorz")
    tr = ms.images("Scrollbar.LowerTrackHorz")
    for st, i in [("normal", 0), ("focused", 1), ("pressed", 2), ("toggled", 2), ("disabled", 3)]:
        kv(s, "scrollslide", st, th[i], 3, 3, 3, 3)
        kv(s, "trough", st, tr[i], 1, 1, 1, 1)
    for st, i in [("normal", 0), ("focused", 1), ("pressed", 2), ("disabled", 3)]:
        ind(s, f"sgrip-{st}", gr[i])
    s.newline()

    # ---- onay kutusu (13 px): Unchecked N/H/P/D, Checked, Mixed
    ck = ms.images("button.checkbox")
    for ki, kind in enumerate(["", "checked-", "tristate-"]):
        for si, st in enumerate(["normal", "focused", "pressed", "disabled"]):
            ind(s, f"checkbox-{kind}{st}", ck[ki * 4 + si])
    rb = ms.images("button.radiobutton")
    for ki, kind in enumerate(["", "checked-"]):
        for si, st in enumerate(["normal", "focused", "pressed", "disabled"]):
            ind(s, f"radio-{kind}{st}", rb[ki * 4 + si])
    s.newline()

    # ---- grup kutusu, sekmeler, sekme sayfası
    gb = ms.images("button.groupbox")[0]
    for st in gk.STATES:
        kv(s, "gbox", st, gb, 4, 4, 4, 4)
    tt = ms.images("Tab.TopTabItem")       # N, Hot, Selected, Disabled, Focused
    for st, i in [("normal", 0), ("focused", 1), ("pressed", 2), ("toggled", 2), ("disabled", 3)]:
        kv(s, "tab", st, tt[i], 4, 4, 4, 0)
    pane = ms.images("Tab.Pane")[0]
    for st in gk.STATES:
        kv(s, "tbf", st, pane, 2, 2, 2, 2)

    # ---- ilerleme çubuğu
    pb = ms.images("Progress.Bar")[0]
    for st in gk.STATES:
        kv(s, "pbgutter", st, pb, 3, 3, 3, 3)
    ch = ms.images("Progress.Chunk")[0]
    for st in ["normal", "focused", "toggled"]:
        ind(s, f"pbc-{st}", ch)
    s.newline()

    # ---- kaydırıcı: oluk + başparmak (Kvantum tabanı 90° döndürür)
    trk = ms.images("TrackBar.Track")[0]
    for st in gk.STATES:
        kv(s, "slider", st, trk, 1, 1, 1, 1)
    thb = ms.images("TrackBar.Thumb")      # N, Hot, Pressed, Focused, Disabled
    for st, i in [("normal", 0), ("focused", 1), ("pressed", 2), ("disabled", 4)]:
        ind(s, f"scursor-{st}", thb[i].rotate(-90, expand=True))
    s.newline()

    # ---- başlık sütunu, ağaç işaretleri, boyutlandırma tutamağı
    hd = ms.images("Header.HeaderItem")    # N, Hot, Pressed, ...
    for st, i in [("normal", 0), ("focused", 1), ("pressed", 2), ("toggled", 2), ("disabled", 0)]:
        kv(s, "hdr", st, hd[i], 2, 1, 2, 3)
    tg = ms.images("TreeView.Glyph")       # kapalı (+), açık (-)
    for st in ["normal", "focused", "pressed"]:
        ind(s, f"tree-plus-{st}", tg[0])
        ind(s, f"tree-minus-{st}", tg[1])
    ind(s, "resize-grip-normal", ms.images("Status.Gripper")[0])
    s.newline()

    # ---- araç çubuğu arka planı (Rebar)
    rb_bg = ms.images("Rebar")[0]
    for st in gk.STATES:
        kv(s, "tlbar", st, rb_bg.resize((max(rb_bg.size[0], 8), rb_bg.size[1])), 1, 1, 1, 1)

    # ---- bitmap'i olmayanlar: Luna renkleriyle çizilenler
    for st in gk.STATES:
        s.add_kv_frame("ledit", st, gk.line_edit(st), 2, 2, 2, 2)
        s.add_kv_frame("combo", st, gk.combo(st), 2, 2, 2, 2)
        s.add_kv_frame("common", st, gk.generic_frame(st), 1, 1, 1, 1)
        s.add_kv_frame("mnu", st, gk.menu(st), 2, 2, 2, 2)
        s.add_kv_frame("menuitem", st, gk.menu_item(st), 1, 1, 1, 1)
        s.add_kv_frame("mbaritem", st, gk.menu_item(st), 1, 1, 1, 1)
        s.add_kv_frame("mubar", st, gk.menubar(st), 1, 1, 1, 1)
        s.add_kv_frame("ttip", st, gk.tooltip(st), 2, 2, 2, 2)
        s.add_kv_frame("iview", st, gk.item_view(st), 1, 1, 1, 1)
        if st in ("toggled", "pressed"):
            s.add_kv_frame("iview", st + "-inactive", gk.item_view(st, inactive=True), 1, 1, 1, 1)
    for st in ["normal", "focused", "pressed", "disabled"]:
        for direction in ["up", "down", "left", "right"]:
            col = "#ACA899" if st == "disabled" else "#000000"
            s.add_image(f"arrow-{direction}-{st}", gk.img(9, 9, gk.triangle(direction, col)), 9, 9)
        sp = ms.images("Spin.Up", "GlyphImageFile")
        sd = ms.images("Spin.Down", "GlyphImageFile")
        si = {"normal": 0, "focused": 1, "pressed": 2, "disabled": 3}[st]
        ind(s, f"spind-up-{st}", sp[si])
        ind(s, f"spind-down-{st}", sd[si])
        s.add_image(f"spind-plus-{st}", gk.img(9, 9, '<path d="M1,4h7v1h-7z M4,1h1v7h-1z" fill="#4D6185"/>'), 9, 9)
        s.add_image(f"spind-minus-{st}", gk.img(9, 9, '<path d="M1,4h7v1h-7z" fill="#4D6185"/>'), 9, 9)
    s.newline()
    s.add_kv_frame("focus", "x", gk.focus_frame(), 1, 1, 1, 1)

    svg = s.svg().replace('id="focus-x-', 'id="focus-').replace('id="focus-x"', 'id="focus"')
    (d / f"{gk.NAME}.svg").write_text(svg)
    cfg = gk.KVCONFIG.replace("slider_handle_width=21\nslider_handle_length=11",
                              f"slider_handle_width={thb[0].size[1]}\nslider_handle_length={thb[0].size[0]}")
    cfg = cfg.replace("comment=Windows XP Luna (Mavi) — sıfırdan çizilmiş Kvantum teması",
                      "comment=Windows XP Luna (Mavi) — luna.msstyles'tan üretildi")
    (d / f"{gk.NAME}.kvconfig").write_text(cfg)
    varyant.dizini_cevir(d, varyant.tablo(msstyles))
    print("kvantum (gerçek Luna) ->", d)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "build/kvantum")
