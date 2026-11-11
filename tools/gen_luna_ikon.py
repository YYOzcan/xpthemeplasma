#!/usr/bin/env python3
"""Kendi XP kopyandaki simgelerden "Windows XP" simge teması üretir.

    python3 tools/gen_luna_ikon.py <xp-dizini> <çıktı: .../icons/Windows XP>

Eksik adlar index.theme'deki Inherits zinciriyle (Windows 7 Aero → breeze) tamamlanır.
"""
import sys

from PIL import Image
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from xpikon import at_size, icon_images  # noqa: E402

BOYUTLAR = [16, 22, 24, 32, 48, 64]
# tepsi/durum simgeleri Plasma'da "-symbolic" adla istenir; aynı görüntüyü o adla da yaz
SEMBOLIK = {"status", "devices", "actions"}

S = "shell32.dll"
MAP = [
    # ------------------------------------------------------------- yerler
    (S, 4, "places", "folder inode-directory folder-blue folder-grey folder-yellow folder-orange folder-red folder-green folder-violet folder-black folder-brown folder-cyan folder-magenta folder-root folder-temp folder-tar"),
    (S, 5, "places", "folder-open folder-drag-accept"),
    ("mydocs.dll", 100, "places", "folder-documents folder-text folder-txt"),
    (S, 236, "places", "folder-pictures folder-image folder-images folder-photo"),
    (S, 237, "places", "folder-music folder-sound folder-audio"),
    (S, 238, "places", "folder-videos folder-video folder-movie"),
    (S, 35, "places", "user-desktop desktop folder-desktop"),
    (S, 16, "places", "computer user-home-computer system computer-symbolic"),
    (S, 18, "places", "network-workgroup folder-network network folder-remote network-server-symbolic"),
    (S, 19, "places", "network-server server"),
    (S, 20, "categories", "applications-accessories applications-development applications-education applications-games applications-graphics applications-internet applications-multimedia applications-office applications-other applications-science applications-system applications-utilities applications-engineering preferences-desktop applications-all folder-favorites-apps"),
    (S, 21, "places", "document-open-recent folder-recent recent-documents"),
    (S, 44, "places", "folder-favorites bookmarks bookmark-new-list starred favorites"),
    (S, 46, "places", "folder-saved-search system-search-folder"),
    (S, 258, "actions", "folder-new"),
    (S, 264, "actions", "go-up go-parent-folder"),
    (S, 267, "places", "folder-publicshare folder-public"),
    (S, 39, "places", "folder-templates folder-fonts"),
    (S, 172, "places", "folder-download folder-downloads"),
    (S, 259, "places", "user-home go-home folder-home"),
    (S, 32, "places", "user-trash trash-empty emptytrash user-trash-symbolic"),
    (S, 33, "places", "user-trash-full trash-full"),
    (S, 36, "places", "folder-development"),
    # ------------------------------------------------------------- aygıtlar
    (S, 9, "devices", "drive-harddisk drive-harddisk-root drive-harddisk-system harddisk drive-harddisk-solidstate drive-multidisk"),
    (S, 8, "devices", "drive-removable-media drive-removable-media-usb drive-removable-media-usb-pendrive media-flash media-flash-sd-mmc media-flash-memory-stick"),
    (S, 12, "devices", "drive-optical media-optical drive-cdrom media-optical-data"),
    (S, 41, "devices", "media-optical-audio media-optical-cd-audio"),
    (S, 292, "devices", "media-optical-dvd media-optical-dvd-video media-optical-blu-ray"),
    (S, 7, "devices", "media-floppy drive-removable-media-floppy"),
    (S, 10, "devices", "folder-remote-smb drive-harddisk-network network-drive"),
    (S, 17, "devices", "printer printer-network printer-symbolic"),
    (S, 312, "devices", "scanner"),
    (S, 248, "devices", "camera-photo camera"),
    (S, 317, "devices", "camera-video camera-web"),
    (S, 310, "devices", "phone smartphone phone-symbolic"),
    (S, 314, "devices", "pda multimedia-player"),
    (S, 13, "devices", "cpu computer-chip memory"),
    ("main.cpl", 100, "devices", "input-mouse preferences-desktop-mouse"),
    ("main.cpl", 200, "devices", "input-keyboard preferences-desktop-keyboard"),
    ("joy.cpl", 102, "devices", "input-gaming applications-games-symbolic preferences-desktop-gaming"),
    ("mmsys.cpl", 3004, "devices", "audio-card audio-speakers"),
    ("desk.cpl", 100, "devices", "video-display preferences-desktop-display display"),
    (S, 229, "devices", "input-touchpad"),
    ("stobject.dll", 210, "devices", "computer-laptop"),
    # ------------------------------------------------------------- dosya türleri
    (S, 1, "mimetypes", "unknown application-octet-stream text-x-generic-template application-x-zerosize"),
    (S, 152, "mimetypes", "text-x-generic text-plain text-x-log text-x-readme text-x-changelog text-markdown text-x-csrc text-x-c++src text-x-chdr text-x-python text-x-java text-css text-x-makefile application-json application-xml text-xml application-x-yaml application-toml"),
    (S, 153, "mimetypes", "application-x-shellscript text-x-script application-x-executable-script text-x-shellscript"),
    (S, 154, "mimetypes", "application-javascript text-javascript application-x-perl application-x-ruby"),
    (S, 3, "mimetypes", "application-x-executable application-x-ms-dos-executable application-x-msdownload application-x-sharedlib application-x-desktop application-x-appimage exec"),
    (S, 224, "mimetypes", "video-x-generic video-mp4 video-x-matroska video-webm video-x-msvideo video-mpeg video-quicktime"),
    (S, 225, "mimetypes", "audio-x-generic audio-mpeg audio-x-wav audio-x-vorbis+ogg audio-flac audio-x-flac audio-mp4 audio-x-mpegurl"),
    (S, 227, "mimetypes", "audio-x-mpegurl application-vnd.apple.mpegurl playlist"),
    ("shimgvw.dll", 1, "mimetypes", "image-x-generic image-png image-jpeg image-bmp image-gif image-tiff image-webp image-x-icon image-svg+xml image-x-xcf image-avif"),
    ("zipfldr.dll", 101, "mimetypes", "application-zip application-x-archive package-x-generic application-x-compressed-tar application-x-7z-compressed application-x-rar application-x-tar application-x-bzip-compressed-tar application-x-xz-compressed-tar application-gzip application-x-zstd-compressed-tar application-x-zip"),
    ("fontext.dll", 2, "mimetypes", "font-x-generic application-x-font-ttf font-ttf font-otf application-x-font-otf"),
    ("wordpad.exe", 154, "mimetypes", "application-rtf text-rtf x-office-document application-msword application-vnd.oasis.opendocument.text application-vnd.openxmlformats-officedocument.wordprocessingml.document"),
    ("iexplore.exe", 32529, "mimetypes", "text-html application-xhtml+xml text-x-uri application-x-mswinurl"),
    (S, 166, "mimetypes", "application-x-cd-image application-x-iso9660-image"),
    (S, 171, "mimetypes", "text-x-registry"),
    # ------------------------------------------------------------- uygulamalar
    ("explorer.exe", 100, "apps", "system-file-manager org.kde.dolphin dolphin file-manager nautilus thunar pcmanfm"),
    ("cmd.exe", "IDI_APPICON", "apps", "utilities-terminal org.kde.konsole konsole terminal xterm Alacritty kitty com.mitchellh.ghostty org.kde.yakuake yakuake"),
    ("notepad.exe", 2, "apps", "accessories-text-editor org.kde.kwrite kwrite org.kde.kate kate gedit text-editor org.gnome.TextEditor mousepad"),
    ("calc.exe", "SC", "apps", "xp-calc accessories-calculator org.kde.kcalc kcalc gnome-calculator org.gnome.Calculator galculator qalculate"),
    ("mspaint.exe", 2, "apps", "xp-mspaint org.kde.kolourpaint kolourpaint applications-graphics pinta"),
    ("iexplore.exe", 32528, "apps", "internet-web-browser web-browser firefox org.mozilla.firefox firefox-esr google-chrome chromium org.chromium.Chromium brave-browser brave librewolf vivaldi opera microsoft-edge org.kde.falkon falkon"),
    ("msimn.exe", 2, "apps", "internet-mail mail-client thunderbird org.mozilla.Thunderbird org.mozilla.thunderbird kmail org.kde.kmail2 evolution geary betterbird"),
    ("wmplayer.exe", "IDR_MAINFRAME", "apps", "multimedia-player multimedia-video-player multimedia-audio-player vlc org.videolan.VLC org.kde.elisa elisa org.kde.haruna haruna mpv smplayer celluloid io.github.celluloid_player.Celluloid audacious clementine strawberry org.kde.dragonplayer dragonplayer applications-multimedia"),
    ("taskmgr.exe", 107, "apps", "xp-gorev utilities-system-monitor org.kde.plasma-systemmonitor plasma-systemmonitor ksysguard gnome-system-monitor htop btop"),
    ("helpctr.exe", 200, "apps", "help-browser help-contents org.kde.khelpcenter khelpcenter help-about help-hint"),
    ("charmap.exe", 111, "apps", "xp-charmap accessories-character-map org.kde.kcharselect kcharselect gucharmap"),
    ("magnify.exe", 128, "apps", "org.kde.kmag kmag accessories-magnifier"),
    ("moviemk.exe", 128, "apps", "org.kde.kdenlive kdenlive shotcut openshot org.openshot.OpenShot"),
    ("mstsc.exe", 101, "apps", "org.kde.krdc krdc remmina org.remmina.Remmina preferences-desktop-remote-desktop"),
    ("shimgvw.dll", 2, "apps", "org.kde.gwenview gwenview image-viewer eog org.gnome.eog ristretto imv"),
    ("wordpad.exe", 128, "apps", "xp-wordpad libreoffice-writer libreoffice-startcenter abiword org.kde.calligrawords accessories-word-processor x-office-document"),
    ("sol.exe", 500, "apps", "xp-sol org.kde.kpat kpat aisleriot"),
    ("winmine.exe", 100, "apps", "xp-winmine org.kde.kmines kmines gnome-mines"),
    ("spider.exe", 103, "apps", "xp-spider org.kde.kspider"),
    ("freecell.exe", 601, "apps", "xp-freecell freecell"),
    ("pinball.exe", "ICON_1", "apps", "xp-pinball"),
    # XP SP2 Bluetooth bildirim alanı simgesi (bthci.dll) ve Bluetooth Dosya Aktarımı (fsquirt.exe)
    ("bthci.dll", 201, "status", "network-bluetooth network-bluetooth-activated network-bluetooth-inactive "
                                 "network-bluetooth-activated-locked bluetooth-active bluetooth-online bluetooth-disabled"),
    ("bthci.dll", 201, "apps", "preferences-system-bluetooth preferences-system-bluetooth-activated "
                               "preferences-system-bluetooth-inactive bluetooth blueman"),
    ("fsquirt.exe", 107, "apps", "org.kde.bluedevilsendfile bluedevil-sendfile"),
    # bildirim alanı ağ simgesi (iki ekran): boşta, gönderiyor, alıyor, ikisi, kablo yok, sınırlı
    ("netshell.dll", 190, "status", "xp-ag-bos"),
    ("netshell.dll", 191, "status", "xp-ag-giden"),
    ("netshell.dll", 192, "status", "xp-ag-gelen"),
    ("netshell.dll", 193, "status", "xp-ag-ikisi"),
    ("netshell.dll", 195, "status", "xp-ag-kopuk"),
    ("netshell.dll", 196, "status", "xp-ag-sinirli"),
    ("sysdm.cpl", 1, "apps", "xp-sistem"),
    ("timedate.cpl", 200, "apps", "xp-tarihsaat"),
    ("desk.cpl", 100, "apps", "xp-gorunum"),
    ("regedit.exe", 100, "apps", "dconf-editor org.kde.kconfigeditor kconfigeditor regedit"),
    ("dxdiag.exe", 7000, "apps", "hwinfo org.kde.kinfocenter kinfocenter hardinfo hardinfo2"),
    ("msconfig.exe", 128, "apps", "systemd-manager org.kde.kded6"),
    ("mmc.exe", 128, "apps", "org.kde.partitionmanager partitionmanager gparted"),
    ("cleanmgr.exe", 104, "apps", "org.kde.filelight filelight bleachbit baobab org.gnome.baobab"),
    ("rstrui.exe", 110, "apps", "timeshift timeshift-gtk backintime"),
    ("osk.exe", "APP_OSK", "apps", "input-keyboard-virtual onboard org.kde.plasma.keyboard"),
    ("narrator.exe", 102, "apps", "orca preferences-desktop-text-to-speech"),
    ("mplayer2.exe", 110, "apps", "media-player-classic"),
    ("sndrec32.exe", 10, "apps", "xp-sndrec32 audio-recorder org.kde.kwave kwave audacity org.gnome.SoundRecorder"),
    ("hh.exe", 101, "apps", "help-faq"),
    ("mshearts.exe", 31234, "apps", "xp-mshearts org.kde.kmahjongg kmahjongg"),
    (S, 137, "apps", "preferences-system systemsettings org.kde.systemsettings preferences-other preferences-desktop-default-applications"),
    (S, 271, "apps", "system-software-install plasmadiscover org.kde.discover pamac octopi system-software-update-install"),
    (S, 270, "apps", "preferences-desktop-theme preferences-desktop-color preferences-desktop-theme-global preferences-desktop-theme-applications preferences-desktop-plasma-theme preferences-desktop-icons"),
    (S, 250, "apps", "preferences-desktop-wallpaper preferences-desktop-screensaver"),
    (S, 273, "apps", "preferences-system-network preferences-system-network-connection preferences-desktop-online-accounts"),
    (S, 277, "apps", "preferences-desktop-sound preferences-desktop-multimedia org.pulseaudio.pavucontrol pavucontrol"),
    (S, 279, "apps", "system-users preferences-system-users user-identity-symbolic preferences-desktop-user"),
    (S, 272, "apps", "preferences-desktop-peripherals preferences-system-hardware"),
    (S, 274, "apps", "preferences-system-performance applications-system-symbolic"),
    (S, 275, "apps", "partitionmanager-disk drive-harddisk-usage"),
    (S, 40, "apps", "preferences-desktop-taskbar preferences-desktop-panel"),
    (S, 38, "apps", "printer-folder preferences-devices-printer org.kde.print-manager"),
    ("timedate.cpl", 200, "apps", "preferences-system-time clock preferences-system-date-time"),
    ("intl.cpl", 200, "apps", "preferences-desktop-locale preferences-desktop-locale-symbolic"),
    ("access.cpl", 110, "apps", "preferences-desktop-accessibility accessibility preferences-desktop-accessibility-symbolic"),
    ("powercfg.cpl", 201, "apps", "preferences-system-power-management preferences-system-power org.kde.powerdevil"),
    ("fontext.dll", 1, "apps", "preferences-desktop-font fonts"),
    ("wscui.cpl", 200, "apps", "security-high preferences-security security-center"),
    ("wuaucpl.cpl", 1, "apps", "system-software-update software-update-available update-none"),
    ("explorer.exe", 101, "apps", "kfind org.kde.kfind"),
    # ------------------------------------------------------------- eylemler
    (S, 28, "actions", "system-shutdown application-exit"),
    (S, 45, "actions", "system-log-out system-switch-user"),
    (S, 48, "actions", "system-lock-screen object-locked"),
    (S, 25, "actions", "system-run run-build"),
    (S, 23, "actions", "system-search edit-find search find-location"),
    (S, 24, "actions", "help-contents-symbolic help-browser-symbolic"),
    (S, 240, "actions", "edit-delete delete process-stop"),
    (S, 243, "actions", "edit-copy"),
    (S, 290, "actions", "go-next go-jump media-skip-forward"),
    (S, 246, "actions", "media-playback-start"),
    (S, 200, "actions", "dialog-cancel action-unavailable"),
    (S, 133, "actions", "edit-paste"),
    (S, 1003, "actions", "document-properties configure"),
    (S, 1001, "actions", "help-about"),
    (S, 30, "emblems", "emblem-symbolic-link"),
    (S, 29, "emblems", "emblem-shared"),
    # ------------------------------------------------------------- durum (tepsi)
    ("user32.dll", 103, "status", "dialog-error data-error"),
    ("user32.dll", 101, "status", "dialog-warning data-warning"),
    ("user32.dll", 104, "status", "dialog-information data-information"),
    ("user32.dll", 102, "status", "dialog-question"),
    ("sndvol32.exe", 304, "status", "audio-volume-high audio-volume-medium audio-volume-low audio-ready audio-volume-high-danger"),
    ("sndvol32.exe", 300, "apps", "multimedia-volume-control kmix org.kde.kmix"),
    ("sndvol32.exe", 303, "status", "audio-volume-muted audio-volume-low-zero audio-off"),
    ("netshell.dll", 190, "status", "network-wired network-wired-activated network-connect network-wired-connected network-transmit-receive network-transmit network-receive network-idle"),
    ("netshell.dll", 195, "status", "network-wired-disconnected network-disconnect network-offline network-wired-unavailable network-error"),
    ("netshell.dll", 2615, "status", "network-wireless network-wireless-connected-100 network-wireless-100 network-wireless-signal-excellent network-wireless-activated"),
    ("netshell.dll", 2614, "status", "network-wireless-connected-75 network-wireless-75 network-wireless-signal-good"),
    ("netshell.dll", 2613, "status", "network-wireless-connected-50 network-wireless-50 network-wireless-signal-ok"),
    ("netshell.dll", 2612, "status", "network-wireless-connected-25 network-wireless-25 network-wireless-signal-weak"),
    ("netshell.dll", 2611, "status", "network-wireless-connected-00 network-wireless-0 network-wireless-signal-none"),
    ("netshell.dll", 2610, "status", "network-wireless-disconnected network-wireless-offline network-wireless-off"),
    ("hotplug.dll", 100, "status", "device-notifier media-eject drive-removable-media-usb-symbolic"),
    ("powercfg.cpl", 200, "status", "ac-adapter battery-full-charging battery-100-charging battery-charging"),
    ("powercfg.cpl", 201, "status", "battery battery-100 battery-full battery-good battery-080 battery-060 battery-040"),
    ("powercfg.cpl", 202, "status", "battery-low battery-caution battery-020 battery-010 battery-000 battery-empty battery-missing"),
    ("wuaucpl.cpl", 4, "status", "update-low update-medium"),
    ("wuaucpl.cpl", 5, "status", "update-high security-low"),
    ("wuaucpl.cpl", 3, "status", "security-medium"),
    (S, 1001, "status", "notifications preferences-desktop-notification-bell notification-active"),
]


