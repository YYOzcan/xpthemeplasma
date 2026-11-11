// XPThemePlasma — XP ekran koruyucusu zamanlayıcısı (yalnızca XP oturumunda, autostart)
// Kullanıcı ~/.config/xpthemeplasma/ekrankoruyucu.conf'taki süre kadar boşta kalınca XP'nin gerçek
// ekran koruyucusunu (kendi XP kopyandan .scr, Wine ile) tam ekran başlatır; kullanıcı dokununca .scr
// kendiliğinden kapanır, "Devamında Hoş Geldiniz ekranını görüntüle" seçiliyse oturum kilitlenir.
// Bir program (video oynatıcı vb.) ekran ayarlarını engellediyse başlatılmaz.
//   xp-ekrankoruyucu            arka planda bekle
//   xp-ekrankoruyucu --simdi    seçili ekran koruyucuyu hemen başlat
// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtCore>
#include <QtDBus>
#include <QGuiApplication>
#include <KIdleTime>

static QString ayarDosyasi() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/xpthemeplasma/ekrankoruyucu.conf";
}

class Bekci : public QObject {
public:
    Bekci() {
        izle.addPath(QFileInfo(ayarDosyasi()).path());
        if (QFileInfo::exists(ayarDosyasi())) izle.addPath(ayarDosyasi());
        connect(&izle, &QFileSystemWatcher::fileChanged, this, [this] { yukle(); });
        connect(&izle, &QFileSystemWatcher::directoryChanged, this, [this] {
            if (QFileInfo::exists(ayarDosyasi()) && !izle.files().contains(ayarDosyasi())) izle.addPath(ayarDosyasi());
            yukle();
        });
        connect(KIdleTime::instance(), &KIdleTime::timeoutReached, this, [this](int id) {
            if (id == kimlik) baslat();
        });
        yukle();
    }

    void baslat() {
        if (calisan || dosya.isEmpty()) return;
        // ekran ayarlarını engelleyen (video, sunum) bir şey varsa XP de ekran koruyucuyu başlatmazdı
        QDBusInterface ajan("org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement/PolicyAgent",
                            "org.kde.Solid.PowerManagement.PolicyAgent");
        QDBusReply<bool> engel = ajan.call("HasInhibition", 4u);   // ChangeScreenSettings
        if (engel.isValid() && engel.value()) return;
        const QString wine = QDir::homePath() + "/.local/bin/xp-wine";
        calisan = new QProcess(this);
        connect(calisan, &QProcess::finished, this, [this] {
            calisan->deleteLater();
            calisan = nullptr;
            if (hosGeldiniz) QProcess::startDetached("loginctl", {"lock-session"});
        });
        calisan->start(wine, {dosya, "/s"});
    }

private:
    void yukle() {
        QSettings a(ayarDosyasi(), QSettings::IniFormat);
        dosya = a.value("dosya", "logon.scr").toString();
        const int dakika = qMax(1, a.value("dakika", 10).toInt());
        hosGeldiniz = a.value("hosgeldiniz", true).toBool();
        if (kimlik >= 0) KIdleTime::instance()->removeIdleTimeout(kimlik);
        kimlik = dosya.isEmpty() ? -1 : KIdleTime::instance()->addIdleTimeout(dakika * 60 * 1000);
    }

    QFileSystemWatcher izle;
    QString dosya;
    bool hosGeldiniz = true;
    int kimlik = -1;
    QProcess *calisan = nullptr;
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);                  // KIdleTime Wayland eklentisi için
    app.setDesktopFileName("xp-ekrankoruyucu");
    app.setQuitOnLastWindowClosed(false);
    QDir().mkpath(QFileInfo(ayarDosyasi()).path());
    if (app.arguments().contains("--simdi")) {           // önizleme: bekçiden bağımsız, hemen
        QSettings a(ayarDosyasi(), QSettings::IniFormat);
        const QString d = a.value("dosya", "logon.scr").toString();
        return d.isEmpty() ? 0 : QProcess::execute(QDir::homePath() + "/.local/bin/xp-wine", {d, "/s"});
    }
    // tek örnek
    if (!QDBusConnection::sessionBus().registerService("io.github.yyozcan.XPEkranKoruyucu")) return 0;
    Bekci b;
    return app.exec();
}
