/*
    XPThemePlasma — Windows XP "Bilgisayarı Kapat" / "Oturumu Kapat" penceresi
    Arayüz (sinyaller, bağlam özellikleri) Plasma'nın Logout.qml'iyle aynıdır.
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window

import "timer.js" as AutoTriggerTimer

Item {
    id: root

    signal logoutRequested()
    signal haltRequested()
    signal haltUpdateRequested()
    signal suspendRequested(int spdMethod)
    signal rebootRequested()
    signal rebootRequested2(int opt)
    signal rebootUpdateRequested()
    signal cancelRequested()
    signal lockScreenRequested()
    signal cancelSoftwareUpdateRequested()

    function sleepRequested() { root.suspendRequested(2); }
    function hibernateRequested() { root.suspendRequested(4); }

    // XP otomatik geri sayım yapmazdı; Plasma'nın beklediği özellikleri yine de sağlıyoruz
    property real timeout: 30
    property real remainingTime: root.timeout
    readonly property bool countingDown: false
    readonly property bool showAllOptions: sdtype === ShutdownType.ShutdownTypeDefault
    readonly property bool logoffDialog: sdtype === ShutdownType.ShutdownTypeNone

    function tr(s) { return i18nd("xpthemeplasma", s); }

    QQC2.Action {
        onTriggered: root.cancelRequested()
        shortcut: "Escape"
    }

    // ekranı karart (XP ekranı gri tona çevirirdi)
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.55
    }
    MouseArea {
        anchors.fill: parent
        onClicked: root.cancelRequested()
    }

    // ------------------------------------------------------------ büyük düğme
    component BigButton: MouseArea {
        id: bb
        property string label
        property string kind          // standby | off | restart | lock | logoff
        property color base
        hoverEnabled: true
        implicitWidth: 88
        implicitHeight: 70
        activeFocusOnTab: true
        Keys.onReturnPressed: clicked(null)
        Keys.onSpacePressed: clicked(null)

        Rectangle {
            id: face
            anchors.horizontalCenter: parent.horizontalCenter
            y: 4
            width: 34
            height: 34
            radius: 5
            border.color: "#FFFFFF"
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.lighter(bb.base, bb.containsMouse ? 1.6 : 1.35) }
                GradientStop { position: 0.55; color: bb.containsMouse ? Qt.lighter(bb.base, 1.15) : bb.base }
                GradientStop { position: 1.0; color: Qt.darker(bb.base, 1.3) }
            }
            Canvas {
                anchors.fill: parent
                onPaint: {
                    const c = getContext("2d");
                    c.reset();
                    c.strokeStyle = "#FFFFFF";
                    c.fillStyle = "#FFFFFF";
                    c.lineWidth = 2.6;
                    c.lineCap = "round";
                    const cx = 17, cy = 18;
                    if (bb.kind === "restart") {
                        c.beginPath();
                        c.arc(cx, cy, 8, -Math.PI * 0.15, Math.PI * 1.45, false);
                        c.stroke();
                        c.beginPath();          // ok ucu
                        c.moveTo(cx + 9.5, cy - 9);
                        c.lineTo(cx + 9.5, cy - 1.5);
                        c.lineTo(cx + 2.5, cy - 3.5);
                        c.closePath();
                        c.fill();
                    } else if (bb.kind === "lock") {
                        c.lineWidth = 2.2;
                        c.beginPath();
                        c.arc(cx, cy - 3, 5, Math.PI, 0, false);
                        c.stroke();
                        c.fillRect(cx - 8, cy - 2, 16, 11);
                    } else if (bb.kind === "logoff") {
                        c.lineWidth = 2;
                        c.strokeRect(cx - 9, cy - 9, 10, 18);
                        c.beginPath();
                        c.moveTo(cx - 2, cy); c.lineTo(cx + 9, cy);
                        c.moveTo(cx + 5, cy - 4); c.lineTo(cx + 9, cy); c.lineTo(cx + 5, cy + 4);
                        c.stroke();
                    } else {                     // güç simgesi (bekleme / kapat)
                        c.beginPath();
                        c.arc(cx, cy + 1, 8, -Math.PI * 0.30, Math.PI * 1.30, false);
                        c.stroke();
                        c.beginPath();
                        c.moveTo(cx, cy - 10);
                        c.lineTo(cx, cy);
                        c.stroke();
                    }
                }
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: face.bottom
            anchors.topMargin: 6
            text: bb.label
            color: "#FFFFFF"
            font.family: "Tahoma"
            font.pixelSize: 11
            font.underline: bb.containsMouse || bb.activeFocus
        }
    }

    // ------------------------------------------------------------ pencere
    Rectangle {
        id: dialog
        anchors.centerIn: parent
        width: 314
        height: 200
        color: "#5A7EDC"
        border.color: "#002D96"
        border.width: 1

        MouseArea { anchors.fill: parent }   // arka plan tıklaması iptal etmesin

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // üst lacivert bant
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#0A3CB4" }
                    GradientStop { position: 1.0; color: "#00309C" }
                }
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.logoffDialog ? root.tr("Log Off") : root.tr("Turn off computer")
                    color: "#FFFFFF"
                    font.family: "Franklin Gothic Medium"
                    font.pixelSize: 19
                }
                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: "KDE"
                    color: "#FFFFFF"
                    opacity: 0.85
                    font.family: "Franklin Gothic Medium"
                    font.italic: true
                    font.pixelSize: 15
                }
                Rectangle {   // alt kenardaki ince açık çizgi
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#00309C" }
                        GradientStop { position: 0.5; color: "#C1D3FB" }
                        GradientStop { position: 1.0; color: "#00309C" }
                    }
                }
            }

            // orta: düğmeler
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#6B8DE3" }
                    GradientStop { position: 0.5; color: "#5A7EDC" }
                    GradientStop { position: 1.0; color: "#4D70D3" }
                }
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    // Oturumu kapat penceresi
                    BigButton {
                        visible: root.logoffDialog
                        kind: "lock"; base: "#3FA43B"
                        label: root.tr("Lock")
                        onClicked: root.lockScreenRequested()
                    }
                    BigButton {
                        visible: root.logoffDialog
                        kind: "logoff"; base: "#E3962B"
                        label: root.tr("Log Off")
                        onClicked: root.logoutRequested()
                        Component.onCompleted: if (root.logoffDialog) forceActiveFocus()
                    }
                    // Bilgisayarı kapat penceresi
                    BigButton {
                        visible: !root.logoffDialog && spdMethods.SuspendState
                        kind: "standby"; base: "#E8B23A"
                        label: root.tr("Stand By")
                        onClicked: root.sleepRequested()
                    }
                    BigButton {
                        visible: !root.logoffDialog && maysd
                        kind: "off"; base: "#D9442B"
                        label: root.tr("Turn Off")
                        onClicked: softwareUpdatePending ? root.haltUpdateRequested() : root.haltRequested()
                        Component.onCompleted: if (!root.logoffDialog) forceActiveFocus()
                    }
                    BigButton {
                        visible: !root.logoffDialog && maysd
                        kind: "restart"; base: "#3FA43B"
                        label: root.tr("Restart")
                        onClicked: softwareUpdatePending ? root.rebootUpdateRequested() : root.rebootRequested()
                    }
                }
            }

            // alt lacivert bant + İptal
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: "#00309C"
                QQC2.Button {
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.tr("Cancel")
                    font.family: "Tahoma"
                    onClicked: root.cancelRequested()
                }
            }
        }
    }

    Component.onCompleted: {
        // Plasma bazı durumlarda zamanlayıcı iptal geri çağrısı bekler
        AutoTriggerTimer.addCancelAutoTriggerCallback(function () {});
    }
}
