// XPThemePlasma — XP "Sistem Özellikleri" (Windows+Pause, Bilgisayarım > Özellikler, Denetim Masası > Sistem)
// Genel / Donanım / Gelişmiş sayfaları kendi XP kopyandaki sysdm.cpl'nin iletişim kutularından kurulur;
// içerik bu bilgisayarın gerçek bilgileridir (dağıtım, çekirdek, Plasma, işlemci, bellek).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"
#include <pwd.h>
#include <unistd.h>

static QString dosyaOku(const QString &yol) {
    QFile f(yol);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

static QString osRelease(const QString &anahtar) {
    for (const QString &satir : dosyaOku("/etc/os-release").split('\n')) {
        if (satir.startsWith(anahtar + "=")) {
            QString v = satir.mid(anahtar.size() + 1);
            if (v.startsWith('"')) v = v.mid(1, v.size() - 2);
            return v;
        }
    }
    return {};
}

static QString plasmaSurumu() {
    QProcess p;
    p.start("plasmashell", {"--version"});
    if (!p.waitForFinished(3000)) return {};
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed().section(' ', -1);
}

static QString islemci() {
    for (const QString &satir : dosyaOku("/proc/cpuinfo").split('\n'))
        if (satir.startsWith("model name")) return satir.section(':', 1).simplified();
    return QSysInfo::currentCpuArchitecture();
}

static QString ondalik(double v, int basamak) {
    return QLocale(QLocale::Turkish).toString(v, 'f', basamak);
}

static void komut(const QString &prog, const QStringList &arg = {}) { QProcess::startDetached(prog, arg); }

// XP ürün kimliği biçiminde (55274-640-1234567-23456), makine kimliğinden türetilmiş
static QString urunKimligi() {
    const QString mid = dosyaOku("/etc/machine-id").trimmed();
    QString r;
    for (QChar c : mid) r += QString::number(c.isDigit() ? c.digitValue() : (c.unicode() - 'a' + 10) % 10);
    r = r.leftJustified(22, '0');
    return r.mid(0, 5) + "-" + r.mid(5, 3) + "-" + r.mid(8, 7) + "-" + r.mid(15, 5);
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-sistem");
    const xp::Modul sysdm = xp::Modul::yukle("sysdm-cpl");
    if (!sysdm.gecerli()) return QProcess::startDetached("kinfocenter", {}) ? 0 : 1;

    xp::Ozellikler d(sysdm.dize(3, "Sistem Özellikleri").remove(QChar(0)));
    d.setWindowIcon(QIcon(sysdm.simge(1, 16)));

    // ------------------------------------------------------------ Genel
    xp::Sayfa *g = d.ekle(sysdm, 101);
    if (auto *logo = g->k<QLabel>(51)) {
        logo->setPixmap(sysdm.bitmap(1));
        logo->setAlignment(Qt::AlignCenter);
    }
    const QString dagitim = osRelease("NAME").isEmpty() ? QStringLiteral("Linux") : osRelease("NAME");
    QStringList sistem{dagitim};
    const QString surum = osRelease("VERSION_ID").isEmpty() ? osRelease("BUILD_ID") : osRelease("VERSION_ID");
    if (!surum.isEmpty()) sistem << QString("Sürüm %1").arg(surum == "rolling" ? QStringLiteral("(sürekli)") : surum);
    const QString ps = plasmaSurumu();
    sistem << (ps.isEmpty() ? QStringLiteral("KDE Plasma") : "KDE Plasma " + ps);
    sistem << "Çekirdek " + QSysInfo::kernelVersion().section('-', 0, 0);
    for (int i = 0; i < 4; ++i)
        if (auto *l = g->k<QLabel>(52 + i)) l->setText(sistem.value(i));

    struct passwd *pw = getpwuid(getuid());
    QString tamAd = pw && pw->pw_gecos ? QString::fromLocal8Bit(pw->pw_gecos).section(',', 0, 0) : QString();
    if (tamAd.isEmpty()) tamAd = qEnvironmentVariable("USER");
    const QStringList kayit{tamAd, QSysInfo::machineHostName(), "", urunKimligi()};
    for (int i = 0; i < 4; ++i)
        if (auto *e = g->k<QLineEdit>(56 + i)) e->setText(kayit.value(i));

    const QString cpu = islemci();
    QStringList bilgisayar;
    // XP işlemci adını iki satıra bölerdi
    if (cpu.size() > 28) {
        const int kes = cpu.lastIndexOf(' ', 28);
        bilgisayar << cpu.left(kes) << cpu.mid(kes + 1);
    } else {
        bilgisayar << cpu;
    }
    const double mhz = dosyaOku("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq").trimmed().toDouble() / 1000.0;
    qint64 kb = 0;
    for (const QString &satir : dosyaOku("/proc/meminfo").split('\n'))
        if (satir.startsWith("MemTotal:")) kb = satir.section(':', 1).simplified().section(' ', 0, 0).toLongLong();
    const QString ram = ondalik(kb / 1024.0 / 1024.0, 2) + " GB RAM";
    bilgisayar << (mhz > 0 ? (mhz >= 1000 ? ondalik(mhz / 1000.0, 2) + " GHz, " : ondalik(mhz, 0) + " MHz, ") + ram : ram);
    for (int i = 0; i < 9; ++i) {
        const int id = i < 6 ? 63 + i : 70 + (i - 6);
        if (auto *l = g->k<QLabel>(id)) l->setText(bilgisayar.value(i));
    }
    // "Üreten ve destekleyen": XP'de OEM logosu; burada dağıtımın logosu ve destek bağlantısı
    const QString logoAdi = osRelease("LOGO");
    const QIcon oem = QIcon::fromTheme(logoAdi.isEmpty() ? "distributor-logo" : logoAdi);
    if (!oem.isNull()) {
        if (auto *l = g->k<QLabel>(60)) l->show();
        if (auto *l = g->k<QLabel>(62)) {
            l->setPixmap(oem.pixmap(64, 64));
            l->setAlignment(Qt::AlignCenter);
        }
        const QString destek = osRelease("SUPPORT_URL").isEmpty() ? osRelease("HOME_URL") : osRelease("SUPPORT_URL");
        if (auto *b = g->k<QPushButton>(69); b && !destek.isEmpty()) {
            b->show();
            b->setEnabled(true);
            QObject::connect(b, &QPushButton::clicked, [destek] { QDesktopServices::openUrl(QUrl(destek)); });
        }
    }

    // ------------------------------------------------------------ Bilgisayar Adı (XP'de netid.dll)
    auto *adSayfa = new QWidget;
    {
        auto *s = new QWidget(adSayfa);
        s->move(1, 1);
        s->setFixedSize(xp::dluX(256), xp::dluY(218));
        auto *ikon = new QLabel(s);
        ikon->setPixmap(sysdm.simge(6));
        ikon->setGeometry(xp::dluX(7), xp::dluY(7), 32, 32);
        auto *aciklama = new QLabel("Windows bilgisayarınızı ağda tanımlamak için aşağıdaki bilgileri kullanır.", s);
        aciklama->setWordWrap(true);
        aciklama->setGeometry(xp::dluX(36), xp::dluY(10), xp::dluX(210), xp::dluY(18));
        auto *l1 = new QLabel("Bilgisayar a&çıklaması:", s);
        l1->setGeometry(xp::dluX(7), xp::dluY(39), xp::dluX(76), xp::dluY(10));
        auto *aciklamaKutu = new QLineEdit(s);
        aciklamaKutu->setGeometry(xp::dluX(86), xp::dluY(37), xp::dluX(162), 21);
        l1->setBuddy(aciklamaKutu);
        QProcess hp;
        hp.start("hostnamectl", {"--pretty"});
        if (hp.waitForFinished(2000)) aciklamaKutu->setText(QString::fromUtf8(hp.readAllStandardOutput()).trimmed());
        auto *ornek = new QLabel("Örneğin: \"Mutfak Bilgisayarı\" ya da \"Ayşe'nin Bilgisayarı\".", s);
        ornek->setWordWrap(true);
        ornek->setGeometry(xp::dluX(86), xp::dluY(53), xp::dluX(162), xp::dluY(18));
        auto *l2 = new QLabel("Tam bilgisayar adı:", s);
        l2->setGeometry(xp::dluX(7), xp::dluY(78), xp::dluX(76), xp::dluY(8));
        auto *tam = new QLabel(QSysInfo::machineHostName() + ".", s);
        tam->setGeometry(xp::dluX(86), xp::dluY(78), xp::dluX(162), xp::dluY(8));
        auto *l3 = new QLabel("Çalışma grubu:", s);
        l3->setGeometry(xp::dluX(7), xp::dluY(92), xp::dluX(76), xp::dluY(8));
        auto *grup = new QLabel("WORKGROUP", s);
        grup->setGeometry(xp::dluX(86), xp::dluY(92), xp::dluX(162), xp::dluY(8));
        auto *t1 = new QLabel("Bir etki alanına katılmak ve yerel kullanıcı hesabı oluşturmak için Ağ Kimliği'ni tıklatın.", s);
        t1->setWordWrap(true);
        t1->setGeometry(xp::dluX(7), xp::dluY(117), xp::dluX(180), xp::dluY(26));
        auto *agKimligi = new QPushButton("&Ağ Kimliği", s);
        agKimligi->setGeometry(xp::dluX(192), xp::dluY(117), xp::dluX(57), xp::dluY(14));
        agKimligi->setEnabled(false);
        auto *t2 = new QLabel("Bu bilgisayarı yeniden adlandırmak ya da bir etki alanına katılmak için Değiştir'i tıklatın.", s);
        t2->setWordWrap(true);
        t2->setGeometry(xp::dluX(7), xp::dluY(150), xp::dluX(180), xp::dluY(26));
        auto *degistir = new QPushButton("&Değiştir...", s);
        degistir->setGeometry(xp::dluX(192), xp::dluY(150), xp::dluX(57), xp::dluY(14));
        QObject::connect(aciklamaKutu, &QLineEdit::textEdited, &d, &xp::Ozellikler::degisti);
        QObject::connect(degistir, &QPushButton::clicked, &d, [&d, tam] {
            bool tamam = false;
            const QString yeni = QInputDialog::getText(&d, "Bilgisayar Adı Değişiklikleri",
                                                       "&Bilgisayar adı:", QLineEdit::Normal,
                                                       QSysInfo::machineHostName(), &tamam).trimmed();
            if (!tamam || yeni.isEmpty() || yeni == QSysInfo::machineHostName()) return;
            QProcess p;
            p.start("hostnamectl", {"set-hostname", yeni});
            p.waitForFinished(60000);
            if (p.exitCode() == 0) {
                tam->setText(yeni + ".");
                QMessageBox::information(&d, "Bilgisayar Adı Değişiklikleri",
                                         "Bu değişikliklerin geçerli olabilmesi için bu bilgisayarı yeniden başlatmanız gerekir.");
            }
        });
        d.uygulaIslevi = [aciklamaKutu] {
            QProcess::execute("hostnamectl", {"set-hostname", "--pretty", aciklamaKutu->text()});
            return true;
        };
    }
    d.sekmeler->addTab(adSayfa, "Bilgisayar Adı");

    // ------------------------------------------------------------ Donanım
    xp::Sayfa *dn = d.ekle(sysdm, 2000);
    if (auto *l = dn->k<QLabel>(2001)) { l->setPixmap(sysdm.simge(8)); l->resize(32, 32); }
    if (auto *l = dn->k<QLabel>(2004)) { l->setPixmap(sysdm.simge(7)); l->resize(32, 32); }
    for (int id : {2003, 2010, 2007})
        if (auto *b = dn->k<QPushButton>(id)) b->setEnabled(false);
    if (auto *b = dn->k<QPushButton>(2006))
        QObject::connect(b, &QPushButton::clicked, [] { komut("kinfocenter", {"kcm_pci"}); });

    // ------------------------------------------------------------ Gelişmiş
    xp::Sayfa *gl = d.ekle(sysdm, 115);
    if (auto *b = gl->k<QPushButton>(110))
        QObject::connect(b, &QPushButton::clicked, [] { komut("kcmshell6", {"kcm_kwin_effects"}); });
    if (auto *b = gl->k<QPushButton>(210))
        QObject::connect(b, &QPushButton::clicked, [] { komut("kcmshell6", {"kcm_users"}); });
    if (auto *b = gl->k<QPushButton>(150))
        QObject::connect(b, &QPushButton::clicked, [] { komut("kcmshell6", {"kcm_autostart"}); });
    for (int id : {130, 4299})
        if (auto *b = gl->k<QPushButton>(id)) b->setEnabled(false);

    // komut satırı: sekme numarası (XP: control sysdm.cpl,,1)
    const QStringList a = app.arguments();
    for (const QString &arg : a.mid(1)) {
        bool sayi = false;
        const int n = arg.section(',', -1).toInt(&sayi);
        if (sayi && n >= 0 && n < d.sekmeler->count()) d.sekmeler->setCurrentIndex(n);
    }
    d.show();
    return d.exec() == QDialog::Accepted ? 0 : 1;
}
