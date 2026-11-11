// XPThemePlasma — XP "Görüntü Özellikleri" (masaüstüne sağ tıklama > Özellikler, Denetim Masası > Görüntü)
// Sekmeler kendi XP kopyandaki themeui.dll (Temalar, Ekran Koruyucu, Görünüm, Ayarlar) ve shell32.dll
// (Masaüstü) iletişim kutularından. Duvar kağıdı Plasma betik arabirimiyle, ekran kilidi kscreenlockerrc ile,
// çözünürlük kscreen-doctor ile, yazı tipi boyutu kdeglobals ile uygulanır.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"

static QString komutCikti(const QString &prog, const QStringList &arg, int ms = 5000) {
    QProcess p;
    p.start(prog, arg);
    p.waitForFinished(ms);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}
static QString plasmaBetik(const QString &js) {
    return komutCikti("qdbus6", {"org.kde.plasmashell", "/PlasmaShell", "evaluateScript", js});
}

// themeui bitmap 126: gümüş monitör; ekran alanı (16,17)-(167,128)
static const QRect EKRAN(16, 17, 152, 112);

class Monitor : public QWidget {
public:
    QPixmap cerceve, icerik;
    QColor renk = Qt::black;
    int konum = 2;              // 0 ortala, 1 döşe, 2 uzat
    bool ornekPencere = false;  // Temalar/Ayarlar: XP penceresi ve görev çubuğu da çizilsin
    QPixmap ornek;
    explicit Monitor(QWidget *p = nullptr) : QWidget(p) {}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        if (cerceve.isNull()) return;
        const QPoint o((width() - cerceve.width()) / 2, (height() - cerceve.height()) / 2);
        p.drawPixmap(o, cerceve);
        const QRect e = EKRAN.translated(o);
        p.setClipRect(e);
        p.fillRect(e, renk);
        if (!icerik.isNull()) {
            // XP önizlemesi ekranı 1024x768 varsayıp orantılı küçültürdü
            const double k = e.width() / double(QGuiApplication::primaryScreen()->size().width());
            const QSize boy(qMax(1, int(icerik.width() * k)), qMax(1, int(icerik.height() * k)));
            if (konum == 2) {
                p.drawPixmap(e, icerik);
            } else if (konum == 1) {
                const QPixmap kucuk = icerik.scaled(boy, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                p.drawTiledPixmap(e, kucuk);
            } else {
                const QPixmap kucuk = icerik.scaled(boy, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                p.drawPixmap(e.center() - QPoint(kucuk.width() / 2, kucuk.height() / 2), kucuk);
            }
        }
        if (ornekPencere && !ornek.isNull()) p.drawPixmap(o, ornek);
    }
};

struct Duvar { QString ad, yol; };

static QList<Duvar> duvarKagitlari() {
    QList<Duvar> l;
    QStringList dizinler = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "wallpapers", QStandardPaths::LocateDirectory);
    QSet<QString> gorulen;
    for (const QString &d : dizinler) {
        for (const QFileInfo &fi : QDir(d).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) {
            if (gorulen.contains(fi.fileName())) continue;
            gorulen.insert(fi.fileName());
            if (fi.isDir()) {
                QString ad = fi.fileName();
                QFile m(fi.filePath() + "/metadata.json");
                if (m.open(QIODevice::ReadOnly)) {
                    const QJsonObject k = QJsonDocument::fromJson(m.readAll()).object().value("KPlugin").toObject();
                    ad = k.value("Name[tr]").toString(k.value("Name").toString(ad));
                }
                QStringList resimler = QDir(fi.filePath() + "/contents/images").entryList(QDir::Files, QDir::Size);
                if (!resimler.isEmpty()) l << Duvar{ad, fi.filePath() + "/"};
            } else if (QStringList{"jpg", "jpeg", "png", "bmp", "webp"}.contains(fi.suffix().toLower())) {
                l << Duvar{fi.completeBaseName(), fi.filePath()};
            }
        }
    }
    std::sort(l.begin(), l.end(), [](const Duvar &a, const Duvar &b) { return QString::localeAwareCompare(a.ad, b.ad) < 0; });
    return l;
}

