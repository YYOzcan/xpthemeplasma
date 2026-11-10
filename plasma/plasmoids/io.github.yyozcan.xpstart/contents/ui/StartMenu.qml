/*
    XPThemePlasma — Windows XP biçemli iki sütunlu Başlat menüsü
    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtCore

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami
import org.kde.coreaddons as KCoreAddons
import org.kde.plasma.private.sessions as Sessions

import "code/tools.js" as Tools

PlasmaCore.Dialog {
    id: menu

    visualParent: startButton
    location: Plasmoid.location
    backgroundHints: PlasmaCore.Dialog.NoBackground
    hideOnWindowDeactivate: true
    flags: Qt.WindowStaysOnTopHint
    appletInterface: kicker
    title: "xp-start-menu"

    // ---- ölçüler ve renkler (Luna) ----
    // Gerçek XP varlıkları varsa (xp_iceaktar) Luna'nın StartPanel bitmap'leri kullanılır
    readonly property string varlik: StandardPaths.writableLocation(StandardPaths.GenericDataLocation) + "/xpthemeplasma/varlik/"
    readonly property bool gercek: probe.status === Image.Ready
    readonly property int menuWidth: gercek ? 380 : 410            // XP: DefaultPaneSize 380
    readonly property int shadowSize: 4
    readonly property color hoverColor: "#316AC5"
    readonly property color rightText: gercek ? "#0A246A" : "#00136B"  // StartPanel.PlacesList TextColor
    readonly property color leftText: gercek ? "#000000" : "#000000"
    readonly property string uiFont: "Tahoma"

    property bool allProgramsOpen: allProgramsMenu.opened

    // "Sık kullanılanlar": sabitlenenleri atla, en fazla recentCount öğe göster
    property var pinnedNames: []
    function refreshPinned() {
        const names = [];
        if (globalFavorites) {
            for (let i = 0; i < globalFavorites.rowCount(); i++) {
                names.push(globalFavorites.data(globalFavorites.index(i, 0), Qt.DisplayRole));
            }
        }
        pinnedNames = names;
    }
    // i. satırdan önce kaç tane "sabitlenmemiş" uygulama var
    function recentRank(i) {
        let r = 0;
        for (let k = 0; k < i; k++) {
            if (pinnedNames.indexOf(recentUsageModel.data(recentUsageModel.index(k, 0), Qt.DisplayRole)) === -1) {
                r++;
            }
        }
        return r;
    }

    // PlasmaCore.Dialog görünürlük sinyalini visibleChangedProxy adıyla yayınlar
    onVisibleChangedProxy: shownChanged()
    onVisibleChanged: shownChanged()
    function shownChanged() {
        if (visible) {
            refreshPinned();
            requestActivate();
            content.forceActiveFocus();
        } else {
            hoverTimer.stop();
        }
    }



    // ---------------------------------------------------------------- eylemler
    function run(entry) {
        if (entry.cmd) {
            kicker.exec(entry.cmd);
        } else if (entry.url) {
            Qt.openUrlExternally(entry.url);
        }
        menu.visible = false;
    }

    function folder(loc) {
        return StandardPaths.writableLocation(loc);
    }

    // "Tüm Programlar" ağacını Kicker kök modelinden kur
    function buildApps(model, depth) {
        const list = [];
        if (!model) {
            return list;
        }
        for (let i = 0; i < model.count; i++) {
            const label = model.data(model.index(i, 0), Qt.DisplayRole);
            if (!label) {
                continue;
            }
            const icon = model.data(model.index(i, 0), Qt.DecorationRole);
            const child = depth < 4 ? model.modelForRow(i) : null;
            if (child && child.count > 0) {
                list.push({ "text": label, "icon": icon, "subActions": buildApps(child, depth + 1) });
            } else {
                const m = model;
                const row = i;
                list.push({ "text": label, "icon": icon, "action": function () {
                    m.trigger(row, "", null);
                    menu.visible = false;
                } });
            }
        }
        return list;
    }

    function openAllPrograms() {
        if (allProgramsMenu.opened) {
            return;
        }
        allProgramsMenu.actionList = buildApps(rootModel, 0);
        allProgramsMenu.menu.placement = PlasmaExtras.Menu.RightPosedBottomAlignedPopup;
        allProgramsMenu.menu.openRelative();
    }

    function openRecentDocs(item) {
        const list = [];
        for (let i = 0; i < recentDocsModel.count && i < 15; i++) {
            const m = recentDocsModel;
            const row = i;
            list.push({ "text": m.data(m.index(i, 0), Qt.DisplayRole), "icon": m.data(m.index(i, 0), Qt.DecorationRole),
                        "action": function () { m.trigger(row, "", null); menu.visible = false; } });
        }
        if (list.length === 0) {
            list.push({ "text": i18n("(Empty)"), "enabled": false });
        }
        recentDocsMenu.visualParent = item;
        recentDocsMenu.actionList = list;
        recentDocsMenu.menu.placement = PlasmaExtras.Menu.RightPosedTopAlignedPopup;
        recentDocsMenu.menu.openRelative();
    }

    function openContextMenu(item, model, index, actionList, favoriteId, x, y) {
        contextMenu.visualParent = item;
        contextMenu.srcModel = model;
        contextMenu.srcIndex = index;
        Tools.fillActionMenu(i18n, contextMenu, actionList, globalFavorites, favoriteId);
        contextMenu.open(x, y);
    }

    // ---------------------------------------------------------------- bileşenler
    component Separator: Item {
        id: sep
        property color tint: "#D5D2CA"
        property bool places: false
        Layout.fillWidth: true
        Layout.preferredHeight: 9
        Image {
            anchors.centerIn: parent
            visible: menu.gercek
            source: menu.varlik + (sep.places ? "panel-placessep.png" : "panel-progsep.png")
            smooth: false
        }
        Rectangle {
            visible: !menu.gercek
            anchors.centerIn: parent
            width: parent.width - 16
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: sep.tint }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }

    // Sol sütun öğesi: 32 piksel simge, kalın başlık + gri alt satır
    component LeftItem: MouseArea {
        id: leftItem
        required property var model
        required property int index
        property QtObject sourceModel
        property bool pinned: false
        readonly property string titleText: model.display
        readonly property string subtitleText: pinned && model.description ? model.description : ""

        Layout.fillWidth: true
        Layout.preferredHeight: 38
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton) {
                menu.openContextMenu(leftItem, sourceModel, index, model.actionList, model.favoriteId, mouse.x, mouse.y);
            } else {
                sourceModel.trigger(index, "", null);
                menu.visible = false;
            }
        }
        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            color: menu.hoverColor
            visible: leftItem.containsMouse
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 7
            anchors.rightMargin: 6
            spacing: 6
            Kirigami.Icon {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                source: leftItem.model.decoration
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    text: leftItem.titleText
                    elide: Text.ElideRight
                    font.family: menu.uiFont
                    font.pixelSize: 11
                    font.bold: leftItem.pinned
                    color: leftItem.containsMouse ? "#FFFFFF" : menu.leftText
                }
                Text {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: leftItem.subtitleText
                    elide: Text.ElideRight
                    font.family: menu.uiFont
                    font.pixelSize: 11
                    color: leftItem.containsMouse ? "#D8E4F8" : "#808080"
                }
            }
        }
    }

    // Sağ sütun öğesi: 24 piksel simge, lacivert yazı
    component RightItem: MouseArea {
        id: rightItem
        property string text
        property string iconName
        property bool bold: false
        property bool hasSubmenu: false
        signal activated

        Layout.fillWidth: true
        Layout.preferredHeight: menu.gercek ? 26 : 28
        hoverEnabled: true
        onClicked: activated()
        onContainsMouseChanged: {
            if (containsMouse && hasSubmenu) {
                subTimer.restart();
            } else {
                subTimer.stop();
            }
        }
        Timer {
            id: subTimer
            interval: 400
            onTriggered: rightItem.activated()
        }
        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            color: menu.hoverColor
            visible: rightItem.containsMouse
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 7
            anchors.rightMargin: 6
            spacing: 6
            Kirigami.Icon {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                source: rightItem.iconName
            }
            Text {
                Layout.fillWidth: true
                text: rightItem.text
                elide: Text.ElideRight
                font.family: menu.uiFont
                font.pixelSize: 11
                font.bold: rightItem.bold
                color: rightItem.containsMouse ? "#FFFFFF" : menu.rightText
            }
            Text {
                visible: rightItem.hasSubmenu
                text: "▶"
                font.pixelSize: 7
                color: rightItem.containsMouse ? "#FFFFFF" : menu.rightText
            }
        }
    }

    // Alt şerit düğmesi (Oturumu Kapat / Bilgisayarı Kapat)
    component FooterButton: MouseArea {
        id: fb
        property string text
        property string iconName
        property color iconTint: "#E8A33D"
        Layout.preferredHeight: 30
        Layout.preferredWidth: fbRow.implicitWidth + 12
        hoverEnabled: true
        Rectangle {
            visible: !menu.gercek
            anchors.fill: parent
            radius: 3
            color: "#FFFFFF"
            opacity: fb.pressed ? 0.25 : (fb.containsMouse ? 0.15 : 0)
        }
        RowLayout {
            id: fbRow
            anchors.centerIn: parent
            spacing: 4
            Item {
                // logoff-icons.png: [Çıkar, Oturumu Kapat (anahtar), Kapat (güç)] 24x24
                visible: menu.gercek
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                clip: true
                Image {
                    x: fb.iconName === "system-shutdown" ? -48 : -24
                    source: menu.varlik + (fb.containsMouse ? "panel-logoffbuttons-hot.png" : "panel-logoffbuttons.png")
                    smooth: false
                }
            }
            Rectangle {
                visible: !menu.gercek
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                radius: 4
                color: fb.iconTint
                border.color: "#FFFFFF"
                border.width: 1
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.lighter(fb.iconTint, 1.35) }
                    GradientStop { position: 1.0; color: Qt.darker(fb.iconTint, 1.25) }
                }
                Canvas {
                    id: glyph
                    anchors.fill: parent
                    onPaint: {
                        const c = getContext("2d");
                        c.reset();
                        c.strokeStyle = "#FFFFFF";
                        c.fillStyle = "#FFFFFF";
                        c.lineWidth = 2;
                        c.lineCap = "round";
                        if (fb.iconName === "system-shutdown") {
                            // güç simgesi: açık çember + dikey çizgi
                            c.beginPath();
                            c.arc(12, 13, 6, -Math.PI * 0.32, Math.PI * 1.32, false);
                            c.stroke();
                            c.beginPath();
                            c.moveTo(12, 5);
                            c.lineTo(12, 12);
                            c.stroke();
                        } else {
                            // oturumu kapat: kapı + dışarı ok
                            c.lineWidth = 1.6;
                            c.strokeRect(6, 5, 8, 14);
                            c.beginPath();
                            c.moveTo(11, 12);
                            c.lineTo(19, 12);
                            c.stroke();
                            c.beginPath();
                            c.moveTo(16, 9);
                            c.lineTo(19, 12);
                            c.lineTo(16, 15);
                            c.stroke();
                        }
                    }
                }
            }
            Text {
                text: fb.text
                font.family: menu.uiFont
                font.pixelSize: 11
                color: "#FFFFFF"
                font.underline: menu.gercek && fb.containsMouse
                style: menu.gercek ? Text.Normal : Text.Raised
                styleColor: "#0A2D7A"
            }
        }
    }

    // ---------------------------------------------------------------- görünüm
    mainItem: FocusScope {
        id: content
        width: menu.menuWidth + menu.shadowSize
        height: frame.height + menu.shadowSize
        focus: true
        Keys.onEscapePressed: menu.visible = false

        // yardımcı nesneler (Dialog'un mainItem'ı tek öğe kabul eder)
        KCoreAddons.KUser {
            id: kuser
        }
        Sessions.SessionManagement {
            id: session
        }
        ActionMenu {
            id: allProgramsMenu
            visualParent: allProgramsButton
        }
        ActionMenu {
            id: recentDocsMenu
        }
        ActionMenu {
            id: contextMenu
            property QtObject srcModel
            property int srcIndex: -1
            onActionClicked: (actionId, actionArgument) => {
                if (Tools.triggerAction(srcModel, srcIndex, actionId, actionArgument)) {
                    menu.visible = false;
                }
            }
        }

        Connections {
            target: globalFavorites
            function onCountChanged() { menu.refreshPinned(); }
            function onModelReset() { menu.refreshPinned(); }
            function onRowsInserted() { menu.refreshPinned(); }
            function onRowsRemoved() { menu.refreshPinned(); }
        }

        Image {
            id: probe
            visible: false
            source: menu.varlik + "panel-user.png"
        }

        Timer {
            id: hoverTimer
            interval: 450
            onTriggered: menu.openAllPrograms()
        }


        // yumuşak sağ-alt gölge
        Repeater {
            model: menu.shadowSize
            Rectangle {
                required property int index
                x: index + 1
                y: index + 1
                width: frame.width
                height: frame.height
                topLeftRadius: menu.gercek ? 6 : 7
                topRightRadius: menu.gercek ? 6 : 7
                color: "#000000"
                opacity: 0.09
            }
        }

        Rectangle {
            id: frame
            width: menu.menuWidth
            height: layout.implicitHeight + (menu.gercek ? 0 : 2)
            topLeftRadius: 7
            topRightRadius: 7
            color: menu.gercek ? "transparent" : "#245EDB"
            border.color: "#1C4FC2"
            border.width: menu.gercek ? 0 : 1

            ColumnLayout {
                id: layout
                x: menu.gercek ? 0 : 1
                y: menu.gercek ? 0 : 1
                width: parent.width - (menu.gercek ? 0 : 2)
                spacing: 0

                // ---- başlık: kullanıcı resmi + ad ----
                Rectangle {
                    id: header
                    Layout.fillWidth: true
                    Layout.preferredHeight: menu.gercek ? 64 : 58
                    topLeftRadius: 6
                    topRightRadius: 6
                    color: "transparent"
                    BorderImage {
                        anchors.fill: parent
                        visible: menu.gercek
                        source: menu.varlik + "panel-user.png"
                        border { left: 59; right: 60; top: 62; bottom: 0 }
                        smooth: false
                    }
                    gradient: menu.gercek ? null : headerGradient
                    Gradient {
                        id: headerGradient
                        GradientStop { position: 0.00; color: "#1868CE" }
                        GradientStop { position: 0.12; color: "#0E60CB" }
                        GradientStop { position: 0.20; color: "#0E60CB" }
                        GradientStop { position: 0.32; color: "#1164CF" }
                        GradientStop { position: 0.47; color: "#1B6CD3" }
                        GradientStop { position: 0.60; color: "#2476DC" }
                        GradientStop { position: 0.77; color: "#3482E3" }
                        GradientStop { position: 0.90; color: "#428EE9" }
                        GradientStop { position: 1.00; color: "#4791EB" }
                    }
                    // üst parıltı
                    Rectangle {
                        visible: !menu.gercek
                        x: 6; y: 1
                        width: parent.width - 12
                        height: 1
                        color: "#5B9BEB"
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: menu.gercek ? 6 : 7
                        spacing: menu.gercek ? 6 : 8
                        Rectangle {
                            Layout.preferredWidth: menu.gercek ? 54 : 48
                            Layout.preferredHeight: menu.gercek ? 54 : 48
                            radius: 4
                            color: menu.gercek ? "transparent" : "#FFFFFF"
                            border.color: "#D8E4F8"
                            border.width: menu.gercek ? 0 : 1
                            BorderImage {
                                // XP: UserTileBackground, kenar payları 6,10,6,10
                                visible: menu.gercek
                                anchors.fill: parent
                                anchors.rightMargin: -4
                                anchors.bottomMargin: -4
                                source: menu.varlik + "panel-usertile.png"
                                border { left: 6; right: 10; top: 6; bottom: 10 }
                                smooth: false
                            }
                            visible: Plasmoid.configuration.showUserPicture
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: menu.gercek ? 3 : 2
                                radius: 3
                                clip: true
                                color: "#6DA7E7"
                                Image {
                                    id: face
                                    anchors.fill: parent
                                    source: kuser.faceIconUrl
                                    fillMode: Image.PreserveAspectCrop
                                    visible: status === Image.Ready
                                    smooth: true
                                }
                                Kirigami.Icon {
                                    anchors.fill: parent
                                    anchors.margins: 3
                                    visible: !face.visible
                                    source: "user-identity"
                                }
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: kuser.fullName || kuser.loginName
                            elide: Text.ElideRight
                            // XP: Franklin Gothic Medium 14, gölge 9,66,139 (2,2)
                            font.family: menu.gercek ? "Franklin Gothic Medium" : "Trebuchet MS"
                            font.pixelSize: menu.gercek ? 19 : 15
                            font.bold: !menu.gercek
                            color: "#FFFFFF"
                            style: Text.Raised
                            styleColor: menu.gercek ? "#09428B" : "#0F2B6B"
                        }
                    }
                }

                // ---- turuncu ayraç ----
                Rectangle {
                    visible: !menu.gercek
                    Layout.fillWidth: true
                    Layout.preferredHeight: 2
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#3C7CDE" }
                        GradientStop { position: 0.5; color: "#F0943C" }
                        GradientStop { position: 1.0; color: "#3C7CDE" }
                    }
                }

                // ---- iki sütun ----
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: menu.gercek ? 0 : 1
                    Layout.rightMargin: menu.gercek ? 0 : 1
                    spacing: 0

                    // sol: beyaz, sabitlenmiş + sık kullanılan
                    Rectangle {
                        Layout.preferredWidth: menu.gercek ? 190 : 200
                        Layout.fillHeight: true
                        Layout.minimumHeight: leftCol.implicitHeight + (menu.gercek ? 14 : 0)
                        color: "#FFFFFF"
                        BorderImage {
                            visible: menu.gercek
                            anchors.fill: parent
                            source: menu.varlik + "panel-proglist.png"
                            border { left: 2; right: 153; top: 3; bottom: 0 }
                            smooth: false
                        }

                        ColumnLayout {
                            id: leftCol
                            anchors.fill: parent
                            anchors.topMargin: menu.gercek ? 9 : 5      // XP ContentMargins 6 4 9 5
                            anchors.bottomMargin: menu.gercek ? 0 : 3
                            anchors.leftMargin: menu.gercek ? 2 : 0
                            spacing: 0

                            Repeater {
                                model: globalFavorites
                                delegate: LeftItem {
                                    sourceModel: globalFavorites
                                    pinned: true
                                    // aynı uygulama iki kimlikle sabitlenmişse (ör. preferred://browser) bir kez göster
                                    visible: menu.pinnedNames.indexOf(model.display) === index
                                             || menu.pinnedNames.indexOf(model.display) === -1
                                }
                            }

                            Separator {
                                visible: globalFavorites && globalFavorites.count > 0
                            }

                            Repeater {
                                model: recentUsageModel
                                delegate: LeftItem {
                                    id: recentItem
                                    sourceModel: recentUsageModel
                                    readonly property bool isFav: menu.pinnedNames.indexOf(model.display) !== -1
                                    visible: !isFav && menu.recentRank(index) < Plasmoid.configuration.recentCount
                                }
                            }

                            Item {
                                Layout.fillHeight: true
                                Layout.minimumHeight: 6
                            }

                            Separator {}

                            // "Tüm Programlar"
                            MouseArea {
                                id: allProgramsButton
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                hoverEnabled: true
                                onClicked: menu.openAllPrograms()
                                onContainsMouseChanged: containsMouse ? hoverTimer.restart() : hoverTimer.stop()
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.leftMargin: 2
                                    anchors.rightMargin: 2
                                    color: menu.hoverColor
                                    visible: allProgramsButton.containsMouse || menu.allProgramsOpen
                                }
                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 8
                                    Text {
                                        text: i18n("All Programs")
                                        font.family: menu.uiFont
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: (allProgramsButton.containsMouse || menu.allProgramsOpen) ? "#FFFFFF" : "#000000"
                                    }
                                    Image {
                                        visible: menu.gercek
                                        source: menu.varlik + ((allProgramsButton.containsMouse || menu.allProgramsOpen)
                                                               ? "panel-more-arrow-hot.png" : "panel-more-arrow.png")
                                        smooth: false
                                    }
                                    Rectangle {
                                        visible: !menu.gercek
                                        Layout.preferredWidth: 18
                                        Layout.preferredHeight: 18
                                        radius: 4
                                        border.color: "#FFFFFF"
                                        gradient: Gradient {
                                            GradientStop { position: 0.0; color: "#73D46F" }
                                            GradientStop { position: 0.5; color: "#36A933" }
                                            GradientStop { position: 1.0; color: "#1E8A1D" }
                                        }
                                        Canvas {
                                            anchors.fill: parent
                                            onPaint: {
                                                const c = getContext("2d");
                                                c.reset();
                                                c.fillStyle = "#FFFFFF";
                                                c.beginPath();
                                                c.moveTo(6, 4.5);
                                                c.lineTo(13, 9);
                                                c.lineTo(6, 13.5);
                                                c.closePath();
                                                c.fill();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // sağ: açık mavi, yerler ve sistem
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: rightCol.implicitHeight + 10
                        color: "#D3E5FA"
                        BorderImage {
                            visible: menu.gercek
                            anchors.fill: parent
                            source: menu.varlik + "panel-places.png"
                            border { left: 172; right: 7; top: 3; bottom: 1 }
                            smooth: false
                        }
                        Rectangle {
                            visible: !menu.gercek
                            width: 1
                            height: parent.height
                            color: "#95BDEE"
                        }

                        ColumnLayout {
                            id: rightCol
                            anchors.fill: parent
                            anchors.leftMargin: menu.gercek ? 2 : 1       // XP ContentMargins 4 6 9 5
                            anchors.rightMargin: menu.gercek ? 6 : 0
                            anchors.topMargin: menu.gercek ? 9 : 5
                            anchors.bottomMargin: 5
                            spacing: 0

                            RightItem {
                                text: i18n("My Documents"); iconName: "folder-documents"; bold: true
                                onActivated: menu.run({ "url": menu.folder(StandardPaths.DocumentsLocation) })
                            }
                            RightItem {
                                id: recentDocsItem
                                text: i18n("My Recent Documents"); iconName: "document-open-recent"; bold: true; hasSubmenu: true
                                onActivated: menu.openRecentDocs(recentDocsItem)
                            }
                            RightItem {
                                text: i18n("My Pictures"); iconName: "folder-pictures"; bold: true
                                onActivated: menu.run({ "url": menu.folder(StandardPaths.PicturesLocation) })
                            }
                            RightItem {
                                text: i18n("My Music"); iconName: "folder-music"; bold: true
                                onActivated: menu.run({ "url": menu.folder(StandardPaths.MusicLocation) })
                            }
                            RightItem {
                                text: i18n("My Computer"); iconName: "computer"; bold: true
                                onActivated: menu.run({ "cmd": "dolphin --new-window /" })
                            }
                            RightItem {
                                text: i18n("My Network Places"); iconName: "network-workgroup"
                                onActivated: menu.run({ "cmd": "dolphin --new-window remote:/" })
                            }
                            Separator { tint: "#A9C5EB"; places: true }
                            RightItem {
                                text: i18n("Control Panel"); iconName: "preferences-system"
                                onActivated: menu.run({ "cmd": "systemsettings" })
                            }
                            RightItem {
                                text: i18n("Printers and Faxes"); iconName: "printer"
                                onActivated: menu.run({ "cmd": "kcmshell6 kcm_printer_manager" })
                            }
                            Separator { tint: "#A9C5EB"; places: true }
                            RightItem {
                                text: i18n("Help and Support"); iconName: "help-browser"
                                onActivated: menu.run({ "cmd": "khelpcenter" })
                            }
                            RightItem {
                                text: i18n("Search"); iconName: "system-search"
                                onActivated: menu.run({ "cmd": "krunner" })
                            }
                            RightItem {
                                text: i18n("Run…"); iconName: "system-run"
                                onActivated: menu.run({ "cmd": "krunner" })
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }

                // ---- alt şerit ----
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "transparent"
                    BorderImage {
                        visible: menu.gercek
                        anchors.fill: parent
                        source: menu.varlik + "panel-logoff.png"
                        border { left: 49; right: 47; top: 0; bottom: 38 }
                        smooth: false
                    }
                    gradient: menu.gercek ? null : footerGradient
                    Gradient {
                        id: footerGradient
                        GradientStop { position: 0.00; color: "#4282D6" }
                        GradientStop { position: 0.05; color: "#418AE3" }
                        GradientStop { position: 0.17; color: "#418AE3" }
                        GradientStop { position: 0.29; color: "#3482E3" }
                        GradientStop { position: 0.49; color: "#2374DF" }
                        GradientStop { position: 0.62; color: "#196EDB" }
                        GradientStop { position: 0.75; color: "#1468D5" }
                        GradientStop { position: 0.88; color: "#0F61CB" }
                        GradientStop { position: 1.00; color: "#0D5AC1" }
                    }
                    RowLayout {
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4
                        FooterButton {
                            text: i18n("Log Off")
                            iconName: "system-log-out"
                            iconTint: "#E8A33D"
                            onClicked: {
                                menu.visible = false;
                                session.requestLogout();
                            }
                        }
                        FooterButton {
                            text: i18n("Turn Off Computer")
                            iconName: "system-shutdown"
                            iconTint: "#D9442B"
                            onClicked: {
                                menu.visible = false;
                                session.requestShutdown();
                            }
                        }
                    }
                }
            }
        }
    }
}
