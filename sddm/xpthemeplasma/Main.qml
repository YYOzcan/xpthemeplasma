/*
    XPThemePlasma — Windows XP "Hoş geldiniz" ekranı (SDDM)
    SPDX-License-Identifier: GPL-3.0-or-later
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#5A7EDC"

    readonly property string uiFont: "Tahoma"
    property int selectedUser: -1
    property int sessionIndex: sessionModel.lastIndex
    property string errorText: ""

    // basit Türkçe/İngilizce metinler (SDDM temalarında i18n yok)
    readonly property bool tr: Qt.locale().name.startsWith("tr")
    function t(en, trText) { return tr ? trText : en; }

    function doLogin() {
        if (selectedUser < 0) {
            return;
        }
        errorText = "";
        sddm.login(userList.currentItem.userName, password.text, root.sessionIndex);
    }

    Connections {
        target: sddm
        function onLoginFailed() {
            root.errorText = root.t("Did you forget your password?", "Parolanızı mı unuttunuz?");
            password.text = "";
            password.forceActiveFocus();
        }
    }

    // --------------------------------------------------------- arka plan
    // orta kısım: sol üstte hafif parıltı
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#7A9BE6" }
            GradientStop { position: 0.45; color: "#5A7EDC" }
            GradientStop { position: 1.0; color: "#5A7EDC" }
        }
    }

    // üst bant
    Rectangle {
        id: topBand
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Math.round(root.height * 0.11)
        color: "#00309C"
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00309C" }
                GradientStop { position: 0.3; color: "#C1D3FB" }
                GradientStop { position: 0.5; color: "#FFFFFF" }
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
        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#003399" }
                GradientStop { position: 0.35; color: "#F99736" }
                GradientStop { position: 0.6; color: "#F99736" }
                GradientStop { position: 1.0; color: "#003399" }
            }
        }

        // Bilgisayarı kapat
        MouseArea {
            id: powerButton
            anchors.left: parent.left
            anchors.leftMargin: 32
            anchors.verticalCenter: parent.verticalCenter
            width: powerRow.implicitWidth
            height: 34
            hoverEnabled: true
            onClicked: sddm.powerOff()
            Row {
                id: powerRow
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    width: 28; height: 28; radius: 5
                    border.color: "#FFFFFF"
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: powerButton.containsMouse ? "#FF8A70" : "#F06A4E" }
                        GradientStop { position: 1.0; color: "#B8301A" }
                    }
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            const c = getContext("2d");
                            c.reset();
                            c.strokeStyle = "#FFFFFF";
                            c.lineWidth = 2.2;
                            c.lineCap = "round";
                            c.beginPath();
                            c.arc(14, 15, 7, -Math.PI * 0.3, Math.PI * 1.3, false);
                            c.stroke();
                            c.beginPath();
                            c.moveTo(14, 6); c.lineTo(14, 14);
                            c.stroke();
                        }
                    }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.t("Turn off computer", "Bilgisayarı kapat")
                    color: "#FFFFFF"
                    font.family: root.uiFont
                    font.pixelSize: 15
                    font.underline: powerButton.containsMouse
                }
            }
        }

        // oturum seçici (XP'de yoktu; ATP / XP arasında geçiş için gerekli)
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 32
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.t("Session:", "Oturum:")
                color: "#D8E4F8"
                font.family: root.uiFont
                font.pixelSize: 12
            }
            ComboBox {
                id: sessionBox
                width: 260
                model: sessionModel
                textRole: "name"
                currentIndex: root.sessionIndex
                onActivated: index => root.sessionIndex = index
                font.family: root.uiFont
            }
        }
    }

    // --------------------------------------------------------- orta
    Item {
        id: middle
        anchors { left: parent.left; right: parent.right; top: topBand.bottom; bottom: bottomBand.top }

        // dikey ayraç
        Rectangle {
            id: divider
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

        // sol: logo + yönerge
        Column {
            anchors.right: divider.left
            anchors.rightMargin: 36
            anchors.verticalCenter: parent.verticalCenter
            spacing: 18
            Item {
                anchors.right: parent.right
                width: logoRow.width
                height: logoRow.height + kdeText.height
                Text {
                    id: kdeText
                    anchors.left: logoRow.left
                    anchors.leftMargin: 4
                    text: "KDE"
                    color: "#FFFFFF"
                    font.family: "Franklin Gothic Medium"
                    font.pixelSize: 15
                    font.bold: true
                }
                Row {
                    id: logoRow
                    anchors.top: kdeText.bottom
                    anchors.topMargin: -6
                    spacing: 2
                    Text {
                        text: "Plasma"
                        color: "#FFFFFF"
                        font.family: "Franklin Gothic Medium"
                        font.pixelSize: 58
                    }
                    Text {
                        text: "xp"
                        color: "#F7931E"
                        font.family: "Franklin Gothic Medium"
                        font.italic: true
                        font.pixelSize: 36
                    }
                }
            }
            Text {
                anchors.right: parent.right
                text: root.t("To begin, click your user name", "Başlamak için kullanıcı adınızı tıklatın")
                color: "#FFFFFF"
                font.family: root.uiFont
                font.pixelSize: 17
            }
        }

        // sağ: kullanıcılar
        ListView {
            id: userList
            anchors.left: divider.right
            anchors.leftMargin: 26
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: Math.min(contentHeight, parent.height * 0.9)
            interactive: contentHeight > height
            spacing: 10
            model: userModel
            currentIndex: root.selectedUser
            clip: true

            delegate: Item {
                id: tile
                required property int index
                required property string name
                required property string realName
                required property string icon
                readonly property string userName: name
                readonly property bool selected: root.selectedUser === index
                width: userList.width
                height: selected ? 120 : 70

                // seçili kullanıcı: sağa doğru solan koyu mavi şerit
                Rectangle {
                    anchors.fill: parent
                    anchors.rightMargin: parent.width * 0.25
                    radius: 8
                    visible: tile.selected
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#00309C" }
                        GradientStop { position: 0.7; color: "#2A52BE" }
                        GradientStop { position: 1.0; color: "#005A7EDC" }
                    }
                }

                MouseArea {
                    id: tileMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.selectedUser = tile.index;
                        root.errorText = "";
                        password.text = "";
                        password.forceActiveFocus();
                    }
                }

                // avatar
                Rectangle {
                    id: frame
                    x: 10
                    y: 10
                    width: 52
                    height: 52
                    radius: 5
                    color: "#FFFFFF"
                    border.width: 2
                    border.color: (tileMouse.containsMouse || tile.selected) ? "#F9B733" : "#FFFFFF"
                    Image {
                        anchors.fill: parent
                        anchors.margins: 3
                        source: tile.icon
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                    }
                }

                Text {
                    anchors.left: frame.right
                    anchors.leftMargin: 12
                    y: 14
                    text: tile.realName.length > 0 ? tile.realName : tile.name
                    color: "#FFFFFF"
                    font.family: root.uiFont
                    font.pixelSize: 20
                    opacity: (tileMouse.containsMouse || tile.selected) ? 1.0 : 0.92
                }

                // parola alanı (yalnızca seçili kullanıcıda)
                Column {
                    visible: tile.selected
                    anchors.left: frame.right
                    anchors.leftMargin: 12
                    y: 48
                    spacing: 4
                    Text {
                        text: root.errorText.length > 0 ? root.errorText : root.t("Type your password", "Parolanızı yazın")
                        color: root.errorText.length > 0 ? "#FFE680" : "#FFFFFF"
                        font.family: root.uiFont
                        font.pixelSize: 12
                    }
                    Row {
                        spacing: 6
                        TextField {
                            id: pwField
                            width: 180
                            height: 24
                            echoMode: TextInput.Password
                            font.family: root.uiFont
                            text: password.text
                            onTextEdited: password.text = text
                            onAccepted: root.doLogin()
                            Component.onCompleted: if (tile.selected) forceActiveFocus()
                            onVisibleChanged: if (visible) forceActiveFocus()
                            background: Rectangle {
                                color: "#FFFFFF"
                                border.color: "#7F9DB9"
                                radius: 2
                            }
                        }
                        // yeşil ok düğmesi
                        MouseArea {
                            id: go
                            width: 24; height: 24
                            hoverEnabled: true
                            onClicked: root.doLogin()
                            Rectangle {
                                anchors.fill: parent
                                radius: 4
                                border.color: "#FFFFFF"
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: go.containsMouse ? "#8EE38A" : "#6FD26B" }
                                    GradientStop { position: 1.0; color: "#1F8A1D" }
                                }
                                Canvas {
                                    anchors.fill: parent
                                    onPaint: {
                                        const c = getContext("2d");
                                        c.reset();
                                        c.fillStyle = "#FFFFFF";
                                        c.beginPath();
                                        c.moveTo(6, 10); c.lineTo(12, 10); c.lineTo(12, 6);
                                        c.lineTo(19, 12); c.lineTo(12, 18); c.lineTo(12, 14);
                                        c.lineTo(6, 14);
                                        c.closePath();
                                        c.fill();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // parola metninin tek kaynağı (temsilciler yeniden oluşsa da kaybolmasın)
    TextInput {
        id: password
        visible: false
    }

    Component.onCompleted: {
        // tek kullanıcı varsa doğrudan seç (XP gibi)
        if (userModel.count === 1) {
            selectedUser = 0;
        } else if (userModel.lastIndex >= 0) {
            selectedUser = userModel.lastIndex;
        }
    }
}
