// XPThemePlasma — "Çalıştır" komutları: XP adlarını (cmd, notepad, control, sysdm.cpl…) bu oturumdaki
// karşılıklarına çevirir; ortam değişkenlerini (%USERPROFILE%) açar, URL/dosya/klasörleri varsayılan
// programla açar. Çalıştır penceresi ve Görev Yöneticisi'nin "Yeni Görev"i kullanır.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtWidgets>

namespace xp {

inline QString yerelBin(const QString &ad) {
    const QString p = QDir::homePath() + "/.local/bin/" + ad;
    return QFileInfo(p).isExecutable() ? p : QString();
}

// XP komutu → çalıştırılacak program + argümanlar (boşsa eşleşme yok)
inline QStringList xpKomutu(const QString &komut) {
    const QString k = komut.toLower();
    auto bin = [](const QString &ad, const QStringList &arg = {}) -> QStringList {
        const QString p = yerelBin(ad);
        return p.isEmpty() ? QStringList() : QStringList{p} + arg;
    };
    auto kcm = [](const QString &modul) { return QStringList{"kcmshell6", modul}; };
    auto wine = [&](const QString &exe) { return bin("xp-wine", {exe}); };
    static const QHash<QString, QString> kcmler = {
        {"appwiz.cpl", ""}, {"mmsys.cpl", "kcm_pulseaudio"}, {"main.cpl", "kcm_mouse"},
        {"ncpa.cpl", "kcm_networkmanagement"}, {"powercfg.cpl", "kcm_powerdevilprofilesconfig"},
        {"intl.cpl", "kcm_regionandlang"}, {"nusrmgr.cpl", "kcm_users"}, {"access.cpl", "kcm_access"},
        {"joy.cpl", "kcm_gamecontroller"}, {"bthprops.cpl", "kcm_bluetooth"}, {"printers", "kcm_printer_manager"},
        {"inetcpl.cpl", "kcm_proxy"}, {"firewall.cpl", "kcm_firewall"}, {"fonts", "kcm_fonts"},
        {"keyboard", "kcm_keyboard"}, {"mouse", "kcm_mouse"}};
    if (k == "cmd" || k == "cmd.exe" || k == "command" || k == "command.com")
        return {"konsole", "--profile", "XP Komut İstemi"};
    if (k == "notepad" || k == "notepad.exe") return bin("xp-notdefteri");
    if (k == "explorer" || k == "explorer.exe") return bin("xp-gezgin");
    if (k == "control" || k == "control.exe" || k == "control panel") return bin("xp-denetim");
    if (k == "taskmgr" || k == "taskmgr.exe") return bin("xp-gorev");
    if (k == "winver" || k == "winver.exe") return bin("xp-hakkinda");
    if (k == "sysdm.cpl" || k == "control sysdm.cpl") return bin("xp-sistem");
    if (k == "timedate.cpl" || k == "control timedate.cpl") return bin("xp-tarihsaat");
    if (k == "desk.cpl" || k == "control desk.cpl" || k == "control desktop") return bin("xp-gorunum");
    if (k == "appwiz.cpl" || k == "control appwiz.cpl") return {"plasma-discover"};
    if (k == "msconfig" || k == "msconfig.exe") return kcm("kcm_autostart");
    if (k == "regedit" || k == "regedit.exe" || k == "regedt32") return wine("regedit.exe");
    if (k == "iexplore" || k == "iexplore.exe") return {"xdg-open", "about:blank"};
    if (k == "wmplayer" || k == "wmplayer.exe" || k == "mplayer2") {
        for (const QString &p : {"haruna", "vlc", "elisa", "mpv"})
            if (!QStandardPaths::findExecutable(p).isEmpty()) return {p};
    }
    if (k == "logoff" || k == "logoff.exe") return {"qdbus6", "org.kde.LogoutPrompt", "/LogoutPrompt", "promptLogout"};
    if (k == "shutdown" || k == "shutdown.exe") return {"qdbus6", "org.kde.LogoutPrompt", "/LogoutPrompt", "promptShutDown"};
    for (const QString &e : {"calc", "mspaint", "wordpad", "write", "charmap", "sndrec32", "winmine", "sol",
                             "freecell", "mshearts", "spider", "pinball", "clipbrd", "magnify", "osk"}) {
        if (k == e || k == e + ".exe") {
            QStringList w = wine(e + ".exe");
            if (!w.isEmpty()) return w;
        }
    }
    QString cpl = k;
    if (cpl.startsWith("control ")) cpl = cpl.mid(8);
    if (kcmler.contains(cpl) && !kcmler.value(cpl).isEmpty()) return kcm(kcmler.value(cpl));
    return {};
}

// %VAR% ve XP kısaltmalarını aç
inline QString ortamAc(QString s) {
    static const QRegularExpression re("%([A-Za-z_][A-Za-z0-9_]*)%");
    auto it = re.globalMatch(s);
    QList<QPair<QString, QString>> degis;
    while (it.hasNext()) {
        auto m = it.next();
        const QString ad = m.captured(1).toUpper();
        QString deger;
        if (ad == "USERPROFILE" || ad == "HOMEPATH" || ad == "HOME") deger = QDir::homePath();
        else if (ad == "WINDIR" || ad == "SYSTEMROOT") deger = "/usr";
        else if (ad == "TEMP" || ad == "TMP") deger = QDir::tempPath();
        else if (ad == "APPDATA") deger = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
        else deger = qEnvironmentVariable(m.captured(1).toUtf8().constData());
        degis << qMakePair(m.captured(0), deger);
    }
    for (const auto &d : degis) s.replace(d.first, d.second);
    if (s.startsWith("~/") || s == "~") s.replace(0, 1, QDir::homePath());
    // "C:\" → kök, ters bölüler → bölü
    static const QRegularExpression surucu("^[A-Za-z]:\\\\");
    if (s.contains(surucu)) { s.remove(0, 2); s.replace('\\', '/'); }
    return s;
}

// çalıştırmayı dene; başarısızsa false
inline bool calistir(const QString &girdi) {
    const QString metin = girdi.trimmed();
    if (metin.isEmpty()) return false;
    // tam eşleşme (argümansız) ya da "komut argümanlar"
    QStringList hedef = xpKomutu(metin);
    QString ilk = metin.section(' ', 0, 0), kalan = metin.section(' ', 1);
    if (hedef.isEmpty() && !kalan.isEmpty()) {
        hedef = xpKomutu(ilk);
        if (!hedef.isEmpty()) hedef << QProcess::splitCommand(ortamAc(kalan));
    }
    if (!hedef.isEmpty()) return QProcess::startDetached(hedef.first(), hedef.mid(1));

    const QString acik = ortamAc(metin);
    if (acik.contains("://") || acik.startsWith("www.", Qt::CaseInsensitive) || acik.startsWith("mailto:"))
        return QProcess::startDetached("xdg-open", {acik.startsWith("www.") ? "http://" + acik : acik});
    if (QFileInfo::exists(acik)) {
        QFileInfo fi(acik);
        if (fi.isFile() && fi.isExecutable()) return QProcess::startDetached(fi.absoluteFilePath(), {});
        return QProcess::startDetached("xdg-open", {fi.absoluteFilePath()});
    }
    QStringList parca = QProcess::splitCommand(acik);
    if (parca.isEmpty()) return false;
    QString prog = parca.takeFirst();
    if (prog.endsWith(".exe", Qt::CaseInsensitive)) prog.chop(4);
    if (!QStandardPaths::findExecutable(prog).isEmpty()) return QProcess::startDetached(prog, parca);
    return false;
}

}  // namespace xp
