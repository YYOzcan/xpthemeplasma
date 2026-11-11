// XPThemePlasma — XP "Tarih ve Saat Özellikleri" (görev çubuğu saatine çift tıklama, Denetim Masası)
// Sayfalar kendi XP kopyandaki timedate.cpl'den: Tarih ve Saat (takvim + analog saat), Saat Dilimi (dünya
// haritası), Internet Saati. Değişiklikler timedatectl ile uygulanır (yetki gerekirse polkit sorar).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"

static const QLocale TR(QLocale::Turkish, QLocale::Turkey);

// ---------------------------------------------------------------- gün ızgarası (CalWndMain)
class Takvim : public QWidget {
public:
    QDate tarih = QDate::currentDate();
    std::function<void()> degisti;
    explicit Takvim(QWidget *p = nullptr) : QWidget(p) { setFocusPolicy(Qt::StrongFocus); }
    void ayarla(const QDate &d) { tarih = d; update(); }
protected:
    QRect hucre(int sutun, int satir) const {
        const int w = (width() - 2) / 7, h = (height() - 2) / 7;
        return QRect(1 + sutun * w, 1 + satir * h, w, h);
    }
    int ilkSutun() const {                 // ayın 1'i hangi sütunda (Pazartesi ilk)
        return QDate(tarih.year(), tarih.month(), 1).dayOfWeek() - 1;
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::white);
        qDrawShadePanel(&p, rect(), palette(), true, 1);        // çökük kenar
        p.setFont(font());
        // gün adları başlığı: etkin başlık rengi üzerinde beyaz harfler
        const QColor baslik = palette().color(QPalette::Highlight);
        p.fillRect(QRect(1, 1, width() - 2, hucre(0, 0).height()), baslik.darker(110));
        p.setPen(Qt::white);
        for (int i = 0; i < 7; ++i) {
            const QString ad = TR.dayName(i + 1, QLocale::ShortFormat).left(1).toUpper();
            p.drawText(hucre(i, 0), Qt::AlignCenter, ad);
        }
        const int ilk = ilkSutun();
        for (int g = 1; g <= tarih.daysInMonth(); ++g) {
            const int n = ilk + g - 1;
            const QRect r = hucre(n % 7, 1 + n / 7);
            if (g == tarih.day()) {
                p.fillRect(r.adjusted(1, 0, -1, 0), palette().color(QPalette::Highlight));
                p.setPen(palette().color(QPalette::HighlightedText));
            } else {
                p.setPen(Qt::black);
            }
            p.drawText(r, Qt::AlignCenter, QString::number(g));
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        setFocus();
        const int ilk = ilkSutun();
        for (int g = 1; g <= tarih.daysInMonth(); ++g) {
            const int n = ilk + g - 1;
            if (hucre(n % 7, 1 + n / 7).contains(e->position().toPoint())) {
                tarih = QDate(tarih.year(), tarih.month(), g);
                update();
                if (degisti) degisti();
            }
        }
    }
    void keyPressEvent(QKeyEvent *e) override {
        int d = 0;
        if (e->key() == Qt::Key_Left) d = -1;
        else if (e->key() == Qt::Key_Right) d = 1;
        else if (e->key() == Qt::Key_Up) d = -7;
        else if (e->key() == Qt::Key_Down) d = 7;
        if (!d) return QWidget::keyPressEvent(e);
        const QDate y = tarih.addDays(d);
        if (y.month() != tarih.month()) return;
        tarih = y;
        update();
        if (degisti) degisti();
    }
};

