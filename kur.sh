#!/usr/bin/env bash
# XPThemePlasma kurulum betiği
#
#   bash kur.sh            görselleri üret + kullanıcıya kur (~/.local, ~/.config) — sudo gerekmez
#   sudo bash kur.sh --sistem   SDDM oturum girdisini (/usr/share/wayland-sessions) ve
#                               /usr/local/bin/startxp-wayland'ı kur
#   bash kur.sh --xp "/yol/Windows XP.iso"   kendi XP kopyandan gerçek varlıkları içe aktar ve kur
#   bash kur.sh --kaldir   kullanıcı dosyalarını kaldır (önceki tema geri yüklenir)
set -euo pipefail

KOK="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$KOK"

if [[ "${1:-}" == "--sddm-xp" ]]; then
    [[ $EUID -eq 0 ]] || { echo "sudo bash $0 --sddm-xp" >&2; exit 1; }
    [[ -d /usr/share/sddm/themes/xpthemeplasma ]] || { echo "Önce: sudo bash $0 --sistem" >&2; exit 1; }
    printf '[Theme]\nCurrent=xpthemeplasma\n' > /etc/sddm.conf.d/zz-xpthemeplasma.conf
    echo "SDDM teması: xpthemeplasma"
    exit 0
fi

if [[ "${1:-}" == "--plymouth" || "${1:-}" == "--plymouth-geri" ]]; then
    [[ $EUID -eq 0 ]] || { echo "sudo bash $0 ${1}" >&2; exit 1; }
    if [[ "$1" == "--plymouth-geri" ]]; then
        onceki=$(cat /usr/share/plymouth/themes/xpthemeplasma/.onceki 2>/dev/null || echo bgrt)
        systemctl disable xp-boot-bekle.service 2>/dev/null || true
        rm -f /etc/systemd/system/xp-boot-bekle.service
        sed -i '/^# XPThemePlasma$/d; /^UseSimpledrm=1$/d' /etc/plymouth/plymouthd.conf
        plymouth-set-default-theme -R "$onceki"
        echo "Plymouth teması geri alındı: $onceki"
        exit 0
    fi
    [[ -f build/plymouth/xpthemeplasma/boot.png ]] || { echo "Önce kullanıcı olarak: bash $0 --xp <ISO>" >&2; exit 1; }
    onceki=$(plymouth-set-default-theme 2>/dev/null || true)
    rm -rf /usr/share/plymouth/themes/xpthemeplasma
    cp -r build/plymouth/xpthemeplasma /usr/share/plymouth/themes/xpthemeplasma
    [[ "$onceki" != "xpthemeplasma" && -n "$onceki" ]] && echo "$onceki" > /usr/share/plymouth/themes/xpthemeplasma/.onceki
    install -Dm755 plymouth/xp-boot-bekle /usr/local/lib/xpthemeplasma/xp-boot-bekle
    install -Dm644 plymouth/xp-boot-bekle.service /etc/systemd/system/xp-boot-bekle.service
    systemctl daemon-reload
    systemctl enable xp-boot-bekle.service
    # Plymouth 24+ simpledrm'i (UEFI çerçeve arabelleği) DeviceTimeout dolana dek yok sayar ve
    # NVIDIA/AMD sürücüsünü bekler; o yüzden XP ekranı açılışın sonunda yalnızca 2-3 sn görünüyordu.
    # UseSimpledrm=1: çizim hemen simpledrm'de başlar, sürücü gelince Plymouth ona geçer.
    plymouth-set-default-theme xpthemeplasma
    conf=/etc/plymouth/plymouthd.conf
    grep -q '^\[Daemon\]' "$conf" 2>/dev/null || printf '[Daemon]\nTheme=xpthemeplasma\n' >> "$conf"
    if ! grep -q '^UseSimpledrm=' "$conf"; then
        sed -i '0,/^\[Daemon\]/s//[Daemon]\n# XPThemePlasma\nUseSimpledrm=1/' "$conf"
    fi
    echo "==> initramfs yeniden oluşturuluyor (birkaç dakika sürebilir)"
    plymouth-set-default-theme -R xpthemeplasma
    echo "Plymouth: XP açılış ekranı (en az 10 sn görünür) ve kapanışta \"Windows kapatılıyor...\"."
    echo "Geri almak: sudo bash $0 --plymouth-geri"
    exit 0
