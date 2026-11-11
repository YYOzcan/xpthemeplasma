/*
    XPThemePlasma — Windows XP "Hoş Geldiniz" ekranı (SDDM)

    Düzen logonui.exe'nin UIFILE kaynağından (1024x768'de rp = piksel; s = yükseklik/768); kilit ekranıyla
    aynıdır (plasma/shells/.../lockscreen/LockScreenUi.qml). Görseller kendi XP kopyandan: `sudo bash kur.sh
    --sistem` logonui ve msgina bitmap'lerini tema dizinindeki xp/ altına kopyalar; yoksa düzen çizilir.
    XP'de olmayan tek şey alt banttaki küçük oturum seçicisidir (ATP / XP arasında geçiş için).
    SPDX-License-Identifier: GPL-3.0-or-later
*/
import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#5A7EDC"

    readonly property real s: height / 768
    readonly property bool gercek: probe.status === Image.Ready
    function v(ad) { return Qt.resolvedUrl("xp/" + ad + ".png"); }
    Image { id: probe; visible: false; source: root.v("logon-logo") }

    property int secili: -1
    property int sessionIndex: sessionModel.lastIndex
    property string balon: ""
    property bool kapatPenceresi: false

    function sec(i) {
        root.secili = i;
        root.balon = "";
        parola.text = "";
    }
    function gonder() {
        if (root.secili < 0) return;
        root.balon = "";
        sddm.login(userList.itemAtIndex(root.secili).kullanici, parola.text, root.sessionIndex);
    }

    Connections {
        target: sddm
        function onLoginFailed() {
            root.balon = "Parolanızı yeniden yazın.\nHarflerin büyük/küçük durumuna dikkat edin.";
            parola.text = "";
        }
    }

    focus: true
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (root.kapatPenceresi) root.kapatPenceresi = false;
            else root.secili = -1;
            event.accepted = true;
        } else if (root.secili < 0 && !root.kapatPenceresi && userModel.count > 0) {
            root.sec(Math.max(0, userModel.lastIndex));
            if (event.text.length > 0 && event.text.charCodeAt(0) >= 32) parola.text = event.text;
            event.accepted = true;
        }
    }

    // parola metninin tek kaynağı (temsilciler yeniden oluşsa da kaybolmasın)
    TextInput { id: parola; visible: false }

    // ------------------------------------------------------------ üst panel
    Rectangle {
        id: ust
        width: parent.width
        height: Math.round(80 * root.s)
        color: "#00309C"
        Image {
            anchors.bottom: parent.bottom
            width: parent.width; height: 2
            source: root.gercek ? root.v("logon-topline") : ""
        }
        Rectangle {
            visible: !root.gercek
            anchors.bottom: parent.bottom
            width: parent.width; height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00309C" }
                GradientStop { position: 0.4; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "#5A7EDC" }
            }
        }
    }

    // ------------------------------------------------------------ alt panel
    Rectangle {
        id: alt
        anchors.bottom: parent.bottom
        width: parent.width
        height: Math.round(88 * root.s)
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#3934AD" }
            GradientStop { position: 1.0; color: "#00309C" }
        }
        Image {
            width: parent.width; height: 3
            source: root.gercek ? root.v("logon-bottomline") : ""
        }
        Rectangle {
            visible: !root.gercek
            width: parent.width; height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#003399" }
                GradientStop { position: 0.5; color: "#F99736" }
                GradientStop { position: 1.0; color: "#003399" }
            }
        }
        MouseArea {
            id: guc
            x: Math.round(25 * root.s)
            y: Math.round(22 * root.s)
            width: gucSatir.width
            height: gucSatir.height
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.kapatPenceresi = true
            Row {
                id: gucSatir
                spacing: Math.round(4 * root.s)
                Item {
                    width: Math.round(26 * root.s); height: width
                    Image {
                        anchors.fill: parent
                        visible: root.gercek
                        source: root.v(guc.containsMouse ? "logon-power-hot" : "logon-power")
                    }
                    Rectangle {
                        visible: !root.gercek
                        anchors.fill: parent
                        radius: 4
                        border.color: "#FFFFFF"
                        color: guc.containsMouse ? "#F06A4E" : "#D9442B"
                    }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Bilgisayarı Kapat"
                    color: "#FFFFFF"
                    font.family: "Tahoma"
                    font.pixelSize: Math.round(18.7 * root.s)
                    font.underline: guc.containsMouse
                }
            }
        }
        // oturum seçici (XP'de yoktu)
        Row {
            anchors.right: yonerge.left
            anchors.rightMargin: Math.round(30 * root.s)
            y: Math.round(22 * root.s)
            spacing: 6
            visible: sessionModel.rowCount() > 1
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Oturum:"
                color: "#D8E4F8"
                font.family: "Tahoma"
                font.pixelSize: Math.round(12 * root.s)
            }
            ComboBox {
                width: Math.round(220 * root.s)
                height: Math.round(24 * root.s)
                model: sessionModel
                textRole: "name"
                currentIndex: root.sessionIndex
                onActivated: index => root.sessionIndex = index
                font.family: "Tahoma"
                font.pixelSize: Math.round(11 * root.s)
            }
        }
        Text {
            id: yonerge
            anchors.right: parent.right
            anchors.rightMargin: Math.round(25 * root.s)
            y: Math.round(22 * root.s)
            width: Math.round(307 * root.s)
            wrapMode: Text.WordWrap
            text: "Oturum açtıktan sonra hesap ekleyebilir ve değiştirebilirsiniz.\nDenetim Masası'na gidin ve Kullanıcı Hesapları'nı tıklatın."
            color: "#FFFFFF"
            font.family: "Tahoma"
            font.pixelSize: Math.round(12 * root.s)
        }
    }

    // ------------------------------------------------------------ orta
    Item {
        id: orta
        anchors { top: ust.bottom; bottom: alt.top; left: parent.left; right: parent.right }

        Image {
            width: Math.round(219 * root.s); height: Math.round(207 * root.s)
            source: root.gercek ? root.v("logon-glow") : ""
        }
        Image {
            visible: root.gercek
            x: Math.round(parent.width / 2)
            anchors.verticalCenter: parent.verticalCenter
            width: 1; height: Math.round(parent.height * 0.82)
            source: root.v("logon-divider")
        }
        Rectangle {
            visible: !root.gercek
            x: Math.round(parent.width / 2)
            anchors.verticalCenter: parent.verticalCenter
            width: 1; height: Math.round(parent.height * 0.82)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#005A7EDC" }
                GradientStop { position: 0.5; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "#005A7EDC" }
            }
        }

        Item {
            id: logo
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: Math.round(20 * root.s)
            y: Math.round(parent.height / 2 - 106 * root.s)
            width: Math.round(137 * root.s); height: Math.round(86 * root.s)
            Image {
                anchors.fill: parent
                visible: root.gercek
                smooth: true
                source: root.v("logon-logo")
            }
            Text {                                     // çizim kipi: XP varlığı yok
                visible: !root.gercek
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                text: "Windows<font color='#F7931E'><i><sup>xp</sup></i></font>"
                textFormat: Text.RichText
                color: "#FFFFFF"
                font.family: "Franklin Gothic Medium"
                font.pixelSize: Math.round(40 * root.s)
            }
        }
        Text {
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: Math.round(40 * root.s)
            anchors.top: logo.bottom
            anchors.topMargin: Math.round(20 * root.s)
            width: Math.round(344 * root.s)
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WordWrap
            text: "Başlamak için kullanıcı adınızı seçin"
            color: "#EFF7FF"
            font.family: "Arial"
            font.pixelSize: Math.round(18.7 * root.s)
        }

        ListView {
            id: userList
            x: Math.round(parent.width / 2) + 1 + Math.round(26 * root.s)
            width: Math.round(330 * root.s)
            anchors.verticalCenter: parent.verticalCenter
            height: Math.min(contentHeight, parent.height * 0.9)
            interactive: contentHeight > height
            model: userModel
            clip: true

            delegate: Item {
                id: kutucuk
                required property int index
                required property string name
                required property string realName
                required property string icon
                readonly property string kullanici: name
                readonly property bool secili: root.secili === index
                width: userList.width
                height: secili ? Math.round(146 * root.s) : Math.round(80 * root.s)
                opacity: root.secili >= 0 && !secili && !fare.containsMouse ? 0.38 : 1.0     // XP: alpha 96
                Behavior on height { NumberAnimation { duration: 150 } }

                BorderImage {
                    visible: kutucuk.secili && root.gercek
                    anchors.fill: parent
                    anchors.margins: Math.round(2 * root.s)
                    source: root.v("logon-selected")
                    border { left: 8; top: 8; bottom: 8; right: 0 }
                }
                Rectangle {
                    visible: kutucuk.secili && !root.gercek
                    anchors.fill: parent
                    radius: 8
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#00309C" }
                        GradientStop { position: 1.0; color: "#005A7EDC" }
                    }
                }
                MouseArea {
                    id: fare
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: kutucuk.secili ? Qt.ArrowCursor : Qt.PointingHandCursor
                    onClicked: if (!kutucuk.secili) root.sec(kutucuk.index)
                }
                Item {
                    id: cerceve
                    x: Math.round(7 * root.s); y: Math.round(11 * root.s)
                    width: Math.round(58 * root.s); height: width
                    Image {
                        anchors.fill: parent
                        visible: root.gercek
                        source: root.v((kutucuk.secili || fare.containsMouse) ? "logon-tile-hot" : "logon-tile")
                    }
                    Rectangle {
                        visible: !root.gercek
                        anchors.fill: parent
                        radius: 4
                        color: "#FFFFFF"
                        border.width: 3
                        border.color: (kutucuk.secili || fare.containsMouse) ? "#F9B733" : "#FFFFFF"
                    }
                    Image {
                        anchors.centerIn: parent
                        width: Math.round(48 * root.s); height: width
                        fillMode: Image.PreserveAspectCrop
                        // SDDM'nin varsayılan yüzü yerine XP'nin kullanıcı resmi
                        readonly property bool varsayilan: kutucuk.icon.length === 0 || kutucuk.icon.indexOf("/sddm/faces/") >= 0
                                                           || kutucuk.icon.indexOf("default") >= 0
                        source: varsayilan && root.gercek ? root.v("logon-userpic") : kutucuk.icon
                    }
                }
                Text {
                    id: ad
                    x: cerceve.x + cerceve.width + Math.round(7 * root.s)
                    y: Math.round(16 * root.s)
                    width: kutucuk.width - x - 8
                    elide: Text.ElideRight
                    text: kutucuk.realName.length > 0 ? kutucuk.realName : kutucuk.name
                    color: "#EFF7FF"
                    font.family: "Tahoma"
                    font.pixelSize: Math.round(18.7 * root.s)
                }
                Column {
                    visible: kutucuk.secili
                    x: Math.round(71 * root.s)
                    y: Math.round(80 * root.s)
                    spacing: Math.round(3 * root.s)
                    Text {
                        text: "Parolanızı yazın"
                        color: "#FFFFFF"
                        font.family: "Arial"
                        font.pixelSize: Math.round(10.7 * root.s)
                    }
                    Row {
                        spacing: Math.round(5 * root.s)
                        TextField {
                            id: alan
                            width: Math.round(163 * root.s)
                            height: Math.round(26 * root.s)
                            echoMode: TextInput.Password
                            passwordCharacter: "●"
                            font.family: "Arial"
                            font.pixelSize: Math.round(16 * root.s)
                            color: "#000000"
                            leftPadding: 5; rightPadding: 5; topPadding: 0; bottomPadding: 0
                            verticalAlignment: TextInput.AlignVCenter
                            text: parola.text
                            onTextEdited: { parola.text = text; root.balon = ""; }
                            onAccepted: root.gonder()
                            onVisibleChanged: if (visible) forceActiveFocus()
                            Component.onCompleted: if (kutucuk.secili) forceActiveFocus()
                            background: Item {
                                BorderImage {
                                    anchors.fill: parent
                                    visible: root.gercek
                                    source: root.v("logon-password")
                                    border { left: 3; top: 3; right: 5; bottom: 5 }
                                }
                                Rectangle { visible: !root.gercek; anchors.fill: parent; color: "#FFFFFF"; radius: 3 }
                            }
                        }
                        MouseArea {
                            id: git
                            width: Math.round(26 * root.s); height: width
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.gonder()
                            Image {
                                anchors.fill: parent
                                visible: root.gercek
                                source: root.v(git.containsMouse || alan.activeFocus ? "logon-go-hot" : "logon-go")
                            }
                            Rectangle {
                                visible: !root.gercek
                                anchors.fill: parent; radius: 4; border.color: "#FFFFFF"; color: "#3FA43B"
                                Text { anchors.centerIn: parent; text: "➜"; color: "#FFFFFF"; font.pixelSize: parent.height * 0.6 }
                            }
                        }
                        MouseArea {
                            id: yardim
                            width: Math.round(28 * root.s); height: width
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.balon = "Parola ipucu ayarlanmamış.\n\nLütfen parolanızı yeniden yazın.\nHarflerin büyük/küçük durumuna dikkat edin."
                            Image {
                                anchors.fill: parent
                                visible: root.gercek
                                source: root.v(yardim.containsMouse ? "logon-help-hot" : "logon-help")
                            }
                        }
                    }
                }
            }
        }

        // XP sarı balon
        Item {
            id: balonKutu
            visible: root.secili >= 0 && root.balon !== ""
            readonly property Item oge: root.secili >= 0 ? userList.itemAtIndex(root.secili) : null
            x: userList.x + Math.round(81 * root.s)
            y: oge ? userList.y + oge.y + oge.height - Math.round(8 * root.s) : 0
            width: Math.min(Math.round(300 * root.s), balonMetin.implicitWidth + 24)
            height: balonSutun.height + 26
            z: 10
            Canvas {
                anchors.fill: parent
                onPaint: {
                    const c = getContext("2d");
                    c.reset();
                    const w = width - 1, h = height - 1, r = 7, t = 12;
                    c.beginPath();
                    c.moveTo(r + 0.5, t + 0.5);
                    c.lineTo(14.5, t + 0.5); c.lineTo(20.5, 0.5); c.lineTo(30.5, t + 0.5);
                    c.lineTo(w - r, t + 0.5); c.arcTo(w, t + 0.5, w, t + r, r);
                    c.lineTo(w, h - r); c.arcTo(w, h, w - r, h, r);
                    c.lineTo(r + 0.5, h); c.arcTo(0.5, h, 0.5, h - r, r);
                    c.lineTo(0.5, t + r); c.arcTo(0.5, t + 0.5, r + 0.5, t + 0.5, r);
                    c.closePath();
                    c.fillStyle = "#FFFFE1"; c.fill();
                    c.strokeStyle = "#000000"; c.lineWidth = 1; c.stroke();
                }
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
            }
            Column {
                id: balonSutun
                x: 10; y: 18
                width: balonKutu.width - 20
                spacing: 4
                Text {
                    text: "Oturum açma sorunu mu yaşıyorsunuz?"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true
                }
                Text {
                    id: balonMetin
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: root.balon
                    font.family: "Tahoma"; font.pixelSize: 11
                }
            }
        }
    }

    // ------------------------------------------------------------ Bilgisayarı Kapat (msgina.dll)
    Item {
        anchors.fill: parent
        visible: root.kapatPenceresi
        z: 20
        Rectangle { anchors.fill: parent; color: "#000000"; opacity: 0.35 }
        MouseArea { anchors.fill: parent; onClicked: root.kapatPenceresi = false }
        Item {
            anchors.centerIn: parent
            width: 313; height: 198
            MouseArea { anchors.fill: parent }
            Image { anchors.fill: parent; source: root.v("kapat-bg"); visible: status === Image.Ready }
            Rectangle { anchors.fill: parent; color: "#5A7EDC"; visible: !root.gercek; border.color: "#00309C" }
            Text {
                x: 11; width: 243; height: 42
                verticalAlignment: Text.AlignVCenter
                text: "Bilgisayarı Kapat"
                color: "#FFFFFF"
                font.family: "Franklin Gothic Medium"
                font.pointSize: 14
            }
            Image { x: 264; y: 1; source: root.v("kapat-bayrak") }
            component Dugme: Item {
                id: d
                property int kare0
                property bool etkin: true
                property string etiket
                property real etiketX
                property real etiketW
                signal tetik()
                width: 32; height: 32
                Image {
                    source: root.v("kapat-dugme-" + (!d.etkin ? 9 : d.kare0 + (ma.pressed ? 1 : (ma.containsMouse ? 2 : 0))))
                }
                MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; enabled: d.etkin; cursorShape: Qt.PointingHandCursor; onClicked: d.tetik() }
                Text {
                    x: d.etiketX - d.x; y: 39; width: d.etiketW
                    horizontalAlignment: Text.AlignHCenter
                    text: d.etiket
                    color: "#FFFFFF"; font.family: "Tahoma"; font.pointSize: 8
                }
            }
            Dugme { x: 42; y: 80; kare0: 3; etiket: "Beklemede"; etiketX: 15; etiketW: 81; etkin: sddm.canSuspend; onTetik: sddm.suspend() }
            Dugme { x: 131; y: 80; kare0: 0; etiket: "Kapat"; etiketX: 110; etiketW: 75; etkin: sddm.canPowerOff; onTetik: sddm.powerOff() }
            Dugme { x: 234; y: 80; kare0: 6; etiket: "Yeniden Başlat"; etiketX: 207; etiketW: 81; etkin: sddm.canReboot; onTetik: sddm.reboot() }
            Button {
                x: 242; y: 167; width: 60; height: 20
                text: "İptal"
                font.family: "Tahoma"; font.pointSize: 8
                onClicked: root.kapatPenceresi = false
            }
        }
    }
}
