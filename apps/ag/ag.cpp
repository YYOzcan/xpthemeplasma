// XPThemePlasma — XP "Yerel Ağ Bağlantısı Durumu" (bildirim alanındaki ağ simgesine tıklama)
// Genel ve Destek sayfaları kendi XP kopyandaki netshell.dll'nin iletişim kutularından (1802, 1806, 1807).
// Veriler /sys/class/net ve NetworkManager'dan (nmcli). Kullanım: xp-ag [arayüz]
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"

static const QLocale TR(QLocale::Turkish, QLocale::Turkey);
static xp::Modul NS;
static QString S(int id, const char *y) { return NS.dize(id, QString::fromUtf8(y)); }

static QString oku(const QString &y) {
    QFile f(y);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();
}
static QString cikti(const QString &p, const QStringList &a) {
    QProcess pr;
    pr.start(p, a);
    pr.waitForFinished(4000);
    return QString::fromUtf8(pr.readAllStandardOutput());
}

// XP biçimi: "%d.%1d Gbps"
static QString hiz(qint64 mbps) {
    if (mbps <= 0) return QString();
    double bps = mbps * 1e6;
    int birim = 0;
    const int idler[] = {23030, 23031, 23032, 23033, 23034};
    while (bps >= 1000 && birim < 4) { bps /= 1000; ++birim; }
    const int tam = int(bps), ondalik = int((bps - tam) * 10 + 0.5) % 10;
    QString k = S(idler[birim], "%d.%1d Mbps");
    return k.replace("%d.%1d", QString("%1.%2").arg(tam).arg(ondalik));
}

static QString varsayilanArayuz() {
    for (const QString &a : QDir("/sys/class/net").entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System))
        if (a != "lo" && QFileInfo::exists("/sys/class/net/" + a + "/device") && !QFileInfo::exists("/sys/class/net/" + a + "/wireless"))
            return a;
    return {};
}