fi

if [[ "${1:-}" == "--sistem" ]]; then
    if [[ $EUID -ne 0 ]]; then
        echo "Bu kısım root ister:  sudo bash $0 --sistem" >&2
        exit 1
    fi
    install -Dm755 session/startxp-wayland /usr/local/bin/startxp-wayland
    install -Dm644 session/xpthemeplasma.desktop /usr/share/wayland-sessions/xpthemeplasma.desktop
    rm -rf /usr/share/sddm/themes/xpthemeplasma
    cp -r sddm/xpthemeplasma /usr/share/sddm/themes/xpthemeplasma
    # kendi XP kopyandan içe aktarılan logonui bitmap'leri (önce kullanıcı olarak: bash kur.sh)
    if ls build/varlik/logon-*.png >/dev/null 2>&1; then
        mkdir -p /usr/share/sddm/themes/xpthemeplasma/xp
        cp build/varlik/logon-*.png /usr/share/sddm/themes/xpthemeplasma/xp/
    fi
    chmod -R a+rX /usr/share/sddm/themes/xpthemeplasma
    echo "Oturum kuruldu: SDDM'de 'XPThemePlasma (Wayland)' seçilebilir."
    echo "XP karşılama ekranı kuruldu (etkin değil). Etkinleştirmek için:"
    echo "  sudo bash $0 --sddm-xp      (geri almak: sudo rm /etc/sddm.conf.d/zz-xpthemeplasma.conf)"
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
           "$DATA/color-schemes/XPLuna.colors" "$DATA/wallpapers/XPThemePlasma" "$DATA"/wallpapers/XP-* \
           "$DATA/locale/tr/LC_MESSAGES/plasma_applet_io.github.yyozcan.xpstart.mo" \
           "$DATA/locale/tr/LC_MESSAGES/xpthemeplasma.mo" \
           "$CFG/plasma-workspace/env/xpthemeplasma.sh" "$BIN/xp-profil" "$BIN/xp-oturum" "$BIN/xp-giris-sesi" "$DATA/xpthemeplasma" \
           "$DATA/icons/Windows XP" "$DATA/sounds/Windows XP" "$DATA/fonts/xpthemeplasma" \
           "$DATA/konsole/XP Komut İstemi.profile" "$DATA/konsole/XPCmd.colorscheme" \
           "$DATA/kwin/tabbox/xp_alttab" "$DATA/kwin/effects/xp_gri" "$BIN/xp-notdefteri" "$BIN/xp-gezgin" "$BIN/xp-denetim"
    sed -i '/>>> XPThemePlasma XP_CMD >>>/,/<<< XPThemePlasma XP_CMD <<</d' "$HOME/.bashrc"
    echo "Kullanıcı dosyaları kaldırıldı. Sistem kısmı için:"
    echo "  sudo rm -rf /usr/local/bin/startxp-wayland /usr/share/wayland-sessions/xpthemeplasma.desktop \\"
    echo "              /usr/share/sddm/themes/xpthemeplasma /etc/sddm.conf.d/zz-xpthemeplasma.conf"
    exit 0
fi

PY="python3 -I"
XPD="$DATA/xpthemeplasma/xp"
if [[ "${1:-}" == "--xp" ]]; then
    [[ -n "${2:-}" ]] || { echo "kullanım: bash $0 --xp <XP ISO | I386 | WINDOWS klasörü>" >&2; exit 1; }
    echo "==> XP varlıkları içe aktarılıyor"
    $PY tools/xp_iceaktar.py "$2"
fi