# Araç çubuğu şeritleri: (dosya, 24px bitmap, 16px bitmap) -> {indeks: adlar}
SERITLER = [
    ("shell32.dll", 214, 216, {
        0: "go-previous go-back draw-arrow-back", 1: "go-next go-forward draw-arrow-forward",
        2: "bookmarks-organize", 3: "bookmark-new",
        5: "edit-cut", 6: "edit-copy", 7: "edit-paste", 8: "edit-undo", 9: "edit-redo",
        10: "edit-delete delete", 11: "document-new", 12: "document-open", 13: "document-save document-save-as",
        14: "document-print-preview", 15: "document-properties", 16: "help-whatsthis",
        17: "edit-find system-search search", 19: "document-print",
        20: "view-list-icons view-preview", 21: "view-list-compact", 22: "view-list-tree",
        23: "view-list-details view-list-text", 24: "view-sort-ascending view-sort",
        28: "go-up go-parent-folder", 31: "folder-new", 32: "view-choose", 43: "view-sidetree sidebar-show",
        44: "edit-move", 45: "edit-copy-path"}),
    ("shell32.dll", 204, 206, {
        2: "process-stop", 3: "view-refresh reload", 4: "go-home",
        12: "view-history", 13: "mail-message-new mail-send"}),
]


def seritler(xp, out, dirs):
    from msstyles import decode_dib
    from peres import PEResources
    n = 0
    cache = {}
    for fname, b24, b16, harita in SERITLER:
        if fname not in cache:
            cache[fname] = PEResources((xp / "ikon" / fname).read_bytes())
        r = cache[fname]
        for bid, size in [(b24, 24), (b16, 16)]:
            d = r.get("BITMAP", bid)
            if d is None:
                continue
            strip, _ = decode_dib(d)
            for i, names in harita.items():
                tile = strip.crop((i * size, 0, (i + 1) * size, size))
                hedefler = [size] if size == 24 else [16, 22]
                for hs in hedefler:
                    img = tile if hs == size else tile.resize((hs, hs), 1)
                    dd = out / f"{hs}x{hs}" / "actions"
                    dd.mkdir(parents=True, exist_ok=True)
                    dirs.add((hs, "actions"))
                    for name in names.split():
                        img.save(dd / f"{name}.png")
                        img.save(dd / f"{name}-symbolic.png")
                        n += 1
    return n


