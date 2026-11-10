/*
    XPThemePlasma — yeşil "başlat" düğmesi
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami

MouseArea {
    id: button

    readonly property bool down: pressed || startMenu.visible
    readonly property int h: height > 0 ? height : 30

    implicitWidth: row.implicitWidth + 30
    implicitHeight: 30

    hoverEnabled: true
    acceptedButtons: Qt.LeftButton
    onClicked: kicker.toggleMenu()

    Accessible.role: Accessible.ButtonMenu
    Accessible.name: label.text

    // gövde: sağ köşesi yuvarlak yeşil degrade
    Rectangle {
        id: body
        anchors.fill: parent
        topRightRadius: Math.round(height / 2.6)
        bottomRightRadius: Math.round(height / 2.6)
        border.width: 0
        gradient: Gradient {
            GradientStop { position: 0.00; color: button.down ? "#2B6E2B" : (button.containsMouse ? "#5CBF5C" : "#3E9D3E") }
            GradientStop { position: 0.10; color: button.down ? "#2F7A2F" : (button.containsMouse ? "#7AD77A" : "#62C462") }
            GradientStop { position: 0.22; color: button.down ? "#2D772D" : (button.containsMouse ? "#56BC56" : "#47AE47") }
            GradientStop { position: 0.55; color: button.down ? "#256A25" : (button.containsMouse ? "#3FA63F" : "#359235") }
            GradientStop { position: 0.85; color: button.down ? "#215E21" : (button.containsMouse ? "#3A9C3A" : "#2E882E") }
            GradientStop { position: 0.94; color: button.down ? "#1E561E" : (button.containsMouse ? "#348E34" : "#2A7A2A") }
            GradientStop { position: 1.00; color: "#1A4F1A" }
        }

        // sağ kenarda koyu çizgi + iç parıltı (XP'deki kabarık görünüm)
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            topRightRadius: parent.topRightRadius
            bottomRightRadius: parent.bottomRightRadius
            border.color: "#1D5C1D"
            border.width: 1
            opacity: 0.7
        }
        Rectangle {
            x: 1; y: 1
            width: parent.width - 3
            height: 1
            color: "#9BE09B"
            opacity: button.down ? 0.0 : 0.55
        }
        // sol tarafta hafif ışık
        Rectangle {
            x: 0; y: 0
            width: parent.width * 0.55
            height: parent.height
            opacity: button.down ? 0.0 : 0.22
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }

    RowLayout {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: button.down ? 1 : 0
        x: 9 + (button.down ? 1 : 0)
        spacing: 4

        Item {
            Layout.preferredWidth: Math.min(22, button.h - 6)
            Layout.preferredHeight: Layout.preferredWidth
            Kirigami.Icon {
                id: shadowIcon
                anchors.fill: parent
                anchors.leftMargin: 1
                anchors.topMargin: 1
                source: Plasmoid.configuration.icon
                isMask: true
                color: "#103810"
                opacity: 0.6
            }
            Kirigami.Icon {
                anchors.fill: parent
                source: Plasmoid.configuration.icon
                isMask: Plasmoid.configuration.icon.endsWith("-symbolic")
                color: "#FFFFFF"
            }
        }

        Item {
            Layout.preferredWidth: label.implicitWidth + 2
            Layout.preferredHeight: label.implicitHeight
            Text {
                x: 1; y: 1
                text: label.text
                font: label.font
                color: "#0F3F0F"
                opacity: 0.85
            }
            Text {
                id: label
                text: i18nc("Start button label (lowercase, like Windows XP)", "start")
                color: "#FFFFFF"
                font.family: "Trebuchet MS"
                font.italic: true
                font.bold: true
                font.pixelSize: Math.round(button.h * 0.6)
            }
        }
    }

    PlasmaCore.ToolTipArea {
        anchors.fill: parent
        mainText: i18n("Click here to begin")
        enabled: !startMenu.visible
    }
}