rm -rf build/aurorae build/kvantum build/desktoptheme build/varlik
if [[ -f "$XPD/stil/luna.msstyles" ]]; then
    echo "==> Görseller gerçek XP varlıklarından üretiliyor ($XPD)"
    $PY tools/gen_luna_aurorae.py "$XPD/stil/luna.msstyles" build/aurorae
    $PY tools/gen_luna_kvantum.py "$XPD/stil/luna.msstyles" build/kvantum
    $PY tools/gen_luna_plasma.py "$XPD" build/desktoptheme build/varlik
    rm -rf build/wallpapers-xp && $PY tools/gen_luna_duvar.py "$XPD" build/wallpapers-xp
    $PY tools/gen_luna_plymouth.py build/varlik build/plymouth/xpthemeplasma "$XPD"
    rm -rf build/icons build/sounds
    $PY tools/gen_luna_ikon.py "$XPD" "build/icons/Windows XP"
    $PY tools/gen_luna_imlec.py "$XPD" "build/icons/Windows XP"
    $PY tools/gen_luna_ses.py "$XPD" "build/sounds/Windows XP"
else
    echo "==> Görseller çiziliyor (XP varlığı yok; gerçekleri için: bash $0 --xp <ISO>)"
    $PY tools/gen_aurorae.py build/aurorae
    $PY tools/gen_kvantum.py build/kvantum
    $PY tools/gen_desktoptheme.py build/desktoptheme
fi
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
if [[ -d build/varlik ]]; then
    kopyala build/varlik "$DATA/xpthemeplasma/varlik"
else
    rm -rf "$DATA/xpthemeplasma/varlik"
fi
for p in plasma/plasmoids/*/; do
    kopyala "$p" "$DATA/plasma/plasmoids/$(basename "$p")"
done
kopyala kwin/tabbox/xp_alttab "$DATA/kwin/tabbox/xp_alttab"
kopyala kwin/effects/xp_gri "$DATA/kwin/effects/xp_gri"
kopyala plasma/shells/io.github.yyozcan.xpshell "$DATA/plasma/shells/io.github.yyozcan.xpshell"
kopyala plasma/layout-templates/io.github.yyozcan.xptaskbar "$DATA/plasma/layout-templates/io.github.yyozcan.xptaskbar"
kopyala plasma/look-and-feel/io.github.yyozcan.xpluna "$DATA/plasma/look-and-feel/io.github.yyozcan.xpluna"
install -Dm644 plasma/color_scheme/XPLuna.colors "$DATA/color-schemes/XPLuna.colors"
kopyala build/wallpapers/XPThemePlasma "$DATA/wallpapers/XPThemePlasma"
if [[ -d build/wallpapers-xp && -f "$XPD/stil/luna.msstyles" ]]; then
    for w in build/wallpapers-xp/*/; do kopyala "$w" "$DATA/wallpapers/$(basename "$w")"; done
fi

