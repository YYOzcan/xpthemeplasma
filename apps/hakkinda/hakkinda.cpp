// XPThemePlasma — XP "Hakkında" kutusu (winver, programların Yardım > Hakkında menüsü)
// XP'nin ShellAbout düzeni: üstte shell32'nin "Windows XP" şeridi ve renkli çizgisi, altında sürüm,
// lisans sahibi ve kullanılabilir bellek. İçerik bu bilgisayarın gerçek bilgileridir.
//   xp-hakkinda                 → "Windows Hakkında"
//   xp-hakkinda "Not Defteri"   → "Not Defteri Hakkında"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpkit.h"
#include "../ortak/xpdlg.h"
#include <pwd.h>
#include <unistd.h>

static QString oku(const QString &y) { QFile f(y); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString(); }
static QString osr(const QString &k) {
    for (const QString &s : oku("/etc/os-release").split('\n'))
        if (s.startsWith(k + "=")) { QString v = s.mid(k.size() + 1); if (v.startsWith('"')) v = v.mid(1, v.size() - 2); return v; }
    return {};
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-hakkinda");
    const QString program = argc > 1 ? QString::fromUtf8(argv[1]) : QStringLiteral("Windows");
    QDialog d;
    d.setWindowTitle(program + " Hakkında");
    d.setWindowIcon(QIcon::fromTheme("help-about"));
    auto *l = new QVBoxLayout(&d);
    l->setContentsMargins(0, 0, 0, xp::dluY(7));
    l->setSpacing(0);
    auto *serit = new QLabel;
    QPixmap banner(xp::varlik("hakkinda-banner"));
    serit->setPixmap(banner);
    if (banner.isNull()) { serit->setText("Windows XP"); serit->setMinimumHeight(72); serit->setStyleSheet("background:#5A7EDC;color:white;font:bold 20pt 'Franklin Gothic Medium';"); }
    l->addWidget(serit);
    auto *cizgi = new QLabel;
    cizgi->setPixmap(QPixmap(xp::varlik("hakkinda-cizgi")));
    l->addWidget(cizgi);

    auto *govde = new QGridLayout;
    govde->setContentsMargins(xp::dluX(7), xp::dluY(9), xp::dluX(9), 0);
    govde->setHorizontalSpacing(xp::dluX(9));
    govde->setVerticalSpacing(0);
    auto *ikon = new QLabel;
    QIcon ik = QIcon::fromTheme(program == "Windows" ? "computer" : "help-about");
    ikon->setPixmap(ik.pixmap(32, 32));
    govde->addWidget(ikon, 0, 0, 6, 1, Qt::AlignTop);

    struct passwd *pw = getpwuid(getuid());
    QString ad = pw && pw->pw_gecos ? QString::fromLocal8Bit(pw->pw_gecos).section(',', 0, 0) : QString();
    if (ad.isEmpty()) ad = qEnvironmentVariable("USER");
    QProcess ps;
    ps.start("plasmashell", {"--version"});
    ps.waitForFinished(3000);
    const QString plasma = QString::fromUtf8(ps.readAllStandardOutput()).trimmed().section(' ', -1);
    qint64 kb = 0;
    for (const QString &s : oku("/proc/meminfo").split('\n'))
        if (s.startsWith("MemTotal:")) kb = s.section(':', 1).simplified().section(' ', 0, 0).toLongLong();
    const QLocale tr(QLocale::Turkish, QLocale::Turkey);

    QStringList satir{
        program == "Windows" ? QString("%1 (XPThemePlasma)").arg(osr("NAME").isEmpty() ? "Linux" : osr("NAME")) : program,
        QString("Sürüm %1 (Çekirdek %2, KDE Plasma %3)").arg(QStringLiteral("5.1"), QSysInfo::kernelVersion(), plasma),
        "XPThemePlasma — Windows XP Luna görünümü",
        "",
        "Bu ürünün kullanıcısı:",
        "      " + ad,
        "      " + QSysInfo::machineHostName()};
    int r = 0;
    for (const QString &s : satir) {
        auto *t = new QLabel(s);
        t->setTextInteractionFlags(Qt::TextSelectableByMouse);
        govde->addWidget(t, r++, 1);
    }
    l->addLayout(govde);
    l->addSpacing(xp::dluY(6));
    auto *ayr = new QFrame;
    ayr->setFrameShape(QFrame::HLine);
    ayr->setFrameShadow(QFrame::Sunken);
    auto *ayl = new QHBoxLayout;
    ayl->setContentsMargins(xp::dluX(30) + 32, 0, xp::dluX(9), 0);
    ayl->addWidget(ayr);
    l->addLayout(ayl);
    l->addSpacing(xp::dluY(4));
    auto *bellek = new QLabel(QString("Windows'un kullanabildiği fiziksel bellek:\t%1 KB").arg(tr.toString(kb)));
    auto *bl = new QHBoxLayout;
    bl->setContentsMargins(xp::dluX(30) + 32, 0, xp::dluX(9), 0);
    bl->addWidget(bellek);
    l->addLayout(bl);
    l->addSpacing(xp::dluY(8));
    auto *alt = new QHBoxLayout;
    alt->setContentsMargins(0, 0, xp::dluX(9), 0);
    alt->addStretch();
    auto *tamam = new QPushButton("Tamam");
    tamam->setDefault(true);
    tamam->setFixedSize(xp::dluX(50), xp::dluY(14));
    alt->addWidget(tamam);
    l->addLayout(alt);
    QObject::connect(tamam, &QPushButton::clicked, &d, &QDialog::accept);
    d.setFixedWidth(qMax(413, banner.width()));
    d.show();
    return d.exec() == QDialog::Accepted ? 0 : 1;
}
