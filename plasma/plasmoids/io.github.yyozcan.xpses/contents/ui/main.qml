/*
    XPThemePlasma — Windows XP tepsi ses düzeyi penceresi ("Ses Düzeyi" + dikey kaydırıcı + "Sessiz")
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.volume
import org.kde.kcmutils as KCMUtils

PlasmoidItem {
    id: root

    readonly property var sink: PreferredDevice.sink
    readonly property int yuzde: sink ? Math.round(sink.volume / PulseAudio.NormalVolume * 100) : 0
    readonly property bool sessiz: sink ? sink.muted : true

    function ayarla(y) {
        if (!sink) {
            return;
        }
        y = Math.max(0, Math.min(100, y));
        sink.volume = Math.round(y / 100 * PulseAudio.NormalVolume);
        if (sink.muted && y > 0) {
            sink.muted = false;
        }
    }

    Plasmoid.icon: AudioIcon.forVolume(yuzde, sessiz, "")
    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    toolTipMainText: "Ses Düzeyi"
    toolTipSubText: sessiz ? "Sessiz" : "Ses: %" + yuzde
    preferredRepresentation: compactRepresentation

    VolumeFeedback {
        id: feedback
    }

    compactRepresentation: MouseArea {
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        onClicked: mouse => {
            if (mouse.button === Qt.MiddleButton) {
                if (root.sink) root.sink.muted = !root.sink.muted;
            } else {
                root.expanded = !root.expanded;
            }
        }
        onWheel: wheel => {
            root.ayarla(root.yuzde + (wheel.angleDelta.y > 0 ? 5 : -5));
        }
        Kirigami.Icon {
            anchors.fill: parent
            source: Plasmoid.icon
            active: parent.containsMouse
        }
    }

    fullRepresentation: Rectangle {
        // XP: küçük, 3B kenarlı "Ses Düzeyi" penceresi
        Layout.preferredWidth: 72
        Layout.preferredHeight: 172
        Layout.minimumWidth: 72
        Layout.minimumHeight: 172
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
                value: root.yuzde
                onMoved: root.ayarla(value)
                onPressedChanged: if (!pressed && root.sink) feedback.play(root.sink.index)
            }
            QQC2.CheckBox {
                Layout.alignment: Qt.AlignHCenter
                text: "Sessiz"
                font.family: "Tahoma"
                font.pixelSize: 11
                checked: root.sessiz
                onToggled: if (root.sink) root.sink.muted = checked
            }
        }
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: "Ses Denetimini Aç"
            icon.name: "preferences-desktop-sound"
            onTriggered: KCMUtils.KCMLauncher.openSystemSettings("kcm_pulseaudio")
        },
        PlasmaCore.Action {
            text: "Ses Özelliklerini Ayarla"
            icon.name: "audio-card"
            onTriggered: KCMUtils.KCMLauncher.openSystemSettings("kcm_pulseaudio")
        }
    ]
}