struct Bilgi {
    QMap<QString, QString> d;          // nmcli device show alanları
    QString deger(const QString &k) const { return d.value(k); }
    QStringList hepsi(const QString &onek) const {
        QStringList l;
        for (auto it = d.begin(); it != d.end(); ++it) if (it.key().startsWith(onek)) l << it.value();
        return l;
    }
};
static Bilgi nmBilgi(const QString &arayuz) {
    Bilgi b;
    for (const QString &s : cikti("nmcli", {"-t", "device", "show", arayuz}).split('\n')) {
        const int i = s.indexOf(':');
        if (i > 0) b.d.insert(s.left(i), s.mid(i + 1).replace("\\:", ":"));
    }
    return b;
}
static QString dhcpSecenek(const Bilgi &b, const QString &ad) {
    for (const QString &v : b.hepsi("DHCP4.OPTION")) {
        const QString t = v.simplified();
        if (t.startsWith(ad + " = ")) return t.mid(ad.size() + 3);
    }
    return {};
}
static QString maske(int on) {
    quint32 m = on ? 0xFFFFFFFFu << (32 - on) : 0;
    return QString("%1.%2.%3.%4").arg(m >> 24).arg((m >> 16) & 255).arg((m >> 8) & 255).arg(m & 255);
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-ag");
    NS = xp::Modul::yukle("netshell-dll");
    const QString arayuz = argc > 1 ? QString::fromUtf8(argv[1]) : varsayilanArayuz();
    if (!NS.gecerli() || arayuz.isEmpty()) return QProcess::startDetached("kcmshell6", {"kcm_networkmanagement"}) ? 0 : 1;
    const QString ad = QFileInfo::exists("/sys/class/net/" + arayuz + "/wireless") ? QStringLiteral("Kablosuz Ağ Bağlantısı")
                                                                                     : S(1811, "Yerel Ağ Bağlantısı");
    xp::Ozellikler d(S(23041, "%1 Durumu").replace("%1", ad));
    d.setWindowIcon(QIcon(NS.simge(190, 16)));
    // durum penceresinde yalnızca "Kapat" vardı
    d.tamam->hide();
    d.uygula->hide();
    d.iptal->setText("&Kapat");
    d.iptal->setDefault(true);

    // ------------------------------------------------------------ Genel
    xp::Sayfa *g = d.ekle(NS, 1802);
    for (int id : {1037, 1004, 1006, 1027, 1012, 1013, 1028}) if (auto *w = g->k(id)) w->hide();
    if (auto *w = g->k(1029)) w->resize(1, xp::dluY(8));
    if (auto *l = g->k<QLabel>(1023)) l->setText(S(23051, "Paketler:"));
    auto *durum = g->k<QLabel>(1007), *sure = g->k<QLabel>(1008), *hizL = g->k<QLabel>(1009);
    auto *giden = g->k<QLabel>(1003), *gelen = g->k<QLabel>(1005), *ikon = g->k<QLabel>(1010);
    giden->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ikon->resize(32, 32);
    auto *ayar = g->k<QPushButton>(1026);
    auto *kes = g->k<QPushButton>(1025);
    kes->setText(S(1617, "&Devre Dışı Bırak"));
    QObject::connect(ayar, &QPushButton::clicked, [] { QProcess::startDetached("kcmshell6", {"kcm_networkmanagement"}); });
    QObject::connect(kes, &QPushButton::clicked, &d, [&] {
        QProcess::execute("nmcli", {"device", "disconnect", arayuz});
        d.reject();
    });

    qint64 oncekiTx = -1, oncekiRx = -1;
    auto yenile = [&] {
        const QString k = "/sys/class/net/" + arayuz + "/";
        const bool bagli = oku(k + "operstate") == "up";
        durum->setText(bagli ? QStringLiteral("Bağlı") : S(1047, "Ağ kablosu takılı değil"));
        // süre: arayüz açılalı beri (sistem açılışı yaklaşığı)
        const qint64 sn = oku("/proc/uptime").section(' ', 0, 0).toDouble();
        const qint64 gun = sn / 86400;
        const QString saat = QTime(0, 0).addSecs(int(sn % 86400)).toString("HH:mm:ss");
        sure->setText(!bagli ? QString() : (gun > 0 ? QString::number(gun) + S(23069, " gün") + " " + saat : saat));
        hizL->setText(bagli ? hiz(oku(k + "speed").toLongLong()) : QString());
        const qint64 tx = oku(k + "statistics/tx_packets").toLongLong(), rx = oku(k + "statistics/rx_packets").toLongLong();
        giden->setText(TR.toString(tx));
        gelen->setText(TR.toString(rx));
        // XP: gönderilirken öndeki, alınırken arkadaki ekran yanar
        const bool g1 = oncekiTx >= 0 && tx > oncekiTx, a1 = oncekiRx >= 0 && rx > oncekiRx;
        ikon->setPixmap(NS.simge(!bagli ? 195 : g1 && a1 ? 193 : g1 ? 191 : a1 ? 192 : 190));
        oncekiTx = tx;
        oncekiRx = rx;
    };
    yenile();
    QTimer zaman;
    QObject::connect(&zaman, &QTimer::timeout, &d, yenile);
    zaman.start(500);

    // ------------------------------------------------------------ Destek
    xp::Sayfa *ds = d.ekle(NS, 1806);
    auto destek = [&] {
        const Bilgi b = nmBilgi(arayuz);
        const QString adres = b.deger("IP4.ADDRESS[1]");
        const QString ip = adres.section('/', 0, 0);
        const int on = adres.section('/', 1).toInt();
        const bool dhcp = !b.hepsi("DHCP4.OPTION").isEmpty();
        ds->k<QLabel>(1053)->setText(ip.isEmpty() ? S(1039, "Geçersiz Adres")
                                     : ip.startsWith("169.254.") ? S(2570, "Otomatik Özel IP Adresi")
                                     : dhcp ? S(2571, "DHCP Tarafından Atanan") : S(2573, "Elle Yapılandırılmış"));
        ds->k<QLabel>(1050)->setText(ip.isEmpty() ? "0.0.0.0" : ip);
        ds->k<QLabel>(1051)->setText(ip.isEmpty() ? "0.0.0.0" : maske(on));
        const QString gw = b.deger("IP4.GATEWAY");
        ds->k<QLabel>(1052)->setText(gw.isEmpty() || gw == "--" ? QString() : gw);
    };
    destek();
    if (auto *ay = ds->k<QPushButton>(1062))
        QObject::connect(ay, &QPushButton::clicked, &d, [&] {
            xp::Iletisim a(NS, 1807, &d);
            if (auto *k = a.sayfa->k<QPushButton>(1)) { QObject::connect(k, &QPushButton::clicked, &a, &QDialog::accept); k->setDefault(true); }
            auto *liste = a.sayfa->k<QTreeWidget>(1070);
            liste->setHeaderLabels({S(23159, "Özellik"), S(23160, "Değer")});
            liste->setColumnWidth(0, xp::dluX(90));
            const Bilgi b = nmBilgi(arayuz);
            const QString adres = b.deger("IP4.ADDRESS[1]");
            auto ekle = [&](const QString &o, const QString &v) { liste->addTopLevelItem(new QTreeWidgetItem({o, v})); };
            ekle(S(23143, "Fiziksel adres"), b.deger("GENERAL.HWADDR").replace(':', '-'));
            ekle(S(23145, "IP Adresi"), adres.section('/', 0, 0));
            ekle(S(23147, "Alt ağ maskesi"), maske(adres.section('/', 1).toInt()));
            ekle(S(23149, "Varsayılan ağ geçidi"), b.deger("IP4.GATEWAY"));
            ekle(S(23152, "DHCP Sunucusu"), dhcpSecenek(b, "dhcp_server_identifier"));
            const QString kira = dhcpSecenek(b, "dhcp_lease_time"), bitis = dhcpSecenek(b, "expiry");
            if (!bitis.isEmpty()) {
                const QDateTime son = QDateTime::fromSecsSinceEpoch(bitis.toLongLong());
                ekle(S(23157, "Kira Alındı"), TR.toString(son.addSecs(-kira.toLongLong()), "dd.MM.yyyy HH:mm:ss"));
                ekle(S(23158, "Kiranın Sona Ermesi"), TR.toString(son, "dd.MM.yyyy HH:mm:ss"));
            }
            const QStringList dns = b.hepsi("IP4.DNS");
            for (int i = 0; i < dns.size(); ++i) ekle(i == 0 ? S(23154, "DNS Sunucuları") : QString(), dns[i]);
            ekle(S(23155, "WINS Sunucusu"), QString());
            a.exec();
        });
    if (auto *onar = ds->k<QPushButton>(1056))
        QObject::connect(onar, &QPushButton::clicked, &d, [&] {
            const QString baglanti = nmBilgi(arayuz).deger("GENERAL.CONNECTION");
            QProgressDialog p(S(1413, "Onarılıyor") + "...", QString(), 0, 0, &d);
            p.setWindowTitle(S(1411, "Bağlantıyı Onar"));
            p.setMinimumDuration(0);
            p.show();
            QProcess pr;
            pr.start("nmcli", baglanti.isEmpty() ? QStringList{"device", "connect", arayuz} : QStringList{"connection", "up", baglanti});
            while (!pr.waitForFinished(100)) QApplication::processEvents();
            p.close();
            QMessageBox::information(&d, S(1411, "Bağlantıyı Onar"),
                                     pr.exitCode() == 0 ? S(1408, "Onarım işlemi tamamlandı.") : S(1410, "Onarım işlemi başarısız oldu."));
            destek();
        });

    d.show();
    return d.exec() == QDialog::Accepted ? 0 : 1;
}
