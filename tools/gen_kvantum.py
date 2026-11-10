#!/usr/bin/env python3
"""XP Luna Kvantum temasını üretir (XPLuna.svg + XPLuna.kvconfig)."""
import sys
from pathlib import Path

from PIL import ImageEnhance

sys.path.insert(0, str(Path(__file__).parent))
from luna import (BUTTON_FACE, BUTTON_PRESSED, Sheet, draw, lingrad, radgrad,  # noqa: E402
                  rrect_path)

SCALE = 2
NAME = "XPLuna"
STATES = ["normal", "focused", "pressed", "toggled", "disabled"]


def rr(x, y, w, h, r):
    return rrect_path(x, y, w, h, r, r, r, r)


def img(w, h, body, defs=""):
    return draw(w, h, body, defs, SCALE)


def empty(w, h):
    return img(w, h, "")


def tint(im, f):
    rgb = ImageEnhance.Brightness(im.convert("RGB")).enhance(f)
    rgb.putalpha(im.getchannel("A"))
    return rgb


# ----------------------------------------------------------- push button ---
def push_button(state, default=False, w=30, h=23):
    if state in ("pressed", "toggled"):
        d, g = lingrad(BUTTON_PRESSED)
    elif state == "disabled":
        d, g = lingrad([(0, "#F5F4EA"), (1, "#F5F4EA")])
    else:
        d, g = lingrad(BUTTON_FACE)
    border = "#C9C7BA" if state == "disabled" else "#003C74"
    body = f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="{g}" stroke="{border}"/>'
    ring = None
    if state == "focused":   # turuncu "sıcak" halka
        ring = [(0, "#FFF0CF"), (0.15, "#FDD889"), (0.5, "#FBC761"), (1, "#E5A01A")]
    elif default and state == "normal":   # varsayılan düğme: mavi halka
        ring = [(0, "#CEE7FF"), (0.15, "#BCD4F6"), (0.5, "#98B8EA"), (1, "#89ADE4")]
    if ring:
        d2, g2 = lingrad(ring)
        d += d2
        body += f'<path d="{rr(1.5, 1.5, w - 3, h - 3, 2)}" fill="none" stroke="{g2}" stroke-width="2"/>'
    return img(w, h, body, d)


# ----------------------------------------------------------- tool button ---
def tool_button(state, w=24, h=24):
    if state == "normal" or state == "disabled":
        return empty(w, h)
    if state == "focused":
        d, g = lingrad([(0, "#FFFFFF"), (1, "#F0EEE5")])
        border = "#CECEC3"
    else:  # pressed / toggled
        d, g = lingrad([(0, "#E3E1D5"), (1, "#F2F1EC")])
        border = "#B9B6A6"
    body = f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="{g}" stroke="{border}"/>'
    if state in ("pressed", "toggled"):
        body += f'<path d="M2,{h - 2} V3 Q2,2 3,2 H{w - 2}" fill="none" stroke="#CFCBB9" stroke-width="1"/>'
    return img(w, h, body, d)


# ----------------------------------------------------------- girdi alanı ---
def line_edit(state, w=24, h=22):
    border = "#C9C7BA" if state == "disabled" else "#7F9DB9"
    fill = "#F5F4EA" if state == "disabled" else "#FFFFFF"
    return img(w, h, f'<rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="{fill}" stroke="{border}"/>')


def generic_frame(state, w=16, h=16):
    return img(w, h, f'<rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="none" stroke="#7F9DB9"/>')


# ----------------------------------------------------------- mavi mini düğme ---
def blue_button(state, w=17, h=17, r=2.5):
    """Kaydırma çubuğu oku / açılır kutu düğmesi / döndürme kutusu düğmesi."""
    if state == "disabled":
        d, g = lingrad([(0, "#F7F6F1"), (1, "#EEEDE5")], 0, 0, 1, 1)
        border = "#E8E8DF"
    else:
        stops = {
            "normal": [(0, "#E3EBFE"), (0.5, "#C6D5FB"), (1, "#B5C8F7")],
            "focused": [(0, "#F4F8FF"), (0.5, "#DAE5FF"), (1, "#C8D8FD")],
            "pressed": [(0, "#A9BEF2"), (0.5, "#B7CAF5"), (1, "#C8D7F8")],
            "toggled": [(0, "#A9BEF2"), (0.5, "#B7CAF5"), (1, "#C8D7F8")],
        }[state]
        d, g = lingrad(stops, 0, 0, 1, 1)
        border = "#B9CDF8" if state != "focused" else "#C4D6FD"
    body = (f'<path d="{rr(0.5, 0.5, w - 1, h - 1, r)}" fill="{g}" stroke="{border}"/>'
            f'<path d="{rr(1.5, 1.5, w - 3, h - 3, r - 1)}" fill="none" stroke="#FFFFFF" stroke-opacity="0.55"/>')
    return img(w, h, body, d)


