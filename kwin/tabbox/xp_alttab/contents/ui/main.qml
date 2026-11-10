/*
    XPThemePlasma — Windows XP Alt+Tab (CoolSwitch): 3B yüzey, simge ızgarası, çökük başlık kutusu
    SPDX-FileCopyrightText: 2011 Martin Gräßlin <mgraesslin@kde.org> (KWin değiştirici iskeleti)
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
import org.kde.kwin as KWin

KWin.TabBoxSwitcher {
    id: tabBox
    currentIndex: grid.currentIndex

    readonly property int cell: 43          // XP: 32 px simge + kenar
    readonly property int columns: Math.max(1, Math.min(7, grid.count))
    readonly property int rows: Math.max(1, Math.ceil(grid.count / 7))

    PlasmaCore.Dialog {
        id: dialog
        location: PlasmaCore.Types.Floating
        visible: tabBox.visible
        backgroundHints: PlasmaCore.Dialog.NoBackground
        flags: Qt.ToolTip | Qt.X11BypassWindowManagerHint
        x: tabBox.screenGeometry.x + (tabBox.screenGeometry.width - main.width) / 2
        y: tabBox.screenGeometry.y + (tabBox.screenGeometry.height - main.height) / 2

        mainItem: Rectangle {
            id: main
            width: tabBox.columns * tabBox.cell + 32
            height: tabBox.rows * tabBox.cell + 64
            color: "#ECE9D8"

            // kabarık 3B kenar (DrawEdge EDGE_RAISED)
            Rectangle { width: parent.width; height: 1; color: "#F1EFE2" }
            Rectangle { width: 1; height: parent.height; color: "#F1EFE2" }
            Rectangle { x: 1; y: 1; width: parent.width - 2; height: 1; color: "#FFFFFF" }
            Rectangle { x: 1; y: 1; width: 1; height: parent.height - 2; color: "#FFFFFF" }
            Rectangle { y: parent.height - 1; width: parent.width; height: 1; color: "#716F64" }
            Rectangle { x: parent.width - 1; width: 1; height: parent.height; color: "#716F64" }
            Rectangle { x: 1; y: parent.height - 2; width: parent.width - 2; height: 1; color: "#ACA899" }
            Rectangle { x: parent.width - 2; y: 1; width: 1; height: parent.height - 2; color: "#ACA899" }

            GridView {
                id: grid
                x: 16
                y: 14
                width: tabBox.columns * tabBox.cell
                height: tabBox.rows * tabBox.cell
                cellWidth: tabBox.cell
                cellHeight: tabBox.cell
                interactive: false
                focus: true
                model: tabBox.model
                highlightFollowsCurrentItem: true
                highlightMoveDuration: 0
                highlight: Rectangle {
                    color: "transparent"
                    border.color: "#316AC5"
                    border.width: 2
                }
                delegate: Item {
                    readonly property string cap: model.minimized ? "(" + model.caption + ")" : model.caption
                    width: tabBox.cell
                    height: tabBox.cell
                    Kirigami.Icon {
                        anchors.centerIn: parent
                        width: 32
                        height: 32
                        source: model.icon
                        opacity: model.minimized ? 0.75 : 1.0
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            grid.currentIndex = index;
                            tabBox.model.activate(index);
                        }
                    }
                }
                onCountChanged: if (count === 0) tabBox.visible = false
            }

            // çökük başlık kutusu
            Rectangle {
                x: 10
                y: parent.height - 34
                width: parent.width - 20
                height: 22
                color: "#ECE9D8"
                Rectangle { width: parent.width; height: 1; color: "#ACA899" }
                Rectangle { width: 1; height: parent.height; color: "#ACA899" }
                Rectangle { y: parent.height - 1; width: parent.width; height: 1; color: "#FFFFFF" }
                Rectangle { x: parent.width - 1; width: 1; height: parent.height; color: "#FFFFFF" }
                Text {
                    anchors.fill: parent
                    anchors.margins: 3
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideMiddle
                    text: grid.currentItem ? grid.currentItem.cap : ""
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    color: "#000000"
                    renderType: Text.NativeRendering
                }
            }
        }
    }

    onCurrentIndexChanged: grid.currentIndex = currentIndex
}
