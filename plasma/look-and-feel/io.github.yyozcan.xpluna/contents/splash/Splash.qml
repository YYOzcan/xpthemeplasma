/*
    XPThemePlasma — XP "Hoş Geldiniz" ekranı (oturum açıldıktan sonra masaüstü yüklenirken)

    Düzen logonui.exe'nin UIFILE kaynağından (1024x768 için, rp = piksel):
      üst panel 80 rp, rgb(0,48,156) + alt kenarda açık çizgi (bitmap 125)
      alt panel rgb(57,52,173) → rgb(0,48,156) yatay degrade + üstte turuncu çizgi (bitmap 126)
      orta rgb(90,126,220), sol üstte parıltı (bitmap 100), ortada ayraç (bitmap 124)
      "Hoş Geldiniz": Arial 36 pt kalın italik, rgb(239,247,255), ayracın 22 rp soluna sağa yaslı;
      gölgesi rgb(49,81,181), 2 rp sağa 3 rp aşağı
    Gerçek XP kipinde logonui bitmap'leri, yoksa aynı düzen çizilerek gösterilir.
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
    readonly property real s: height / 768
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
        height: Math.round(80 * root.s)
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
        height: Math.round(88 * root.s)
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#3934AD" }
            GradientStop { position: 1.0; color: "#00309C" }
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
            width: Math.round(219 * root.s)
            height: Math.round(207 * root.s)
            source: root.varlik + "logon-glow.png"
        }
        Image {
            visible: root.gercek
            x: Math.round(parent.width * 0.5)
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: Math.round(parent.height * 0.82)
            source: root.varlik + "logon-divider.png"
        }
        Rectangle {
            visible: !root.gercek
            x: Math.round(parent.width * 0.5)
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: Math.round(parent.height * 0.82)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#005A7EDC" }
                GradientStop { position: 0.5; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "#005A7EDC" }
            }
        }

        Item {
            id: welcome
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: Math.round(22 * root.s)
            anchors.verticalCenter: parent.verticalCenter
            width: yazi.implicitWidth
            height: yazi.implicitHeight
            opacity: 0
            NumberAnimation on opacity { from: 0; to: 1; duration: 700; running: true }

            Text {
                x: Math.round(2 * root.s)
                y: Math.round(3 * root.s)
                text: yazi.text
                font: yazi.font
                color: "#3151B5"                 // rgb(49,81,181)
            }
            Text {
                id: yazi
                text: "Hoş Geldiniz"
                color: "#EFF7FF"                 // rgb(239,247,255)
                font.family: "Arial"
                font.pixelSize: Math.round(48 * root.s)   // 36 pt
                font.italic: true
                font.bold: true
            }
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
