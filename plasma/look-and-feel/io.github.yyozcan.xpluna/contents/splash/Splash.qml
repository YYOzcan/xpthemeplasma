/*
    XPThemePlasma — XP "Hoş Geldiniz" ekranı (oturum açıldıktan sonra masaüstü yüklenirken)
    Gerçek XP kipinde logonui.exe bitmap'leri, yoksa aynı düzen çizilerek gösterilir.
    SPDX-License-Identifier: GPL-3.0-or-later
*/
import QtQuick
import QtCore

Rectangle {
    id: root
    color: "#5A7EDC"

    property int stage

    readonly property string varlik: StandardPaths.writableLocation(StandardPaths.GenericDataLocation) + "/xpthemeplasma/varlik/"
    readonly property bool gercek: probe.status === Image.Ready
    Image { id: probe; visible: false; source: root.varlik + "logon-topline.png" }

    onStageChanged: {
        if (stage === 6) {
            fade.start();
        }
    }

    // üst bant
    Rectangle {
        id: topBand
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Math.round(root.height * 0.11)
        color: "#00309C"
        Image {
            visible: root.gercek
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            source: root.varlik + "logon-topline.png"
        }
        Rectangle {
            visible: !root.gercek
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00309C" }
                GradientStop { position: 0.4; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "#5A7EDC" }
            }
        }
    }

    // alt bant
    Rectangle {
        id: bottomBand
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: Math.round(root.height * 0.12)
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#003399" }
            GradientStop { position: 1.0; color: "#00287F" }
        }
        Image {
            visible: root.gercek
            anchors.top: parent.top
            width: parent.width
            height: 3
            source: root.varlik + "logon-bottomline.png"
        }
        Rectangle {
            visible: !root.gercek
            anchors.top: parent.top
            width: parent.width
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#003399" }
                GradientStop { position: 0.5; color: "#F99736" }
                GradientStop { position: 1.0; color: "#003399" }
            }
        }
    }

    // orta: parıltı + ayraç + "Hoş Geldiniz"
    Item {
        id: middle
        anchors { left: parent.left; right: parent.right; top: topBand.bottom; bottom: bottomBand.top }

        Image {
            visible: root.gercek
            source: root.varlik + "logon-glow.png"
        }
        Image {
            visible: root.gercek
            x: Math.round(parent.width * 0.5)
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: parent.height * 0.82
            source: root.varlik + "logon-divider.png"
        }
        Rectangle {
            visible: !root.gercek
            x: Math.round(parent.width * 0.5)
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: parent.height * 0.82
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#005A7EDC" }
                GradientStop { position: 0.5; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "#005A7EDC" }
            }
        }

        // XP: logonui "Hoş Geldiniz", arial 36, beyaz italik, ayracın solunda
        Text {
            id: welcome
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            text: "Hoş Geldiniz"
            color: "#FFFFFF"
            font.family: "Arial"
            font.pixelSize: 48
            font.italic: true
            font.bold: true
            style: Text.Raised
            styleColor: "#3555B5"
            opacity: 0
            NumberAnimation on opacity { from: 0; to: 1; duration: 700; running: true }
        }
    }

    OpacityAnimator {
        id: fade
        target: root
        from: 1
        to: 0
        duration: 500
        easing.type: Easing.InOutQuad
    }
}