def tepsi_oku(xp, out, dirs):
    """Sistem tepsisi genişletme oku: XP'nin yuvarlak '‹' / '›' düğmeleri (yalnızca -symbolic adlar)."""
    from msstyles import Msstyles
    ms = Msstyles(str(xp / "stil/luna.msstyles"))
    n = 0
    for cls, ad in [("TrayNotifyHoriz::Button", "arrow-up-symbolic"), ("TrayNotifyHorizOpen::Button", "arrow-down-symbolic")]:
        img = ms.images(cls)[0]
        for size in BOYUTLAR:
            kare = Image.new("RGBA", (max(img.size), max(img.size)))
            kare.paste(img, ((kare.width - img.width) // 2, (kare.height - img.height) // 2))
            d = out / f"{size}x{size}" / "actions"
            d.mkdir(parents=True, exist_ok=True)
            dirs.add((size, "actions"))
            kare.resize((size, size), Image.LANCZOS).save(d / f"{ad}.png")
            n += 1
    return n


def main(xp, out):
    xp, out = Path(xp), Path(out)
    cache = {}
    eksik = []
    dirs = set()
    yazilan = 0
    for fname, gid, context, names in MAP:
        if fname not in cache:
            p = xp / "ikon" / fname
            cache[fname] = icon_images(p.read_bytes()) if p.exists() else {}
        groups = cache[fname]
        imgs = groups.get(gid)
        if imgs is None:
            eksik.append(f"{fname}:{gid}")
            continue
        for size in BOYUTLAR:
            img = at_size(imgs, size)
            d = out / f"{size}x{size}" / context
            d.mkdir(parents=True, exist_ok=True)
            dirs.add((size, context))
            for name in names.split():
                img.save(d / f"{name}.png")
                yazilan += 1
                if context in SEMBOLIK and not name.endswith("-symbolic"):
                    img.save(d / f"{name}-symbolic.png")
    yazilan += seritler(xp, out, dirs)
    yazilan += tepsi_oku(xp, out, dirs)
    dlist = sorted(f"{s}x{s}/{c}" for s, c in dirs)
    ctx = {"apps": "Applications", "places": "Places", "devices": "Devices", "mimetypes": "MimeTypes",
           "actions": "Actions", "status": "Status", "categories": "Categories", "emblems": "Emblems"}
    lines = ["[Icon Theme]", "Name=Windows XP", "Name[tr]=Windows XP",
             "Comment=Windows XP simgeleri (kendi XP kopyandan içe aktarıldı)",
             "Inherits=Windows 7 Aero,breeze,hicolor", "Example=folder",
             "DisplayDepth=32", "DesktopDefault=48", "ToolbarDefault=16", "SmallDefault=16", "PanelDefault=16",
             "Directories=" + ",".join(dlist), ""]
    for d in dlist:
        size, c = d.split("/")
        n = int(size.split("x")[0])
        lines += [f"[{d}]", f"Size={n}", f"Context={ctx[c]}", "Type=Fixed", ""]
    (out / "index.theme").write_text("\n".join(lines))
    print(f"simge teması -> {out} ({yazilan} dosya)")
    if eksik:
        print("bulunamayan:", " ".join(eksik))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
