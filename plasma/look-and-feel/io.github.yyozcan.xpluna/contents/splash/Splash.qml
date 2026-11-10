/*
    XPThemePlasma — XP açılış ekranı (KSplash)
    Siyah zemin, ortada yazı logosu, altında kayan üç mavi blok.
    SPDX-License-Identifier: GPL-3.0-or-later
*/
import QtQuick
import QtCore

Rectangle {
    id: root
    color: "#000000"

    property int stage

    // Gerçek XP açılış ekranı (kendi XP kopyandan içe aktarıldıysa)
    readonly property string varlik: StandardPaths.writableLocation(StandardPaths.GenericDataLocation) + "/xpthemeplasma/varlik/"
    readonly property bool gercek: bootImg.status === Image.Ready

    Image {
        id: bootImg
        // XP 640x480'de çizilir; 4:3 korunarak ekrana ölçeklenir (yanlar siyah)
        readonly property real k: Math.min(root.width / 640, root.height / 480)
        width: 640 * k
        height: 480 * k
        anchors.centerIn: parent
        source: root.varlik + "boot.png"
        smooth: true
        visible: root.gercek

        // ilerleme kutusunun içi: x 258..377, y 354..362 (640x480 koordinatları)
        Item {
            x: 258 * bootImg.k
            y: 354 * bootImg.k
            width: (377 - 258 + 1) * bootImg.k
            height: 9 * bootImg.k
            clip: true
            Image {
                id: bootBlocks
                width: 22 * bootImg.k
                height: 9 * bootImg.k
                source: root.varlik + "boot-blok.png"
                smooth: false
                // XP: bloklar 8 piksellik adımlarla soldan sağa kayar
                property int adim: 0
                x: (adim * 8 - 22) * bootImg.k
                Timer {
                    interval: 50
                    running: root.gercek
                    repeat: true
                    onTriggered: bootBlocks.adim = (bootBlocks.adim + 1) % 19
                }
            }
        }
    }

    onStageChanged: {
        if (stage === 6) {
            fade.start();
        }
    }

    Item {
        id: logo
        visible: !root.gercek
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -40
        width: row.width
        height: row.height + kde.height

        Text {
            id: kde
            anchors.left: row.left
            anchors.leftMargin: 4
            text: "KDE"
            color: "#FFFFFF"
            font.family: "Franklin Gothic Medium"
            font.pixelSize: 16
            font.bold: true
        }
        Row {
            id: row
            anchors.top: kde.bottom
            anchors.topMargin: -6
            spacing: 2
            Text {
                text: "Plasma"
                color: "#FFFFFF"
                font.family: "Franklin Gothic Medium"
                font.pixelSize: 64
            }
            Text {
                text: "xp"
                color: "#F7931E"
                font.family: "Franklin Gothic Medium"
                font.italic: true
                font.pixelSize: 40
                anchors.top: parent.top
                anchors.topMargin: 2
            }
        }
    }

    // ilerleme kutusu: 3 mavi blok soldan sağa kayar
    Rectangle {
        id: box
        visible: !root.gercek
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: logo.bottom
        anchors.topMargin: 60
        width: 122
        height: 18
        radius: 4
        color: "#000000"
        border.color: "#B2B2B2"
        border.width: 2
        clip: true

        Item {
            id: blocks
            x: -width
            y: 4
            width: 3 * 9 + 2 * 2
            height: 10
            Row {
                spacing: 2
                Repeater {
                    model: 3
                    Rectangle {
                        width: 9
                        height: 10
                        radius: 1
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#2838C8" }
                            GradientStop { position: 0.35; color: "#6C86FF" }
                            GradientStop { position: 0.65; color: "#4F6BED" }
                            GradientStop { position: 1.0; color: "#1E2CA8" }
                        }
                    }
                }
            }
            SequentialAnimation on x {
                loops: Animation.Infinite
                NumberAnimation { from: -blocks.width; to: box.width; duration: 1800 }
            }
        }
    }

    Text {
        visible: !root.gercek
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 30
        text: "XPThemePlasma"
        color: "#8C8C8C"
        font.family: "Tahoma"
        font.pixelSize: 11
    }

    OpacityAnimator {
        id: fade
        target: root
        from: 1
        to: 0
        duration: 600
        easing.type: Easing.InOutQuad
    }
}
