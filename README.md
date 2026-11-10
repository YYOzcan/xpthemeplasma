# XPThemePlasma

KDE Plasma 6 için **Windows XP Luna (Mavi)** görünümü. [AeroThemePlasma](https://gitgud.io/wackyideas/aerothemeplasma)'nın
Windows 7 için yaptığını, Windows XP için yapar ve ATP ile **yan yana** çalışır: SDDM'de
"XPThemePlasma (Wayland)" oturumunu seçersin, çıkınca ATP olduğu gibi geri gelir.

> Hiçbir Microsoft dosyası (msstyles, bitmap, Bliss fotoğrafı, ses, logo) kullanılmaz.
> Bütün görseller `tools/` altındaki Python betikleriyle Luna paletine göre **sıfırdan çizilir**.

## İçerik

| Bileşen | Ne yapar | Kaynak |
|---|---|---|
| **XP-Luna** (Aurorae) | Mavi yuvarlak köşeli başlık çubuğu, turuncu kapat / mavi küçült-büyüt düğmeleri | `tools/gen_aurorae.py` |
| **XPLuna** (Kvantum) | Uygulama içleri: XP düğmeleri (turuncu üzerine gelme halkası), yeşil onay işaretleri, mavi kaydırma çubukları, yeşil bloklu ilerleme çubuğu, turuncu çizgili sekmeler, beyaz menüler | `tools/gen_kvantum.py` |
| **XP-Luna** (Plasma teması) | Mavi görev çubuğu, XP görev düğmeleri, sarı araç ipuçları, açılır pencereler | `tools/gen_desktoptheme.py` |
| **XP Başlat** (plasmoid) | Yeşil "başlat" düğmesi; iki sütunlu menü (kullanıcı resmi, sabitlenenler + sık kullanılanlar, Belgelerim / Bilgisayarım / Denetim Masası…, Oturumu Kapat / Bilgisayarı Kapat). "Tüm Programlar" gerçek kademeli menüler açar | `plasma/plasmoids/io.github.yyozcan.xpstart` |
| **XP Paneli** (kapsayıcı) | ATP panelinin çatalı: bildirim alanının arkasına XP'nin açık mavi tepsisini çizer, panel yazılarını beyaz yapar | `plasma/plasmoids/io.github.yyozcan.xppanel` |
| **XP Kabuğu** + yerleşim | İlk açılışta: başlat · görevler · tepsi · saat (Tahoma 8) | `plasma/shells`, `plasma/layout-templates` |
| **XP Luna** (Görünüm ve His) | XP açılış ekranı (siyah zemin, kayan üç mavi blok) | `plasma/look-and-feel` |
| **Tepe** duvar kağıdı | Bliss'ten esinlenmiş, prosedürel yeşil tepe + mavi gök | `tools/gen_wallpaper.py` |
| **XPLuna** renk şeması | `#ECE9D8` yüzey, `#316AC5` seçim | `plasma/color_scheme` |
| Türkçe | Başlat, Tüm Programlar, Belgelerim, Bilgisayarım, Denetim Masası… (XP'nin Türkçe terimleri) | `po/tr` |

## Kurulum

```bash
bash kur.sh                    # görselleri üretir, ~/.local ve ~/.config altına kurar (sudo gerekmez)
sudo bash kur.sh --sistem      # SDDM oturum girdisi + /usr/local/bin/startxp-wayland
```

Sonra oturumu kapat, SDDM'de **XPThemePlasma (Wayland)** seç.

Gerekenler: Plasma 6.8, `kvantum`, `python3` + `python-pillow` + `python-numpy`, `rsvg-convert` (librsvg), `msgfmt` (gettext),
Tahoma / Trebuchet MS / Franklin Gothic yazı tipleri (ör. `ttf-ms-win11-auto`).

## ATP ile birlikte nasıl çalışır?

KDE ayarları (`kdeglobals`, `kwinrc`, `plasmarc`…) iki oturumda ortaktır. Bu yüzden:

* `~/.config/plasma-workspace/env/xpthemeplasma.sh` her Plasma oturumunun başında çalışır.
* XP oturumunda `xp-profil xp`: yönetilen tema anahtarlarının (renk şeması, Kvantum teması, pencere çerçevesi,
  Plasma teması, yazı tipleri, ATP'nin cam/parıltı efektleri…) mevcut değerlerini
  `~/.local/state/xpthemeplasma/onceki.json`'a kaydeder ve XP değerlerini yazar.
* XP'den çıkınca (ya da XP oturumu çökse bile bir sonraki ATP girişinde) `xp-profil geri`: XP ayarlarını
  `xp.json`'a saklar, önceki değerleri birebir geri yükler.
* Panel düzeni ayrıdır: XP kabuğu kendi `plasma-io.github.yyozcan.xpshell-appletsrc` dosyasını kullanır.

Elle: `xp-profil durum`, `xp-profil geri`.

## Kaldırma

```bash
bash kur.sh --kaldir
sudo rm /usr/local/bin/startxp-wayland /usr/share/wayland-sessions/xpthemeplasma.desktop
```

## Geliştirme

`tools/luna.py` ortak çekirdek: SVG çizer → `rsvg-convert` ile rasterlar → 9 parçaya böler → Kvantum / Plasma /
Aurorae'nin beklediği eleman kimlikleriyle tek SVG'ye gömer (2x çözünürlük, HiDPI'da da keskin).
Renkleri değiştirmek için `luna.py` içindeki degrade listelerini düzenleyip `bash kur.sh` çalıştırmak yeter.

## Lisans

GPL-3.0-or-later. Çatallanan parçalar (XP Paneli ← ATP/KDE paneli, ActionMenu/tools.js ← Kicker/SevenStart)
kendi GPL-2.0-or-later başlıklarını korur.
