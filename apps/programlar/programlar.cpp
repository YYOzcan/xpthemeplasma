// XPThemePlasma — XP "Program Ekle veya Kaldır" (Denetim Masası, Çalıştır: appwiz.cpl)
// XP'de bu bir HTA'ydı (appwiz.cpl: DEFAULT.HTA, ARP.CSS, PLACES.HTC); düzen, renkler, metinler ve
// büyük düğme görselleri oradan. Programlar: menüde görünen uygulamalar, sahibi olan paket (pacman) ya da
// Flatpak; boyut paket yöneticisinden, son kullanma tarihi programın erişim zamanından.
// Kaldır, ne kaldırılacağını gösterip onay isteyen bir Komut İstemi penceresinde çalışır.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"
#include <sys/stat.h>

static const QLocale TR(QLocale::Turkish, QLocale::Turkey);
static xp::Modul AW;
static QString S(int id, const char *y) { return AW.dize(id, QString::fromUtf8(y)).trimmed(); }

struct Program {
    QString ad, simge, paket, kaynak;      // kaynak: pacman | flatpak
    qint64 bayt = 0;
    QDateTime sonKullanim;
    QString yorum, destek;
};

static QString cikti(const QString &p, const QStringList &a, int ms = 20000) {
    QProcess pr;
    QProcessEnvironment e = QProcessEnvironment::systemEnvironment();
    e.insert("LANG", "C");
    e.insert("LC_ALL", "C");
    pr.setProcessEnvironment(e);
    pr.start(p, a);
    pr.waitForFinished(ms);
    return QString::fromUtf8(pr.readAllStandardOutput());
}

static qint64 boyutCoz(const QString &s) {     // "12.34 MiB"
    const double v = s.section(' ', 0, 0).toDouble();
    const QString b = s.section(' ', 1, 1);
    const qint64 k = b.startsWith("Ki") ? 1024LL : b.startsWith("Mi") ? 1024LL * 1024 : b.startsWith("Gi") ? 1024LL * 1024 * 1024 : 1;
    return qint64(v * k);
}

static QList<Program> programlar() {
    QMap<QString, Program> paketler;               // paket -> program (ilk masaüstü dosyası)
    QStringList dosyalar;
    QHash<QString, QPair<QString, QString>> bilgi;  // dosya -> (ad, simge)
    QHash<QString, QString> calistirilan;
    for (const QString &d : QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "applications", QStandardPaths::LocateDirectory)) {
        if (d.contains("xpthemeplasma")) continue;  // XP katmanının kopyaları sistem programlarıdır
        for (const QFileInfo &fi : QDir(d).entryInfoList({"*.desktop"}, QDir::Files)) {
            QSettings ds(fi.filePath(), QSettings::IniFormat);
            ds.beginGroup("Desktop Entry");
            if (ds.value("NoDisplay").toString() == "true" || ds.value("Hidden").toString() == "true" || ds.value("Type").toString() != "Application")
                continue;
            const QString ad = ds.value("Name[tr]", ds.value("Name")).toString();
            if (ad.isEmpty()) continue;
            dosyalar << fi.filePath();
            bilgi.insert(fi.filePath(), {ad, ds.value("Icon").toString()});
            calistirilan.insert(fi.filePath(), QProcess::splitCommand(ds.value("Exec").toString()).value(0));
        }
    }
    // pacman: hangi paket hangi masaüstü dosyasının sahibi
    QStringList sistem;
    for (const QString &f : dosyalar) if (f.startsWith("/usr/")) sistem << f;
    for (const QString &satir : cikti("pacman", QStringList{"-Qo"} + sistem).split('\n')) {
        const int i = satir.indexOf(" is owned by ");
        if (i < 0) continue;
        const QString dosya = satir.left(i), paket = satir.mid(i + 13).section(' ', 0, 0);
        if (paketler.contains(paket)) continue;
        Program p;
        p.ad = bilgi.value(dosya).first;
        p.simge = bilgi.value(dosya).second;
        p.paket = paket;
        p.kaynak = "pacman";
        const QString exe = QStandardPaths::findExecutable(calistirilan.value(dosya));
        struct stat st;
        if (!exe.isEmpty() && stat(exe.toLocal8Bit().constData(), &st) == 0)
            p.sonKullanim = QDateTime::fromSecsSinceEpoch(st.st_atime);
        paketler.insert(paket, p);
    }
    if (!paketler.isEmpty()) {
        QString paket;
        for (const QString &satir : cikti("pacman", QStringList{"-Qi"} + paketler.keys()).split('\n')) {
            const QString a = satir.section(':', 0, 0).trimmed(), v = satir.section(':', 1).trimmed();
            if (a == "Name") paket = v;
            else if (a == "Installed Size" && paketler.contains(paket)) paketler[paket].bayt = boyutCoz(v);
            else if (a == "URL" && paketler.contains(paket)) paketler[paket].destek = v;
            else if (a == "Description" && paketler.contains(paket)) paketler[paket].yorum = v;
        }
    }
    QList<Program> l = paketler.values();
    // Flatpak uygulamaları
    if (!QStandardPaths::findExecutable("flatpak").isEmpty()) {
        for (const QString &satir : cikti("flatpak", {"list", "--app", "--columns=application,name,size"}).split('\n', Qt::SkipEmptyParts)) {
            const QStringList f = satir.split('\t');
            if (f.size() < 2) continue;
            Program p;
            p.paket = f[0];
            p.ad = f[1];
            p.simge = f[0];
            p.kaynak = "flatpak";
            p.bayt = f.size() > 2 ? boyutCoz(QString(f[2]).replace(" MB", " MiB").replace(" GB", " GiB").replace(" kB", " KiB")) : 0;
            l << p;
        }
    }
    return l;
}

