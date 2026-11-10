#!/usr/bin/env bash
# XPThemePlasma kurulum betiği
#
#   bash kur.sh            görselleri üret + kullanıcıya kur (~/.local, ~/.config) — sudo gerekmez
#   sudo bash kur.sh --sistem   SDDM oturum girdisini (/usr/share/wayland-sessions) ve
#                               /usr/local/bin/startxp-wayland'ı kur
#   bash kur.sh --kaldir   kullanıcı dosyalarını kaldır (önceki tema geri yüklenir)
set -euo pipefail

KOK="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$KOK"

if [[ "${1:-}" == "--sistem" ]]; then
    if [[ $EUID -ne 0 ]]; then
        echo "Bu kısım root ister:  sudo bash $0 --sistem" >&2
        exit 1
    fi
    install -Dm755 session/startxp-wayland /usr/local/bin/startxp-wayland
    install -Dm644 session/xpthemeplasma.desktop /usr/share/wayland-sessions/xpthemeplasma.desktop
    echo "Oturum kuruldu: SDDM'de 'XPThemePlasma (Wayland)' seçilebilir."
    exit 0
fi

DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
CFG="${XDG_CONFIG_HOME:-$HOME/.config}"
BIN="$HOME/.local/bin"

if [[ "${1:-}" == "--kaldir" ]]; then
    [[ -x "$BIN/xp-profil" ]] && "$BIN/xp-profil" geri || true
    rm -rf "$DATA/aurorae/themes/XP-Luna" "$CFG/Kvantum/XPLuna" \
           "$DATA/plasma/desktoptheme/XP-Luna" \
           "$DATA/plasma/plasmoids/io.github.yyozcan.xpstart" \
           "$DATA/plasma/plasmoids/io.github.yyozcan.xppanel" \
           "$DATA/plasma/shells/io.github.yyozcan.xpshell" \
           "$DATA/plasma/layout-templates/io.github.yyozcan.xptaskbar" \
           "$DATA/plasma/look-and-feel/io.github.yyozcan.xpluna" \
           "$DATA/color-schemes/XPLuna.colors" "$DATA/wallpapers/XPThemePlasma" \
           "$DATA/locale/tr/LC_MESSAGES/plasma_applet_io.github.yyozcan.xpstart.mo" \
           "$DATA/locale/tr/LC_MESSAGES/xpthemeplasma.mo" \
           "$CFG/plasma-workspace/env/xpthemeplasma.sh" "$BIN/xp-profil"
    echo "Kullanıcı dosyaları kaldırıldı. Sistem kısmı için:"
    echo "  sudo rm /usr/local/bin/startxp-wayland /usr/share/wayland-sessions/xpthemeplasma.desktop"
    exit 0
fi

PY=python3
echo "==> Görseller üretiliyor"
$PY tools/gen_aurorae.py build/aurorae
$PY tools/gen_kvantum.py build/kvantum
$PY tools/gen_desktoptheme.py build/desktoptheme
[[ -f build/wallpapers/XPThemePlasma/metadata.json ]] || $PY tools/gen_wallpaper.py build/wallpapers/XPThemePlasma

kopyala() {  # kopyala <kaynak> <hedef-dizin>
    rm -rf "$2"
    mkdir -p "$(dirname "$2")"
    cp -r "$1" "$2"
}

echo "==> Kuruluyor ($DATA, $CFG)"
kopyala build/aurorae/XP-Luna "$DATA/aurorae/themes/XP-Luna"
kopyala build/kvantum/XPLuna "$CFG/Kvantum/XPLuna"
kopyala build/desktoptheme/XP-Luna "$DATA/plasma/desktoptheme/XP-Luna"
for p in plasma/plasmoids/*/; do
    kopyala "$p" "$DATA/plasma/plasmoids/$(basename "$p")"
done
kopyala plasma/shells/io.github.yyozcan.xpshell "$DATA/plasma/shells/io.github.yyozcan.xpshell"
kopyala plasma/layout-templates/io.github.yyozcan.xptaskbar "$DATA/plasma/layout-templates/io.github.yyozcan.xptaskbar"
kopyala plasma/look-and-feel/io.github.yyozcan.xpluna "$DATA/plasma/look-and-feel/io.github.yyozcan.xpluna"
install -Dm644 plasma/color_scheme/XPLuna.colors "$DATA/color-schemes/XPLuna.colors"
kopyala build/wallpapers/XPThemePlasma "$DATA/wallpapers/XPThemePlasma"

echo "==> Türkçe çeviriler"
for po in po/tr/*.po; do
    mkdir -p "$DATA/locale/tr/LC_MESSAGES"
    msgfmt -o "$DATA/locale/tr/LC_MESSAGES/$(basename "${po%.po}").mo" "$po"
done

echo "==> Oturum yardımcıları"
install -Dm755 session/xp-profil "$BIN/xp-profil"
install -Dm644 session/xpthemeplasma-env.sh "$CFG/plasma-workspace/env/xpthemeplasma.sh"

# Plasma'nın paket önbelleğini tazele
rm -rf "${XDG_CACHE_HOME:-$HOME/.cache}"/plasma_theme_XP-Luna* 2>/dev/null || true

echo
echo "Kullanıcı kurulumu tamam."
if [[ ! -f /usr/share/wayland-sessions/xpthemeplasma.desktop ]]; then
    echo "Oturum girdisi için bir kez çalıştır:  sudo bash $KOK/kur.sh --sistem"
fi
echo "Sonra oturumu kapatıp SDDM'de 'XPThemePlasma (Wayland)' seç."
