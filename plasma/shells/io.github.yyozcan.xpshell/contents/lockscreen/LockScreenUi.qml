/*
    XPThemePlasma — XP "Hoş Geldiniz" ekranı olarak kilit ekranı (Windows+L, ekran koruyucu dönüşü)

    Düzen logonui.exe'nin UIFILE kaynağından (1024x768'de rp = piksel; burada s = yükseklik/768):
      üst panel 80, alt panel (Bilgisayarı Kapat + açıklama), sol panelde XP logosu ve
      "Başlamak için kullanıcı adınızı seçin", 1 piksel ayraç, sağda kullanıcı kutucukları (58x58 resim
      çerçevesi, Tahoma 14 pt ad, seçilince turuncu çerçeve ve parola kutusu, yeşil ok, ? düğmesi).
    Görseller kendi XP kopyandan (logonui.exe, msgina.dll); yoksa aynı düzen çizilir.

    Kimlik doğrulama Plasma 6.8 kilidiyle aynıdır (authenticator.respond / onSucceeded / PAM zaman aşımı).
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtCore
import org.kde.kscreenlocker as ScreenLocker
import org.kde.plasma.private.sessions
import org.kde.plasma.private.keyboardindicator as KeyboardIndicator

Rectangle {
    id: ui
    color: "#5A7EDC"                                   // rgb(90,126,220)

    readonly property real s: height / 768
    readonly property string varlik: StandardPaths.writableLocation(StandardPaths.GenericDataLocation) + "/xpthemeplasma/varlik/"
    readonly property bool gercek: yokla.status === Image.Ready
    Image { id: yokla; visible: false; source: ui.varlik + "logon-logo.png" }
    function v(ad) { return ui.varlik + ad + ".png"; }

    property bool secili: false                        // kullanıcı kutucuğu seçildi mi
    property bool expectingPrompt: false
    property bool showPrompt: false
    property string pendingPassword
    property string balon: ""                          // sarı balon metni
    property bool kapatPenceresi: false

    readonly property string kullanici: kscreenlocker_userName
    readonly property string resim: kscreenlocker_userImage !== ""
        ? "file://" + kscreenlocker_userImage.split("/").map(encodeURIComponent).join("/") : ""

    function sec() {
        if (!ui.secili) {
            ui.secili = true;
            authenticator.startAuthenticating();
        }
        parola.forceActiveFocus();
    }
    function gonder() {
        if (parola.text.length === 0 && ui.expectingPrompt === false && authenticator.hadPrompt === false) {
            // parolasız hesap: XP oku doğrudan açardı
        }
        ui.balon = "";
        if (authenticator.pamTimeout) {
            ui.pendingPassword = parola.text;
            parola.enabled = false;
        } else {
            authenticator.respond(parola.text);
        }
    }

    Connections {
        target: root
        function onClearPassword() { parola.text = ""; }
    }

    Connections {
        target: authenticator
        function onFailed(kind) {
            if (kind != 0) return;                     // parolasız yöntemlerden gelen
            parola.text = "";
            parola.enabled = true;
            ui.balon = "Parolanızı yeniden yazın.\nHarflerin büyük/küçük durumuna dikkat edin.";
            authenticator.startAuthenticating();
            parola.forceActiveFocus();
        }
        function onSucceeded() {
            authenticator.stopAuthenticating();
            Qt.quit();
        }
        function onPamTimeoutChanged() {
            if (!authenticator.pamTimeout && ui.pendingPassword.length > 0) {
                authenticator.respond(ui.pendingPassword);
                ui.pendingPassword = "";
                parola.enabled = true;
            }
        }
        function onErrorMessageChanged() {
            if (authenticator.errorMessage) ui.balon = authenticator.errorMessage;
        }
        function onPromptForSecretChanged() {
            if (ui.secili) parola.forceActiveFocus();
        }
    }

    // varsayılan doğrulayıcının (parola) özellikleri
    Repeater {
        model: ScreenLocker.AuthenticatorModel
        delegate: Item {
            required property int type
            required property bool expectingPrompt
            required property bool showPrompt
            Component.onCompleted: {
                if (authenticator.authenticator === type) {
                    ui.expectingPrompt = expectingPrompt;
                    ui.showPrompt = showPrompt;
                }
            }
        }
    }

    Timer {                                            // arka ucu canlı tut
        interval: 1000
        running: !authenticator.unlocked
        repeat: true
        onTriggered: authenticator.startAuthenticating()
    }

    SessionManagement { id: oturum }
    KeyboardIndicator.KeyState { id: buyukHarf; key: Qt.Key_CapsLock }

    focus: true
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (ui.kapatPenceresi) ui.kapatPenceresi = false;
            else { ui.secili = false; parola.text = ""; ui.balon = ""; }
            event.accepted = true;
            return;
        }
        if (!ui.secili && !ui.kapatPenceresi) {
            ui.sec();
            if (event.text.length > 0 && event.text.charCodeAt(0) >= 32) parola.text = event.text;
            event.accepted = true;
        }
    }
    MouseArea { anchors.fill: parent; onClicked: ui.forceActiveFocus() }

    // ------------------------------------------------------------ üst panel
    Rectangle {
        id: ust
        width: parent.width
        height: Math.round(80 * ui.s)
        color: "#00309C"
        Image {
            anchors.bottom: parent.bottom
            width: parent.width; height: 2
            source: ui.gercek ? ui.v("logon-topline") : ""
        }
    }

    // ------------------------------------------------------------ alt panel
    Rectangle {
        id: alt
        anchors.bottom: parent.bottom
        width: parent.width
        height: Math.round(88 * ui.s)
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#3934AD" }
            GradientStop { position: 1.0; color: "#00309C" }
        }
        Image {
            width: parent.width; height: 3
            source: ui.gercek ? ui.v("logon-bottomline") : ""
        }
        MouseArea {
            id: guc
            x: Math.round(25 * ui.s)
            y: Math.round(22 * ui.s)
            width: gucSatir.width
            height: gucSatir.height
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: ui.kapatPenceresi = true
            Row {
                id: gucSatir
                spacing: Math.round(4 * ui.s)
                Image {
                    width: Math.round(26 * ui.s); height: width
                    source: ui.gercek ? ui.v(guc.containsMouse ? "logon-power-hot" : "logon-power") : ""
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Bilgisayarı Kapat"
                    color: "#FFFFFF"
                    font.family: "Tahoma"
                    font.pixelSize: Math.round(18.7 * ui.s)          // 14 pt
                    font.underline: guc.containsMouse
                }
            }
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: Math.round(25 * ui.s)
            y: Math.round(22 * ui.s)
            width: Math.round(307 * ui.s)
            wrapMode: Text.WordWrap
            text: "Oturum açtıktan sonra hesap ekleyebilir ve değiştirebilirsiniz.\nDenetim Masası'na gidin ve Kullanıcı Hesapları'nı tıklatın."
            color: "#FFFFFF"
            font.family: "Tahoma"
            font.pixelSize: Math.round(12 * ui.s)                    // 9 pt
        }
    }

    // ------------------------------------------------------------ orta
    Item {
        id: orta
        anchors { top: ust.bottom; bottom: alt.top; left: parent.left; right: parent.right }

        Image {
            width: Math.round(219 * ui.s); height: Math.round(207 * ui.s)
            source: ui.gercek ? ui.v("logon-glow") : ""
        }
        Image {
            x: Math.round(parent.width / 2)
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: Math.round(parent.height * 0.82)
            source: ui.gercek ? ui.v("logon-divider") : ""
        }

        // sol: logo ve yönerge
        Image {
            id: logo
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: Math.round(20 * ui.s)
            y: Math.round(parent.height / 2 - 106 * ui.s)
            width: Math.round(137 * ui.s); height: Math.round(86 * ui.s)
            smooth: true
            source: ui.gercek ? ui.v("logon-logo") : ""
        }
        Text {
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: Math.round(40 * ui.s)
            anchors.top: logo.bottom
            anchors.topMargin: Math.round(20 * ui.s)
            width: Math.round(344 * ui.s)
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WordWrap
            text: "Başlamak için kullanıcı adınızı seçin"
            color: "#EFF7FF"
            font.family: "Arial"
            font.pixelSize: Math.round(18.7 * ui.s)                  // 14 pt
        }

        // sağ: kullanıcı kutucuğu
        Item {
            id: kutucuk
            x: Math.round(parent.width / 2) + 1 + Math.round(26 * ui.s)
            width: Math.round(330 * ui.s)
            height: ui.secili ? Math.round(146 * ui.s) : Math.round(80 * ui.s)
            anchors.verticalCenter: parent.verticalCenter
            Behavior on height { NumberAnimation { duration: 150 } }

            // seçili arka plan (bitmap 112, sola doğru solar)
            BorderImage {
                visible: ui.secili && ui.gercek
                anchors.fill: parent
                anchors.margins: Math.round(2 * ui.s)
                source: ui.v("logon-selected")
                border { left: 8; top: 8; bottom: 8; right: 0 }
            }
            MouseArea {
                id: kutucukFare
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: ui.secili ? Qt.ArrowCursor : Qt.PointingHandCursor
                onClicked: ui.sec()
            }
            // resim çerçevesi 58x58 (bitmap 113; üzerinde/seçili 119)
            Image {
                id: cerceve
                x: Math.round(7 * ui.s); y: Math.round(11 * ui.s)
                width: Math.round(58 * ui.s); height: width
                source: ui.gercek ? ui.v((ui.secili || kutucukFare.containsMouse) ? "logon-tile-hot" : "logon-tile") : ""
                Image {
                    anchors.centerIn: parent
                    width: Math.round(48 * ui.s); height: width
                    fillMode: Image.PreserveAspectCrop
                    source: ui.resim !== "" ? ui.resim : (ui.gercek ? ui.v("logon-userpic") : "")
                }
            }
            Text {
                id: ad
                x: cerceve.x + cerceve.width + Math.round(7 * ui.s)
                y: Math.round(16 * ui.s)
                width: kutucuk.width - x - 8
                elide: Text.ElideRight
                text: ui.kullanici
                color: "#EFF7FF"
                font.family: "Tahoma"
                font.pixelSize: Math.round(18.7 * ui.s)              // 14 pt
            }
            Text {
                anchors.left: ad.left
                anchors.top: ad.bottom
                anchors.topMargin: Math.round(2 * ui.s)
                visible: !ui.secili
                text: "Oturum Açılmış Durumda"
                color: "#00309C"
                font.family: "Tahoma"
                font.pixelSize: Math.round(10.7 * ui.s)              // 8 pt kalın
                font.bold: true
            }

            // parola bölmesi (71 rp girintili)
            Column {
                visible: ui.secili
                x: Math.round(71 * ui.s)
                y: Math.round(80 * ui.s)
                spacing: Math.round(3 * ui.s)
                Text {
                    text: "Parolanızı yazın"
                    color: "#FFFFFF"
                    font.family: "Arial"
                    font.pixelSize: Math.round(10.7 * ui.s)          // 8 pt
                }
                Row {
                    spacing: Math.round(5 * ui.s)
                    TextField {
                        id: parola
                        width: Math.round(163 * ui.s)
                        height: Math.round(26 * ui.s)
                        echoMode: TextInput.Password
                        passwordCharacter: "●"
                        font.family: "Arial"
                        font.pixelSize: Math.round(16 * ui.s)
                        color: "#000000"
                        leftPadding: 5; rightPadding: 5; topPadding: 0; bottomPadding: 0
                        verticalAlignment: TextInput.AlignVCenter
                        background: BorderImage {
                            source: ui.gercek ? ui.v("logon-password") : ""
                            border { left: 3; top: 3; right: 5; bottom: 5 }
                            Rectangle { visible: !ui.gercek; anchors.fill: parent; color: "#FFFFFF"; radius: 3 }
                        }
                        Keys.onReturnPressed: ui.gonder()
                        Keys.onEnterPressed: ui.gonder()
                        onTextEdited: ui.balon = ""
                    }
                    Image {
                        id: git
                        width: Math.round(26 * ui.s); height: width
                        source: ui.gercek ? ui.v(gitFare.containsMouse || parola.activeFocus ? "logon-go-hot" : "logon-go") : ""
                        MouseArea { id: gitFare; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: ui.gonder() }
                    }
                    Image {
                        width: Math.round(28 * ui.s); height: width
                        source: ui.gercek ? ui.v(yardimFare.containsMouse ? "logon-help-hot" : "logon-help") : ""
                        MouseArea {
                            id: yardimFare
                            anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: ui.balon = "Parola ipucu ayarlanmamış.\n\nLütfen parolanızı yeniden yazın.\nHarflerin büyük/küçük durumuna dikkat edin."
                        }
                    }
                }
            }
        }

        // XP sarı balon ipucu (yanlış parola, büyük harf kilidi)
        Item {
            id: balonKutu
            visible: ui.secili && (ui.balon !== "" || (buyukHarf.locked && parola.activeFocus))
            x: kutucuk.x + Math.round(71 * ui.s) + 10
            y: kutucuk.y + kutucuk.height - Math.round(8 * ui.s)
            width: Math.min(Math.round(300 * ui.s), balonMetin.implicitWidth + 24)
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
                    text: ui.balon !== "" ? "Oturum açma sorunu mu yaşıyorsunuz?" : "Caps Lock Açık"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true
                    color: "#000000"
                }
                Text {
                    id: balonMetin
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: ui.balon !== "" ? ui.balon
                        : "Caps Lock açıkken parolanızı yanlış girebilirsiniz.\n\nParolanızı yazmadan önce Caps Lock tuşuna basarak kapatmalısınız."
                    font.family: "Tahoma"; font.pixelSize: 11
                    color: "#000000"
                }
            }
        }
    }

    // ------------------------------------------------------------ Bilgisayarı Kapat (msgina.dll)
    Item {
        anchors.fill: parent
        visible: ui.kapatPenceresi
        z: 20
        Rectangle { anchors.fill: parent; color: "#000000"; opacity: 0.35 }
        MouseArea { anchors.fill: parent; onClicked: ui.kapatPenceresi = false }
        Item {
            id: kp
            anchors.centerIn: parent
            width: 313; height: 198
            MouseArea { anchors.fill: parent }
            Image { anchors.fill: parent; source: ui.v("kapat-bg") }
            Text {
                x: 11; width: 243; height: 42
                verticalAlignment: Text.AlignVCenter
                text: "Bilgisayarı Kapat"
                color: "#FFFFFF"
                font.family: "Franklin Gothic Medium"
                font.pointSize: 14
            }
            Image { x: 264; y: 1; source: ui.v("kapat-bayrak") }
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
                    source: ui.v("kapat-dugme-" + (!d.etkin ? 9 : d.kare0 + (ma.pressed ? 1 : (ma.containsMouse ? 2 : 0))))
                }
                MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; enabled: d.etkin; cursorShape: Qt.PointingHandCursor; onClicked: d.tetik() }
                Text {
                    x: d.etiketX - d.x; y: 39; width: d.etiketW
                    horizontalAlignment: Text.AlignHCenter
                    text: d.etiket
                    color: "#FFFFFF"; font.family: "Tahoma"; font.pointSize: 8
                }
            }
            Dugme {
                x: 42; y: 80; kare0: 3; etiket: "Beklemede"; etiketX: 15; etiketW: 81
                etkin: oturum.canSuspend
                onTetik: { ui.kapatPenceresi = false; oturum.suspend(); }
            }
            Dugme {
                x: 131; y: 80; kare0: 0; etiket: "Kapat"; etiketX: 110; etiketW: 75
                etkin: oturum.canShutdown
                onTetik: { ui.kapatPenceresi = false; oturum.requestShutdown(0); }
            }
            Dugme {
                x: 234; y: 80; kare0: 6; etiket: "Yeniden Başlat"; etiketX: 207; etiketW: 81
                etkin: oturum.canReboot
                onTetik: { ui.kapatPenceresi = false; oturum.requestReboot(0); }
            }
            Button {
                x: 242; y: 167; width: 60; height: 20
                text: "İptal"
                font.family: "Tahoma"; font.pointSize: 8
                onClicked: ui.kapatPenceresi = false
            }
        }
    }

    Loader {
        z: 30
        active: root.viewVisible
        source: "LockOsd.qml"
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 20 }
    }

    Component.onCompleted: {
        authenticator.startAuthenticating();
        ui.forceActiveFocus();
    }
}