// XP: "Boyut 12,34MB"
static QString boyutYazi(qint64 b) {
    if (b <= 0) return {};
    if (b >= 1024LL * 1024 * 1024) return TR.toString(b / 1024.0 / 1024 / 1024, 'f', 2) + "GB";
    if (b >= 1024 * 1024) return TR.toString(b / 1024.0 / 1024, 'f', 2) + "MB";
    return TR.toString(b / 1024.0, 'f', 2) + "KB";
}
static QString siklik(const QDateTime &t) {
    if (!t.isValid()) return {};
    const qint64 gun = t.daysTo(QDateTime::currentDateTime());
    return gun <= 7 ? "sık" : gun <= 31 ? "ara sıra" : "nadiren";
}

// ---------------------------------------------------------------- yerler çubuğu (PLACES.HTC)
class YerDugmesi : public QAbstractButton {
public:
    YerDugmesi(const QPixmap &p, const QString &t, QWidget *e = nullptr) : QAbstractButton(e), resim(p) {
        setText(t);
        setCheckable(true);
        setAutoExclusive(true);
        setFixedWidth(79);
        const QRect m = fontMetrics().boundingRect(QRect(0, 0, 75, 1000), Qt::TextWordWrap | Qt::AlignHCenter, t);
        setFixedHeight(6 + p.height() + 2 + m.height() + 8);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        const QRect r = rect().adjusted(0, 0, -1, -1);
        if (isChecked()) p.fillRect(rect(), palette().color(QPalette::Window).darker(108));
        if (isChecked() || isDown() || underMouse()) {
            const bool cokuk = isChecked() || isDown();
            const QColor ust = cokuk ? palette().color(QPalette::Shadow) : palette().color(QPalette::Light);
            const QColor alt = cokuk ? palette().color(QPalette::Light) : palette().color(QPalette::Shadow);
            p.setPen(ust);
            p.drawLine(r.topLeft(), r.topRight());
            p.drawLine(r.topLeft(), r.bottomLeft());
            p.setPen(alt);
            p.drawLine(r.bottomLeft(), r.bottomRight());
            p.drawLine(r.topRight(), r.bottomRight());
        }
        p.drawPixmap((width() - resim.width()) / 2, 6, resim);
        p.setPen(isChecked() ? palette().color(QPalette::WindowText) : Qt::white);
        p.drawText(QRect(2, 6 + resim.height() + 2, width() - 4, height() - resim.height() - 8),
                   Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, text());
    }
private:
    QPixmap resim;
};

