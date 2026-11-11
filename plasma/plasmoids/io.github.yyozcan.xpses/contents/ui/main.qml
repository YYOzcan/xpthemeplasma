/*
    XPThemePlasma — Windows XP bildirim alanı ses düzeyi ("Ses Düzeyi" + dikey kaydırıcı + "Sessiz")

    Plasma'nın özel ses QML API'si sürümden sürüme değiştiği için kullanılmaz; PipeWire'ın kendi
    komut satırı aracı wpctl ile çalışır (KDE güncellemelerinden etkilenmez).
      tek tıklama: Ses Düzeyi penceresi        çift tıklama: Ses Denetimi (karıştırıcı)
      orta tıklama: sessiz                     tekerlek: ses düzeyi
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    property int yuzde: 0
    property bool sessiz: false
    property bool hazir: false
    property bool surukleniyor: false

    readonly property string hedef: "@DEFAULT_AUDIO_SINK@"

    Plasmoid.icon: !hazir || sessiz || yuzde === 0 ? "audio-volume-muted"
                   : yuzde < 34 ? "audio-volume-low" : yuzde < 67 ? "audio-volume-medium" : "audio-volume-high"
    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    toolTipMainText: "Ses Düzeyi"
    toolTipSubText: !hazir ? "Ses aygıtı yok" : sessiz ? "Sessiz" : "Ses: %" + yuzde
    preferredRepresentation: compactRepresentation

    // ------------------------------------------------------------ wpctl
    P5Support.DataSource {
        id: komut
        engine: "executable"
        connectedSources: []
        onNewData: source => disconnectSource(source)
        function calistir(k) { connectSource(k); }
    }
    // durum: "Volume: 0.50" / "Volume: 0.50 [MUTED]"
    P5Support.DataSource {
        id: oku
        engine: "executable"
        interval: root.expanded ? 400 : 1500
        connectedSources: ["LC_ALL=C wpctl get-volume " + root.hedef]
        onNewData: (source, data) => {
            const s = (data["stdout"] || "").trim();
            const m = s.match(/Volume:\s*([0-9.]+)/);
            root.hazir = m !== null;
            if (!m) return;
            if (!root.surukleniyor) root.yuzde = Math.round(parseFloat(m[1]) * 100);
            root.sessiz = s.indexOf("[MUTED]") !== -1;
        }
    }
    function ayarla(y) {
        y = Math.max(0, Math.min(100, Math.round(y)));
        root.yuzde = y;
        komut.calistir("wpctl set-volume " + root.hedef + " " + (y / 100).toFixed(2)
                       + (root.sessiz && y > 0 ? "; wpctl set-mute " + root.hedef + " 0" : ""));
    }
    function sessizYap(s) {
        root.sessiz = s;
        komut.calistir("wpctl set-mute " + root.hedef + " " + (s ? 1 : 0));
    }
    // XP: kaydırıcı bırakılınca "Varsayılan Bip" çalardı
    function bip() {
        komut.calistir("f=\"${XDG_DATA_HOME:-$HOME/.local/share}/sounds/Windows XP/stereo/bell.wav\"; [ -f \"$f\" ] && pw-play \"$f\"");
    }
    function program(ad, yedek) {
        komut.calistir("if [ -x \"$HOME/.local/bin/" + ad + "\" ]; then setsid -f \"$HOME/.local/bin/" + ad + "\"; else setsid -f " + yedek + "; fi");
    }

    compactRepresentation: MouseArea {
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        Layout.minimumWidth: Kirigami.Units.iconSizes.small
        Layout.minimumHeight: Kirigami.Units.iconSizes.small
        property bool cift: false
        Timer { id: tekTik; interval: 250; onTriggered: root.expanded = !root.expanded }
        onClicked: mouse => {
            if (mouse.button === Qt.MiddleButton) { root.sessizYap(!root.sessiz); return; }
            tekTik.restart();
        }
        onDoubleClicked: { tekTik.stop(); root.expanded = false; root.program("xp-sesdenetimi", "kcmshell6 kcm_pulseaudio"); }
        onWheel: wheel => root.ayarla(root.yuzde + (wheel.angleDelta.y > 0 ? 5 : -5))
        Kirigami.Icon {
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.small
            height: width
            source: Plasmoid.icon
        }
    }

    fullRepresentation: Rectangle {
        // XP: küçük, 3B kenarlı "Ses Düzeyi" penceresi
        Layout.preferredWidth: 72
        Layout.preferredHeight: 172
        Layout.minimumWidth: 72
        Layout.minimumHeight: 172
        Layout.maximumWidth: 72
        Layout.maximumHeight: 172
        color: "#ECE9D8"
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 4

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Ses Düzeyi"
                font.family: "Tahoma"
                font.pixelSize: 11
                color: "#000000"
                renderType: Text.NativeRendering
            }
            QQC2.Slider {
                id: kaydirici
                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                orientation: Qt.Vertical
                from: 0
                to: 100
                stepSize: 1
                enabled: root.hazir
                value: root.yuzde
                onMoved: root.ayarla(value)
                onPressedChanged: {
                    root.surukleniyor = pressed;
                    if (!pressed) root.bip();
                }
            }
            QQC2.CheckBox {
                Layout.alignment: Qt.AlignHCenter
                text: "Sessiz"
                font.family: "Tahoma"
                font.pixelSize: 11
                enabled: root.hazir
                checked: root.sessiz
                onToggled: root.sessizYap(checked)
            }
        }
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: "Ses &Denetimini Aç"
            icon.name: "preferences-desktop-sound"
            onTriggered: root.program("xp-sesdenetimi", "kcmshell6 kcm_pulseaudio")
        },
        PlasmaCore.Action {
            text: "Ses Ö&zelliklerini Ayarla"
            icon.name: "audio-card"
            onTriggered: root.program("xp-ses", "kcmshell6 kcm_pulseaudio")
        }
    ]
}
