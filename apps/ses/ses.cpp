// XPThemePlasma — XP "Ses ve Ses Aygıtları Özellikleri" (mmsys.cpl; Denetim Masası > Sesler ve Ses Aygıtları,
// hoparlöre sağ tıklama > Ses Özelliklerini Ayarla). Sekmeler kendi XP kopyandaki mmsys.cpl'den.
//   Ses Ayarı : varsayılan çıkışın ses düzeyi, Sessiz, görev çubuğu simgesi        (pactl)
//   Sesler    : ses düzeni (Windows Varsayılanı / Ses Yok), olay başına XP sesi    (Windows XP ses teması)
//   Ses       : varsayılan çalma ve kayıt aygıtı                                     (pactl set-default-*)
//   Donanım   : ses kartları                                                         (pactl list cards)
// Kullanım: xp-ses [sekme numarası]
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"
#include "../ortak/xpses.h"

static xp::Modul MM;
static QString S(int id, const char *y) { return MM.dize(id, QString::fromUtf8(y)); }

// freedesktop ses adı -> XP olay adı (XP ses teması gen_luna_ses.py ile aynı eşleme)
static const QList<QPair<QString, QString>> OLAYLAR = {
    {"Varsayılan Bip", "bell"},
    {"Kritik Durma", "dialog-error"},
    {"Ünlem", "dialog-warning"},
    {"Yıldız", "dialog-information"},
    {"Soru", "dialog-question"},
    {"Windows'u Başlat", "desktop-login"},
    {"Windows'tan Çık", "desktop-logout"},
    {"Windows Oturum Açma", "service-login"},
    {"Windows Oturum Kapatma", "service-logout"},
    {"Aygıt Bağlantısı", "device-added"},
    {"Aygıt Bağlantısı Kesilmesi", "device-removed"},
    {"Aygıt Bağlantısı Başarısız", "device-added-failure"},
    {"Düşük Pil Alarmı", "battery-low"},
    {"Kritik Pil Alarmı", "battery-caution"},
    {"Geri Dönüşüm Kutusunu Boşalt", "trash-empty"},
    {"Yazdırma Tamamlandı", "complete-print"},
    {"Sistem Bildirimi", "message"},
    {"Yeni Posta Bildirimi", "message-new-email"},
    {"Simge Durumuna Küçült", "window-minimized"},
    {"Önceki Boyut", "window-unminimized"},
    {"Gezintiyi Başlat", "menu-click"},
};

