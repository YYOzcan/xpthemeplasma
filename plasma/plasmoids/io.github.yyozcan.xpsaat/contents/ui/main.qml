/*
    XPThemePlasma — XP bildirim alanı saati
    XP'deki gibi: yalnızca saat (Tahoma 8, beyaz), üzerine gelince uzun tarih ipucu
    ("11 Kasım 2026 Çarşamba"), çift tıklayınca Tarih ve Saat Özellikleri, sağ tıklamada
    "Tarihi/Saati Ayarla". Tek tıklama hiçbir şey açmaz.
    SPDX-License-Identifier: GPL-3.0-or-later
*/
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support

PlasmoidItem {
    id: root

    property date simdi: new Date()
    readonly property var tr: Qt.locale("tr_TR")

    preferredRepresentation: compactRepresentation
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground
    toolTipMainText: ""
    toolTipSubText: root.simdi.toLocaleDateString(root.tr, "dd MMMM yyyy dddd")

    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: root.simdi = new Date()
    }

    P5Support.DataSource {
        id: komut
        engine: "executable"
        connectedSources: []
        onNewData: source => disconnectSource(source)
        function calistir(k) { connectSource(k); }
    }
    function tarihSaat() {
        komut.calistir("if [ -x \"$HOME/.local/bin/xp-tarihsaat\" ]; then setsid -f \"$HOME/.local/bin/xp-tarihsaat\"; else setsid -f kcmshell6 kcm_clock; fi");
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: "Tarihi/Saati Ayarla"
            icon.name: "preferences-system-time"
            onTriggered: root.tarihSaat()
        }
    ]

    // açılır pencere yok (XP); Plasma kompakt gösterimi yalnızca tam gösterim tanımlıyken kurar
    fullRepresentation: Item {}

    compactRepresentation: MouseArea {
        Layout.minimumWidth: yazi.implicitWidth + 14
        Layout.preferredWidth: yazi.implicitWidth + 14
        Layout.fillHeight: true
        acceptedButtons: Qt.LeftButton
        onDoubleClicked: root.tarihSaat()
        Text {
            id: yazi
            anchors.centerIn: parent
            text: root.simdi.toLocaleTimeString(root.tr, "HH:mm")
            color: "#FFFFFF"
            font.family: "Tahoma"
            font.pointSize: 8
        }
    }
}