// ---------------------------------------------------------------- program satırı (LISTBOX.HTC)
class Satir : public QFrame {
public:
    Program p;
    bool secili = false;
    std::function<void(Satir *)> secildi;
    std::function<void(const Program &)> kaldir;
    QLabel *ikon, *ad, *boyutE, *boyut, *ayrinti;
    QPushButton *dugme;
    explicit Satir(const Program &pr) : p(pr) {
        setAutoFillBackground(true);
        auto *g = new QGridLayout(this);
        g->setContentsMargins(4, 3, 7, 3);
        g->setHorizontalSpacing(4);
        g->setVerticalSpacing(1);
        ikon = new QLabel;
        ikon->setPixmap(QIcon::fromTheme(p.simge, QIcon::fromTheme("application-x-executable")).pixmap(16, 16));
        ikon->setFixedSize(20, 18);
        ad = new QLabel(p.ad);
        boyutE = new QLabel(S(21, "Boyut"));
        boyut = new QLabel(boyutYazi(p.bayt));
        boyut->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        boyut->setMinimumWidth(70);
        g->addWidget(ikon, 0, 0, Qt::AlignTop);
        g->addWidget(ad, 0, 1);
        g->addWidget(boyutE, 0, 2, Qt::AlignRight);
        g->addWidget(boyut, 0, 3);
        g->setColumnStretch(1, 1);
        ayrinti = new QLabel;
        ayrinti->setTextFormat(Qt::RichText);
        ayrinti->setOpenExternalLinks(true);
        g->addWidget(ayrinti, 1, 1, 1, 3);
        dugme = new QPushButton(S(19, "Değiştir/Kaldır"));
        auto *alt = new QHBoxLayout;
        auto *ac = new QLabel(S(88, "Bu program bilgisayardan kaldırmak için Kaldır'ı tıklatın."));
        ac->setWordWrap(true);
        ac->setObjectName("aciklama");
        alt->addWidget(ac, 1);
        alt->addWidget(dugme);
        g->addLayout(alt, 2, 1, 1, 3);
        connect(dugme, &QPushButton::clicked, this, [this] { if (kaldir) kaldir(p); });
        sec(false);
        if (boyut->text().isEmpty()) boyutE->hide();
    }
    void sec(bool s) {
        secili = s;
        QPalette pal = palette();
        pal.setColor(QPalette::Window, s ? QColor("#316AC5") : Qt::white);
        const QColor yazi = s ? Qt::white : Qt::black;
        setPalette(pal);
        for (QLabel *l : findChildren<QLabel *>()) {
            QPalette lp = l->palette();
            lp.setColor(QPalette::WindowText, yazi);
            l->setPalette(lp);
        }
        QStringList satirlar;
        const QString renk = s ? "#FFFFFF" : "#000000";
        if (!siklik(p.sonKullanim).isEmpty())
            satirlar << QString("<tr><td align=right>%1</td><td width=8></td><td align=right>%2</td></tr>")
                            .arg("Kullanım", siklik(p.sonKullanim))
                     << QString("<tr><td align=right>%1</td><td></td><td align=right>%2</td></tr>")
                            .arg(S(62, "Son Kullanma Tarihi"), TR.toString(p.sonKullanim.date(), "dd.MM.yyyy"));
        QString h = "<table style='color:" + renk + "' align=right cellspacing=0 cellpadding=1>" + satirlar.join("") + "</table>";
        if (!p.destek.isEmpty())
            h = QString("<a style='color:%1' href='%2'>%3</a>").arg(renk, p.destek, S(16, "Destek bilgisi için burayı tıklatın.")) + h;
        ayrinti->setText(h);
        ayrinti->setVisible(s);
        dugme->setVisible(s);
        findChild<QLabel *>("aciklama")->setVisible(s);
    }
protected:
    void mousePressEvent(QMouseEvent *) override { if (secildi) secildi(this); }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-programlar");
    AW = xp::Modul::yukle("appwiz-cpl");
    if (!AW.gecerli()) return QProcess::startDetached("plasma-discover", {}) ? 0 : 1;
    const QString dlg = xp::dlgDizini();

    QWidget w;
    w.setWindowTitle(S(56, "Program Ekle veya Kaldır"));
    w.setWindowIcon(QIcon::fromTheme("system-software-install"));
    auto *ana = new QHBoxLayout(&w);
    ana->setContentsMargins(0, 0, 0, 0);
    ana->setSpacing(0);

    // yerler çubuğu: arka plan threedshadow
    auto *yerler = new QWidget;
    yerler->setAutoFillBackground(true);
    QPalette yp = yerler->palette();
    yp.setColor(QPalette::Window, yp.color(QPalette::Dark));
    yerler->setPalette(yp);
    yerler->setFixedWidth(83);
    auto *yl = new QVBoxLayout(yerler);
    yl->setContentsMargins(2, 2, 2, 2);
    yl->setSpacing(2);
    auto *degistir = new YerDugmesi(QPixmap(dlg + "appwiz-install_lg.png"), "Programları Değiştir veya Kaldır");
    auto *ekle = new YerDugmesi(QPixmap(dlg + "appwiz-add_lg.png"), "Yeni Program Ekle");
    auto *bilesen = new YerDugmesi(QPixmap(dlg + "appwiz-config_lg.png"), "Windows Bileşenlerini Ekle ya da Kaldır");
    bilesen->setCheckable(false);
    for (auto *b : {degistir, ekle, bilesen}) yl->addWidget(b);
    yl->addStretch();
    degistir->setChecked(true);
    ana->addWidget(yerler);

    auto *yigin = new QStackedWidget;
    ana->addWidget(yigin, 1);

