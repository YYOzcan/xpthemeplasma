#!/usr/bin/env python3
"""Bliss'ten esinlenmiş özgün, prosedürel duvar kağıdı: yeşil tepe + mavi gök + bulutlar.

Fotoğraf kullanılmaz; her şey gürültü fonksiyonları ve degradelerle üretilir.
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

W, H = 1920, 1080
rng = np.random.default_rng(2001)  # XP'nin çıkış yılı :)


def smooth_noise(w, h, scale, rng):
    """Düşük çözünürlüklü rastgele ızgarayı büyütüp yumuşatarak değer gürültüsü."""
    gw, gh = max(2, int(w / scale)), max(2, int(h / scale))
    small = rng.random((gh, gw)).astype(np.float32)
    img = Image.fromarray((small * 255).astype(np.uint8), "L").resize((w, h), Image.BICUBIC)
    return np.asarray(img, dtype=np.float32) / 255.0


def fbm(w, h, base_scale, octaves, rng, gain=0.5):
    total = np.zeros((h, w), np.float32)
    amp, norm, scale = 1.0, 0.0, base_scale
    for _ in range(octaves):
        total += amp * smooth_noise(w, h, scale, rng)
        norm += amp
        amp *= gain
        scale /= 2.0
    return total / norm


def lerp(a, b, t):
    return a + (b - a) * t[..., None]


def hex2rgb(h):
    h = h.lstrip("#")
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)], np.float32)


def main(out):
    y = np.linspace(0, 1, H, dtype=np.float32)[:, None] * np.ones((1, W), np.float32)
    x = np.linspace(0, 1, W, dtype=np.float32)[None, :] * np.ones((H, 1), np.float32)

    # --- gökyüzü: üstte koyu mavi, ufka doğru açık
    top, mid, horizon = hex2rgb("#1F5FD0"), hex2rgb("#4A8FE6"), hex2rgb("#B5D6F5")
    t = np.clip(y / 0.62, 0, 1)
    sky = np.where((t < 0.55)[..., None], lerp(top, mid, t / 0.55),
                   lerp(mid, horizon, (t - 0.55) / 0.45))

    # --- bulutlar: fbm eşiklenmiş, yatayda uzatılmış
    n = fbm(W, H, 420, 6, rng)
    stretch = Image.fromarray((n * 255).astype(np.uint8), "L").resize((W * 2, H)).crop((W // 3, 0, W // 3 + W, H))
    n = np.asarray(stretch, np.float32) / 255.0
    band = np.exp(-((y - 0.27) / 0.24) ** 2)            # bulutlar gökyüzünün ortasında yoğun
    cloud = np.clip((n - 0.45) * 2.8, 0, 1) * band
    cloud = np.asarray(Image.fromarray((cloud * 255).astype(np.uint8), "L")
                       .filter(ImageFilter.GaussianBlur(6)), np.float32) / 255.0
    shade = np.clip(1.0 - (n - 0.5) * 0.6, 0.75, 1.0)   # bulut altları hafif gri-mavi
    cloud_col = np.ones_like(sky) * 255 * shade[..., None] + (1 - shade[..., None]) * hex2rgb("#B8CCE8")
    img = lerp(sky, cloud_col, np.clip(cloud * 0.95, 0, 1))

    # --- tepe çizgisi: geniş, yumuşak bir tepe + sağda alçak ikinci tepe
    xs = np.linspace(0, 1, W, dtype=np.float32)
    ridge = (0.70
             - 0.20 * np.exp(-((xs - 0.38) / 0.33) ** 2)
             - 0.07 * np.exp(-((xs - 0.92) / 0.18) ** 2)
             + 0.012 * np.sin(xs * 9.0))
    ridge_px = ridge * H
    yy = np.arange(H, dtype=np.float32)[:, None]
    # kenar yumuşatma (1.5 px)
    hill_mask = np.clip((yy - ridge_px[None, :]) / 1.5 + 0.5, 0, 1)

    # --- çimen: tepede parlak sarımsı yeşil, aşağı ve sağa doğru koyu
    depth = np.clip((yy - ridge_px[None, :]) / (H - ridge_px[None, :] + 1), 0, 1)
    light = np.clip(1.0 - depth * 1.1 + (0.5 - x) * 0.35, 0, 1)
    grass_dark, grass_mid, grass_light = hex2rgb("#2A6A12"), hex2rgb("#4F9A1E"), hex2rgb("#8CC63A")
    grass = np.where((light < 0.5)[..., None], lerp(grass_dark, grass_mid, light / 0.5),
                     lerp(grass_mid, grass_light, (light - 0.5) / 0.5))
    # çim dokusu: ince, yatay uzamış gürültü
    tex = fbm(W, H, 40, 4, rng)
    tex2 = fbm(W, H, 180, 3, rng)
    grass *= (0.88 + 0.18 * tex[..., None]) * (0.9 + 0.15 * tex2[..., None])
    # tepe sırtında ışık bandı
    rim = np.exp(-((yy - ridge_px[None, :]) / 18.0) ** 2) * hill_mask
    grass = grass + rim[..., None] * np.array([28, 30, 6], np.float32)

    img = lerp(img, grass, hill_mask)

    # --- hafif atmosfer ve köşe kararması
    haze = np.exp(-((yy - ridge_px[None, :]) / 60.0) ** 2) * (1 - hill_mask) * 0.25
    img = lerp(img, np.ones_like(img) * hex2rgb("#DCEBFA"), haze)
    vig = 1.0 - 0.18 * (((x - 0.5) / 0.75) ** 2 + ((y - 0.45) / 0.9) ** 2)
    img *= vig[..., None]

    # Plasma duvar kağıdı paketi: metadata.json + contents/images/<WxH>.png
    out = Path(out)
    (out / "contents/images").mkdir(parents=True, exist_ok=True)
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), "RGB")
    im.save(out / f"contents/images/{W}x{H}.png", optimize=True)
    im.resize((400, 225), Image.LANCZOS).save(out / "contents/screenshot.png")
    (out / "metadata.json").write_text(json.dumps({
        "KPlugin": {"Id": "XPThemePlasma", "Name": "Tepe", "Name[tr]": "Tepe",
                    "Description": "Bliss'ten esinlenmiş özgün, prosedürel duvar kağıdı",
                    "License": "CC-BY-SA-4.0", "Authors": [{"Name": "XPThemePlasma"}]}},
        indent=2, ensure_ascii=False))
    print("duvar kağıdı ->", out)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "build/wallpapers/XPThemePlasma")
