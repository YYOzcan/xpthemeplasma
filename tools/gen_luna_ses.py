#!/usr/bin/env python3
"""XP'nin "Windows Varsayılanı" ses düzeninden freedesktop ses teması.

    python3 tools/gen_luna_ses.py <xp-dizini> <çıktı: .../sounds/Windows XP>
"""
import shutil
import sys
from pathlib import Path

# freedesktop ses adı -> XP dosyası (XP'nin varsayılan düzenindeki olay karşılığı)
SESLER = {
    "desktop-login": "xpstartu.wav",          # Windows'u Başlat
    "desktop-logout": "xpshutdn.wav",         # Windows'tan Çık
    "service-login": "xplogon.wav",           # Windows Oturum Açma
    "service-logout": "xplogoff.wav",         # Windows Oturum Kapatma
    "system-ready": "xplogon.wav",
    "bell": "xpding.wav",                     # Varsayılan Bip
    "bell-window-system": "xpding.wav",
    "bell-terminal": "xpding.wav",
    "dialog-error": "xpcrtstp.wav",           # Kritik Durma
    "dialog-error-critical": "xpcrtstp.wav",
    "dialog-error-serious": "xpcrtstp.wav",
    "dialog-warning": "xpexcl.wav",           # Ünlem
    "dialog-warning-auth": "xpexcl.wav",
    "dialog-information": "xperror.wav",      # Yıldız (XP'de "Windows XP Error.wav")
    "dialog-question": "xpding.wav",
    "message": "xpballn.wav",                 # Sistem Bildirimi (balon)
    "message-new-instant": "xpnotify.wav",    # Bildirim
    "message-new-email": "xpnotify.wav",
    "message-sent-instant": "xpding.wav",
    "complete": "xpding.wav",
    "outcome-success": "xpding.wav",
    "device-added": "xphdinst.wav",           # Aygıt Bağlantısı
    "device-removed": "xphdrem.wav",          # Aygıt Bağlantısı Kesilmesi
    "device-added-failure": "xphdfail.wav",
    "battery-low": "xpbatlow.wav",            # Düşük Pil Alarmı
    "battery-caution": "xpbatcrt.wav",        # Kritik Pil Alarmı
    "power-unplug-battery-low": "xpbatlow.wav",
    "trash-empty": "xprecycl.wav",            # Geri Dönüşüm Kutusunu Boşalt
    "complete-print": "xpprint.wav",          # Yazdırma Tamamlandı
    "window-minimized": "xpmin.wav",
    "window-unminimized": "xprestor.wav",
    "phone-incoming-call": "xpringin.wav",
    "phone-outgoing-calling": "xprngout.wav",
    "network-connectivity-lost": "xpexcl.wav",
    "menu-click": "xpstart.wav",              # Gezintiyi Başlat
}


def main(xp, out):
    src, out = Path(xp) / "ses", Path(out)
    stereo = out / "stereo"
    shutil.rmtree(out, ignore_errors=True)
    stereo.mkdir(parents=True)
    n = 0
    for name, fn in SESLER.items():
        if (src / fn).exists():
            shutil.copy2(src / fn, stereo / f"{name}.wav")
            n += 1
    (out / "index.theme").write_text("[Sound Theme]\nName=Windows XP\nName[tr]=Windows XP\n"
                                     "Comment=Windows XP varsayılan ses düzeni (kendi XP kopyandan)\n"
                                     "Inherits=freedesktop\nDirectories=stereo\n\n[stereo]\nOutputProfile=stereo\n")
    print("ses teması ->", out, f"({n} ses)")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
