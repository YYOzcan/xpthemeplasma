# XPThemePlasma — her Plasma oturumunun başında çalışır (startplasma tarafından source edilir)
if [ -x "$HOME/.local/bin/xp-profil" ]; then
    if [ "$PLASMA_DEFAULT_SHELL" = "io.github.yyozcan.xpshell" ]; then
        "$HOME/.local/bin/xp-profil" xp >/dev/null 2>&1
        # KDE/Qt güncellendiyse XP bileşenlerini arka planda uyarla (girişi geciktirmez)
        [ -x "$HOME/.local/bin/xp-bakim" ] && "$HOME/.local/bin/xp-bakim" >/dev/null 2>&1
    else
        # XP oturumu düzgün kapanmadıysa önceki temayı geri getir
        "$HOME/.local/bin/xp-profil" geri >/dev/null 2>&1
    fi
fi
