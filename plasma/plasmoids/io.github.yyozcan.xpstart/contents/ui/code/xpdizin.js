/*
    XPThemePlasma — "Tüm Programlar"ı Windows XP düzenine göre kurar.
    SPDX-License-Identifier: GPL-2.0-or-later
*/
.pragma library

var FAV_ROLE = 259;     // Kicker: applications:xxx.desktop

// desktop kimliği -> [XP adı, yer]   (yer: "ust" | "don" | "don/Sistem Araçları" | ...)
// Aynı XP adı için listede ilk bulunan uygulama kullanılır (ör. tek bir "Internet Explorer").
var ESLEME = [
    ["firefox.desktop", "Internet Explorer", "ust"],
    ["org.mozilla.firefox.desktop", "Internet Explorer", "ust"],
    ["google-chrome.desktop", "Internet Explorer", "ust"],
    ["chromium.desktop", "Internet Explorer", "ust"],
    ["brave-browser.desktop", "Internet Explorer", "ust"],
    ["org.kde.falkon.desktop", "Internet Explorer", "ust"],
    ["org.mozilla.Thunderbird.desktop", "Outlook Express", "ust"],
    ["thunderbird.desktop", "Outlook Express", "ust"],
    ["org.kde.kmail2.desktop", "Outlook Express", "ust"],
    ["org.kde.elisa.desktop", "Windows Media Player", "ust"],
    ["vlc.desktop", "Windows Media Player", "ust"],
    ["org.kde.haruna.desktop", "Windows Media Player", "ust"],
    ["mpv.desktop", "Windows Media Player", "ust"],
    ["xp-notdefteri.desktop", "Not Defteri", "don"],
    ["org.kde.kwrite.desktop", "Not Defteri", "don"],
    ["org.kde.kate.desktop", "Not Defteri", "don"],
    ["org.kde.kcalc.desktop", "Hesap Makinesi", "don"],
    ["org.kde.kolourpaint.desktop", "Paint", "don"],
    ["org.kde.konsole.desktop", "Komut İstemi", "don"],
    ["org.kde.dolphin.desktop", "Windows Gezgini", "don"],
    ["org.kde.kdenlive.desktop", "Windows Movie Maker", "don"],
    ["libreoffice-writer.desktop", "WordPad", "don"],
    ["org.kde.kcharselect.desktop", "Karakter Eşlem", "don/Sistem Araçları"],
    ["org.kde.kinfocenter.desktop", "Sistem Bilgisi", "don/Sistem Araçları"],
    ["org.kde.filelight.desktop", "Disk Temizleme", "don/Sistem Araçları"],
    ["timeshift-gtk.desktop", "Sistem Geri Yükleme", "don/Sistem Araçları"],
    ["org.kde.partitionmanager.desktop", "Disk Yönetimi", "don/Sistem Araçları"],
    ["org.kde.plasma-systemmonitor.desktop", "Görev Yöneticisi", "don/Sistem Araçları"],
    ["org.kde.kmag.desktop", "Büyüteç", "don/Erişilebilirlik"],
    ["onboard.desktop", "Ekran Klavyesi", "don/Erişilebilirlik"],
    ["orca.desktop", "Ekran Okuyucusu", "don/Erişilebilirlik"],
    ["org.kde.kwave.desktop", "Ses Kaydedicisi", "don/Eğlence"],
    ["audacity.desktop", "Ses Kaydedicisi", "don/Eğlence"],
    ["org.pulseaudio.pavucontrol.desktop", "Ses Denetimi", "don/Eğlence"],
    ["org.kde.krdc.desktop", "Uzak Masaüstü Bağlantısı", "don/İletişim"],
    ["org.remmina.Remmina.desktop", "Uzak Masaüstü Bağlantısı", "don/İletişim"],
];

// KDE kategori adı (Türkçe/İngilizce) -> XP yeri
var KATEGORI = {
    "İzlenceler": "don", "Utilities": "don", "Donatılar": "don", "Accessories": "don",
    "Sistem": "don/Sistem Araçları", "System": "don/Sistem Araçları",
    "Çoklu Ortam": "don/Eğlence", "Multimedia": "don/Eğlence",
    "Erişilebilirlik": "don/Erişilebilirlik", "Accessibility": "don/Erişilebilirlik",
    "Oyunlar": "oyun", "Games": "oyun",
};

function desktopId(fav) {
    if (!fav) {
        return "";
    }
    var s = String(fav);
    return s.indexOf("applications:") === 0 ? s.substring(13) : s.split("/").pop();
}

// Sol sütun ve menüler için XP adı (eşleşme yoksa olduğu gibi)
function xpAd(fav, label) {
    var id = desktopId(fav);
    for (var i = 0; i < ESLEME.length; i++) {
        if (ESLEME[i][0] === id) {
            return ESLEME[i][1];
        }
    }
    return label;
}