if [[ -f "$XPD/stil/luna.msstyles" ]]; then
    echo "==> XP simgeleri, imleçleri, sesleri, yazı tipleri"
    kopyala "build/icons/Windows XP" "$DATA/icons/Windows XP"
    kopyala "build/sounds/Windows XP" "$DATA/sounds/Windows XP"
    mkdir -p "$DATA/fonts/xpthemeplasma"
    # XP'nin raster yazı tipleri: Terminal (Türkçe cp857), MS Sans Serif, Small Fonts
    for f in vga857.fon sserife.fon smalle.fon; do
        [[ -f "$XPD/font/$f" ]] && cp "$XPD/font/$f" "$DATA/fonts/xpthemeplasma/"
    done
    # sistemde eksik olan XP TrueType yazı tipleri
    for f in "$XPD"/font/*.ttf; do
        fam=$(fc-scan --format '%{family[0]}' "$f" 2>/dev/null || true)
        [[ -n "$fam" ]] && ! fc-list ":family=$fam" | grep -q . && cp "$f" "$DATA/fonts/xpthemeplasma/"
    done
    fc-cache -f "$DATA/fonts" >/dev/null 2>&1 || true
fi

echo "==> XP programları (yalnızca XP oturumunda görünür)"
XPAPP="$DATA/xpthemeplasma/data/applications"
mkdir -p "$XPAPP" build/bin
if command -v g++ >/dev/null && pkg-config --exists Qt6Widgets Qt6PrintSupport; then
    for app in notdefteri:xp-notdefteri:"Qt6Widgets Qt6PrintSupport" gezgin:xp-gezgin:Qt6Widgets denetim:xp-denetim:Qt6Widgets; do
        IFS=: read -r dizin ikili moduller <<< "$app"
        moduller=${moduller//\"/}
        echo "    derleniyor: $ikili"
        g++ -std=c++17 -O2 -fPIC "apps/$dizin/$dizin.cpp" -o "build/bin/$ikili" $(pkg-config --cflags --libs $moduller) \
            && install -Dm755 "build/bin/$ikili" "$BIN/$ikili" \
            && install -Dm644 "apps/$dizin/$ikili.desktop" "$XPAPP/$ikili.desktop"
    done
else
    echo "    (g++/Qt6 geliştirme dosyaları yok: XP programları derlenmedi)"
fi

echo "==> Komut İstemi (XP) ve yazı tipi işleme"
mkdir -p "$DATA/konsole"
cp "session/konsole/XP Komut İstemi.profile" session/konsole/XPCmd.colorscheme "$DATA/konsole/"
install -Dm644 session/xp-fonts.conf "$DATA/xpthemeplasma/fonts.conf"
if ! grep -q ">>> XPThemePlasma XP_CMD >>>" "$HOME/.bashrc" 2>/dev/null; then
    { echo; cat session/bashrc-xp-cmd.sh; } >> "$HOME/.bashrc"
fi

echo "==> Türkçe çeviriler"
for po in po/tr/*.po; do
    mkdir -p "$DATA/locale/tr/LC_MESSAGES"
    msgfmt -o "$DATA/locale/tr/LC_MESSAGES/$(basename "${po%.po}").mo" "$po"
done

echo "==> XP oturumu varsayılanları (XDG katmanı)"
XDGK="$DATA/xpthemeplasma/xdg"
mkdir -p "$XDGK/autostart"
cp session/xdg/* "$XDGK/" 2>/dev/null || true
# oturum açma sesi: yalnızca XP oturumunda (XDG_CONFIG_DIRS katmanındaki autostart)
cat > "$XDGK/autostart/xp-giris-sesi.desktop" <<EOD
[Desktop Entry]
Type=Application
Name=XP oturum açma sesi
Exec=$BIN/xp-giris-sesi
NoDisplay=true
OnlyShowIn=KDE;
X-KDE-autostart-phase=2
EOD
SIMGE="breeze"
for t in "Windows 7 Aero" "Windows XP" "Windows-XP"; do
    if [[ -d "/usr/share/icons/$t" || -d "$DATA/icons/$t" ]]; then SIMGE="$t"; fi
done
printf '[Icons]\nTheme=%s\n' "$SIMGE" > "$XDGK/kdeglobals"
echo "    simge teması: $SIMGE"

echo "==> Oturum yardımcıları"
install -Dm755 session/xp-profil "$BIN/xp-profil"
install -Dm755 session/xp-oturum "$BIN/xp-oturum"
install -Dm755 session/xp-giris-sesi "$BIN/xp-giris-sesi"
install -Dm644 session/xpthemeplasma-env.sh "$CFG/plasma-workspace/env/xpthemeplasma.sh"

# Plasma'nın paket önbelleğini tazele
rm -rf "${XDG_CACHE_HOME:-$HOME/.cache}"/plasma_theme_XP-Luna* 2>/dev/null || true

echo
echo "Kullanıcı kurulumu tamam."
if [[ ! -f /usr/share/wayland-sessions/xpthemeplasma.desktop ]] || ! cmp -s session/startxp-wayland /usr/local/bin/startxp-wayland; then
    echo "Oturum girdisi için bir kez çalıştır:  sudo bash $KOK/kur.sh --sistem"
fi
echo "Sonra oturumu kapatıp SDDM'de 'XPThemePlasma (Wayland)' seç."
