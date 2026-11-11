// XPThemePlasma görev çubuğu: başlat | görevler | bildirim alanı + saat
var panel = new Panel("io.github.yyozcan.xppanel");
panel.location = "bottom";
panel.height = 30;
panel.floating = false;
panel.hiding = "none";

var start = panel.addWidget("io.github.yyozcan.xpstart");

var tasks = panel.addWidget("org.kde.plasma.taskmanager");
tasks.currentConfigGroup = ["General"];
tasks.writeConfig("groupingStrategy", "0");      // gruplama yok (XP varsayılanı gibi)
tasks.writeConfig("maxStripes", "1");
tasks.writeConfig("showToolTips", "true");
tasks.writeConfig("highlightWindows", "false");
tasks.writeConfig("indicateAudioStreams", "true");
tasks.writeConfig("middleClickAction", "Close");
tasks.writeConfig("launchers", "");
tasks.writeConfig("iconSpacing", "0");

var tray = panel.addWidget("org.kde.plasma.systemtray");
// Yalnızca stok KDE tepsi öğeleri (ATP'nin Win7 çatalları ikilenmesin)
var trayItems = ["io.github.yyozcan.xpses", "io.github.yyozcan.xpag",
                 "org.kde.plasma.bluetooth", "org.kde.plasma.battery",
                 "io.github.yyozcan.xpbalon", "org.kde.plasma.devicenotifier",
                 "org.kde.plasma.keyboardlayout", "org.kde.plasma.mediacontroller",
                 "org.kde.plasma.clipboard", "org.kde.plasma.printmanager",
                 "org.kde.kdeconnect", "org.kde.plasma.cameraindicator",
                 "org.kde.plasma.brightness", "org.kde.plasma.manage-inputmethod",
                 "org.kde.plasma.keyboardindicator", "org.kde.kscreen"];
// Plasma 6'da tepsi uygulamacığı kendi içinde bir kapsayıcıdır: ayarlar doğrudan onun [General] grubunda
tray.currentConfigGroup = ["General"];
tray.writeConfig("extraItems", trayItems);
// tepsi, bilmediği her eklentiyi kendiliğinden ekler; istemediklerimizi "bilinen" say
tray.writeConfig("knownItems", trayItems.concat([
    "io.gitgud.wackyideas.battery", "io.gitgud.wackyideas.networkmanagement",
    "io.gitgud.wackyideas.volume", "io.gitgud.wackyideas.notifications",
    "io.gitgud.wackyideas.keyboardlayout", "org.kde.plasma.weather",
    "org.kde.plasma.dwellclicker", "org.kde.plasma.vault",
    // XP sürümleriyle değiştirilen stok öğeler
    "org.kde.plasma.volume", "org.kde.plasma.notifications", "org.kde.plasma.networkmanagement"]));
tray.writeConfig("hiddenItems", ["org.kde.plasma.clipboard", "org.kde.kdeconnect",
    "org.kde.kscreen", "org.kde.plasma.manage-inputmethod", "org.kde.plasma.keyboardindicator",
    "org.kde.plasma.printmanager", "org.kde.plasma.brightness"]);
tray.writeConfig("iconSpacing", "1");

// XP saati: üzerine gelince tarih, çift tıklayınca Tarih ve Saat Özellikleri
panel.addWidget("io.github.yyozcan.xpsaat");
