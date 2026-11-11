/*
    XPThemePlasma — oturum kapatma penceresi kökü
    XP "Bilgisayarı Kapat" / "Windows Oturumunu Kapat" arayüzünü (XPLogout.qml) yükler ve sinyallerini
    ksmserver-logout-greeter'a iletir. Plasma'nın sağladığı bağlam (sdtype, ShutdownType, spdMethods)
    beklenen biçimde değilse ya da XP arayüzü yüklenemezse Plasma'nın kendi penceresi açılır.
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick

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

    property real timeout: arayuz.item && arayuz.item.timeout !== undefined ? arayuz.item.timeout : 30
    property real remainingTime: arayuz.item && arayuz.item.remainingTime !== undefined ? arayuz.item.remainingTime : timeout

    readonly property url stok: "file:///usr/share/plasma/shells/org.kde.plasma.desktop/contents/logout/Logout.qml"
    readonly property bool uyumlu: typeof sdtype !== "undefined" && typeof ShutdownType !== "undefined"
                                   && typeof spdMethods !== "undefined" && typeof maysd !== "undefined"
                                   && ShutdownType.ShutdownTypeNone !== undefined

    Loader {
        id: arayuz
        anchors.fill: parent
        focus: true
        source: root.uyumlu ? Qt.resolvedUrl("XPLogout.qml") : root.stok
        onStatusChanged: {
            if (status === Loader.Error && source.toString() !== root.stok.toString()) {
                console.warn("XPThemePlasma: XP kapatma penceresi yüklenemedi, Plasma'nınki kullanılıyor");
                source = root.stok;
            }
        }
        onLoaded: {
            const ilet = ["logoutRequested", "haltRequested", "haltUpdateRequested", "suspendRequested",
                          "rebootRequested", "rebootRequested2", "rebootUpdateRequested", "cancelRequested",
                          "lockScreenRequested", "cancelSoftwareUpdateRequested"];
            for (const ad of ilet) {
                if (item[ad] && typeof item[ad].connect === "function") item[ad].connect(root[ad]);
            }
            item.forceActiveFocus();
        }
    }
}