function _esleme(id) {
    for (var i = 0; i < ESLEME.length; i++) {
        if (ESLEME[i][0] === id) {
            return ESLEME[i];
        }
    }
    return null;
}

// Kök modeli gez, yaprak uygulamaları [kategori yolu] ile topla
function _topla(model, yol, out, derinlik) {
    for (var i = 0; i < model.count; i++) {
        var idx = model.index(i, 0);
        var label = model.data(idx, 0);
        if (!label) {
            continue;
        }
        var child = derinlik < 4 ? model.modelForRow(i) : null;
        if (child && child.count > 0) {
            _topla(child, yol.concat([label]), out, derinlik + 1);
        } else {
            out.push({ "model": model, "row": i, "label": label, "icon": model.data(idx, 1),
                       "id": desktopId(model.data(idx, FAV_ROLE)), "yol": yol });
        }
    }
}

function _sirala(a, b) {
    // XP: önce klasörler, sonra kısayollar; her biri alfabetik
    var ka = a.subActions ? 0 : 1, kb = b.subActions ? 0 : 1;
    if (ka !== kb) {
        return ka - kb;
    }
    return a.text.localeCompare(b.text, "tr");
}

/*
   kur(rootModel, kapat, i18n) -> ActionMenu actionList
   kapat(): menüyü kapatan geri çağrı
*/
function kur(rootModel, kapat, klasorIkonu) {
    var hepsi = [];
    _topla(rootModel, [], hepsi, 0);

    var agac = { "ust": [], "don": [], "oyun": [], "dis": {} };   // dis: diğer program klasörleri
    var donAlt = { "Erişilebilirlik": [], "Eğlence": [], "İletişim": [], "Sistem Araçları": [] };
    var gorulen = {}, xpAdlar = {};

    function eylem(e, ad) {
        return { "text": ad, "icon": e.icon, "action": function () {
            e.model.trigger(e.row, "", null);
            kapat();
        } };
    }
    function yerlestir(yer, item, kategori) {
        if (yer === "ust") {
            agac.ust.push(item);
        } else if (yer === "don") {
            agac.don.push(item);
        } else if (yer === "oyun") {
            agac.oyun.push(item);
        } else if (yer.indexOf("don/") === 0) {
            donAlt[yer.substring(4)].push(item);
        } else {
            if (!agac.dis[kategori]) {
                agac.dis[kategori] = [];
            }
            agac.dis[kategori].push(item);
        }
    }

    // 1. geçiş: XP adı alan uygulamalar (her XP adı tek uygulamaya)
    var eslenen = {};
    for (var i = 0; i < hepsi.length; i++) {
        var e = hepsi[i];
        var es = _esleme(e.id);
        if (es && !xpAdlar[es[1]] && !gorulen[e.id]) {
            xpAdlar[es[1]] = true;
            gorulen[e.id] = true;
            eslenen[e.id] = true;
            yerlestir(es[2], eylem(e, es[1]), e.yol.length > 0 ? e.yol[0] : "");
        }
    }
    // 2. geçiş: geri kalanlar kendi kategorilerine
    for (var j = 0; j < hepsi.length; j++) {
        var f = hepsi[j];
        if (f.id && gorulen[f.id]) {
            continue;   // aynı uygulama birden çok kategoride olabilir
        }
        gorulen[f.id] = true;
        var kategori = f.yol.length > 0 ? f.yol[0] : "";
        var yer = KATEGORI[kategori] || (kategori ? "dis" : "ust");
        if (f.yol.length > 1 && KATEGORI[f.yol[1]]) {
            yer = KATEGORI[f.yol[1]];   // alt kategori (ör. İzlenceler ▸ Erişilebilirlik)
        }
        // XP adı başka bir uygulamaya verildiyse (ör. Kate de "Not Defteri" adlıysa) ayırt et
        var ad = xpAdlar[f.label] ? f.label + " (" + f.id.replace(".desktop", "").split(".").pop() + ")" : f.label;
        yerlestir(yer, eylem(f, ad), kategori);
    }

    var don = agac.don.slice();
    for (var alt in donAlt) {
        if (donAlt[alt].length > 0) {
            don.push({ "text": alt, "icon": klasorIkonu, "subActions": donAlt[alt].sort(_sirala) });
        }
    }
    var liste = [];
    liste.push({ "text": "Donatılar", "icon": klasorIkonu, "subActions": don.sort(_sirala) });
    liste.push({ "text": "Başlangıç", "icon": klasorIkonu,
                 "subActions": [{ "text": "(Boş)", "enabled": false }] });
    if (agac.oyun.length > 0) {
        liste.push({ "text": "Oyunlar", "icon": klasorIkonu, "subActions": agac.oyun.sort(_sirala) });
    }
    for (var k in agac.dis) {
        liste.push({ "text": k, "icon": klasorIkonu, "subActions": agac.dis[k].sort(_sirala) });
    }
    liste = liste.concat(agac.ust);
    return liste.sort(_sirala);
}
