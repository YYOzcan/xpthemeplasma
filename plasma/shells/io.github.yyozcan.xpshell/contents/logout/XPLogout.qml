/*
    XPThemePlasma — Windows XP "Bilgisayarı Kapat" / "Windows Oturumunu Kapat" pencereleri
    ve ardından gelen logonui durum ekranı ("Ayarlarınız kaydediliyor...", "Windows kapatılıyor...").

    Gerçek XP kipinde görseller kendi XP kopyandan gelir: msgina.dll (Bilgisayarı Kapat: 313x198
    arka plan, bayrak, 10 durumlu düğme şeridi), shell32.dll (Oturumu Kapat), logonui.exe (durum ekranı).
    Yerleşim msgina.dll'nin 20100 ve shell32.dll'nin iletişim kutusu kaynaklarından (iletişim birimi → piksel,
    Tahoma 8: x*1.5, y*1.625).

    Bir eylem seçilince pencere kapanmaz: XP gibi önce durum ekranı gelir, XP sesi çalar, ses bitince
    oturum gerçekten kapatılır. Ekran, oturum sonlanana dek açık kalır; bir uygulama kapanmayı
    durdurursa 20 sn sonra çekilir ki uygulamanın sorusu görülebilsin.

    Arayüz (sinyaller, bağlam özellikleri) Plasma'nın Logout.qml'iyle aynıdır.
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import QtCore
import QtMultimedia
import org.kde.plasma.private.sessions as Sessions

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

    readonly property string veri: StandardPaths.writableLocation(StandardPaths.GenericDataLocation)
    readonly property string varlik: veri + "/xpthemeplasma/varlik/"
    readonly property string sesler: veri + "/sounds/Windows XP/stereo/"
    readonly property bool gercek: yokla.status === Image.Ready
    Image { id: yokla; visible: false; source: root.varlik + "kapat-bg.png" }

    readonly property bool hazirdaBekletVar: spdMethods.HibernateState === true
    property bool shiftBasili: false
    property bool altBasili: false
    property string durum: ""              // "" | kapat | yeniden | oturum
    property string durumMetni: ""
    property bool deneme: false            // --windowed xp-test-durum=<tür>: yalnızca ekranı göster

    function tr(s) { return i18nd("xpthemeplasma", s); }

    Sessions.SessionManagement { id: oturum }

    // ------------------------------------------------------------ klavye
    focus: true
    Keys.onPressed: (e) => {
        if (e.key === Qt.Key_Shift) root.shiftBasili = true;
        if (e.key === Qt.Key_Alt) root.altBasili = true;
    }
    Keys.onReleased: (e) => {
        if (e.key === Qt.Key_Shift) root.shiftBasili = false;
        if (e.key === Qt.Key_Alt) root.altBasili = false;
    }
    QQC2.Action {
        enabled: root.durum === ""
        onTriggered: root.cancelRequested()
        shortcut: "Escape"
    }

    // ------------------------------------------------------------ eylemler
    function bekleme() {
        if (root.shiftBasili && root.hazirdaBekletVar) root.hibernateRequested();
        else root.sleepRequested();
    }
    function kapat() {
        if (softwareUpdatePending) { root.haltUpdateRequested(); return; }
        durumBaslat("kapat");
    }
    function yenidenBaslat() {
        if (softwareUpdatePending) { root.rebootUpdateRequested(); return; }
        durumBaslat("yeniden");
    }
    function oturumuKapat() { durumBaslat("oturum"); }
    function kullaniciDegistir() {
        oturum.switchUser();
        root.cancelRequested();
    }

    // XP sırası — Oturumu Kapat: "Oturum kapatılıyor..." (oturum kapatma sesi) → "Ayarlarınız kaydediliyor..."
    //             Kapat/Yeniden Başlat: "Ayarlarınız kaydediliyor..." → "Windows kapatılıyor..." (kapanış sesi)
    function durumBaslat(tur) {
        root.durum = tur;
        ipucu.hedef = null;
        asama.adim = 0;
        if (tur === "oturum") {
            root.durumMetni = "Oturum kapatılıyor...";
            sesCal("service-logout.wav");
            asama.interval = 2300;
        } else {
            root.durumMetni = "Ayarlarınız kaydediliyor...";
            asama.interval = 1500;
        }
        asama.restart();
    }
    function sesCal(ad) {
        if (Window.window && Window.window.screen && Qt.application.screens.length > 0
                && Window.window.screen.name !== Qt.application.screens[0].name)
            return;                 // birden çok ekranda ses bir kez çalsın
        ses.source = root.sesler + ad;          // StandardPaths zaten file:// URL verir
        ses.play();
    }
    SoundEffect { id: ses; volume: 1.0 }

    Timer {
        id: asama
        property int adim: 0
        onTriggered: {
            if (adim === 0) {
                adim = 1;
                if (root.durum === "oturum") {
                    root.durumMetni = "Ayarlarınız kaydediliyor...";
                    interval = 1200;
                } else {
                    root.durumMetni = "Windows kapatılıyor...";
                    root.sesCal("desktop-logout.wav");
                    interval = 3500;            // xpshutdn.wav 3.2 sn: ses bitsin
                }
                restart();
                return;
            }
            if (root.deneme) return;
            if (root.durum === "oturum") oturum.requestLogout(0);
            else if (root.durum === "yeniden") oturum.requestReboot(0);
            else oturum.requestShutdown(0);
            bekci.start();
        }
    }
    Timer {
        id: bekci                       // oturum kapanmadıysa (ör. kaydedilmemiş belge) ekranı çek
        interval: 20000
        onTriggered: root.cancelRequested()
    }

    // ------------------------------------------------------------ arka plan
    // XP masaüstünü yavaşça griye çevirirdi: bunu xp_gri KWin efekti yapar. Burada yalnızca tıklama yakalanır.
    MouseArea {
        anchors.fill: parent
        enabled: root.durum === ""
        onClicked: root.cancelRequested()
    }

    // ------------------------------------------------------------ gerçek XP penceresi
    component XPDugme: Item {
        id: d
        property string serit              // kapat-dugme | oturum-dugme
        property int kare0                 // şeritteki ilk kare (normal, basılı, üzerinde)
        property bool etkin: true
        property string baslik
        property string aciklama
        signal tetiklendi()
        width: 32
        height: 32
        readonly property bool uzerinde: ma.containsMouse
        Image {
            source: root.varlik + d.serit + "-"
                    + (!d.etkin ? 9 : d.kare0 + (ma.pressed && ma.containsMouse ? 1 : (ma.containsMouse ? 2 : 0))) + ".png"
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            enabled: d.etkin
            cursorShape: Qt.PointingHandCursor
            onClicked: d.tetiklendi()
            onContainsMouseChanged: {
                if (containsMouse) { ipucu.hedef = d; ipucuZaman.restart(); }
                else if (ipucu.hedef === d) { ipucu.hedef = null; ipucuZaman.stop(); }
            }
        }
    }
    component Etiket: Text {
        property string ad
        property int harf: 0               // altı çizili harf (Alt basılınca, XP klavye ipuçları)
        height: 13
        horizontalAlignment: Text.AlignHCenter
        color: "#FFFFFF"
        font.family: "Tahoma"
        font.pointSize: 8
        textFormat: Text.StyledText
        text: root.altBasili ? ad.substring(0, harf) + "<u>" + ad.charAt(harf) + "</u>" + ad.substring(harf + 1) : ad
    }

    Item {
        id: pencere
        visible: root.gercek && root.durum === ""
        anchors.centerIn: parent
        width: 313
        height: 198

        MouseArea { anchors.fill: parent }   // pencereye tıklamak iptal etmesin

        Image {
            anchors.fill: parent
            source: root.varlik + (root.logoffDialog ? "oturum-bg.png" : "kapat-bg.png")
        }
        Text {
            x: 11
            width: 243
            height: 42
            verticalAlignment: Text.AlignVCenter
            text: root.logoffDialog ? "Windows Oturumunu Kapat" : "Bilgisayarı Kapat"
            color: "#FFFFFF"
            font.family: "Franklin Gothic Medium"
            font.pointSize: 14
        }
        Image {
            x: 264
            y: 1
            source: root.varlik + (root.logoffDialog ? "oturum-bayrak.png" : "kapat-bayrak.png")
        }

        // --- Bilgisayarı Kapat
        XPDugme {
            id: dBekleme
            visible: !root.logoffDialog
            x: 42; y: 80
            serit: "kapat-dugme"; kare0: 3
            etkin: spdMethods.SuspendState === true
            baslik: root.shiftBasili && root.hazirdaBekletVar ? "Hazırda Beklet" : "Beklemede"
            aciklama: root.shiftBasili && root.hazirdaBekletVar
                      ? "Geçerli masaüstü durumunuzu kaldığınız yerden devam edebilmeniz için sabit diskinize kaydeder; ardından bilgisayarınızı kapatır."
                      : (root.hazirdaBekletVar
                         ? "Bilgisayarınızı düşük güç durumuna geçirir, böylece Windows oturumunuzu çabucak sürdürebilirsiniz.\n\nBunun yerine hazırda bekletmek istiyorsanız, ÜstKrkt tuşunu basılı tutun."
                         : "Bilgisayarınızı düşük güç durumuna geçirir, böylece Windows oturumunuzu çabucak sürdürebilirsiniz.")
            onTetiklendi: root.bekleme()
        }
        XPDugme {
            id: dKapat
            visible: !root.logoffDialog
            x: 131; y: 80
            serit: "kapat-dugme"; kare0: 0
            etkin: maysd
            baslik: "Kapat"
            aciklama: "Bilgisayarınızı güvenle kapatabilmeniz için Windows'u kapatır."
            onTetiklendi: root.kapat()
        }
        XPDugme {
            id: dYeniden
            visible: !root.logoffDialog
            x: 234; y: 80
            serit: "kapat-dugme"; kare0: 6
            etkin: maysd
            baslik: "Yeniden Başlat"
            aciklama: "Windows'u kapatıp, yeniden başlatır."
            onTetiklendi: root.yenidenBaslat()
        }
        Etiket { visible: !root.logoffDialog; x: 15; y: 119; width: 81; ad: dBekleme.baslik; harf: 0 }
        Etiket { visible: !root.logoffDialog; x: 110; y: 119; width: 75; ad: "Kapat"; harf: 0 }
        Etiket { visible: !root.logoffDialog; x: 207; y: 119; width: 81; ad: "Yeniden Başlat"; harf: 9 }

        // --- Windows Oturumunu Kapat
        XPDugme {
            id: dDegistir
            visible: root.logoffDialog
            x: 83; y: 80
            serit: "oturum-dugme"; kare0: 0
            etkin: oturum.canSwitchUser
            baslik: "Kullanıcı Değiştir"
            aciklama: "Program ve dosyalarınız açıkken başka bir kullanıcının oturum açmasına olanak sağlar.\n\n(Windows logo tuşu + L birleşimini kullanarak kullanıcıları değiştirebilirsiniz.)"
            onTetiklendi: root.kullaniciDegistir()
        }
        XPDugme {
            id: dOturum
            visible: root.logoffDialog
            x: 198; y: 80
            serit: "oturum-dugme"; kare0: 3
            baslik: "Oturumu Kapat"
            aciklama: "Programlarınızı kapatıp Windows oturumunuzu sonlandırır."
            onTetiklendi: root.oturumuKapat()
        }
        Etiket { visible: root.logoffDialog; x: 53; y: 119; width: 93; ad: "Kullanıcı Değiştir"; harf: 0 }
        Etiket { visible: root.logoffDialog; x: 168; y: 119; width: 93; ad: "Oturumu Kapat"; harf: 0 }

        QQC2.Button {
            x: 242; y: 167
            width: 60; height: 20
            text: "İptal"
            font.family: "Tahoma"
            font.pointSize: 8
            onClicked: root.cancelRequested()
        }
    }

    // klavye kısayolları (Türkçe XP: &Beklemede, &Kapat, Yeniden B&aşlat / &Kullanıcı Değiştir, &Oturumu Kapat)
    Shortcut { enabled: root.durum === "" && !root.logoffDialog && dBekleme.etkin; sequences: ["B", "Alt+B"]; onActivated: root.bekleme() }
    Shortcut { enabled: root.durum === "" && !root.logoffDialog && maysd; sequences: ["K", "Alt+K"]; onActivated: root.kapat() }
    Shortcut { enabled: root.durum === "" && !root.logoffDialog && maysd; sequences: ["A", "Alt+A"]; onActivated: root.yenidenBaslat() }
    Shortcut { enabled: root.durum === "" && root.logoffDialog && oturum.canSwitchUser; sequences: ["K", "Alt+K"]; onActivated: root.kullaniciDegistir() }
    Shortcut { enabled: root.durum === "" && root.logoffDialog; sequences: ["O", "Alt+O"]; onActivated: root.oturumuKapat() }

    // XP balon ipucu: sarı, kuyruklu, kalın başlık
    Timer { id: ipucuZaman; interval: 500; onTriggered: ipucu.goster = true }
    Item {
        id: ipucu
        property Item hedef: null
        property bool goster: false
        onHedefChanged: if (!hedef) goster = false
        visible: hedef !== null && goster && root.durum === ""
        readonly property point konum: hedef ? hedef.mapToItem(root, hedef.width / 2, hedef.height) : Qt.point(0, 0)
        x: Math.min(konum.x - 18, root.width - width - 4)
        y: konum.y + 2
        width: Math.min(260, Math.max(baslikMetni.implicitWidth, aciklamaMetni.implicitWidth) + 18)
        height: kutu.height + 12
        z: 100
        Canvas {
            id: kutuCizim
            anchors.fill: parent
            onPaint: {
                const c = getContext("2d");
                c.reset();
                const w = width - 1, h = height - 1, r = 6, t = 12;
                c.beginPath();
                c.moveTo(r + 0.5, t + 0.5);
                c.lineTo(12.5, t + 0.5);
                c.lineTo(18.5, 0.5);          // kuyruk
                c.lineTo(28.5, t + 0.5);
                c.lineTo(w - r, t + 0.5);
                c.arcTo(w, t + 0.5, w, t + r, r);
                c.lineTo(w, h - r);
                c.arcTo(w, h, w - r, h, r);
                c.lineTo(r + 0.5, h);
                c.arcTo(0.5, h, 0.5, h - r, r);
                c.lineTo(0.5, t + r);
                c.arcTo(0.5, t + 0.5, r + 0.5, t + 0.5, r);
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
        Column {
            id: kutu
            x: 9
            y: 18
            width: ipucu.width - 18
            spacing: 4
            Text {
                id: baslikMetni
                width: parent.width
                text: ipucu.hedef ? ipucu.hedef.baslik : ""
                font.family: "Tahoma"; font.pointSize: 8; font.bold: true
                color: "#000000"
            }
            Text {
                id: aciklamaMetni
                width: parent.width
                wrapMode: Text.WordWrap
                text: ipucu.hedef ? ipucu.hedef.aciklama : ""
                font.family: "Tahoma"; font.pointSize: 8
                color: "#000000"
            }
        }
    }

    // ------------------------------------------------------------ çizim kipi (XP varlığı yoksa)
    component CizimDugme: MouseArea {
        id: bb
        property string label
        property string kind          // standby | off | restart | switch | logoff
        property color base
        hoverEnabled: true
        implicitWidth: 88
        implicitHeight: 70
        Rectangle {
            id: face
            anchors.horizontalCenter: parent.horizontalCenter
            y: 4
            width: 32; height: 32; radius: 5
            border.color: "#FFFFFF"
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
                    c.strokeStyle = "#FFFFFF"; c.fillStyle = "#FFFFFF"; c.lineWidth = 2.6; c.lineCap = "round";
                    const cx = 16, cy = 17;
                    if (bb.kind === "restart") {
                        c.beginPath(); c.arc(cx, cy, 8, -Math.PI * 0.15, Math.PI * 1.45, false); c.stroke();
                        c.beginPath(); c.moveTo(cx + 9.5, cy - 9); c.lineTo(cx + 9.5, cy - 1.5); c.lineTo(cx + 2.5, cy - 3.5);
                        c.closePath(); c.fill();
                    } else if (bb.kind === "switch") {
                        c.lineWidth = 2;
                        c.beginPath(); c.moveTo(cx - 8, cy - 4); c.lineTo(cx + 8, cy - 4);
                        c.moveTo(cx + 4, cy - 8); c.lineTo(cx + 8, cy - 4); c.lineTo(cx + 4, cy);
                        c.moveTo(cx + 8, cy + 5); c.lineTo(cx - 8, cy + 5);
                        c.moveTo(cx - 4, cy + 1); c.lineTo(cx - 8, cy + 5); c.lineTo(cx - 4, cy + 9); c.stroke();
                    } else if (bb.kind === "logoff") {
                        c.lineWidth = 2;
                        c.strokeRect(cx - 9, cy - 9, 10, 18);
                        c.beginPath(); c.moveTo(cx - 2, cy); c.lineTo(cx + 9, cy);
                        c.moveTo(cx + 5, cy - 4); c.lineTo(cx + 9, cy); c.lineTo(cx + 5, cy + 4); c.stroke();
                    } else {
                        c.beginPath(); c.arc(cx, cy + 1, 8, -Math.PI * 0.30, Math.PI * 1.30, false); c.stroke();
                        c.beginPath(); c.moveTo(cx, cy - 10); c.lineTo(cx, cy); c.stroke();
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
            font.pointSize: 8
            font.underline: bb.containsMouse
        }
    }

    Rectangle {
        visible: !root.gercek && root.durum === ""
        anchors.centerIn: parent
        width: 313
        height: 198
        color: "#5A7EDC"
        border.color: "#002D96"
        MouseArea { anchors.fill: parent }
        Rectangle {
            x: 1; y: 1; width: parent.width - 2; height: 42
            color: "#00309C"
            Text {
                x: 10; height: parent.height
                verticalAlignment: Text.AlignVCenter
                text: root.logoffDialog ? "Windows Oturumunu Kapat" : "Bilgisayarı Kapat"
                color: "#FFFFFF"
                font.family: "Franklin Gothic Medium"
                font.pointSize: 14
            }
        }
        Rectangle { x: 1; y: parent.height - 41; width: parent.width - 2; height: 40; color: "#00309C" }
        RowLayout {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -6
            spacing: 6
            CizimDugme { visible: root.logoffDialog; kind: "switch"; base: "#3FA43B"; label: "Kullanıcı Değiştir"; onClicked: root.kullaniciDegistir() }
            CizimDugme { visible: root.logoffDialog; kind: "logoff"; base: "#E3962B"; label: "Oturumu Kapat"; onClicked: root.oturumuKapat() }
            CizimDugme { visible: !root.logoffDialog && spdMethods.SuspendState === true; kind: "standby"; base: "#E8B23A"; label: "Beklemede"; onClicked: root.bekleme() }
            CizimDugme { visible: !root.logoffDialog && maysd; kind: "off"; base: "#D9442B"; label: "Kapat"; onClicked: root.kapat() }
            CizimDugme { visible: !root.logoffDialog && maysd; kind: "restart"; base: "#3FA43B"; label: "Yeniden Başlat"; onClicked: root.yenidenBaslat() }
        }
        QQC2.Button {
            x: 242; y: 167; width: 60; height: 20
            text: "İptal"
            onClicked: root.cancelRequested()
        }
    }

    // ------------------------------------------------------------ logonui durum ekranı
    Rectangle {
        id: durumEkrani
        visible: root.durum !== ""
        anchors.fill: parent
        color: "#5A7EDC"                      // rgb(90,126,220)
        readonly property real s: height / 768   // logonui 1024x768'e göre tasarlandı

        MouseArea { anchors.fill: parent; cursorShape: Qt.BusyCursor }

        Rectangle {
            id: ustBant
            width: parent.width
            height: Math.round(80 * durumEkrani.s)
            color: "#00309C"
            Image {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 2
                source: root.gercek ? root.varlik + "logon-topline.png" : ""
            }
        }
        Rectangle {
            id: altBant
            anchors.bottom: parent.bottom
            width: parent.width
            height: Math.round(88 * durumEkrani.s)
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#3934AD" }
                GradientStop { position: 1.0; color: "#00309C" }
            }
            Image {
                width: parent.width
                height: 3
                source: root.gercek ? root.varlik + "logon-bottomline.png" : ""
            }
        }
        Item {
            id: orta
            anchors { top: ustBant.bottom; bottom: altBant.top; left: parent.left; right: parent.right }
            Image {
                width: Math.round(219 * durumEkrani.s)
                height: Math.round(207 * durumEkrani.s)
                source: root.gercek ? root.varlik + "logon-glow.png" : ""
            }
            Image {
                x: Math.round(parent.width / 2)
                anchors.verticalCenter: parent.verticalCenter
                width: 1
                height: Math.round(parent.height * 0.82)
                source: root.gercek ? root.varlik + "logon-divider.png" : ""
            }
            Image {
                id: logo
                anchors.right: parent.horizontalCenter
                anchors.rightMargin: Math.round(20 * durumEkrani.s)
                y: Math.round(parent.height / 2 - 106 * durumEkrani.s)
                width: Math.round(137 * durumEkrani.s)
                height: Math.round(86 * durumEkrani.s)
                smooth: true
                source: root.gercek ? root.varlik + "logon-logo.png" : ""
            }
            Text {
                anchors.right: parent.horizontalCenter
                anchors.rightMargin: Math.round(40 * durumEkrani.s)
                anchors.top: logo.bottom
                anchors.topMargin: Math.round(20 * durumEkrani.s)
                text: root.durumMetni
                color: "#EFF7FF"                   // rgb(239,247,255)
                font.family: "Arial"
                font.pixelSize: Math.round(18.7 * durumEkrani.s)     // 14 pt
            }
        }
    }

    Component.onCompleted: {
        // Plasma bazı durumlarda zamanlayıcı iptal geri çağrısı bekler
        AutoTriggerTimer.addCancelAutoTriggerCallback(function () {});
        root.forceActiveFocus();
        for (const a of Qt.application.arguments) {
            if (a.startsWith("xp-test-durum=")) {
                root.deneme = true;
                durumBaslat(a.substring(14));
            }
        }
    }
}
