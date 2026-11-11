/*
    XPThemePlasma — XP "Yerel Ağ Bağlantısı" bildirim alanı simgesi
    İki ekranlı XP simgesi (netshell.dll 190-195): gönderirken öndeki, alırken arkadaki ekran yanar;
    kablo çıkarsa kırmızı X. İpucu "Hız / Durum", çift tıklama Durum penceresi (xp-ag),
    sağ tıklama: Devre Dışı Bırak, Durum, Onar, Ağ Bağlantılarını Aç.
    Kablolu arayüz yoksa kablosuz arayüzü izler; kablosuz ağ seçimi "Ağ Bağlantılarını Aç" ile.
    SPDX-License-Identifier: GPL-3.0-or-later
*/
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    property string arayuz: ""
    property bool bagli: false
    property real oncekiTx: -1
    property real oncekiRx: -1
    property int etkinlik: 0          // 0 boş, 1 gönderiyor, 2 alıyor, 3 ikisi
    property int hizMbps: 0
    property bool kablosuz: false

    preferredRepresentation: compactRepresentation
    Plasmoid.status: arayuz === "" ? PlasmaCore.Types.HiddenStatus : PlasmaCore.Types.ActiveStatus
    Plasmoid.icon: !bagli ? "xp-ag-kopuk" : ["xp-ag-bos", "xp-ag-giden", "xp-ag-gelen", "xp-ag-ikisi"][etkinlik]
    toolTipMainText: kablosuz ? "Kablosuz Ağ Bağlantısı" : "Yerel Ağ Bağlantısı"
    toolTipSubText: bagli ? "Hız: " + hizMetni(hizMbps) + "\nDurum: Bağlı" : "Ağ kablosu takılı değil"

    function hizMetni(m) {
        if (m <= 0) return "";
        let b = m * 1e6, i = 0;
        const birim = ["bps", "Kbps", "Mbps", "Gbps", "Tbps"];
        while (b >= 1000 && i < 4) { b /= 1000; i++; }
        return Math.floor(b) + "." + Math.round((b - Math.floor(b)) * 10) % 10 + " " + birim[i];
    }

    P5Support.DataSource {
        id: komut
        engine: "executable"
        connectedSources: []
        onNewData: source => disconnectSource(source)
        function calistir(k) { connectSource(k); }
    }
    function durum() {
        komut.calistir("if [ -x \"$HOME/.local/bin/xp-ag\" ]; then setsid -f \"$HOME/.local/bin/xp-ag\" " + root.arayuz
                       + "; else setsid -f kcmshell6 kcm_networkmanagement; fi");
    }

    // kablolu arayüzü bul (fiziksel aygıtı olan, kablosuz olmayan ilk arayüz)
    P5Support.DataSource {
        engine: "executable"
        // önce kablolu, yoksa kablosuz arayüz
        connectedSources: ["for i in /sys/class/net/*; do [ -e \"$i/device\" ] && [ ! -e \"$i/wireless\" ] && basename \"$i\" && exit; done; for i in /sys/class/net/*; do [ -e \"$i/wireless\" ] && basename \"$i\" && exit; done"]
        onNewData: (source, data) => {
            root.arayuz = (data["stdout"] || "").trim();
            root.kablosuz = root.arayuz.startsWith("wl");
            disconnectSource(source);
        }
    }
    // yarım saniyede bir durum ve sayaçlar
    P5Support.DataSource {
        engine: "executable"
        interval: 500
        connectedSources: root.arayuz === "" ? [] : ["d=/sys/class/net/" + root.arayuz + "; cat $d/operstate $d/statistics/tx_packets $d/statistics/rx_packets $d/speed 2>/dev/null"]
        onNewData: (source, data) => {
            const s = (data["stdout"] || "").trim().split("\n");
            root.bagli = s[0] === "up";
            const tx = Number(s[1]), rx = Number(s[2]);
            root.hizMbps = Number(s[3]) > 0 ? Number(s[3]) : 0;
            const g = root.oncekiTx >= 0 && tx > root.oncekiTx, a = root.oncekiRx >= 0 && rx > root.oncekiRx;
            root.etkinlik = (g ? 1 : 0) + (a ? 2 : 0);
            root.oncekiTx = tx;
            root.oncekiRx = rx;
        }
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: root.bagli ? "&Devre Dışı Bırak" : "&Etkinleştir"
            onTriggered: komut.calistir("nmcli device " + (root.bagli ? "disconnect " : "connect ") + root.arayuz)
        },
        PlasmaCore.Action {
            text: "D&urum"
            onTriggered: root.durum()
        },
        PlasmaCore.Action {
            text: "&Onar"
            onTriggered: komut.calistir("nmcli device reapply " + root.arayuz + " || nmcli device connect " + root.arayuz)
        },
        PlasmaCore.Action { isSeparator: true },
        PlasmaCore.Action {
            text: "Ağ Bağ&lantılarını Aç"
            onTriggered: komut.calistir("setsid -f kcmshell6 kcm_networkmanagement")
        }
    ]

    // açılır pencere yok (XP); Plasma kompakt gösterimi yalnızca tam gösterim tanımlıyken kurar
    fullRepresentation: Item {}

    compactRepresentation: MouseArea {
        Layout.minimumWidth: Kirigami.Units.iconSizes.small
        Layout.minimumHeight: Kirigami.Units.iconSizes.small
        acceptedButtons: Qt.LeftButton
        onDoubleClicked: root.durum()          // XP: kalın "Durum" varsayılan eylem
        Kirigami.Icon {
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.small
            height: width
            source: Plasmoid.icon
        }
    }
}