// ---------------------------------------------------------------- analog saat (ClockWndMain)
class Saat : public QWidget {
public:
    QTime zaman = QTime::currentTime();
    explicit Saat(QWidget *p = nullptr) : QWidget(p) {}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QPointF m(width() / 2.0, height() / 2.0);
        const double r = qMin(width(), height()) / 2.0 - 4;
        const QColor yuz = palette().color(QPalette::Button), acik = palette().color(QPalette::Light),
                     koyu = palette().color(QPalette::Dark), cokKoyu = palette().color(QPalette::Shadow);
        // dakika noktaları ve 3B saat kareleri (XP/2000 timedate saati)
        for (int i = 0; i < 60; ++i) {
            const double a = i * M_PI / 30.0;
            const QPointF q(m.x() + std::sin(a) * r, m.y() - std::cos(a) * r);
            if (i % 5 == 0) {
                const double s = (i % 15 == 0) ? 3.5 : 2.5;
                QRectF kare(q.x() - s, q.y() - s, 2 * s, 2 * s);
                p.setRenderHint(QPainter::Antialiasing, false);
                p.fillRect(kare, yuz);
                p.setPen(acik);
                p.drawLine(kare.topLeft(), kare.topRight());
                p.drawLine(kare.topLeft(), kare.bottomLeft());
                p.setPen(cokKoyu);
                p.drawLine(kare.bottomLeft(), kare.bottomRight());
                p.drawLine(kare.topRight(), kare.bottomRight());
                p.setRenderHint(QPainter::Antialiasing, true);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(koyu);
                p.drawEllipse(q, 0.9, 0.9);
            }
        }
        auto el = [&](double aci, double uzun, double gen, const QColor &dolgu) {
            const double a = aci * M_PI / 180.0;
            const QPointF uc(m.x() + std::sin(a) * uzun, m.y() - std::cos(a) * uzun);
            const QPointF kuyruk(m.x() - std::sin(a) * gen * 1.5, m.y() + std::cos(a) * gen * 1.5);
            const QPointF sol(m.x() + std::cos(a) * gen, m.y() + std::sin(a) * gen);
            const QPointF sag(m.x() - std::cos(a) * gen, m.y() - std::sin(a) * gen);
            QPolygonF yarim1{kuyruk, sol, uc}, yarim2{kuyruk, sag, uc};
            p.setPen(QPen(cokKoyu, 1));
            p.setBrush(dolgu);
            p.drawPolygon(QPolygonF{kuyruk, sol, uc, sag});
            p.setPen(Qt::NoPen);
            p.setBrush(acik);
            p.drawPolygon(yarim1);           // ışık alan yarı
        };
        const double sa = (zaman.hour() % 12) * 30 + zaman.minute() * 0.5;
        const double dk = zaman.minute() * 6 + zaman.second() * 0.1;
        el(sa, r * 0.55, 4.5, yuz);
        el(dk, r * 0.82, 3.5, yuz);
        // saniye ibresi
        const double sn = zaman.second() * 6 * M_PI / 180.0;
        p.setPen(QPen(cokKoyu, 1));
        p.drawLine(m, QPointF(m.x() + std::sin(sn) * r * 0.85, m.y() - std::cos(sn) * r * 0.85));
    }
};

// ---------------------------------------------------------------- dünya haritası (CplWorldMapClass)
class Harita : public QWidget {
public:
    double fark = 0;           // seçili dilimin standart saat farkı
    explicit Harita(QWidget *p = nullptr) : QWidget(p) {
        kaynak = QImage(xp::dlgDizini() + "timedate-harita.png").convertToFormat(QImage::Format_ARGB32);
    }
    void sec(double f) { fark = f; cizim = QImage(); update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(0, 0, 128));
        if (kaynak.isNull()) return;
        if (cizim.isNull()) {
            cizim = QImage(kaynak.size(), QImage::Format_RGB32);
            const int kod = qRound((fark + 14) * 4);
            for (int y = 0; y < kaynak.height(); ++y) {
                const QRgb *s = reinterpret_cast<const QRgb *>(kaynak.constScanLine(y));
                QRgb *d = reinterpret_cast<QRgb *>(cizim.scanLine(y));
                for (int x = 0; x < kaynak.width(); ++x) {
                    const bool kara = qGreen(s[x]) > 0;
                    const bool secili = qRed(s[x]) == kod;
                    d[x] = kara ? (secili ? qRgb(0, 255, 0) : qRgb(0, 128, 0))
                                : (secili ? qRgb(0, 96, 255) : qRgb(0, 0, 128));
                }
            }
        }
        const QPoint o((width() - cizim.width()) / 2, (height() - cizim.height()) / 2);
        p.drawImage(o, cizim);
        qDrawShadePanel(&p, rect(), palette(), true, 1);
    }