static QString temaDizini() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/sounds/Windows XP/stereo/";
}
static QString xpSesDizini() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/xpthemeplasma/xp/ses/";
}
static QString okuAyar(const QString &dosya, const QString &grup, const QString &anahtar) {
    QProcess p;
    p.start("kreadconfig6", {"--file", dosya, "--group", grup, "--key", anahtar});
    p.waitForFinished(2000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}
static QByteArray sessizWav() {      // 0,1 sn sessizlik: "(Yok)" seçilen olay için
    QByteArray d;
    QDataStream s(&d, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    const quint32 veri = 4410 * 2;
    s.writeRawData("RIFF", 4); s << quint32(36 + veri); s.writeRawData("WAVEfmt ", 8);
    s << quint32(16) << quint16(1) << quint16(1) << quint32(44100) << quint32(88200) << quint16(2) << quint16(16);
    s.writeRawData("data", 4); s << veri;
    d.append(QByteArray(veri, 0));
    return d;
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-ses");
    MM = xp::Modul::yukle("mmsys-cpl");
    if (!MM.gecerli()) return QProcess::startDetached("kcmshell6", {"kcm_pulseaudio"}) ? 0 : 1;
    const QString bin = QDir::homePath() + "/.local/bin/";

    xp::Ozellikler d("Ses ve Ses Aygıtları Özellikleri");
    d.setWindowIcon(QIcon(MM.simge(4379, 16)));

    // ------------------------------------------------------------ Ses Ayarı
    xp::Sayfa *sa = d.ekle(MM, 4905);
    xp::ses::Aygit cikis = xp::ses::varsayilanAygit(true);
    // üstteki aygıt adı (SysLink 6696) ve simgesi (3004)
    if (auto *l = sa->k<QLabel>(6696)) {
        l->setObjectName("aygitAdi");
        l->setTextFormat(Qt::PlainText);
        l->setText(cikis.aciklama.isEmpty() ? QStringLiteral("Ses Aygıtı Yok") : cikis.aciklama);
    }
    if (auto *l = sa->k<QLabel>(5406)) l->setPixmap(MM.simge(3004));
    if (auto *b = sa->k<QPushButton>(6704))
        QObject::connect(b, &QPushButton::clicked, &d, [bin] { QProcess::startDetached(bin + "xp-sesdenetimi", {}); });
    if (auto *b = sa->k<QPushButton>(6670))
        QObject::connect(b, &QPushButton::clicked, [] { QProcess::startDetached("kcmshell6", {"kcm_pulseaudio"}); });
    if (auto *ik = sa->k<QLabel>(5404)) ik->setPixmap(MM.simge(4379));
    auto *duzey = sa->k<QSlider>(6671);
    auto *sessiz = sa->k<QCheckBox>(6697);
    auto *tepsi = sa->k<QCheckBox>(6666);
    duzey->setRange(0, 100);
    duzey->setPageStep(10);
    duzey->setTickInterval(10);
    duzey->setTickPosition(QSlider::TicksBelow);
    bool guncelleniyor = false;
    auto sesGoster = [&] {
        cikis = xp::ses::varsayilanAygit(true);
        if (duzey->isSliderDown()) return;
        guncelleniyor = true;
        duzey->setValue(cikis.yuzde());
        sessiz->setChecked(cikis.sessiz);
        duzey->setEnabled(!cikis.ad.isEmpty());
        sessiz->setEnabled(!cikis.ad.isEmpty());
        guncelleniyor = false;
    };
    sesGoster();
    QObject::connect(duzey, &QSlider::valueChanged, &d, [&](int v) {
        if (!guncelleniyor) xp::ses::sesAyarla("sink", cikis.ad, v, cikis.denge(), cikis.kanal);
    });
    QObject::connect(duzey, &QSlider::sliderReleased, &d, [] { xp::ses::bip(); });
    QObject::connect(sessiz, &QCheckBox::toggled, &d, [&](bool s) { if (!guncelleniyor) xp::ses::sessizAyarla("sink", cikis.ad, s); });
    // görev çubuğundaki hoparlör: XP tepsisindeki xpses görünür/gizli
    {
        QProcess p;
        p.start("qdbus6", {"org.kde.plasmashell", "/PlasmaShell", "evaluateScript",
                           "var g=true;panels().forEach(function(p){p.widgets('org.kde.plasma.systemtray').forEach(function(t){"
                           "t.currentConfigGroup=['General'];var h=t.readConfig('hiddenItems');h=(typeof h==='string')?h.split(','):h;"
                           "if(h.indexOf('io.github.yyozcan.xpses')>=0)g=false;});});print(g)"});
        p.waitForFinished(3000);
        tepsi->setChecked(QString::fromUtf8(p.readAllStandardOutput()).trimmed() != "false");
    }
    QObject::connect(tepsi, &QCheckBox::toggled, &d, &xp::Ozellikler::degisti);
    if (auto *b = sa->k<QPushButton>(6667))
        QObject::connect(b, &QPushButton::clicked, &d, [bin] { QProcess::startDetached(bin + "xp-sesdenetimi", {}); });
    xp::ses::Izleyici izle;
    izle.degisti = [&] { sesGoster(); };

    // ------------------------------------------------------------ Sesler
    xp::Sayfa *sl = d.ekle(MM, 4876);
    sl->setProperty("baslik", "Sesler");
    auto *duzen = sl->k<QComboBox>(8966);
    duzen->addItem("Ses Yok");
    duzen->addItem("Windows Varsayılanı");
    const bool seslerAcik = okuAyar("kdeglobals", "Sounds", "Enable") != "false";
    duzen->setCurrentIndex(seslerAcik ? 1 : 0);
    for (int id : {8965, 8964}) if (auto *b = sl->k<QPushButton>(id)) b->setEnabled(false);
    auto *olaylar = sl->k<QTreeWidget>(6682);
    olaylar->setHeaderHidden(true);
    olaylar->setRootIsDecorated(true);
    auto *kok = new QTreeWidgetItem({"Windows"});
    kok->setIcon(0, QIcon::fromTheme("computer"));
    for (const auto &o : OLAYLAR) {
        auto *it = new QTreeWidgetItem({o.first});
        it->setData(0, Qt::UserRole, o.second);
        const bool var = QFileInfo::exists(temaDizini() + o.second + ".wav");
        it->setIcon(0, var ? QIcon::fromTheme("audio-volume-high") : QIcon());
        kok->addChild(it);
    }
    olaylar->addTopLevelItem(kok);
    kok->setExpanded(true);
    auto *efekt = sl->k<QComboBox>(6681);
    efekt->addItem("(Yok)", QString());
    for (const QString &f : QDir(xpSesDizini()).entryList({"*.wav"}, QDir::Files, QDir::Name)) efekt->addItem(f, xpSesDizini() + f);
    QHash<QString, QString> secimler;                 // olay -> wav yolu ("" = Yok)
    // bir olayın şu anki sesi: temadaki dosyayı XP seslerinden biriyle eşleştir
    auto mevcut = [&](const QString &ad) -> QString {
        if (secimler.contains(ad)) return secimler.value(ad);
        QFile f(temaDizini() + ad + ".wav");
        if (!f.open(QIODevice::ReadOnly)) return QString();
        const QByteArray v = f.readAll();
        if (v.size() < 2000) return QString();       // sessizlik = (Yok)
        for (int i = 1; i < efekt->count(); ++i) {
            QFile g(efekt->itemData(i).toString());
            if (g.size() == v.size() && g.open(QIODevice::ReadOnly) && g.readAll() == v) return efekt->itemData(i).toString();
        }
        return temaDizini() + ad + ".wav";
    };
    auto olaySecildi = [&] {
        auto *it = olaylar->currentItem();
        const QString ad = it ? it->data(0, Qt::UserRole).toString() : QString();
        efekt->setEnabled(!ad.isEmpty());
        const QSignalBlocker b(efekt);
        if (ad.isEmpty()) { efekt->setCurrentIndex(0); return; }
        const QString y = mevcut(ad);
        int i = efekt->findData(y);
        if (i < 0 && !y.isEmpty()) { efekt->addItem(QFileInfo(y).fileName(), y); i = efekt->count() - 1; }
        efekt->setCurrentIndex(qMax(0, i));
    };
    QObject::connect(olaylar, &QTreeWidget::currentItemChanged, &d, olaySecildi);
    olaySecildi();
    QObject::connect(efekt, &QComboBox::activated, &d, [&] {
        auto *it = olaylar->currentItem();
        if (!it || it->data(0, Qt::UserRole).toString().isEmpty()) return;
        secimler.insert(it->data(0, Qt::UserRole).toString(), efekt->currentData().toString());
        d.degisti();
    });
    if (auto *cal = sl->k<QPushButton>(8962)) {
        cal->setIcon(QIcon::fromTheme("media-playback-start"));
        cal->setIconSize(QSize(10, 10));
        cal->setToolTip("Sesi çal");
        QObject::connect(cal, &QPushButton::clicked, &d, [&] {
            const QString y = efekt->currentData().toString();
            if (!y.isEmpty()) QProcess::startDetached("pw-play", {y});
        });
    }
    if (auto *gz = sl->k<QPushButton>(8971))
        QObject::connect(gz, &QPushButton::clicked, &d, [&] {
            QString filtre = S(6166, "Sesler (*.wav)#*.wav#").section('#', 0, 0);
            const QString y = QFileDialog::getOpenFileName(&d, "Gözat", xpSesDizini(), filtre);
            if (y.isEmpty()) return;
            int i = efekt->findData(y);
            if (i < 0) { efekt->addItem(QFileInfo(y).fileName(), y); i = efekt->count() - 1; }
            efekt->setCurrentIndex(i);
            efekt->activated(i);
        });
    QObject::connect(duzen, &QComboBox::activated, &d, &xp::Ozellikler::degisti);

    // ------------------------------------------------------------ Ses
    xp::Sayfa *se = d.ekle(MM, 4866);
    for (auto [id, ik] : {std::pair{8515, 4379}, {8516, 4380}, {8517, 4381}})
        if (auto *l = se->k<QLabel>(id)) l->setPixmap(MM.simge(ik));
    auto *calma = se->k<QComboBox>(6662), *kayit = se->k<QComboBox>(6663), *midi = se->k<QComboBox>(6664);
    auto doldur = [](QComboBox *c, const QString &tur, bool cikisMi) {
        c->clear();
        const QString v = xp::ses::varsayilan(cikisMi);
        for (const auto &a : xp::ses::liste(tur)) {
            c->addItem(a.aciklama, a.ad);
            if (a.ad == v) c->setCurrentIndex(c->count() - 1);
        }
        c->setEnabled(c->count() > 0);
    };
    doldur(calma, "sinks", true);
    doldur(kayit, "sources", false);
    midi->addItem("(Yok)");
    midi->setEnabled(false);
    for (int id : {6669, 6673, 6665}) if (auto *w = se->k(id)) w->setEnabled(false);
    QObject::connect(calma, &QComboBox::activated, &d, &xp::Ozellikler::degisti);
    QObject::connect(kayit, &QComboBox::activated, &d, &xp::Ozellikler::degisti);
    if (auto *b = se->k<QPushButton>(6667)) QObject::connect(b, &QPushButton::clicked, &d, [bin] { QProcess::startDetached(bin + "xp-sesdenetimi", {}); });
    if (auto *b = se->k<QPushButton>(6668)) QObject::connect(b, &QPushButton::clicked, &d, [bin] { QProcess::startDetached(bin + "xp-sesdenetimi", {"--kayit"}); });
    for (int id : {6670, 6672})
        if (auto *b = se->k<QPushButton>(id)) QObject::connect(b, &QPushButton::clicked, [] { QProcess::startDetached("kcmshell6", {"kcm_pulseaudio"}); });

    // ------------------------------------------------------------ Donanım
    auto *dn = new QWidget;
    {
        auto *l = new QVBoxLayout(dn);
        l->setContentsMargins(xp::dluX(7), xp::dluY(7), xp::dluX(7), xp::dluY(7));
        l->addWidget(new QLabel("&Aygıtlar:"));
        auto *liste = new QTreeWidget;
        liste->setRootIsDecorated(false);
        liste->setHeaderLabels({"Ad", "Tür"});
        liste->setColumnWidth(0, xp::dluX(140));
        for (const QJsonValue &v : QJsonDocument::fromJson(xp::ses::pactl({"-f", "json", "list", "cards"})).array()) {
            const QJsonObject o = v.toObject().value("properties").toObject();
            auto *it = new QTreeWidgetItem({o.value("device.description").toString(v.toObject().value("name").toString()),
                                            "Ses, video ve oyun denetleyicileri"});
            it->setIcon(0, QIcon(MM.simge(4379, 16)));
            it->setData(0, Qt::UserRole, o.value("device.vendor.name").toString());
            it->setData(1, Qt::UserRole, o.value("device.bus_path").toString(o.value("device.bus").toString()));
            liste->addTopLevelItem(it);
        }
        l->addWidget(liste, 1);
        auto *g = new QGroupBox("Aygıt Özellikleri");
        auto *gl = new QFormLayout(g);
        auto *uretici = new QLabel, *konum = new QLabel, *durum = new QLabel;
        konum->setWordWrap(true);
        gl->addRow("Üretici:", uretici);
        gl->addRow("Konum:", konum);
        gl->addRow("Aygıt durumu:", durum);
        l->addWidget(g);
        auto *b = new QHBoxLayout;
        b->addStretch();
        auto *sorun = new QPushButton("&Sorun Giderme...");
        sorun->setEnabled(false);
        auto *oz = new QPushButton("Özelli&kler");
        b->addWidget(sorun);
        b->addWidget(oz);
        l->addLayout(b);
        QObject::connect(liste, &QTreeWidget::currentItemChanged, dn, [=](QTreeWidgetItem *it) {
            uretici->setText(it ? it->data(0, Qt::UserRole).toString() : QString());
            konum->setText(it ? it->data(1, Qt::UserRole).toString() : QString());
            durum->setText(it ? "Bu aygıt düzgün çalışıyor." : QString());
        });
        if (liste->topLevelItemCount()) liste->setCurrentItem(liste->topLevelItem(0));
        QObject::connect(oz, &QPushButton::clicked, [] { QProcess::startDetached("kcmshell6", {"kcm_pulseaudio"}); });
    }
    d.sekmeler->addTab(dn, "Donanım");
    d.sekmeler->setTabText(1, "Sesler");

    // ------------------------------------------------------------ Uygula
    d.uygulaIslevi = [&] {
        // görev çubuğu simgesi
        QProcess::execute("qdbus6", {"org.kde.plasmashell", "/PlasmaShell", "evaluateScript",
            QString("panels().forEach(function(p){p.widgets('org.kde.plasma.systemtray').forEach(function(t){"
                    "t.currentConfigGroup=['General'];var h=t.readConfig('hiddenItems');h=(typeof h==='string')?(h?h.split(','):[]):h;"
                    "h=h.filter(function(x){return x!=='io.github.yyozcan.xpses';});%1t.writeConfig('hiddenItems',h);});});")
                .arg(tepsi->isChecked() ? "" : "h.push('io.github.yyozcan.xpses');")});
        // ses düzeni ve olay sesleri
        QProcess::execute("kwriteconfig6", {"--file", "kdeglobals", "--group", "Sounds", "--key", "Enable", duzen->currentIndex() == 1 ? "true" : "false"});
        if (duzen->currentIndex() == 1)
            QProcess::execute("kwriteconfig6", {"--file", "kdeglobals", "--group", "Sounds", "--key", "Theme", "Windows XP"});
        QDir().mkpath(temaDizini());
        for (auto it = secimler.begin(); it != secimler.end(); ++it) {
            const QString hedef = temaDizini() + it.key() + ".wav";
            if (it.value() == hedef) continue;
            QFile::remove(hedef);
            if (it.value().isEmpty()) {
                QFile f(hedef);
                if (f.open(QIODevice::WriteOnly)) f.write(sessizWav());
            } else {
                QFile::copy(it.value(), hedef);
            }
        }
        secimler.clear();
        // varsayılan aygıtlar
        if (calma->currentIndex() >= 0 && calma->currentData().toString() != xp::ses::varsayilan(true))
            xp::ses::pactl({"set-default-sink", calma->currentData().toString()});
        if (kayit->currentIndex() >= 0 && kayit->currentData().toString() != xp::ses::varsayilan(false))
            xp::ses::pactl({"set-default-source", kayit->currentData().toString()});
        if (auto *ad = sa->findChild<QLabel *>("aygitAdi")) ad->setText(xp::ses::varsayilanAygit(true).aciklama);
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