def chevron(direction, color, w=17, h=17, size=1.0):
    """Kalın XP oku (ok ucu)."""
    cx, cy = w / 2, h / 2
    s = 3.2 * size
    pts = {
        "up": f"M{cx - s},{cy + s / 2} L{cx},{cy - s / 2} L{cx + s},{cy + s / 2}",
        "down": f"M{cx - s},{cy - s / 2} L{cx},{cy + s / 2} L{cx + s},{cy - s / 2}",
        "left": f"M{cx + s / 2},{cy - s} L{cx - s / 2},{cy} L{cx + s / 2},{cy + s}",
        "right": f"M{cx - s / 2},{cy - s} L{cx + s / 2},{cy} L{cx - s / 2},{cy + s}",
    }[direction]
    return f'<path d="{pts}" fill="none" stroke="{color}" stroke-width="{2 * size:.2f}" stroke-linecap="square"/>'


def triangle(direction, color, w=9, h=9):
    cx, cy = w / 2, h / 2
    s = 3.5
    p = {
        "down": f"M{cx - s},{cy - s / 2} L{cx + s},{cy - s / 2} L{cx},{cy + s / 2 + 0.5}Z",
        "up": f"M{cx - s},{cy + s / 2} L{cx + s},{cy + s / 2} L{cx},{cy - s / 2 - 0.5}Z",
        "right": f"M{cx - s / 2},{cy - s} L{cx - s / 2},{cy + s} L{cx + s / 2 + 0.5},{cy}Z",
        "left": f"M{cx + s / 2},{cy - s} L{cx + s / 2},{cy + s} L{cx - s / 2 - 0.5},{cy}Z",
    }[direction]
    return f'<path d="{p}" fill="{color}"/>'


def scroll_arrow(direction, state):
    base = blue_button(state)
    col = "#C9C7BA" if state == "disabled" else "#4D6185"
    glyph = img(17, 17, chevron(direction, col, size=0.85))
    base.alpha_composite(glyph)
    return base


# ----------------------------------------------------------- kaydırma ---
def trough(state, w=24, h=17):
    d, g = lingrad([(0, "#EEEDE5"), (0.15, "#F3F1EC"), (0.85, "#FEFEFE"), (1, "#FCFCF7")])
    return img(w, h, f'<rect width="{w}" height="{h}" fill="{g}"/>', d)


def scroll_slider(state, w=30, h=17):
    if state == "disabled":
        return empty(w, h)
    stops = {
        "normal": [(0, "#C9D8FC"), (0.5, "#BBCEFB"), (1, "#AEC4F8")],
        "focused": [(0, "#DAE6FE"), (0.5, "#CCDBFE"), (1, "#BDD0FC")],
        "pressed": [(0, "#A8BFF5"), (0.5, "#B3C8F6"), (1, "#BCCFF8")],
        "toggled": [(0, "#A8BFF5"), (0.5, "#B3C8F6"), (1, "#BCCFF8")],
    }[state]
    d, g = lingrad(stops)
    body = (f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 2.5)}" fill="{g}" stroke="#A7BEF4"/>'
            f'<path d="{rr(1.5, 1.5, w - 3, h - 3, 1.5)}" fill="none" stroke="#FFFFFF" stroke-opacity="0.6"/>')
    return img(w, h, body, d)


def scroll_grip(state, w=10, h=8):
    """Sürgü ortasındaki tutamak çizgileri (yatay sürgü için dikey çizgiler)."""
    body = ""
    for i in range(4):
        x = 1 + i * 2
        body += f'<rect x="{x}" y="0" width="1" height="{h - 1}" fill="#EEF4FE"/>'
        body += f'<rect x="{x + 1}" y="1" width="1" height="{h - 1}" fill="#8CB0F8"/>'
    return img(w, h, body)


