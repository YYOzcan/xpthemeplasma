/*
    XPThemePlasma — Windows XP bildirim balonları
    Plasma'nın bildirim sunucusundaki açılır bildirimleri XP tepsi balonu olarak gösterir:
    sarı (#FFFFE1) yuvarlak balon, siyah kenar, tepsiye bakan kuyruk, kalın başlık,
    XP balon kapatma düğmesi ve "Sistem Bildirimi" sesi. Aynı anda tek balon (XP gibi sırayla).
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtCore
import QtMultimedia

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
import org.kde.notificationmanager as NotificationManager

PlasmoidItem {
    id: root

    readonly property string varlik: StandardPaths.writableLocation(StandardPaths.GenericDataLocation) + "/xpthemeplasma/varlik/"
    readonly property string sesDosyasi: StandardPaths.writableLocation(StandardPaths.GenericDataLocation) + "/sounds/Windows XP/stereo/message.wav"

    // XP'de bildirim merkezi yoktu: simge yalnızca gizli simgeler listesinde durur
    Plasmoid.status: PlasmaCore.Types.PassiveStatus
    Plasmoid.icon: "preferences-desktop-notification-bell"
    toolTipMainText: "Bildirimler"
    preferredRepresentation: compactRepresentation
    compactRepresentation: Kirigami.Icon { source: Plasmoid.icon }

    NotificationManager.Settings {
        id: ayarlar
    }

    NotificationManager.Notifications {
        id: bildirimler
        limit: 8
        showExpired: false
        showDismissed: false
        showAddedDuringInhibition: false
        showJobs: false
        showNotifications: true
        blacklistedDesktopEntries: ayarlar.popupBlacklistedApplications
        blacklistedNotifyRcNames: ayarlar.popupBlacklistedServices
        sortMode: NotificationManager.Notifications.SortByDate
        sortOrder: Qt.AscendingOrder
        groupMode: NotificationManager.Notifications.GroupDisabled
        urgencies: NotificationManager.Notifications.CriticalUrgency
                   | NotificationManager.Notifications.NormalUrgency
                   | (ayarlar.lowPriorityPopups ? NotificationManager.Notifications.LowUrgency : 0)
    }

    SoundEffect {
        id: ses
        source: root.sesDosyasi
    }

    // Ekran: sistem tepsisinin üstü, sağ alt köşe
    readonly property rect ekran: Plasmoid.containment ? Plasmoid.containment.availableScreenRect : Qt.rect(0, 0, Screen.width, Screen.height - 30)

    Repeater {
        model: bildirimler
        delegate: Item {
            id: balonOge
            required property int index
            required property var model
            readonly property bool etkin: index === 0

            onEtkinChanged: if (etkin) { ses.play(); zaman.restart(); }
            Component.onCompleted: if (etkin) { ses.play(); zaman.start(); }


            Timer {
                id: zaman
                interval: balonOge.model.timeout > 0 ? Math.max(balonOge.model.timeout, 4000) : 7000
                running: balonOge.etkin && !alan.containsMouse && balonOge.model.urgency !== NotificationManager.Notifications.CriticalUrgency
                onTriggered: bildirimler.expire(bildirimler.index(balonOge.index, 0))
            }

            PlasmaCore.Dialog {
                id: balon
                visible: balonOge.etkin
                type: PlasmaCore.Dialog.Notification
                location: PlasmaCore.Types.Floating
                backgroundHints: PlasmaCore.Dialog.NoBackground
                flags: Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
                x: root.ekran.x + root.ekran.width - icerik.width - 8
                y: root.ekran.y + root.ekran.height - icerik.height

                mainItem: Item {
                    id: icerik
                    width: Math.min(360, Math.max(220, govde.implicitWidth + 24))
                    height: govde.implicitHeight + 22 + kuyruk
                    readonly property int kuyruk: 18

                    MouseArea {
                        id: alan
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            const i = bildirimler.index(balonOge.index, 0);
                            if (balonOge.model.hasDefaultAction) {
                                bildirimler.invokeDefaultAction(i, NotificationManager.Notifications.Close);
                            } else {
                                bildirimler.close(i);
                            }
                        }
                    }

                    // balon gövdesi + kuyruk (sağ altta tepsiye bakar)
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            const c = getContext("2d");
                            c.reset();
                            const w = width - 1, h = height - icerik.kuyruk - 1, r = 7;
                            const tx = w - 40;
                            c.beginPath();
                            c.moveTo(0.5 + r, 0.5);
                            c.lineTo(w - r, 0.5);
                            c.arcTo(w, 0.5, w, r, r);
                            c.lineTo(w, h - r);
                            c.arcTo(w, h, w - r, h, r);
                            c.lineTo(tx, h);
                            c.lineTo(tx + 12, height - 1);     // kuyruk ucu
                            c.lineTo(tx - 14, h);
                            c.lineTo(0.5 + r, h);
                            c.arcTo(0.5, h, 0.5, h - r, r);
                            c.lineTo(0.5, 0.5 + r);
                            c.arcTo(0.5, 0.5, 0.5 + r, 0.5, r);
                            c.closePath();
                            c.fillStyle = "#FFFFE1";
                            c.fill();
                            c.strokeStyle = "#000000";
                            c.lineWidth = 1;
                            c.stroke();
                        }
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                    }

                    ColumnLayout {
                        id: govde
                        x: 10
                        y: 9
                        width: icerik.width - 20
                        spacing: 4
                        RowLayout {
                            spacing: 6
                            Layout.fillWidth: true
                            Kirigami.Icon {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                source: balonOge.model.urgency === NotificationManager.Notifications.CriticalUrgency
                                        ? "dialog-warning"
                                        : (balonOge.model.applicationIconName || balonOge.model.iconName || "dialog-information")
                            }
                            Text {
                                Layout.fillWidth: true
                                text: balonOge.model.summary || balonOge.model.applicationName || ""
                                font.family: "Tahoma"
                                font.pixelSize: 11
                                font.bold: true
                                elide: Text.ElideRight
                                color: "#000000"
                                renderType: Text.NativeRendering
                            }
                            Item { Layout.preferredWidth: 18 }   // kapatma düğmesine yer
                        }
                        Text {
                            Layout.fillWidth: true
                            Layout.maximumWidth: 330
                            text: balonOge.model.body || ""
                            visible: text.length > 0
                            textFormat: Text.StyledText
                            wrapMode: Text.Wrap
                            maximumLineCount: 6
                            elide: Text.ElideRight
                            font.family: "Tahoma"
                            font.pixelSize: 11
                            color: "#000000"
                            linkColor: "#0000FF"
                            renderType: Text.NativeRendering
                            onLinkActivated: link => Qt.openUrlExternally(link)
                        }
                    }

                    // XP balon kapatma düğmesi (msstyles Tooltip.Close)
                    MouseArea {
                        id: kapat
                        x: icerik.width - 26
                        y: 7
                        width: 18
                        height: 18
                        hoverEnabled: true
                        onClicked: bildirimler.close(bildirimler.index(balonOge.index, 0))
                        Image {
                            id: kapatImg
                            anchors.fill: parent
                            source: root.varlik + (kapat.pressed ? "balloon-close-pressed.png"
                                                   : (kapat.containsMouse ? "balloon-close-hot.png" : "balloon-close-normal.png"))
                        }
                        Text {
                            visible: kapatImg.status !== Image.Ready
                            anchors.centerIn: parent
                            text: "✕"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }
    }
}