    // --- Programları Değiştir veya Kaldır
    auto *sayfa = new QWidget;
    auto *sl = new QVBoxLayout(sayfa);
    sl->setContentsMargins(11, 11, 11, 11);
    sl->setSpacing(5);
    auto *ust = new QHBoxLayout;
    ust->addWidget(new QLabel(S(8, "Şu anda yüklü programlar:")));
    ust->addStretch();
    auto *siraEt = new QLabel("&" + S(40, "Sıralama ölçütü:").remove('&'));
    auto *sira = new QComboBox;
    sira->addItems({"Ad", S(21, "Boyut"), S(23, "Kullanım Sıklığı"), S(44, "Son Kullanma")});
    siraEt->setBuddy(sira);
    ust->addWidget(siraEt);
    ust->addWidget(sira);
    sl->addLayout(ust);
    auto *kaydir = new QScrollArea;
    kaydir->setWidgetResizable(true);
    kaydir->setFrameShape(QFrame::StyledPanel);
    auto *liste = new QWidget;
    liste->setAutoFillBackground(true);
    QPalette lpal = liste->palette();
    lpal.setColor(QPalette::Window, Qt::white);
    liste->setPalette(lpal);
    auto *ll = new QVBoxLayout(liste);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->setSpacing(0);
    kaydir->setWidget(liste);
    sl->addWidget(kaydir, 1);
    yigin->addWidget(sayfa);

    // --- Yeni Program Ekle
    auto *ekleS = new QWidget;
    auto *el = new QGridLayout(ekleS);
    el->setContentsMargins(11, 11, 11, 11);
    auto *b1 = new QLabel("<b>Yazılım Merkezi'nden programlar ekle</b>");
    el->addWidget(b1, 0, 0, 1, 3);
    auto *r1 = new QLabel;
    r1->setPixmap(QPixmap(dlg + "appwiz-winupd_lg.png"));
    el->addWidget(r1, 1, 0, Qt::AlignTop);
    auto *a1 = new QLabel("Internet üzerinden yeni programlar, aygıt sürücüleri ve sistem güncelleştirmeleri eklemek için Yazılım Merkezi'ni tıklatın.");
    a1->setWordWrap(true);
    el->addWidget(a1, 1, 1);
    auto *yazilim = new QPushButton("&Yazılım Merkezi");
    yazilim->setMinimumWidth(110);
    el->addWidget(yazilim, 1, 2, Qt::AlignTop);
    el->setColumnStretch(1, 1);
    el->setRowStretch(2, 1);
    yigin->addWidget(ekleS);
    QObject::connect(yazilim, &QPushButton::clicked, [] { QProcess::startDetached("plasma-discover", {}); });

    QList<Satir *> satirlar;
    QList<Program> tumu;
    std::function<void()> doldur;
    doldur = [&] {
        while (QLayoutItem *i = ll->takeAt(0)) {
            if (i->widget()) i->widget()->deleteLater();
            delete i;
        }
        satirlar.clear();
        QList<Program> l = tumu;
        QCollator c(TR);
        c.setCaseSensitivity(Qt::CaseInsensitive);
        const int k = sira->currentIndex();
        std::sort(l.begin(), l.end(), [&](const Program &a, const Program &b) {
            if (k == 1) return a.bayt > b.bayt;
            if (k == 2 || k == 3) return a.sonKullanim > b.sonKullanim;
            return c.compare(a.ad, b.ad) < 0;
        });
        for (const Program &p : l) {
            auto *s = new Satir(p);
            s->secildi = [&](Satir *x) { for (Satir *y : satirlar) y->sec(y == x); };
            s->kaldir = [&](const Program &pr) {
                if (QMessageBox::question(&w, S(56, "Program Ekle veya Kaldır"),
                                          QString("%1 programını bilgisayarınızdan kaldırmak istediğinizden emin misiniz?").arg(pr.ad))
                    != QMessageBox::Yes) return;
                QStringList komut = pr.kaynak == "flatpak" ? QStringList{"flatpak", "uninstall", pr.paket}
                                                          : QStringList{"sudo", "pacman", "-Rns", pr.paket};
                QProcess::execute("konsole", QStringList{"--profile", "XP Komut İstemi", "--hold", "-e"} + komut);
                tumu = programlar();
                QTimer::singleShot(0, &w, [&] { doldur(); });
            };
            ll->addWidget(s);
            satirlar << s;
        }
        ll->addStretch();
    };
    QObject::connect(sira, &QComboBox::activated, &w, [&] { doldur(); });
    QObject::connect(degistir, &QAbstractButton::clicked, &w, [&] { yigin->setCurrentIndex(0); });
    QObject::connect(ekle, &QAbstractButton::clicked, &w, [&] { yigin->setCurrentIndex(1); });
    QObject::connect(bilesen, &QAbstractButton::clicked, &w, [] { QProcess::startDetached("plasma-discover", {"--mode", "installed"}); });

    w.resize(720, 520);
    w.show();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    tumu = programlar();
    doldur();
    QApplication::restoreOverrideCursor();
    return app.exec();
}