# ----------------------------------------------------------- onay / radyo ---
def checkbox(kind, state):
    s = 13
    if state == "disabled":
        border, d, g = "#CAC8BB", *lingrad([(0, "#FFFFFF"), (1, "#FFFFFF")])
        mark = "#CAC8BB"
    elif state == "pressed":
        border = "#1C5180"
        d, g = lingrad([(0, "#B0B0A7"), (1, "#E3E1D2")], 0, 0, 1, 1)
        mark = "#21A121"
    else:
        border = "#1C5180"
        d, g = lingrad([(0, "#DCDCD7"), (1, "#FFFFFF")], 0, 0, 1, 1)
        mark = "#21A121"
    body = f'<rect x="0.5" y="0.5" width="{s - 1}" height="{s - 1}" fill="{g}" stroke="{border}"/>'
    if state == "focused":
        d2, g2 = lingrad([(0, "#FEDF9C"), (1, "#F8B636")], 0, 0, 1, 1)
        d += d2
        body += f'<rect x="2" y="2" width="{s - 4}" height="{s - 4}" fill="none" stroke="{g2}" stroke-width="2"/>'
    if kind == "checked":
        body += f'<path d="M3.2,6.2 L5.3,8.6 L9.8,3.6 L9.8,5.4 L5.3,10.4 L3.2,8.0 Z" fill="{mark}"/>'
    elif kind == "tristate":
        body += f'<rect x="3" y="3" width="7" height="7" fill="{mark}"/>'
    return img(s, s, body, d)


def radio(kind, state):
    s = 13
    if state == "disabled":
        border = "#CAC8BB"
        d, g = lingrad([(0, "#FFFFFF"), (1, "#FFFFFF")])
    elif state == "pressed":
        border = "#1C5180"
        d, g = lingrad([(0, "#B0B0A7"), (1, "#E3E1D2")], 0, 0, 1, 1)
    else:
        border = "#1C5180"
        d, g = lingrad([(0, "#DCDCD7"), (1, "#FFFFFF")], 0, 0, 1, 1)
    body = f'<circle cx="6.5" cy="6.5" r="6" fill="{g}" stroke="{border}"/>'
    if state == "focused":
        d2, g2 = lingrad([(0, "#FEDF9C"), (1, "#F8B636")], 0, 0, 1, 1)
        d += d2
        body += f'<circle cx="6.5" cy="6.5" r="4.4" fill="none" stroke="{g2}" stroke-width="2"/>'
    if kind == "checked":
        if state == "disabled":
            body += '<circle cx="6.5" cy="6.5" r="2.5" fill="#CAC8BB"/>'
        else:
            d3, g3 = radgrad([(0, "#62D562"), (0.6, "#21A121"), (1, "#0D7A0D")], 0.4, 0.35, 0.65)
            d += d3
            body += f'<circle cx="6.5" cy="6.5" r="2.8" fill="{g3}"/>'
    return img(s, s, body, d)


# ----------------------------------------------------------- ilerleme ---
def pb_gutter(state, w=24, h=16):
    d, g = lingrad([(0, "#FFFFFF"), (1, "#F2F2EE")])
    return img(w, h, f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="{g}" stroke="#686868"/>', d)


def pb_chunk(state, w=10, h=12):
    d, g = lingrad([(0, "#ACEDAD"), (0.15, "#7BE47D"), (0.5, "#2FC232"), (0.85, "#7BE47D"), (1, "#ACEDAD")])
    return img(w, h, f'<rect x="0" y="0" width="8" height="{h}" fill="{g}"/>', d)


# ----------------------------------------------------------- sekmeler ---
def tab(state, w=40, h=22):
    if state == "toggled":
        d, g = lingrad([(0, "#FFFFFF"), (1, "#FCFCFE")])
    elif state == "focused":
        d, g = lingrad([(0, "#FFFFFF"), (1, "#F4F3EE")])
    else:
        d, g = lingrad([(0, "#FFFFFF"), (0.8, "#F3F3EF"), (1, "#ECEBE5")])
    body = f'<path d="{rrect_path(0.5, 0.5, w - 1, h + 3, 3, 3, 0, 0)}" fill="{g}" stroke="#919B9C"/>'
    if state in ("toggled", "focused", "pressed"):
        body += (f'<path d="{rrect_path(0.5, 0.5, w - 1, 3, 3, 3, 0, 0)}" fill="#FFC73C" stroke="#E68B2C" '
                 f'stroke-width="1"/>')
    if state == "toggled":   # sekme sayfasıyla birleşsin: alt kenar beyaz
        body += f'<rect x="1" y="{h - 1}" width="{w - 2}" height="1" fill="#FCFCFE"/>'
    return img(w, h, body, d)


def tab_frame(state, w=24, h=24):
    return img(w, h, f'<rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="#FCFCFE" stroke="#919B9C"/>')


def group_box(state, w=24, h=24):
    return img(w, h, f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 3)}" fill="none" stroke="#D0D0BF"/>')


