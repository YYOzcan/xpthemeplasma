/*
    XPThemePlasma — XP Başlat
    SPDX-FileCopyrightText: 2013-2015 Eike Hein <hein@kde.org> (Kicker modelleri)
    SPDX-FileCopyrightText: wackyideas (SevenStart)
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.private.kicker as Kicker
import org.kde.plasma.plasma5support as Plasma5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: kicker

    // Kicker modelleri "kicker" kimlikli bir appletInterface bekler
    property QtObject globalFavorites: rootModel.favoritesModel
    property QtObject systemFavorites: rootModel.systemFavoritesModel
    property Item dragSource: null

    signal reset

    // Açılır pencere yok: düğme doğrudan uygulamacığın içinde, menü kendi Dialog'u
    activationTogglesExpanded: false
    toolTipMainText: ""

    Plasmoid.constraintHints: Plasmoid.CanFillArea
    Layout.fillHeight: true
    Layout.minimumWidth: startButton.implicitWidth
    Layout.preferredWidth: startButton.implicitWidth
    Layout.maximumWidth: startButton.implicitWidth

    StartButton {
        id: startButton
        anchors.fill: parent
    }
    Plasmoid.icon: Plasmoid.configuration.icon

    function toggleMenu() {
        startMenu.visible = !startMenu.visible;
    }

    Plasmoid.onActivated: toggleMenu()

    function exec(cmd) {
        executable.exec(cmd);
    }

    Plasma5Support.DataSource {
        id: executable
        engine: "executable"
        connectedSources: []
        onNewData: (sourceName, data) => disconnectSource(sourceName)
        function exec(cmd) {
            if (cmd) {
                connectSource(cmd);
            }
        }
    }

    Kicker.RootModel {
        id: rootModel

        autoPopulate: true
        appletInterface: kicker

        flat: false
        sorted: true
        showSeparators: false
        showTopLevelItems: true
        showAllApps: false
        showAllAppsCategorized: false
        showRecentApps: false
        showRecentDocs: false
        showPowerSession: false
        highlightNewlyInstalledApps: false

        property bool favoritesReady: false
        function initFavorites() {
            if (favoritesReady || !favoritesModel) {
                return;
            }
            favoritesReady = true;
            if ("initForClient" in favoritesModel) {
                favoritesModel.initForClient("io.github.yyozcan.xpstart.pinned-" + Plasmoid.id);
                // Sık kullanılanlar KActivities'te tüm başlatıcılar arasında ortaktır (ATP SevenStart dahil):
                // ortak listeye hiçbir şey yazmıyoruz, kullanıcının mevcut sabitlenenlerini gösteriyoruz.
                Plasmoid.configuration.favoritesPortedToKAstats = true;
            } else {
                favoritesModel.favorites = Plasmoid.configuration.favoriteApps;
            }
            favoritesModel.maxFavorites = -1;
        }

        onFavoritesModelChanged: initFavorites()
        Component.onCompleted: initFavorites()
    }

    Connections {
        target: globalFavorites
        function onFavoritesChanged() {
            Plasmoid.configuration.favoriteApps = target.favorites;
        }
    }

    Kicker.RecentUsageModel {
        id: recentUsageModel
        favoritesModel: globalFavorites
        ordering: 1 // en sık kullanılan önce (XP davranışı)
        shownItems: Kicker.RecentUsageModel.OnlyApps
    }

    Kicker.RecentUsageModel {
        id: recentDocsModel
        favoritesModel: globalFavorites
        ordering: 0
        shownItems: Kicker.RecentUsageModel.OnlyDocs
    }

    Kicker.DragHelper {
        id: dragHelper
    }

    Kicker.ProcessRunner {
        id: processRunner
    }

    Kicker.WindowSystem {
        id: windowSystem
    }

    StartMenu {
        id: startMenu
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: i18n("Edit Applications…")
            icon.name: "kmenuedit"
            onTriggered: kicker.exec("kmenuedit")
        }
    ]
}