private:
    QImage kaynak, cizim;
};

static QString dilimEtiketi(const QByteArray &id) {
    const QTimeZone tz(id);
    const int sn = tz.standardTimeOffset(QDateTime::currentDateTimeUtc());
    const int sa = std::abs(sn) / 3600, dk = (std::abs(sn) % 3600) / 60;
    QString sehir = QString::fromLatin1(id).section('/', -1).replace('_', ' ');
    QString bolge = QString::fromLatin1(id).section('/', 0, 0);
    return QString("(GMT%1%2:%3) %4%5").arg(sn < 0 ? "-" : "+").arg(sa, 2, 10, QChar('0')).arg(dk, 2, 10, QChar('0'))
        .arg(sehir, bolge != sehir ? " (" + bolge + ")" : QString());
}

static QString calistirCikti(const QString &prog, const QStringList &arg) {
    QProcess p;
    p.start(prog, arg);
    p.waitForFinished(5000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-tarihsaat");
    const xp::Modul td = xp::Modul::yukle("timedate-cpl");
    if (!td.gecerli()) return QProcess::startDetached("kcmshell6", {"kcm_clock"}) ? 0 : 1;

    xp::Ozellikler d(td.dize(304, "Tarih ve Saat Özellikleri"));
    d.setWindowIcon(QIcon(td.simge(200, 16)));

    // ------------------------------------------------------------ Tarih ve Saat (24 saat biçimli şablon: 10)
    xp::Sayfa *s = d.ekle(td, 10);
    auto *ay = s->k<QComboBox>(713);
    for (int m = 1; m <= 12; ++m) ay->addItem(TR.standaloneMonthName(m));
    // Yıl: düzenleme kutusu + yukarı/aşağı düğmesi → tek döndürme kutusu
    auto *yil = new QSpinBox;
    yil->setRange(1980, 2099);
    s->yerlestir(706, yil);
    yil->resize(xp::dluX(50), yil->height());
    auto *takvim = new Takvim;
    s->yerlestir(711, takvim);
    auto *saat = new Saat;
    s->yerlestir(712, saat);
    for (int id : {701, 702, 703, 707, 708, 709, 710}) if (auto *w = s->k(id)) w->hide();
    auto *saatKutu = new QTimeEdit;
    saatKutu->setDisplayFormat("HH:mm:ss");
    s->yerlestir(715, saatKutu);
    saatKutu->setGeometry(saatKutu->x(), saatKutu->y() - 2, xp::dluX(79), 21);
    saatKutu->show();
    saatKutu->setEnabled(true);

    bool elleDegisti = false;
    QDateTime simdi = QDateTime::currentDateTime();
    auto goster = [&] {
        const QSignalBlocker b1(ay), b2(yil), b3(saatKutu);
        ay->setCurrentIndex(takvim->tarih.month() - 1);
        yil->setValue(takvim->tarih.year());
        if (!elleDegisti) saatKutu->setTime(QTime::currentTime());
        saat->zaman = elleDegisti ? saatKutu->time() : QTime::currentTime();
        saat->update();
        takvim->update();
    };
    takvim->tarih = simdi.date();
    goster();
    auto tarihDegisti = [&] { elleDegisti = true; d.degisti(); goster(); };
    takvim->degisti = tarihDegisti;
    QObject::connect(ay, &QComboBox::activated, &d, [&](int i) {
        const QDate t = takvim->tarih;
        takvim->ayarla(QDate(t.year(), i + 1, qMin(t.day(), QDate(t.year(), i + 1, 1).daysInMonth())));
        tarihDegisti();
    });
    QObject::connect(yil, &QSpinBox::valueChanged, &d, [&](int y) {
        const QDate t = takvim->tarih;
        takvim->ayarla(QDate(y, t.month(), qMin(t.day(), QDate(y, t.month(), 1).daysInMonth())));
        tarihDegisti();
    });
    QObject::connect(saatKutu, &QTimeEdit::timeChanged, &d, [&](const QTime &) { elleDegisti = true; d.degisti(); goster(); });
    QTimer tik;
    QObject::connect(&tik, &QTimer::timeout, &d, [&] {
        if (elleDegisti) {                  // XP: elle değiştirilen saat de işlemeye devam ederdi
            const QSignalBlocker b(saatKutu);
            saatKutu->setTime(saatKutu->time().addSecs(1));
        }
        goster();
    });
    tik.start(1000);

    auto *dilimYazi = s->k<QLabel>(716);
    auto dilimGoster = [&](const QByteArray &id) {
        const QTimeZone tz(id);
        QString ad = tz.displayName(QDateTime::currentDateTime(), QTimeZone::LongName, TR);
        if (ad.isEmpty()) ad = QString::fromLatin1(id);
        dilimYazi->setText(QString("Geçerli saat dilimi:  %1").arg(ad));
    };
    const QByteArray mevcutDilim = QTimeZone::systemTimeZoneId();
    dilimGoster(mevcutDilim);

    // ------------------------------------------------------------ Saat Dilimi
    xp::Sayfa *sd = d.ekle(td, 2);
    auto *dilimler = sd->k<QComboBox>(101);
    QList<QByteArray> kimlikler;
    for (const QByteArray &id : QTimeZone::availableTimeZoneIds())
        if (id.contains('/') && !id.startsWith("Etc/") && !id.startsWith("SystemV/") && !id.startsWith("US/")) kimlikler << id;
    const QDateTime su = QDateTime::currentDateTimeUtc();
    std::sort(kimlikler.begin(), kimlikler.end(), [&](const QByteArray &a, const QByteArray &b) {
        const int fa = QTimeZone(a).standardTimeOffset(su), fb = QTimeZone(b).standardTimeOffset(su);
        return fa != fb ? fa < fb : dilimEtiketi(a) < dilimEtiketi(b);
    });
    for (const QByteArray &id : kimlikler) dilimler->addItem(dilimEtiketi(id), id);
    dilimler->setMaxVisibleItems(20);
    auto *harita = new Harita;
    sd->yerlestir(102, harita);
    auto dilimSec = [&](int i) {
        const QByteArray id = dilimler->itemData(i).toByteArray();
        harita->sec(QTimeZone(id).standardTimeOffset(su) / 3600.0);
    };
    dilimler->setCurrentIndex(qMax(0, kimlikler.indexOf(mevcutDilim)));
    dilimSec(dilimler->currentIndex());
    QObject::connect(dilimler, &QComboBox::activated, &d, [&](int i) { dilimSec(i); d.degisti(); });
    if (auto *yaz = sd->k<QCheckBox>(100)) { yaz->setChecked(true); yaz->setEnabled(false); }   // tzdata kendiliğinden yapar

    // ------------------------------------------------------------ Internet Saati
    xp::Sayfa *is = d.ekle(td, 3);
    auto *esitle = is->k<QCheckBox>(720);
    auto *sunucu = is->k<QComboBox>(721);
    auto *simdiGuncelle = is->k<QPushButton>(726);
    auto *durum = is->k<QLabel>(725);
    auto *sonraki = is->k<QLabel>(723);
    auto ntpOku = [&] {
        const QString ozellik = calistirCikti("timedatectl", {"show"});
        const QString es = calistirCikti("timedatectl", {"show-timesync"});
        auto deger = [](const QString &metin, const QString &anahtar) {
            for (const QString &l : metin.split('\n')) if (l.startsWith(anahtar + "=")) return l.mid(anahtar.size() + 1);
            return QString();
        };
        const QSignalBlocker b(esitle);
        esitle->setChecked(deger(ozellik, "NTP") == "yes");
        const QString ad = deger(es, "ServerName");
        sunucu->clear();
        QStringList adlar{ad};
        adlar << deger(es, "FallbackNTPServers").split(' ', Qt::SkipEmptyParts);
        adlar.removeAll(QString());
        adlar.removeDuplicates();
        sunucu->addItems(adlar);
        sunucu->setEnabled(false);       // sunucu sistem yapılandırmasıdır (timesyncd.conf)
        // son eşitleme: NTPMessage içindeki ReceiveTimestamp; sonraki = son + PollIntervalUSec
        QDateTime t = QDateTime::currentDateTime();
        static const QRegularExpression alinan("ReceiveTimestamp=\\w+ (\\d{4}-\\d\\d-\\d\\d \\d\\d:\\d\\d:\\d\\d)");
        const auto ma = alinan.match(deger(es, "NTPMessage"));
        if (ma.hasMatch()) t = QDateTime::fromString(ma.captured(1), "yyyy-MM-dd HH:mm:ss");
        qint64 aralikSn = 0;
        static const QRegularExpression parca("(\\d+)\\s*(h|min|s|ms|us)\\b");
        for (auto it = parca.globalMatch(deger(es, "PollIntervalUSec")); it.hasNext();) {
            const auto m = it.next();
            const qint64 v = m.captured(1).toLongLong();
            const QString b = m.captured(2);
            aralikSn += b == "h" ? v * 3600 : b == "min" ? v * 60 : b == "s" ? v : 0;
        }
        if (deger(ozellik, "NTPSynchronized") == "yes" && !ad.isEmpty()) {
            QString m = td.dize(502, "Saat başarıyla şuradan eşitlendi: %3!s! Tarih: %1!s! Saat: %2!s!.");
            m.replace("%1!s!", TR.toString(t.date(), QLocale::ShortFormat))
             .replace("%2!s!", TR.toString(t.time(), "HH:mm"))
             .replace("%3!s!", ad);
            durum->setText(m);
        } else {
            durum->setText(td.dize(510, "Windows şimdiye dek bir Internet saat sunucusuyla eşitlemeyi denemedi."));
        }
        QString n = td.dize(501, "Sonraki eşitleme: %1!s! %2!s!");
        const QDateTime sonra = t.addSecs(aralikSn);
        sonraki->setText(!esitle->isChecked() || aralikSn == 0 ? QString()
                         : n.replace("%1!s!", TR.toString(sonra.date(), QLocale::ShortFormat))
                               .replace("%2!s!", TR.toString(sonra.time(), "HH:mm")));
        simdiGuncelle->setEnabled(esitle->isChecked());
    };
    ntpOku();
    QObject::connect(esitle, &QCheckBox::toggled, &d, [&] { d.degisti(); simdiGuncelle->setEnabled(esitle->isChecked()); });
    QObject::connect(simdiGuncelle, &QPushButton::clicked, &d, [&] {
        QString m = td.dize(500, "Windows %1!s! ile eşitleme yaparken bekleyin");
        durum->setText(m.replace("%1!s!", sunucu->currentText()) + "...");
        QProcess::execute("timedatectl", {"set-ntp", "false"});
        QProcess::execute("timedatectl", {"set-ntp", "true"});
        QTimer::singleShot(3000, &d, ntpOku);
    });

    d.uygulaIslevi = [&] {
        const QByteArray secili = dilimler->currentData().toByteArray();
        if (!secili.isEmpty() && secili != QTimeZone::systemTimeZoneId())
            QProcess::execute("timedatectl", {"set-timezone", QString::fromLatin1(secili)});
        if (elleDegisti) {
            if (esitle->isChecked()) QProcess::execute("timedatectl", {"set-ntp", "false"});
            const QDateTime yeni(takvim->tarih, saatKutu->time());
            if (QProcess::execute("timedatectl", {"set-time", yeni.toString("yyyy-MM-dd HH:mm:ss")}) != 0) {
                QMessageBox::warning(&d, d.windowTitle(), td.dize(305, "Sistem Saatini değiştirmek için gereken yetkiye sahip değilsiniz."));
                return false;
            }
            elleDegisti = false;
        } else {
            QProcess::execute("timedatectl", {"set-ntp", esitle->isChecked() ? "true" : "false"});
        }
        dilimGoster(QTimeZone::systemTimeZoneId());
        ntpOku();
        return true;
    };

    const QStringList a = app.arguments();
    for (const QString &arg : a.mid(1)) {
        bool sayi = false;
        const int n = arg.section(',', -1).toInt(&sayi);
        if (sayi && n >= 0 && n < d.sekmeler->count()) d.sekmeler->setCurrentIndex(n);
    }
    d.show();
    return d.exec() == QDialog::Accepted ? 0 : 1;
}