def header(state, w=24, h=20):
    d, g = lingrad([(0, "#FFFFFF"), (0.9, "#F4F3EE"), (1, "#EBEAE3")])
    body = f'<rect width="{w}" height="{h}" fill="{g}"/>'
    if state in ("focused",):
        body += f'<rect x="0" y="{h - 3}" width="{w}" height="3" fill="#F9B119"/>'
        body += f'<rect x="0" y="{h - 3}" width="{w}" height="1" fill="#F9D58D"/>'
    elif state in ("pressed", "toggled"):
        d, g = lingrad([(0, "#DEDFD8"), (1, "#E8E7E0")])
        body = f'<rect width="{w}" height="{h}" fill="{g}"/>'
    else:
        body += f'<rect x="0" y="{h - 3}" width="{w}" height="1" fill="#E2DED4"/>'
        body += f'<rect x="0" y="{h - 2}" width="{w}" height="1" fill="#DDD9CB"/>'
        body += f'<rect x="0" y="{h - 1}" width="{w}" height="1" fill="#D6D2C2"/>'
    body += (f'<rect x="{w - 2}" y="3" width="1" height="{h - 7}" fill="#C7C5B2"/>'
             f'<rect x="{w - 1}" y="3" width="1" height="{h - 7}" fill="#FFFFFF"/>')
    return img(w, h, body, d)


# ----------------------------------------------------------- menüler ---
def menu(state, w=24, h=24):
    return img(w, h, f'<rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="#FFFFFF" stroke="#ACA899"/>')


def menu_item(state, w=24, h=20):
    if state in ("focused", "pressed", "toggled"):
        return img(w, h, f'<rect width="{w}" height="{h}" fill="#316AC5"/>')
    return empty(w, h)


def menubar(state, w=24, h=20):
    return img(w, h, f'<rect width="{w}" height="{h}" fill="#ECE9D8"/>')


def tooltip(state, w=24, h=24):
    return img(w, h, f'<rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="#FFFFE1" stroke="#000000"/>')


def toolbar(state, w=24, h=24):
    d, g = lingrad([(0, "#FAF9F4"), (0.5, "#F1EFE6"), (1, "#E5E2D3")])
    body = (f'<rect width="{w}" height="{h}" fill="{g}"/>'
            f'<rect x="0" y="{h - 1}" width="{w}" height="1" fill="#D6D2C2"/>'
            f'<rect x="0" y="0" width="{w}" height="1" fill="#FFFFFF"/>')
    return img(w, h, body, d)


def item_view(state, w=24, h=20, inactive=False):
    if state in ("toggled", "pressed"):
        col = "#ECE9D8" if inactive else "#316AC5"
        return img(w, h, f'<rect width="{w}" height="{h}" fill="{col}"/>')
    if state == "focused":
        return img(w, h, f'<rect width="{w}" height="{h}" fill="#316AC5" fill-opacity="0.12"/>')
    return empty(w, h)


def focus_frame(w=8, h=8):
    body = ""
    for i in range(0, w, 2):
        body += f'<rect x="{i}" y="0" width="1" height="1" fill="#000"/><rect x="{i}" y="{h - 1}" width="1" height="1" fill="#000"/>'
    for j in range(0, h, 2):
        body += f'<rect x="0" y="{j}" width="1" height="1" fill="#000"/><rect x="{w - 1}" y="{j}" width="1" height="1" fill="#000"/>'
    return img(w, h, body)


def tree_box(kind, state):
    s = 9
    d, g = lingrad([(0, "#FFFFFF"), (1, "#C4BCA4")], 0, 0, 1, 1)
    body = f'<rect x="0.5" y="0.5" width="8" height="8" fill="{g}" stroke="#7A98AF"/>'
    body += '<rect x="2" y="4" width="5" height="1" fill="#000"/>'
    if kind == "plus":
        body += '<rect x="4" y="2" width="1" height="5" fill="#000"/>'
    return img(s, s, body, d)


def slider_groove(state, w=24, h=4):
    body = (f'<rect width="{w}" height="{h}" fill="#ECEBE4"/>'
            f'<rect width="{w}" height="1" fill="#9D9C99"/>'
            f'<rect y="{h - 1}" width="{w}" height="1" fill="#FFFFFF"/>'
            f'<rect x="0" width="1" height="{h}" fill="#9D9C99"/>'
            f'<rect x="{w - 1}" width="1" height="{h}" fill="#FFFFFF"/>')
    return img(w, h, body)


