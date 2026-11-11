/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.configuration
import org.kde.plasma.plasma5support as P5Support
import QtCore

AppletConfiguration {
    id: root
    isContainment: true
    Layout.minimumWidth: Kirigami.Units.gridUnit * 35
    Layout.minimumHeight: Kirigami.Units.gridUnit * 30
    Layout.preferredWidth: Kirigami.Units.gridUnit * 32
    Layout.preferredHeight: Kirigami.Units.gridUnit * 36

//BEGIN model
    globalConfigModel: globalContainmentConfigModel

    ConfigModel {
        id: globalContainmentConfigModel
        ConfigCategory {
            name: i18ndc("plasma_shell_org.kde.plasma.desktop", "@title:group for configuration dialog page", "Wallpaper")
            icon: "preferences-desktop-wallpaper"
            source: "ConfigurationContainmentAppearance.qml"
        }
        ConfigCategory {
            name: i18ndc("plasma_shell_org.kde.plasma.desktop", "@title:group for configuration dialog page", "Mouse Actions")
            icon: "preferences-desktop-mouse"
            source: "ConfigurationContainmentActions.qml"
        }
    }
//END model

    // XP: masaüstüne sağ tıklama > "Özellikler" Görüntü Özellikleri'ni açar (XPThemePlasma)
    P5Support.DataSource {
        id: xpKomut
        engine: "executable"
        connectedSources: []
        onNewData: source => disconnectSource(source)
    }
    Component.onCompleted: {
        const masaustu = ["org.kde.plasma.folder", "org.kde.desktopcontainment"].indexOf(configDialog.containmentPlugin) !== -1;
        const yerel = StandardPaths.writableLocation(StandardPaths.HomeLocation).toString().replace("file://", "") + "/.local/bin";
        const gorunum = StandardPaths.findExecutable("xp-gorunum", [yerel]);
        if (masaustu && gorunum.toString() !== "") {
            xpKomut.connectSource("setsid -f \"" + gorunum.toString().replace("file://", "") + "\"");
            Qt.callLater(configDialog.close);
        }
    }
}