static QString resimDosyasi(const QString &yol) {
    if (!yol.endsWith('/')) return yol;
    QStringList r = QDir(yol + "contents/images").entryList(QDir::Files, QDir::Name);
    // ekran boyutuna en yakını
    const QSize ekran = QGuiApplication::primaryScreen()->size() * QGuiApplication::primaryScreen()->devicePixelRatio();
    QString en;
    qint64 fark = LLONG_MAX;
    for (const QString &f : r) {
        const QSize s(f.section('x', 0, 0).toInt(), f.section('x', 1).section('.', 0, 0).toInt());
        const qint64 d = qAbs(qint64(s.width()) * s.height() - qint64(ekran.width()) * ekran.height());
        if (d < fark) { fark = d; en = f; }
    }
    return en.isEmpty() ? QString() : yol + "contents/images/" + en;
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-gorunum");
    const xp::Modul tu = xp::Modul::yukle("themeui-dll");
    const xp::Modul sh = xp::Modul::yukle("shell32-dll");
    if (!tu.gecerli()) return QProcess::startDetached("systemsettings", {}) ? 0 : 1;
    xp::Modul dsk = xp::Modul::yukle("desk-cpl");
    // shell32'nin Masaüstü sayfası (29952) dışa aktarılmadıysa da çalışsın
    xp::Ozellikler d("Görüntü Özellikleri");
    d.setWindowIcon(QIcon(dsk.simge(100, 16)));
    const QPixmap monitor = tu.bitmap(126);
    QPixmap ornek;
    {   // bitmap 126'daki örnek masaüstünden pencere ve görev çubuğunu ayır (mavi zemin saydam)
        QImage im = monitor.toImage().convertToFormat(QImage::Format_ARGB32);
        const QRgb mavi = im.pixel(30, 30);
        for (int y = EKRAN.top(); y <= EKRAN.bottom(); ++y)
            for (int x = EKRAN.left(); x <= EKRAN.right(); ++x)
                if (im.pixel(x, y) == mavi) im.setPixel(x, y, 0);
        QImage yalniz(im.size(), QImage::Format_ARGB32);
        yalniz.fill(0);
        QPainter p(&yalniz);
        p.drawImage(EKRAN.topLeft(), im.copy(EKRAN));
        p.end();
        ornek = QPixmap::fromImage(yalniz);
    }

    // mevcut duvar kağıdı
    const QStringList mevcut = plasmaBetik(
        "var d=desktops()[0]; d.currentConfigGroup=['Wallpaper','org.kde.image','General'];"
        "var r=d.wallpaperPlugin+'|'+d.readConfig('Image')+'|'+d.readConfig('FillMode')+'|';"
        "d.currentConfigGroup=['Wallpaper',d.wallpaperPlugin,'General']; r+=d.readConfig('Color'); print(r)").split('|');
    QString seciliYol = mevcut.value(0) == "org.kde.color" ? QString() : QUrl(mevcut.value(1)).toLocalFile();
    const int plasmaKonum = mevcut.value(2).toInt();
    int konum = plasmaKonum == 3 ? 1 : plasmaKonum == 6 ? 0 : 2;
    QColor renk(mevcut.value(3).isEmpty() ? QStringLiteral("#004E98") : mevcut.value(3));
    if (!renk.isValid()) renk = QColor("#004E98");

    // ------------------------------------------------------------ Temalar
    xp::Sayfa *tm = d.ekle(tu, 1000);
    auto *temaKutu = tm->k<QComboBox>(1101);
    temaKutu->addItem("Windows XP (Değiştirildi)");
    temaKutu->addItem("Windows XP");
    temaKutu->setCurrentIndex(1);
    for (int id : {1107, 1108}) if (auto *b = tm->k<QPushButton>(id)) b->setEnabled(false);
    auto *temaOnizleme = new Monitor;
    temaOnizleme->ornekPencere = true;
    tm->yerlestir(1103, temaOnizleme);

    // ------------------------------------------------------------ Masaüstü (shell32 29952)
    xp::Sayfa *ms = nullptr;
    if (!sh.dialog(29952).isEmpty()) ms = d.ekle(sh, 29952);
    Monitor *onizleme = new Monitor;
    QListWidget *liste = nullptr;
    QComboBox *konumKutu = nullptr;
    QPushButton *renkDugme = nullptr;
    auto renkBoya = [&] {
        if (!renkDugme) return;
        QPixmap p(xp::dluX(50) - 14, xp::dluY(13) - 10);
        p.fill(renk);
        renkDugme->setIcon(QIcon(p));
        renkDugme->setIconSize(p.size());
    };
    auto onizlemeGuncelle = [&] {
        for (Monitor *m : {onizleme, temaOnizleme}) {
            m->cerceve = monitor;
            m->renk = renk;
            m->konum = konum;
            m->icerik = seciliYol.isEmpty() ? QPixmap() : QPixmap(resimDosyasi(seciliYol));
            m->update();
        }
    };
    if (ms) {
        ms->yerlestir(30053, onizleme);
        liste = new QListWidget;
        ms->yerlestir(30048, liste);
        liste->setIconSize(QSize(16, 16));
        auto *yok = new QListWidgetItem(QIcon::fromTheme("dialog-cancel"), "(Yok)");
        yok->setData(Qt::UserRole, QString());
        liste->addItem(yok);
        for (const Duvar &w : duvarKagitlari()) {
            auto *it = new QListWidgetItem(QIcon::fromTheme("image-x-generic"), w.ad);
            it->setData(Qt::UserRole, w.yol);
            liste->addItem(it);
            if (QDir::cleanPath(w.yol) == QDir::cleanPath(seciliYol)) liste->setCurrentItem(it);
        }
        if (!liste->currentItem()) {
            if (seciliYol.isEmpty()) liste->setCurrentRow(0);
            else {
                auto *it = new QListWidgetItem(QIcon::fromTheme("image-x-generic"), QFileInfo(seciliYol).completeBaseName());
                it->setData(Qt::UserRole, seciliYol);
                liste->addItem(it);
                liste->setCurrentItem(it);
            }
        }
        konumKutu = ms->k<QComboBox>(30051);
        konumKutu->addItems({"Ortala", "Döşe", "Uzat"});
        konumKutu->setCurrentIndex(konum);
        renkDugme = new QPushButton;
        ms->yerlestir(30054, renkDugme);
        renkBoya();
        QObject::connect(liste, &QListWidget::currentItemChanged, &d, [&](QListWidgetItem *it) {
            if (!it) return;
            seciliYol = it->data(Qt::UserRole).toString();
            onizlemeGuncelle();
            d.degisti();
        });
        QObject::connect(konumKutu, &QComboBox::activated, &d, [&](int i) { konum = i; onizlemeGuncelle(); d.degisti(); });
        QObject::connect(renkDugme, &QPushButton::clicked, &d, [&] {
            const QColor c = QColorDialog::getColor(renk, &d, "Renk");
            if (!c.isValid()) return;
            renk = c;
            renkBoya();
            onizlemeGuncelle();
            d.degisti();
        });
        if (auto *g = ms->k<QPushButton>(30049))
            QObject::connect(g, &QPushButton::clicked, &d, [&] {
                const QString y = QFileDialog::getOpenFileName(&d, sh.dize(9015, "Gözat"), QDir::homePath() + "/Resimler",
                                                               "Arka Plan (*.bmp *.gif *.jpg *.jpeg *.png *.webp);;Tüm Dosyalar (*)");
                if (y.isEmpty()) return;
                auto *it = new QListWidgetItem(QIcon::fromTheme("image-x-generic"), QFileInfo(y).completeBaseName());
                it->setData(Qt::UserRole, y);
                liste->addItem(it);
                liste->setCurrentItem(it);
            });
        if (auto *o = ms->k<QPushButton>(30050)) o->setEnabled(false);
    }
    onizlemeGuncelle();

    // ------------------------------------------------------------ Ekran Koruyucu
    xp::Sayfa *ek = d.ekle(tu, 1015);
    auto *ekOnizleme = new Monitor;
    ek->yerlestir(1302, ekOnizleme);
    ekOnizleme->cerceve = monitor;
    ekOnizleme->renk = Qt::black;
    // XP'nin gerçek ekran koruyucuları (kendi XP kopyandan .scr, Wine ile; xp-ekrankoruyucu zamanlar)
    const QString ekDizin = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/xpthemeplasma/xp/ekrankoruyucu/";
    const QString ekAyar = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/xpthemeplasma/ekrankoruyucu.conf";
    auto *koruyucu = ek->k<QComboBox>(1300);
    koruyucu->addItem(tu.dize(2022, "(Yok)"), QString());
    {
        QFile lf(ekDizin + "liste.json");
        QList<QPair<QString, QString>> l;
        if (lf.open(QIODevice::ReadOnly)) {
            const QJsonObject o = QJsonDocument::fromJson(lf.readAll()).object();
            for (auto it = o.begin(); it != o.end(); ++it) l << qMakePair(it.value().toString(), it.key());
        }
        QCollator c(QLocale(QLocale::Turkish));
        std::sort(l.begin(), l.end(), [&](const auto &a, const auto &b) { return c.compare(a.first, b.first) < 0; });
        for (const auto &e : l) koruyucu->addItem(e.first, e.second);
    }
    QSettings ekA(ekAyar, QSettings::IniFormat);
    koruyucu->setCurrentIndex(qMax(0, koruyucu->findData(ekA.value("dosya", koruyucu->count() > 1 ? "logon.scr" : "").toString())));
    auto *dakika = new QSpinBox;
    dakika->setRange(1, 9999);
    dakika->setValue(ekA.value("dakika", 10).toInt());
    ek->yerlestir(1306, dakika);
    dakika->resize(xp::dluX(33), 21);           // UDS_ALIGNRIGHT: düğme kutunun içinde
    auto *parola = ek->k<QCheckBox>(1320);
    parola->setText("Devamında &Hoş Geldiniz ekranı");
    parola->setChecked(ekA.value("hosgeldiniz", true).toBool());
    if (auto *l = ek->k<QLabel>(1305)) { l->setPixmap(tu.bitmap(122)); l->setAlignment(Qt::AlignCenter); }
    if (auto *b = ek->k<QPushButton>(1314))
        QObject::connect(b, &QPushButton::clicked, [] { QProcess::startDetached("kcmshell6", {"kcm_powerdevilprofilesconfig"}); });
    const QString wineYol = QDir::homePath() + "/.local/bin/xp-wine";
    if (auto *b = ek->k<QPushButton>(1303))
        QObject::connect(b, &QPushButton::clicked, &d, [&, wineYol] {
            const QString f = koruyucu->currentData().toString();
            if (!f.isEmpty()) QProcess::startDetached(wineYol, {f, "/c"});
        });
    if (auto *b = ek->k<QPushButton>(1304))
        QObject::connect(b, &QPushButton::clicked, &d, [&, wineYol] {
            const QString f = koruyucu->currentData().toString();
            if (!f.isEmpty()) QProcess::startDetached(wineYol, {f, "/s"});
        });
    // küçük monitörde önizleme: "Windows XP" logosu XP'deki gibi rastgele yerlerde belirir
    const QPixmap xpLogo(ekDizin + "logon-onizleme.png");
    QTimer ekZaman;
    auto ekCiz = [&] {
        const QString f = koruyucu->currentData().toString();
        ekOnizleme->icerik = QPixmap();
        if (f == "logon.scr" && !xpLogo.isNull()) {
            const double k = EKRAN.width() / double(QGuiApplication::primaryScreen()->size().width());
            const QSize boy(qMax(8, int(xpLogo.width() * k * 1.6)), qMax(5, int(xpLogo.height() * k * 1.6)));
            QPixmap ekran(EKRAN.size());
            ekran.fill(Qt::black);
            QPainter p(&ekran);
            const int x = QRandomGenerator::global()->bounded(qMax(1, EKRAN.width() - boy.width()));
            const int y = QRandomGenerator::global()->bounded(qMax(1, EKRAN.height() - boy.height()));
            p.drawPixmap(QRect(QPoint(x, y), boy), xpLogo);
            p.end();
            ekOnizleme->icerik = ekran;
            ekOnizleme->konum = 2;
        }
        ekOnizleme->update();
    };
    QObject::connect(&ekZaman, &QTimer::timeout, &d, ekCiz);
    ekZaman.start(3000);
    ekCiz();
    auto koruyucuDurum = [&] {
        const bool var = koruyucu->currentIndex() > 0;
        dakika->setEnabled(var);
        parola->setEnabled(var);
        for (int id : {1303, 1304}) if (auto *b = ek->k<QPushButton>(id)) b->setEnabled(var);
    };
    koruyucuDurum();
    QObject::connect(koruyucu, &QComboBox::activated, &d, [&] { koruyucuDurum(); ekCiz(); d.degisti(); });
    QObject::connect(dakika, &QSpinBox::valueChanged, &d, [&] { d.degisti(); });
    QObject::connect(parola, &QCheckBox::toggled, &d, [&] { d.degisti(); });

    // ------------------------------------------------------------ Görünüm
    xp::Sayfa *gr = d.ekle(tu, 1001);
    auto *gOnizleme = new Monitor;
    gOnizleme->ornekPencere = true;
    gr->yerlestir(1110, gOnizleme);
    gOnizleme->cerceve = monitor;
    gOnizleme->ornek = ornek;
    auto *pencereStil = gr->k<QComboBox>(1111);
    pencereStil->addItem("Windows XP stili");
    auto *renkDuzeni = gr->k<QComboBox>(1114);
    // luna.msstyles'ın üç renk düzeni; "varlik" bağlantısı etkin olanı gösterir
    const QString xpVeri = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/xpthemeplasma";
    const QStringList duzenDizin{"varlik-mavi", "varlik-zeytin", "varlik-gumus"};
    renkDuzeni->addItem("Varsayılan (mavi)");
    if (QFileInfo::exists(xpVeri + "/varlik-zeytin")) renkDuzeni->addItem("Zeytin Yeşili");
    if (QFileInfo::exists(xpVeri + "/varlik-gumus")) renkDuzeni->addItem("Gümüş");
    const int duzen = qMax(0, int(duzenDizin.indexOf(QFileInfo(xpVeri + "/varlik").symLinkTarget().section('/', -1))));
    renkDuzeni->setCurrentIndex(qMin(duzen, renkDuzeni->count() - 1));
    QObject::connect(renkDuzeni, &QComboBox::activated, &d, [&] { d.degisti(); });
    auto *boyut = gr->k<QComboBox>(1116);
    boyut->addItems({"Normal", "Büyük Yazı Tipleri", "Çok Büyük Yazı Tipleri"});
    QSettings kg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdeglobals", QSettings::IniFormat);
    const int pt = kg.value("General/font").toString().section(',', 1, 1).toInt();
    boyut->setCurrentIndex(pt >= 12 ? 2 : pt >= 10 ? 1 : 0);
    QObject::connect(boyut, &QComboBox::activated, &d, [&] { d.degisti(); });
    for (int id : {1117, 1118}) if (auto *b = gr->k<QPushButton>(id)) b->setEnabled(false);

    // ------------------------------------------------------------ Ayarlar
    xp::Sayfa *ay = d.ekle(tu, 1007);
    auto *ayOnizleme = new Monitor;
    ay->yerlestir(1812, ayOnizleme);
    ayOnizleme->cerceve = monitor;
    ayOnizleme->renk = renk;
    ayOnizleme->icerik = seciliYol.isEmpty() ? QPixmap() : QPixmap(resimDosyasi(seciliYol));
    ayOnizleme->konum = 2;
    ayOnizleme->ornekPencere = true;
    ayOnizleme->ornek = ornek;
    temaOnizleme->ornek = ornek;
    for (int id : {1801, 1819, 1800}) if (auto *w = ay->k(id)) w->hide();
    if (auto *l = ay->k<QLabel>(1820)) l->setText("Görüntü:");
    if (auto *l = ay->k<QLabel>(1811)) {
        // XP: "Plug and Play Monitor ve <ekran kartı>"
        QString kart;
        for (const QString &s2 : komutCikti("lspci", {}).split('\n'))
            if (s2.contains("VGA compatible") || s2.contains("3D controller")) {
                kart = s2.section(": ", 1);
                const QRegularExpression ko("\\[([^\\]]+)\\]");
                const auto m = ko.match(kart);
                if (m.hasMatch() && !m.captured(1).contains("Lite")) kart = kart.section(' ', 0, 0) + " " + m.captured(1);
                else if (m.hasMatch()) kart = kart.section(' ', 0, 0) + " " + m.captured(1).remove(" Lite Hash Rate");
                break;
            }
        l->setText("Plug and Play Monitor" + (kart.isEmpty() ? QString() : " ve " + kart));
        l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    auto *renkKalitesi = ay->k<QComboBox>(1807);
    renkKalitesi->addItems({"En Yüksek (32 bit)"});
    if (auto *l = ay->k<QLabel>(1813)) {
        l->setPixmap(tu.bitmap(125).scaled(l->width() - 2, l->height() - 2, Qt::IgnoreAspectRatio));
        l->setFrameShape(QFrame::Panel);
        l->setFrameShadow(QFrame::Sunken);
    }
    // kscreen-doctor: birincil çıkışın kipleri
    QString cikti, kipler;
    const QString kd = komutCikti("kscreen-doctor", {"-o"}).remove(QRegularExpression("\\x1b\\[[0-9;]*m"));
    QList<QSize> boyutlar;
    QSize simdiki;
    for (const QString &s : kd.split('\n')) {
        if (s.startsWith("Output:") && cikti.isEmpty()) cikti = s.section(' ', 2, 2);
        if (s.trimmed().startsWith("Modes:") && kipler.isEmpty()) kipler = s;
        if (s.trimmed().startsWith("Geometry:") && !simdiki.isValid()) {
            const QString g = s.section(' ', -1);
            simdiki = QSize(g.section('x', 0, 0).toInt(), g.section('x', 1).toInt());
        }
    }
    static const QRegularExpression kipRe("\\d+:(\\d+)x(\\d+)@");
    for (auto it = kipRe.globalMatch(kipler); it.hasNext();) {
        const auto m = it.next();
        const QSize s(m.captured(1).toInt(), m.captured(2).toInt());
        if (!boyutlar.contains(s) && s.width() >= 800) boyutlar << s;
    }
    std::sort(boyutlar.begin(), boyutlar.end(), [](const QSize &a, const QSize &b) {
        return qint64(a.width()) * a.height() < qint64(b.width()) * b.height();
    });
    auto *kaydirici = ay->k<QSlider>(1808);
    auto *cozunurluk = ay->k<QLabel>(1814);
    kaydirici->setRange(0, qMax(0, int(boyutlar.size()) - 1));
    kaydirici->setPageStep(1);
    kaydirici->setTickPosition(QSlider::TicksBelow);
    kaydirici->setValue(qMax(0, int(boyutlar.indexOf(simdiki))));
    auto cozGoster = [&] {
        const QSize s = boyutlar.value(kaydirici->value(), simdiki);
        cozunurluk->setText(QString("%1 x %2 piksel").arg(s.width()).arg(s.height()));
    };
    cozGoster();
    QObject::connect(kaydirici, &QSlider::valueChanged, &d, [&] { cozGoster(); d.degisti(); });
    if (auto *b = ay->k<QPushButton>(1821)) b->setEnabled(false);
    if (auto *b = ay->k<QPushButton>(1802))
        QObject::connect(b, &QPushButton::clicked, [] { QProcess::startDetached("kcmshell6", {"kcm_kscreen"}); });

    // ------------------------------------------------------------ Uygula
    d.uygulaIslevi = [&] {
        // duvar kağıdı
        if (ms) {
            const int fm = konum == 0 ? 6 : konum == 1 ? 3 : 0;
            const QString c = renk.name();
            QString js;
            if (seciliYol.isEmpty()) {
                js = QString("desktops().forEach(function(d){d.wallpaperPlugin='org.kde.color';"
                             "d.currentConfigGroup=['Wallpaper','org.kde.color','General'];d.writeConfig('Color','%1');});").arg(c);
            } else {
                const QString url = QUrl::fromLocalFile(seciliYol).toString() + (seciliYol.endsWith('/') ? "/" : "");
                js = QString("desktops().forEach(function(d){d.wallpaperPlugin='org.kde.image';"
                             "d.currentConfigGroup=['Wallpaper','org.kde.image','General'];"
                             "d.writeConfig('Image','%1');d.writeConfig('FillMode',%2);d.writeConfig('Color','%3');});")
                         .arg(QString(url).replace("'", "\\'")).arg(fm).arg(c);
            }
            plasmaBetik(js);
        }
        // ekran koruyucu: xp-ekrankoruyucu ayarı; Plasma'nın kendi boşta kilitlemesi XP oturumunda kapalı
        {
            QSettings a(ekAyar, QSettings::IniFormat);
            a.setValue("dosya", koruyucu->currentData().toString());
            a.setValue("dakika", dakika->value());
            a.setValue("hosgeldiniz", parola->isChecked());
            a.sync();
            QProcess::execute("kwriteconfig6", {"--file", "kscreenlockerrc", "--group", "Daemon", "--key", "Autolock", "false"});
            QProcess::execute("qdbus6", {"org.freedesktop.ScreenSaver", "/ScreenSaver", "org.kde.screensaver.configure"});
        }
        // yazı tipi boyutu (Tahoma 8 / 10 / 12)
        const int yeniPt = boyut->currentIndex() == 2 ? 12 : boyut->currentIndex() == 1 ? 10 : 8;
        if (yeniPt != (pt ? pt : 8)) {
            const QString f = QString("Tahoma,%1,-1,5,400,0,0,0,0,0,0,0,0,0,0,1").arg(yeniPt);
            for (const QString &k : {"font", "menuFont", "toolBarFont"})
                QProcess::execute("kwriteconfig6", {"--file", "kdeglobals", "--group", "General", "--key", k, f});
            QProcess::execute("dbus-send", {"--session", "--type=signal", "/KGlobalSettings", "org.kde.KGlobalSettings.notifyChange", "int32:1", "int32:0"});
        }
        // XP büyük yazı tiplerinde başlık çubuğu yazısı da büyür (Trebuchet MS 10 / 14 / 17)
        if (yeniPt != (pt ? pt : 8)) {
            const int baslik = yeniPt == 12 ? 17 : yeniPt == 10 ? 14 : 10;
            QProcess::execute("kwriteconfig6", {"--file", "kdeglobals", "--group", "WM", "--key", "activeFont",
                                                QString("Trebuchet MS,%1,-1,5,700,0,0,0,0,0,0,0,0,0,0,1").arg(baslik)});
            QProcess::execute("qdbus6", {"org.kde.KWin", "/KWin", "reconfigure"});
        }
        // renk düzeni: pencere çerçevesi, Kvantum, Plasma teması, renk şeması, Başlat/görev çubuğu görselleri
        const int yeniDuzen = renkDuzeni->currentText() == "Zeytin Yeşili" ? 1 : renkDuzeni->currentText() == "Gümüş" ? 2 : 0;
        const int eskiDuzen = qMax(0, int(duzenDizin.indexOf(QFileInfo(xpVeri + "/varlik").symLinkTarget().section('/', -1))));
        if (yeniDuzen != eskiDuzen) {
            QProgressDialog bekle("Lütfen bekleyin...", QString(), 0, 0, &d);
            bekle.setWindowTitle("Görüntü Özellikleri");
            bekle.setMinimumDuration(0);
            bekle.show();
            QApplication::processEvents();
            const QString ek = QStringList{"", "Zeytin", "Gumus"}.at(yeniDuzen);
            QFile::remove(xpVeri + "/varlik");
            QFile::link(duzenDizin.at(yeniDuzen), xpVeri + "/varlik");
            QProcess::execute("plasma-apply-colorscheme", {"XPLuna" + ek});
            QProcess::execute("plasma-apply-desktoptheme", {ek.isEmpty() ? QString("XP-Luna") : "XP-Luna-" + ek});
            QProcess::execute("kwriteconfig6", {"--file", "kwinrc", "--group", "org.kde.kdecoration2", "--key", "theme",
                                                ek.isEmpty() ? QString("__aurorae__svg__XP-Luna") : "__aurorae__svg__XP-Luna-" + ek});
            QProcess::execute("kwriteconfig6", {"--file", "Kvantum/kvantum.kvconfig", "--group", "General", "--key", "theme", "XPLuna" + ek});
            QProcess::execute("qdbus6", {"org.kde.KWin", "/KWin", "reconfigure"});
            const QString wine = QDir::homePath() + "/.local/bin/xp-wine";
            if (QFileInfo(wine).isExecutable())
                QProcess::startDetached(wine, {"--renk", QStringList{"NormalColor", "HomeStead", "Metallic"}.at(yeniDuzen)});
            // görev çubuğu ve Başlat menüsü yeni görselleri yüklesin
            QProcess::startDetached("systemctl", {"--user", "restart", "plasma-plasmashell"});
        }
        // çözünürlük
        const QSize s = boyutlar.value(kaydirici->value());
        if (s.isValid() && s != simdiki && !cikti.isEmpty()) {
            QProcess::execute("kscreen-doctor", {QString("output.%1.mode.%2x%3").arg(cikti).arg(s.width()).arg(s.height())});
            if (QMessageBox::question(&d, "Monitör Ayarları", "Masaüstünüz yeniden yapılandırıldı. Bu ayarları korumak istiyor musunuz?",
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
                QProcess::execute("kscreen-doctor", {QString("output.%1.mode.%2x%3").arg(cikti).arg(simdiki.width()).arg(simdiki.height())});
                kaydirici->setValue(qMax(0, int(boyutlar.indexOf(simdiki))));
            } else {
                simdiki = s;
            }
        }
        temaKutu->setCurrentIndex(1);
        return true;
    };

    const QStringList a = app.arguments();
    for (const QString &arg : a.mid(1)) {
        bool sayi = false;
        const int n = arg.section(',', -1).toInt(&sayi);
        if (sayi && n >= 0 && n < d.sekmeler->count()) d.sekmeler->setCurrentIndex(n);
    }
    // XP: Görüntü Özellikleri "Masaüstü" yerine son açılan sekmeyle açılırdı; varsayılan Temalar
    d.show();
    return d.exec() == QDialog::Accepted ? 0 : 1;
}