def slider_cursor(state):
    w, h = 21, 11
    if state == "disabled":
        d, g = lingrad([(0, "#F5F4EA"), (1, "#F5F4EA")])
        acc = "#D3D1C7"
    else:
        d, g = lingrad([(0, "#FFFFFF"), (1, "#E5E3DA")], 0, 0, 1, 0)
        acc = {"focused": "#F8B636", "pressed": "#F8B636"}.get(state, "#21C721")
    body = (f'<path d="{rr(0.5, 0.5, w - 1, h - 1, 2)}" fill="{g}" stroke="#8C8E8F"/>'
            f'<rect x="1" y="1.5" width="2" height="{h - 3}" fill="{acc}"/>'
            f'<rect x="{w - 3}" y="1.5" width="2" height="{h - 3}" fill="{acc}"/>')
    return img(w, h, body, d)


def size_grip():
    body = ""
    for (x, y) in [(9, 1), (9, 5), (5, 5), (9, 9), (5, 9), (1, 9)]:
        body += f'<rect x="{x + 1}" y="{y + 1}" width="2" height="2" fill="#FFFFFF"/><rect x="{x}" y="{y}" width="2" height="2" fill="#B9B5A4"/>'
    return img(12, 12, body)


def combo(state, w=30, h=21):
    """Düzenlenemez açılır kutu: beyaz alan + #7F9DB9 kenar."""
    border = "#C9C7BA" if state == "disabled" else "#7F9DB9"
    fill = "#F5F4EA" if state == "disabled" else "#FFFFFF"
    return img(w, h, f'<rect x="0.5" y="0.5" width="{w - 1}" height="{h - 1}" fill="{fill}" stroke="{border}"/>')


def main(out):
    d = Path(out) / NAME
    d.mkdir(parents=True, exist_ok=True)
    s = Sheet(scale=SCALE, width=900)

    for st in STATES:
        s.add_kv_frame("btn", st, push_button(st), 3, 3, 3, 3)
    s.add_kv_frame("btn", "default", push_button("normal", default=True), 3, 3, 3, 3)
    for st in STATES:
        s.add_kv_frame("tbaritem", st, tool_button(st), 3, 3, 3, 3)
        s.add_kv_frame("ledit", st, line_edit(st), 2, 2, 2, 2)
        s.add_kv_frame("combo", st, combo(st), 2, 2, 2, 2)
        s.add_kv_frame("common", st, generic_frame(st), 1, 1, 1, 1)
        s.add_kv_frame("ddown", st, blue_button(st, 17, 19), 3, 3, 3, 3)
        s.add_kv_frame("spinbtn", st, blue_button(st, 16, 11, 2), 2, 2, 2, 2)
        s.add_kv_frame("trough", st, trough(st), 1, 1, 1, 1)
        s.add_kv_frame("scrollslide", st, scroll_slider(st), 3, 3, 3, 3)
        s.add_kv_frame("pbgutter", st, pb_gutter(st), 3, 3, 3, 3)
        s.add_kv_frame("tab", st, tab(st), 4, 4, 4, 0)
        s.add_kv_frame("tbf", st, tab_frame(st), 1, 1, 1, 1)
        s.add_kv_frame("gbox", st, group_box(st), 4, 4, 4, 4)
        s.add_kv_frame("hdr", st, header(st), 1, 1, 2, 3)
        s.add_kv_frame("mnu", st, menu(st), 2, 2, 2, 2)
        s.add_kv_frame("menuitem", st, menu_item(st), 1, 1, 1, 1)
        s.add_kv_frame("mbaritem", st, menu_item(st), 1, 1, 1, 1)
        s.add_kv_frame("mubar", st, menubar(st), 1, 1, 1, 1)
        s.add_kv_frame("ttip", st, tooltip(st), 2, 2, 2, 2)
        s.add_kv_frame("tlbar", st, toolbar(st), 1, 1, 1, 1)
        s.add_kv_frame("iview", st, item_view(st), 1, 1, 1, 1)
        s.add_kv_frame("slider", st, slider_groove(st), 1, 1, 1, 1)
        if st in ("toggled", "pressed"):
            s.add_kv_frame("iview", st + "-inactive", item_view(st, inactive=True), 1, 1, 1, 1)
    # ilerleme çubuğu dolgu (desenli, sadece iç kısım)
    s.add_image("pbc-normal", pb_chunk("normal"), 10, 12)
    s.add_image("pbc-focused", pb_chunk("normal"), 10, 12)
    s.add_image("pbc-toggled", pb_chunk("normal"), 10, 12)
    s.newline()

    for st in ["normal", "focused", "pressed", "disabled"]:
        for kind in ["", "checked-", "tristate-"]:
            s.add_image(f"checkbox-{kind}{st}", checkbox(kind.rstrip("-") or "off", st), 13, 13)
        for kind in ["", "checked-"]:
            s.add_image(f"radio-{kind}{st}", radio(kind.rstrip("-") or "off", st), 13, 13)
        for direction in ["up", "down", "left", "right"]:
            s.add_image(f"scarrow-{direction}-{st}", scroll_arrow(direction, st), 17, 17)
            col = {"disabled": "#ACA899"}.get(st, "#000000")
            s.add_image(f"arrow-{direction}-{st}", img(9, 9, triangle(direction, col)), 9, 9)
            s.add_image(f"spind-{direction}-{st}",
                        img(9, 9, triangle(direction, "#C9C7BA" if st == "disabled" else "#4D6185")), 9, 9)
        s.add_image(f"spind-plus-{st}", img(9, 9, '<path d="M1,4h7v1h-7z M4,1h1v7h-1z" fill="#4D6185"/>'), 9, 9)
        s.add_image(f"spind-minus-{st}", img(9, 9, '<path d="M1,4h7v1h-7z" fill="#4D6185"/>'), 9, 9)
        s.add_image(f"scursor-{st}", slider_cursor(st), 21, 11)
        s.add_image(f"carrow-{st}", scroll_arrow("down", st), 17, 17)
        s.add_image(f"sgrip-{st}", scroll_grip(st), 10, 8)
        s.newline()
    s.add_image("carrow-toggled", scroll_arrow("down", "pressed"), 17, 17)
    for st in ["normal", "focused", "pressed"]:
        s.add_image(f"tree-plus-{st}", tree_box("plus", st), 9, 9)
        s.add_image(f"tree-minus-{st}", tree_box("minus", st), 9, 9)
    s.add_image("resize-grip-normal", size_grip(), 12, 12)
    s.newline()
    s.add_kv_frame("focus", "x", focus_frame(), 1, 1, 1, 1)
    # Kvantum odak çerçevesi "focus-top" vb. adlar bekler (durum yok)
    (d / f"{NAME}.svg").write_text(s.svg().replace('id="focus-x-', 'id="focus-').replace('id="focus-x"', 'id="focus"'))
    (d / f"{NAME}.kvconfig").write_text(KVCONFIG)
    print("kvantum ->", d)


KVCONFIG = r"""[%General]
author=XPThemePlasma
comment=Windows XP Luna (Mavi) — sıfırdan çizilmiş Kvantum teması
x11drag=menubar_and_primary_toolbar
alt_mnemonic=true
left_tabs=true
attach_active_tab=true
mirror_doc_tabs=true
group_toolbar_buttons=false
toolbar_item_spacing=1
toolbar_interior_spacing=2
spread_progressbar=true
progressbar_thickness=16
composite=true
menu_shadow_depth=4
tooltip_shadow_depth=0
splitter_width=5
scroll_width=17
scroll_min_extent=20
scroll_arrows=true
scrollbar_in_view=false
transient_scrollbar=false
slider_width=4
slider_handle_width=21
slider_handle_length=11
center_toolbar_handle=true
check_size=13
textless_progressbar=false
menubar_mouse_tracking=true
toolbutton_style=0
double_click=false
translucent_windows=false
blurring=false
popup_blurring=false
vertical_spin_indicators=true
inline_spin_indicators=false
spin_button_width=16
fill_rubberband=false
merge_menubar_with_toolbar=false
small_icon_size=16
large_icon_size=32
button_icon_size=16
toolbar_icon_size=24
combo_as_lineedit=true
animate_states=false
button_contents_shift=true
combo_menu=true
hide_combo_checkboxes=true
combo_focus_rect=false
groupbox_top_label=false
inline_spin_separator=false
square_combo_button=true
tree_branch_line=true
no_inactive_tab_expansion=true
tab_button_extra_margin=0
layout_spacing=6
layout_margin=7
submenu_overlap=2
submenu_delay=250
tooltip_delay=500
scrollable_menu=true
contrast=1.00
intensity=1.00
saturation=1.00
dialog_button_layout=0
respect_DE=true
menu_separator_height=7
shadowless_popup=false
reduce_menu_opacity=0
reduce_window_opacity=0
no_window_pattern=false
drag_from_buttons=false
opaque=kaffeine,kmplayer,subtitlecomposer,kdenlive,vlc,smplayer,smplayer2,avidemux,avidemux2_qt4,avidemux3_qt4,avidemux3_qt5,kamoso,QtCreator,VirtualBox,VirtualBoxVM,trojita,dragon,digikam,lyx

[GeneralColors]
window.color=#ECE9D8
base.color=#FFFFFF
alt.base.color=#F7F7F2
button.color=#ECE9D8
light.color=#FFFFFF
mid.light.color=#F1EFE2
dark.color=#ACA899
mid.color=#D4D0C8
highlight.color=#316AC5
inactive.highlight.color=#ECE9D8
text.color=#000000
window.text.color=#000000
button.text.color=#000000
disabled.text.color=#ACA899
tooltip.text.color=#000000
highlight.text.color=#FFFFFF
inactive.highlight.text.color=#000000
link.color=#0000FF
link.visited.color=#800080
progress.indicator.text.color=#000000
progress.inactive.indicator.text.color=#000000

[Hacks]
transparent_dolphin_view=false
blur_konsole=false
transparent_ktitle_label=true
transparent_menutitle=true
respect_darkness=true
kcapacitybar_as_progressbar=true
force_size_grip=false
iconless_pushbutton=true
iconless_menu=false
disabled_icon_opacity=60
lxqtmainmenu_iconsize=16
normal_default_pushbutton=false
single_top_toolbar=false
tint_on_mouseover=0
no_selection_tint=true
transparent_arrow_button=false
middle_click_scroll=false
centered_forms=false
kinetic_scrolling=false
noninteger_translucency=false
style_vertical_toolbars=false

[PanelButtonCommand]
frame=true
frame.element=btn
frame.top=3
frame.bottom=3
frame.left=3
frame.right=3
interior=true
interior.element=btn
indicator.size=9
text.normal.color=#000000
text.focus.color=#000000
text.press.color=#000000
text.toggle.color=#000000
text.disabled.color=#ACA899
text.shadow=0
text.margin=1
text.iconspacing=4
min_width=+2font
min_height=+0.6font

[PanelButtonTool]
inherits=PanelButtonCommand
frame.element=tbaritem
interior.element=tbaritem
frame.top=3
frame.bottom=3
frame.left=3
frame.right=3
text.margin.left=2
text.margin.right=2
min_width=0
min_height=0

[ToolbarButton]
inherits=PanelButtonTool
text.margin.top=2
text.margin.bottom=2
text.margin.left=3
text.margin.right=3

[Dock]
inherits=PanelButtonCommand
interior.element=mubar
frame.element=mubar
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1

[DockTitle]
inherits=PanelButtonCommand
frame=false
interior=false
text.normal.color=#000000
text.focus.color=#000000
text.bold=true

[IndicatorSpinBox]
frame=true
interior=true
frame.element=spinbtn
interior.element=spinbtn
frame.top=2
frame.bottom=2
frame.left=2
frame.right=2
indicator.element=spind
indicator.size=9

[RadioButton]
inherits=PanelButtonCommand
frame=false
interior=true
interior.element=radio
text.normal.color=#000000
text.focus.color=#000000
min_width=0
min_height=0

[CheckBox]
inherits=PanelButtonCommand
frame=false
interior=true
interior.element=checkbox
text.normal.color=#000000
text.focus.color=#000000
min_width=0
min_height=0

[Focus]
frame=true
frame.element=focus
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1
frame.patternsize=8

[GenericFrame]
frame=true
interior=false
frame.element=common
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1

[LineEdit]
frame=true
frame.element=ledit
interior=true
interior.element=ledit
frame.top=2
frame.bottom=2
frame.left=2
frame.right=2
text.margin.left=2
text.margin.right=2
indicator.element=carrow
indicator.size=17

[ToolbarLineEdit]
inherits=LineEdit

[DropDownButton]
inherits=PanelButtonCommand
indicator.element=arrow-down
indicator.size=9

[IndicatorArrow]
indicator.element=arrow
indicator.size=9

[ToolboxTab]
inherits=PanelButtonCommand
text.normal.color=#000000
text.press.color=#000000
text.focus.color=#000000

[Tab]
inherits=PanelButtonCommand
frame.element=tab
interior.element=tab
frame.top=4
frame.bottom=0
frame.left=4
frame.right=4
text.margin.left=8
text.margin.right=8
text.margin.top=2
text.margin.bottom=2
indicator.element=tab
indicator.size=12
focusFrame=false
min_width=+1font
min_height=+0.4font

[TabFrame]
frame=true
frame.element=tbf
interior=true
interior.element=tbf
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1

[TabBarFrame]
frame=false
interior=false

[TreeExpander]
indicator.size=9
indicator.element=tree

[HeaderSection]
inherits=PanelButtonCommand
frame.element=hdr
interior.element=hdr
frame.top=1
frame.bottom=3
frame.left=1
frame.right=2
text.margin.left=4
text.margin.right=4
text.margin.top=1
text.margin.bottom=1
indicator.element=arrow
indicator.size=9
frame.expansion=0

[SizeGrip]
indicator.element=resize-grip
indicator.size=12

[Toolbar]
inherits=PanelButtonCommand
frame=false
interior=true
interior.element=tlbar
frame.element=tlbar
indicator.element=tbar
indicator.size=8
text.margin=0
frame.expansion=0

[Slider]
frame=true
interior=true
frame.element=slider
interior.element=slider
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1
frame.expansion=0

[SliderCursor]
frame=false
interior=true
interior.element=scursor

[Progressbar]
frame=true
interior=true
frame.element=pbgutter
interior.element=pbgutter
frame.top=3
frame.bottom=3
frame.left=3
frame.right=3
text.normal.color=#000000
text.focus.color=#000000
text.bold=false
frame.expansion=0

[ProgressbarContents]
frame=false
interior=true
interior.element=pbc
interior.x.patternsize=10
frame.expansion=0

[ItemView]
frame=true
interior=true
frame.element=iview
interior.element=iview
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1
text.normal.color=#000000
text.focus.color=#000000
text.press.color=#FFFFFF
text.toggle.color=#FFFFFF
text.toggle.inactive.color=#000000
text.press.inactive.color=#000000
text.margin.left=2
text.margin.right=2
min_width=+0.3font
min_height=+0.3font
frame.expansion=0

[Splitter]
indicator.size=0
frame=false
interior=false

[Scrollbar]
indicator.element=scarrow
indicator.size=17
frame=false
interior=false

[ScrollbarGroove]
frame=true
interior=true
frame.element=trough
interior.element=trough
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1
frame.expansion=0

[ScrollbarSlider]
frame=true
interior=true
frame.element=scrollslide
interior.element=scrollslide
frame.top=3
frame.bottom=3
frame.left=3
frame.right=3
frame.expansion=0
indicator.element=sgrip
indicator.size=8

[Menu]
frame=true
interior=true
frame.element=mnu
interior.element=mnu
frame.top=2
frame.bottom=2
frame.left=2
frame.right=2
frame.expansion=0
text.normal.color=#000000
text.shadow=0

[MenuItem]
frame=true
interior=true
frame.element=menuitem
interior.element=menuitem
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1
indicator.element=arrow
indicator.size=9
text.normal.color=#000000
text.focus.color=#FFFFFF
text.press.color=#FFFFFF
text.toggle.color=#FFFFFF
text.margin.top=2
text.margin.bottom=2
text.margin.left=4
text.margin.right=4
text.shadow=0
frame.expansion=0
min_width=+0.3font
min_height=+0.6font

[MenuBar]
frame=false
interior=true
interior.element=mubar
frame.element=mubar
frame.expansion=0

[MenuBarItem]
frame=true
interior=true
frame.element=mbaritem
interior.element=mbaritem
frame.top=1
frame.bottom=1
frame.left=1
frame.right=1
text.normal.color=#000000
text.focus.color=#FFFFFF
text.press.color=#FFFFFF
text.toggle.color=#FFFFFF
text.margin.top=2
text.margin.bottom=2
text.margin.left=5
text.margin.right=5
frame.expansion=0

[TitleBar]
inherits=PanelButtonCommand
frame=false
interior=false
text.margin.top=2
text.margin.bottom=2
text.margin.left=4
text.margin.right=4
text.bold=true

[ComboBox]
inherits=PanelButtonCommand
frame.element=combo
interior.element=combo
frame.top=2
frame.bottom=2
frame.left=2
frame.right=2
text.margin.left=3
text.margin.right=2
text.focus.color=#000000
text.press.color=#000000
text.toggle.color=#000000
indicator.element=carrow
indicator.size=17

[ToolTip]
frame=true
interior=true
frame.element=ttip
interior.element=ttip
frame.top=2
frame.bottom=2
frame.left=2
frame.right=2
text.normal.color=#000000
text.margin.top=2
text.margin.bottom=2
text.margin.left=3
text.margin.right=3
frame.expansion=0

[GroupBox]
frame=true
interior=false
frame.element=gbox
frame.top=4
frame.bottom=4
frame.left=4
frame.right=4
text.normal.color=#0046D5
text.focus.color=#0046D5
text.bold=false
frame.expansion=0

[Window]
interior=false
frame=false

[WindowTranslucent]
inherits=Window
"""


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "build/kvantum")
