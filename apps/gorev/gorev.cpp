// XPThemePlasma — "Windows Görev Yöneticisi" (Ctrl+Shift+Esc, Ctrl+Alt+Del)
// Menüler, sütun adları, iletiler ve simgeler kendi XP kopyandaki taskmgr.exe'den. Uygulamalar sekmesi
// Plasma'nın görev modelini (Wayland pencere yönetimi), İşlemler/Performans/Ağ /proc ve /sys'i,
// Kullanıcılar loginctl'i kullanır. Çalışırken bildirim alanında XP'nin yeşil CPU göstergesi durur.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"
#include "../ortak/xpkomut.h"
#include <QtNetwork>
#include <taskmanager/abstracttasksmodel.h>
#include <taskmanager/tasksmodel.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>

static xp::Modul TM;
static QString S(int id, const char *yedek) { return TM.dize(id, QString::fromUtf8(yedek)); }
static const QLocale TR(QLocale::Turkish, QLocale::Turkey);
static QString kb(qint64 k) { return TR.toString(k) + " K"; }

static QString oku(const QString &yol) {
    QFile f(yol);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

// ---------------------------------------------------------------- /proc ölçümleri
struct Islem {
    int pid = 0, ppid = 0, is = 0, nice = 0;
    QString ad, kullanici;
    uid_t uid = 0;
    qint64 tik = 0, rss = 0, vm = 0, hata = 0;
    double cpu = 0;
};

struct Olcum {
    QVector<Islem> islemler;
    double cpu = 0, cekirdek = 0;
    QVector<double> cpular, cpularCekirdek;
    qint64 toplamK = 0, kullanilabilirK = 0, onbellekK = 0, ayrilmisK = 0, sinirK = 0, enYuksekK = 0,
           cekirdekK = 0, diskBellekliK = 0, diskBelleksizK = 0, kullanilanK = 0;
    int isleyici = 0, isParcacigi = 0;

    QHash<int, qint64> oncekiTik;
    QVector<qint64> oncekiToplam, oncekiBos, oncekiSistem;
    qint64 tikFarki = 1;

    void yenile() {
        // CPU (/proc/stat: toplam ve çekirdek başına)
        const QStringList satirlar = oku("/proc/stat").split('\n');
        QVector<qint64> toplam, bos, sistem;
        for (const QString &s : satirlar) {
            if (!s.startsWith("cpu")) continue;
            const QStringList p = s.split(' ', Qt::SkipEmptyParts);
            qint64 t = 0;
            for (int i = 1; i < p.size(); ++i) t += p[i].toLongLong();
            toplam << t;
            bos << p.value(4).toLongLong() + p.value(5).toLongLong();
            sistem << p.value(3).toLongLong() + p.value(6).toLongLong() + p.value(7).toLongLong();
        }
        cpular.clear();
        cpularCekirdek.clear();
        for (int i = 0; i < toplam.size(); ++i) {
            const qint64 dt = i < oncekiToplam.size() ? toplam[i] - oncekiToplam[i] : 0;
            const double yuzde = dt > 0 ? 100.0 * (dt - (bos[i] - oncekiBos[i])) / dt : 0;
            const double cek = dt > 0 ? 100.0 * (sistem[i] - oncekiSistem[i]) / dt : 0;
            if (i == 0) {
                cpu = qBound(0.0, yuzde, 100.0);
                cekirdek = qBound(0.0, cek, 100.0);
                tikFarki = qMax<qint64>(1, dt);
            } else {
                cpular << qBound(0.0, yuzde, 100.0);
                cpularCekirdek << qBound(0.0, cek, 100.0);
            }
        }
        oncekiToplam = toplam;
        oncekiBos = bos;
        oncekiSistem = sistem;

        // işlemler
        QHash<int, qint64> yeniTik;
        QHash<uid_t, QString> adlar;
        islemler.clear();
        isParcacigi = 0;
        for (const QString &d : QDir("/proc").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            bool sayi = false;
            const int pid = d.toInt(&sayi);
            if (!sayi) continue;
            const QString stat = oku("/proc/" + d + "/stat");
            const int kapa = stat.lastIndexOf(')');
            if (kapa < 0) continue;
            Islem is;
            is.pid = pid;
            is.ad = stat.mid(stat.indexOf('(') + 1, kapa - stat.indexOf('(') - 1);
            const QStringList f = stat.mid(kapa + 2).split(' ');
            is.ppid = f.value(1).toInt();
            is.hata = f.value(7).toLongLong() + f.value(9).toLongLong();
            is.tik = f.value(11).toLongLong() + f.value(12).toLongLong();
            is.nice = f.value(16).toInt();
            is.is = f.value(17).toInt();
            is.vm = f.value(20).toLongLong() / 1024;
            is.rss = f.value(21).toLongLong() * (sysconf(_SC_PAGESIZE) / 1024);
            // tam program adı (XP'deki gibi "firefox.exe" değil, Linux adı): cmdline'ın ilk parçası
            const QString cmd = oku("/proc/" + d + "/cmdline").section(QChar(0), 0, 0);
            if (!cmd.isEmpty() && !cmd.startsWith('[')) {
                const QString taban = QFileInfo(cmd).fileName();
                if (!taban.isEmpty() && taban.size() > is.ad.size() && taban.startsWith(is.ad)) is.ad = taban;
            }
            QFileInfo fi("/proc/" + d);
            is.uid = fi.ownerId();
            if (!adlar.contains(is.uid)) {
                struct passwd *pw = getpwuid(is.uid);
                adlar.insert(is.uid, pw ? QString::fromLocal8Bit(pw->pw_name) : QString::number(is.uid));
            }
            is.kullanici = adlar.value(is.uid);
            const qint64 once = oncekiTik.value(pid, is.tik);
            is.cpu = 100.0 * (is.tik - once) / tikFarki;
            yeniTik.insert(pid, is.tik);
            isParcacigi += is.is;
            islemler << is;
        }
        oncekiTik = yeniTik;
        isleyici = oku("/proc/sys/fs/file-nr").section('\t', 0, 0).toInt();

        // bellek
        QHash<QString, qint64> m;
        for (const QString &s : oku("/proc/meminfo").split('\n')) {
            const QString a = s.section(':', 0, 0);
            m.insert(a, s.section(':', 1).simplified().section(' ', 0, 0).toLongLong());
        }
        toplamK = m.value("MemTotal");
        kullanilabilirK = m.value("MemAvailable");
        onbellekK = m.value("Cached") + m.value("Buffers");
        kullanilanK = toplamK - kullanilabilirK + (m.value("SwapTotal") - m.value("SwapFree"));
        ayrilmisK = kullanilanK;
        sinirK = toplamK + m.value("SwapTotal");
        enYuksekK = qMax(enYuksekK, ayrilmisK);
        cekirdekK = m.value("Slab") + m.value("KernelStack") + m.value("PageTables");
        diskBellekliK = m.value("SReclaimable");
        diskBelleksizK = cekirdekK - diskBellekliK;
    }
};

// ---------------------------------------------------------------- yeşil grafik ve gösterge
class Grafik : public QWidget {
public:
    struct Seri { QVector<double> v; QColor renk; };
    QVector<Seri> seriler;
    double olcek = 100;
    int kayma = 0;
    explicit Grafik(QWidget *p = nullptr) : QWidget(p) { setMinimumSize(60, 40); }
    void ekle(const QVector<double> &degerler) {
        for (int i = 0; i < seriler.size() && i < degerler.size(); ++i) {
            seriler[i].v.prepend(degerler[i]);
            if (seriler[i].v.size() > 400) seriler[i].v.removeLast();
        }
        kayma = (kayma + 2) % 12;
        update();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);
        p.setPen(QColor(0, 128, 64));
        for (int x = width() - 1 - (12 - kayma) % 12; x >= 0; x -= 12) p.drawLine(x, 0, x, height());
        for (int y = height() - 1; y >= 0; y -= 12) p.drawLine(0, y, width(), y);
        for (const Seri &s : seriler) {
            p.setPen(s.renk);
            QPolygon cizgi;
            for (int i = 0; i < s.v.size(); ++i) {
                const int x = width() - 1 - i * 2;
                if (x < 0) break;
                cizgi << QPoint(x, height() - 1 - int(s.v[i] / olcek * (height() - 1)));
            }
            p.drawPolyline(cizgi);
        }
    }
};

class Gosterge : public QWidget {
public:
    double deger = 0;      // 0..1
    QString yazi;
    explicit Gosterge(QWidget *p = nullptr) : QWidget(p) { setMinimumSize(50, 60); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);
        QFont f("Arial");
        f.setPixelSize(10);
        p.setFont(f);
        const int yaziH = 14;
        const int alanH = height() - yaziH - 6;
        const int cubukW = 14, x0 = width() / 2 - cubukW - 1;
        const int adet = alanH / 3;
        const int yanik = qRound(deger * adet);
        for (int i = 0; i < adet; ++i) {
            const int y = 4 + alanH - (i + 1) * 3;
            const QColor r = i < yanik ? QColor(0, 255, 0) : QColor(0, 128, 0);
            p.fillRect(x0, y, cubukW, 2, r);
            p.fillRect(x0 + cubukW + 2, y, cubukW, 2, r);
        }
        p.setPen(QColor(0, 255, 0));
        p.drawText(QRect(0, height() - yaziH - 2, width(), yaziH), Qt::AlignCenter, yazi);
    }
};

static QGroupBox *cerceve(const QString &baslik, QWidget *icerik) {
    auto *g = new QGroupBox(baslik);
    auto *l = new QVBoxLayout(g);
    l->setContentsMargins(6, 4, 6, 6);
    l->addWidget(icerik);
    return g;
}

// sayı tablosu: "Toplam   123456" satırları
static QGroupBox *tablo(const QString &baslik, const QStringList &adlar, QVector<QLabel *> &degerler) {
    auto *g = new QGroupBox(baslik);
    auto *l = new QGridLayout(g);
    l->setContentsMargins(8, 2, 8, 6);
    l->setVerticalSpacing(1);
    for (int i = 0; i < adlar.size(); ++i) {
        l->addWidget(new QLabel(adlar[i]), i, 0);
        auto *d = new QLabel("0");
        d->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->addWidget(d, i, 1);
        degerler << d;
    }
    return g;
}

// ---------------------------------------------------------------- ana pencere
class GorevYoneticisi : public QMainWindow {
public:
    GorevYoneticisi() {
        setWindowTitle(S(10003, "Windows Görev Yöneticisi"));
        setWindowIcon(QIcon(TM.simge(107, 16)));
        sekmeler = new QTabWidget;
        auto *orta = new QWidget;
        auto *ol = new QVBoxLayout(orta);
        ol->setContentsMargins(6, 6, 6, 4);
        ol->addWidget(sekmeler);
        setCentralWidget(orta);

        uygulamaSayfasi();
        islemSayfasi();
        performansSayfasi();
        agSayfasi();
        kullaniciSayfasi();

        durumIslem = new QLabel;
        durumCpu = new QLabel;
        durumBellek = new QLabel;
        for (QLabel *l : {durumIslem, durumCpu, durumBellek}) {
            l->setFrameShape(QFrame::Panel);
            l->setFrameShadow(QFrame::Sunken);
            statusBar()->addWidget(l, 1);
        }
        statusBar()->setSizeGripEnabled(true);

        // XP son kullanılan sekmeyi hatırlardı
        QSettings ayar("xpthemeplasma", "gorev");
        sekmeler->setCurrentIndex(ayar.value("sekme", 0).toInt());
        connect(sekmeler, &QTabWidget::currentChanged, this, [this](int i) {
            QSettings("xpthemeplasma", "gorev").setValue("sekme", i);
            menuKur();
        });
        menuKur();

        tepsi = new QSystemTrayIcon(this);
        tepsi->setIcon(QIcon(TM.simge(127, 16)));
        tepsi->setToolTip(QString(S(30041, "CPU Kullanımı: %d%%")).replace("%d%%", "0%"));
        connect(tepsi, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
            if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) { showNormal(); raise(); activateWindow(); }
        });
        tepsi->show();

        olcum.yenile();
        zaman = new QTimer(this);
        connect(zaman, &QTimer::timeout, this, [this] { guncelle(); });
        hizAyarla(1000);
        QTimer::singleShot(300, this, [this] { guncelle(); });
        sekmeler->tabBar()->setUsesScrollButtons(false);    // XP: beş sekme de görünür
        resize(qMax(sizeHint().width(), xp::dluX(270)), xp::dluY(300));
    }

private:
    // ------------------------------------------------------------ Uygulamalar
    void uygulamaSayfasi() {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(8, 8, 8, 8);
        gorevListe = new QTreeWidget;
        gorevListe->setRootIsDecorated(false);
        gorevListe->setHeaderLabels({S(21000, "Görev"), S(21001, "Durum")});
        gorevListe->setSelectionMode(QAbstractItemView::ExtendedSelection);
        gorevListe->setContextMenuPolicy(Qt::CustomContextMenu);
        gorevListe->header()->setStretchLastSection(false);
        gorevListe->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        gorevListe->setColumnWidth(1, 90);
        l->addWidget(gorevListe);
        auto *b = new QHBoxLayout;
        b->addStretch();
        auto *sonlandir = new QPushButton("G&örevi Sonlandır");
        auto *gecis = new QPushButton("G&eçiş Yap");
        auto *yeni = new QPushButton("Ye&ni Görev...");
        for (auto *x : {sonlandir, gecis, yeni}) { x->setMinimumWidth(xp::dluX(53)); b->addWidget(x); }
        l->addLayout(b);
        sekmeler->addTab(w, S(10021, "Uygulamalar"));

        gorevler = new TaskManager::TasksModel(this);
        gorevler->setFilterByVirtualDesktop(false);
        gorevler->setFilterByActivity(false);
        gorevler->setFilterByScreen(false);
        gorevler->setGroupMode(TaskManager::TasksModel::GroupDisabled);
        auto yenile = [this] { gorevleriDoldur(); };
        connect(gorevler, &QAbstractItemModel::rowsInserted, this, yenile);
        connect(gorevler, &QAbstractItemModel::rowsRemoved, this, yenile);
        connect(gorevler, &QAbstractItemModel::dataChanged, this, yenile);
        connect(gorevler, &QAbstractItemModel::modelReset, this, yenile);

        connect(sonlandir, &QPushButton::clicked, this, [this] { gorevSonlandir(); });
        connect(gecis, &QPushButton::clicked, this, [this] { gorevGecis(); });
        connect(gorevListe, &QTreeWidget::itemDoubleClicked, this, [this] { gorevGecis(); });
        connect(yeni, &QPushButton::clicked, this, [this] { yeniGorev(); });
        connect(gorevListe, &QWidget::customContextMenuRequested, this, [this](const QPoint &p) {
            if (!gorevListe->itemAt(p)) return;
            QMenu m;
            const QJsonArray menu = TM.menu(117);
            const QJsonArray ogeler = menu.isEmpty() ? QJsonArray() : menu.first().toObject().value("items").toArray();
            const auto eylemler = xp::menuKur(&m, ogeler, [this](int id) { menuEylemi(id); });
            for (int kapali : {40041, 40042, 30043}) if (eylemler.value(kapali)) eylemler.value(kapali)->setEnabled(false);
            m.exec(gorevListe->viewport()->mapToGlobal(p));
        });
    }
    QModelIndex gorevIndeksi(QTreeWidgetItem *it) const {
        return it ? gorevler->index(it->data(0, Qt::UserRole).toInt(), 0) : QModelIndex();
    }
    void gorevleriDoldur() {
        const QString seciliAd = gorevListe->currentItem() ? gorevListe->currentItem()->text(0) : QString();
        gorevListe->clear();
        for (int i = 0; i < gorevler->rowCount(); ++i) {
            const QModelIndex ix = gorevler->index(i, 0);
            if (!ix.data(TaskManager::AbstractTasksModel::IsWindow).toBool()) continue;
            QString ad = ix.data(Qt::DisplayRole).toString();
            const bool yanitsiz = ad.contains("(Yanıt Vermiyor)") || ad.contains("(Not Responding)");
            auto *it = new QTreeWidgetItem({ad, yanitsiz ? S(10022, "Yanıt Vermiyor") : S(10023, "Çalışıyor")});
            it->setIcon(0, ix.data(Qt::DecorationRole).value<QIcon>());
            it->setData(0, Qt::UserRole, i);
            gorevListe->addTopLevelItem(it);
            if (ad == seciliAd) gorevListe->setCurrentItem(it);
        }
    }
    void gorevGecis() {
        for (QTreeWidgetItem *it : gorevListe->selectedItems()) gorevler->requestActivate(gorevIndeksi(it));
        if (eylemSimge && eylemSimge->isChecked()) showMinimized();
    }
    void gorevSonlandir() {
        for (QTreeWidgetItem *it : gorevListe->selectedItems()) {
            const QModelIndex ix = gorevIndeksi(it);
            if (it->text(1) == S(10022, "Yanıt Vermiyor")) {
                const int pid = ix.data(TaskManager::AbstractTasksModel::AppPid).toInt();
                if (pid > 0) ::kill(pid, SIGKILL);
            } else {
                gorevler->requestClose(ix);
            }
        }
    }
    void yeniGorev() {
        const xp::Modul shell = xp::Modul::yukle("shell32-dll");
        if (!shell.gecerli()) { QProcess::startDetached(xp::yerelBin("xp-calistir"), {}); return; }
        xp::Iletisim d(shell, 1003, this);
        d.setWindowTitle(S(10001, "Yeni Görev Oluştur"));
        if (auto *l = d.sayfa->k<QLabel>(12289)) l->setText(S(10002, "Program, klasör, belge veya bir Internet kaynağının adını yazın, Windows sizin için açacaktır."));
        if (auto *c = d.sayfa->k(12306)) c->hide();
        auto *kutu = d.sayfa->k<QComboBox>(12298);
        if (auto *g = d.sayfa->k<QPushButton>(12288))
            connect(g, &QPushButton::clicked, &d, [&] {
                const QString y = QFileDialog::getOpenFileName(&d, shell.dize(9015, "Gözat"), QDir::homePath());
                if (!y.isEmpty()) kutu->setEditText(y);
            });
        if (d.exec() == QDialog::Accepted && !xp::calistir(kutu->currentText())) {
            QString ileti = shell.dize(6211, "Windows '%1!ls!' öğesini bulamıyor.");
            QMessageBox::critical(this, kutu->currentText(), ileti.replace("%1!ls!", kutu->currentText()));
        }
    }

    // ------------------------------------------------------------ İşlemler
    void islemSayfasi() {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(8, 8, 8, 8);
        islemListe = new QTreeWidget;
        islemListe->setRootIsDecorated(false);
        islemListe->setUniformRowHeights(true);
        islemListe->setSortingEnabled(true);
        islemListe->setHeaderLabels({S(20001, "Yansıma Adı"), S(20018, "Kullanıcı Adı"), S(20003, "CPU"), S(20005, "Bellek Kullanımı")});
        islemListe->setColumnWidth(0, 130);
        islemListe->setColumnWidth(1, 90);
        islemListe->setColumnWidth(2, 40);
        islemListe->setContextMenuPolicy(Qt::CustomContextMenu);
        islemListe->sortByColumn(0, Qt::AscendingOrder);
        l->addWidget(islemListe);
        auto *b = new QHBoxLayout;
        tumKullanicilar = new QCheckBox("&Tüm kullanıcıların işlemlerini göster");
        b->addWidget(tumKullanicilar);
        b->addStretch();
        auto *sonlandir = new QPushButton("&İşlemi Sonlandır");
        b->addWidget(sonlandir);
        l->addLayout(b);
        sekmeler->addTab(w, S(10004, "İşlemler"));
        connect(tumKullanicilar, &QCheckBox::toggled, this, [this] { islemleriDoldur(); });
        connect(sonlandir, &QPushButton::clicked, this, [this] { islemSonlandir(false); });
        connect(islemListe, &QWidget::customContextMenuRequested, this, [this](const QPoint &p) {
            if (!islemListe->itemAt(p)) return;
            QMenu m;
            const QJsonArray menu = TM.menu(111);
            const QJsonArray ogeler = menu.isEmpty() ? QJsonArray() : menu.first().toObject().value("items").toArray();
            const auto eylemler = xp::menuKur(&m, ogeler, [this](int id) { menuEylemi(id); });
            if (auto *h = eylemler.value(40027)) h->setEnabled(false);
            // geçerli önceliği işaretle
            const int nice = islemListe->currentItem() ? islemListe->currentItem()->data(0, Qt::UserRole + 1).toInt() : 0;
            const int secili = nice <= -20 ? 40029 : nice < -5 ? 40031 : nice < 0 ? 40030 : nice == 0 ? 40032 : nice < 10 ? 40033 : 40034;
            if (auto *e = eylemler.value(secili)) { e->setCheckable(true); e->setChecked(true); }
            m.exec(islemListe->viewport()->mapToGlobal(p));
        });
    }
    void islemleriDoldur() {
        const int seciliPid = islemListe->currentItem() ? islemListe->currentItem()->data(0, Qt::UserRole).toInt() : -1;
        const int kaydirma = islemListe->verticalScrollBar()->value();
        islemListe->setSortingEnabled(false);
        islemListe->clear();
        const uid_t ben = getuid();
        double toplamCpu = 0;
        for (const Islem &is : olcum.islemler) {
            if (!tumKullanicilar->isChecked() && is.uid != ben) continue;
            toplamCpu += is.cpu;
            auto *it = new QTreeWidgetItem;
            it->setText(0, is.ad);
            it->setText(1, is.kullanici);
            it->setText(2, QString("%1").arg(qRound(is.cpu), 2, 10, QChar('0')));
            it->setText(3, kb(is.rss));
            it->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
            it->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
            it->setData(0, Qt::UserRole, is.pid);
            it->setData(0, Qt::UserRole + 1, is.nice);
            it->setData(2, Qt::UserRole, is.cpu);
            islemListe->addTopLevelItem(it);
            if (is.pid == seciliPid) islemListe->setCurrentItem(it);
        }
        // XP'deki "Sistem Boşta İşlemi"
        auto *bos = new QTreeWidgetItem({S(10005, "Sistem Boşta İşlemi"), "SYSTEM",
                                         QString("%1").arg(qRound(100 - olcum.cpu), 2, 10, QChar('0')), kb(16)});
        bos->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        bos->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
        bos->setData(0, Qt::UserRole, 0);
        if (tumKullanicilar->isChecked()) islemListe->addTopLevelItem(bos);
        else delete bos;
        islemListe->setSortingEnabled(true);
        islemListe->verticalScrollBar()->setValue(kaydirma);
    }
    QList<int> seciliPidler() const {
        QList<int> l;
        for (QTreeWidgetItem *it : islemListe->selectedItems()) if (it->data(0, Qt::UserRole).toInt() > 0) l << it->data(0, Qt::UserRole).toInt();
        return l;
    }
    void islemSonlandir(bool agac) {
        const QList<int> pidler = seciliPidler();
        if (pidler.isEmpty()) return;
        const QString uyari = agac ? S(40106, "Bu eylem, bu işlemi ve bununla başlatılmış tüm işlemleri sonlandırmayı deneyecek.")
                                   : S(10009, "UYARI: Bir işlemi sonlandırmak veri kayıplarına neden olabilir.");
        if (QMessageBox::warning(this, S(10007, "Görev Yöneticisi Uyarısı"), uyari, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
        QList<int> hedef = pidler;
        if (agac) {
            for (int i = 0; i < hedef.size(); ++i)
                for (const Islem &is : olcum.islemler) if (is.ppid == hedef[i] && !hedef.contains(is.pid)) hedef << is.pid;
        }
        for (int pid : hedef) {
            if (pid == 1 || ::kill(pid, SIGKILL) != 0) {
                QMessageBox::critical(this, S(10017, "İşlem Sonlandırılamıyor"),
                                      S(10016, "Eylem tamamlanamadı.\n\n") + QString::fromLocal8Bit(strerror(errno)));
                break;
            }
        }
        QTimer::singleShot(300, this, [this] { guncelle(); });
    }
    void oncelikAta(int nice) {
        for (int pid : seciliPidler()) {
            if (setpriority(PRIO_PROCESS, pid, nice) != 0) {
                QMessageBox::critical(this, S(10019, "Öncelik Değiştirilemiyor"),
                                      S(10016, "Eylem tamamlanamadı.\n\n") + QString::fromLocal8Bit(strerror(errno)));
                return;
            }
        }
    }
    void benzesme() {
        const QList<int> pidler = seciliPidler();
        if (pidler.isEmpty()) return;
        xp::Iletisim d(TM, 124, this);
        const int n = qMin(32, int(sysconf(_SC_NPROCESSORS_ONLN)));
        cpu_set_t kume;
        CPU_ZERO(&kume);
        sched_getaffinity(pidler.first(), sizeof(kume), &kume);
        for (int i = 0; i < 32; ++i) {
            if (auto *c = d.sayfa->k<QCheckBox>(2000 + i)) {
                c->setVisible(i < n);
                c->setChecked(i < n && CPU_ISSET(i, &kume));
            }
        }
        if (d.exec() != QDialog::Accepted) return;
        CPU_ZERO(&kume);
        int secili = 0;
        for (int i = 0; i < n; ++i)
            if (auto *c = d.sayfa->k<QCheckBox>(2000 + i); c && c->isChecked()) { CPU_SET(i, &kume); ++secili; }
        if (!secili) { QMessageBox::warning(this, S(40058, "Geçersiz Seçenek"), S(40057, "İşlem, en az bir işlemci ile ilişkili olmalıdır.")); return; }
        if (sched_setaffinity(pidler.first(), sizeof(kume), &kume) != 0)
            QMessageBox::critical(this, S(30056, "Erişilemiyor veya İşlem İlgisi Ayarlanamıyor"), QString::fromLocal8Bit(strerror(errno)));
    }

    // ------------------------------------------------------------ Performans
    void performansSayfasi() {
        auto *w = new QWidget;
        auto *g = new QGridLayout(w);
        g->setContentsMargins(8, 6, 8, 8);
        g->setHorizontalSpacing(8);
        g->setVerticalSpacing(4);
        cpuGosterge = new Gosterge;
        cpuGecmis = new Grafik;
        cpuGecmis->seriler = {{{}, QColor(0, 255, 0)}, {{}, QColor(255, 0, 0)}};
        bellekGosterge = new Gosterge;
        bellekGecmis = new Grafik;
        bellekGecmis->seriler = {{{}, QColor(255, 255, 0)}};
        auto *c1 = cerceve(S(40060, "CPU Kullanımı").replace("Toplam ", ""), cpuGosterge);
        c1->setTitle("CPU Kullanımı");
        c1->setFixedWidth(xp::dluX(60));
        g->addWidget(c1, 0, 0);
        g->addWidget(cerceve("CPU Kullanımı Geçmişi", cpuGecmis), 0, 1);
        auto *c3 = cerceve("SD Kullanımı", bellekGosterge);
        c3->setFixedWidth(xp::dluX(60));
        g->addWidget(c3, 1, 0);
        g->addWidget(cerceve("Sayfa Dosyası Kullanımı Geçmişi", bellekGecmis), 1, 1);
        auto *alt = new QGridLayout;
        alt->setHorizontalSpacing(8);
        alt->addWidget(tablo("Toplam", {"İşleçler", "İş Parçacıkları", "İşlemler"}, toplamlar), 0, 0);
        alt->addWidget(tablo("Fiziksel Bellek (K)", {"Toplam", "Kullanılabilir", "Sistem Önbelleği"}, fiziksel), 0, 1);
        alt->addWidget(tablo("Ayrılmış Bellek (K)", {"Toplam", "Sınır", "En Yüksek"}, ayrilmis), 1, 0);
        alt->addWidget(tablo("Çekirdek Bellek (K)", {"Toplam", "Disk Bellekli", "Disk Belleksiz"}, cekirdek), 1, 1);
        g->addLayout(alt, 2, 0, 1, 2);
        g->setRowStretch(0, 1);
        g->setRowStretch(1, 1);
        sekmeler->addTab(w, S(10000, "Performans"));
    }

    // ------------------------------------------------------------ Ağ İletişimi
    struct Bagdastirici { QString arayuz; qint64 rx = -1, tx = -1; Grafik *grafik = nullptr; QGroupBox *kutu = nullptr; };
    void agSayfasi() {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(8, 8, 8, 8);
        agGrafikler = new QVBoxLayout;
        l->addLayout(agGrafikler, 2);
        agListe = new QTreeWidget;
        agListe->setRootIsDecorated(false);
        agListe->setHeaderLabels({S(21100, "Bağdaştırıcı Adı"), S(21102, "Ağ Kullanımı"), S(21103, "Bağlantı Hızı"), S(21104, "Durum")});
        agListe->setColumnWidth(0, 150);
        l->addWidget(agListe, 1);
        int kablolu = 0, kablosuz = 0;
        for (const QString &a : QDir("/sys/class/net").entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
            if (a == "lo" || a.startsWith("veth") || a.startsWith("docker") || a.startsWith("br-") || a.startsWith("virbr")) continue;
            Bagdastirici b;
            b.arayuz = a;
            const bool wifi = QFileInfo::exists("/sys/class/net/" + a + "/wireless");
            const QString ad = wifi ? QString("Kablosuz Ağ Bağlantısı%1").arg(kablosuz++ ? QString(" %1").arg(kablosuz) : "")
                                    : QString("Yerel Ağ Bağlantısı%1").arg(kablolu++ ? QString(" %1").arg(kablolu) : "");
            b.grafik = new Grafik;
            b.grafik->seriler = {{{}, QColor(0, 255, 0)}, {{}, QColor(255, 255, 0)}, {{}, QColor(255, 0, 0)}};
            b.kutu = cerceve(ad, b.grafik);
            b.kutu->setToolTip(a);
            agGrafikler->addWidget(b.kutu);
            auto *it = new QTreeWidgetItem({ad, "0 %", "", ""});
            it->setToolTip(0, a);
            agListe->addTopLevelItem(it);
            adaptorler << b;
        }
        if (adaptorler.isEmpty()) agGrafikler->addWidget(new QLabel("Etkin Ağ Bağdaştırıcısı Bulunamadı."), 0, Qt::AlignCenter);
        sekmeler->addTab(w, S(10030, "Ağ İletişimi"));
    }

    // ------------------------------------------------------------ Kullanıcılar
    void kullaniciSayfasi() {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(8, 8, 8, 8);
        kullaniciListe = new QTreeWidget;
        kullaniciListe->setRootIsDecorated(false);
        kullaniciListe->setHeaderLabels({S(3100, "Kullanıcı"), S(3101, "Kimlik"), S(3102, "Durum"), S(3103, "İstemci Adı"), S(3104, "Oturum")});
        l->addWidget(kullaniciListe);
        auto *b = new QHBoxLayout;
        b->addStretch();
        auto *kes = new QPushButton("&Bağlantıyı Kes");
        auto *kapat = new QPushButton("&Oturumu Kapat");
        auto *ileti = new QPushButton("&İleti Gönder...");
        kes->setEnabled(false);
        ileti->setEnabled(false);
        for (auto *x : {kes, kapat, ileti}) b->addWidget(x);
        l->addLayout(b);
        sekmeler->addTab(w, S(3001, "Kullanıcılar"));
        connect(kapat, &QPushButton::clicked, this, [this] {
            auto *it = kullaniciListe->currentItem();
            if (!it) return;
            if (QMessageBox::question(this, windowTitle(), S(3310, "Seçili kullanıcı oturumunu kapatmayı gerçekten istiyor musunuz?")) == QMessageBox::Yes)
                QProcess::startDetached("loginctl", {"terminate-session", it->data(0, Qt::UserRole).toString()});
        });
        kullanicilariDoldur();
    }
    void kullanicilariDoldur() {
        kullaniciListe->clear();
        QProcess p;
        p.start("loginctl", {"list-sessions", "--no-legend"});
        p.waitForFinished(2000);
        for (const QString &s : QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
            const QStringList f = s.split(' ', Qt::SkipEmptyParts);
            if (f.size() < 3) continue;
            QProcess q;
            q.start("loginctl", {"show-session", f[0], "-p", "Class", "-p", "State", "-p", "TTY", "-p", "Remote"});
            q.waitForFinished(2000);
            const QString bilgi = QString::fromUtf8(q.readAllStandardOutput());
            if (!bilgi.contains("Class=user")) continue;
            const bool etkin = bilgi.contains("State=active");
            auto *it = new QTreeWidgetItem({f[2], f[0], etkin ? S(3020, "Etkin") : S(21020, "Bağlantı kesildi"), "",
                                            bilgi.contains("Remote=yes") ? "RDP" : "Konsol"});
            it->setIcon(0, QIcon(TM.simge(3003, 16)));
            it->setData(0, Qt::UserRole, f[0]);
            kullaniciListe->addTopLevelItem(it);
        }
    }

    // ------------------------------------------------------------ menüler
    void menuKur() {
        static const int menuler[] = {106, 121, 120, 2400, 3200};
        const int i = qBound(0, sekmeler->currentIndex(), 4);
        menuBar()->clear();
        eylemler = xp::menuKur(menuBar(), TM.menu(menuler[i]), [this](int id) { menuEylemi(id); });
        // "Bilgisayarı Kapat" alt menüsü (taskmgr menü 4001)
        if (QAction *k = eylemler.value(4000)) {
            auto *alt = new QMenu(k->text(), menuBar());
            QString kullanici = qEnvironmentVariable("USER");
            const auto altlar = xp::menuKur(alt, TM.menu(4001), [this](int id) { menuEylemi(id); });
            if (auto *o = altlar.value(4006)) o->setText(o->text().replace("%s", kullanici));
            for (int gizli : {4008, 4009}) if (auto *a = altlar.value(gizli)) a->setVisible(false);
            menuBar()->insertMenu(k, alt);
            menuBar()->removeAction(k);
        }
        for (int id : {40049, 40098, 40056, 40019, 2404, 2405, 2406, 2407, 2408, 2410, 3220, 40109, 40052, 40015, 40016})
            if (QAction *a = eylemler.value(id)) a->setEnabled(false);
        for (int id : {40006, 40007, 40065, 40018, 40017, 2401, 2402, 2403}) if (QAction *a = eylemler.value(id)) a->setCheckable(true);
        if (QAction *a = eylemler.value(40006)) a->setChecked(ustte);
        if (QAction *a = eylemler.value(40007)) { a->setChecked(kucult); eylemSimge = a; }
        if (QAction *a = eylemler.value(40065)) a->setChecked(gizle);
        if (QAction *a = eylemler.value(40018)) a->setChecked(true);
        if (QAction *a = eylemler.value(40017)) a->setChecked(true);
        for (int id : {2401, 2402, 2403}) if (QAction *a = eylemler.value(id)) a->setChecked(true);
        for (int id : {40022, 40023, 40024, 40025}) {
            if (QAction *a = eylemler.value(id)) {
                a->setCheckable(true);
                const int h = id == 40022 ? 500 : id == 40023 ? 2000 : id == 40024 ? 4000 : 0;
                a->setChecked(h == hiz);
            }
        }
    }
    void menuEylemi(int id) {
        auto gorevlereUygula = [this](auto f) {
            for (QTreeWidgetItem *it : gorevListe->selectedItems()) f(gorevIndeksi(it));
        };
        switch (id) {
        case 40001: yeniGorev(); break;
        case 40002: qApp->quit(); break;
        case 40006: ustte = !ustte; setWindowFlag(Qt::WindowStaysOnTopHint, ustte); show(); break;
        case 40007: kucult = !kucult; break;
        case 40065: gizle = !gizle; break;
        case 40053: guncelle(); break;
        case 40022: hizAyarla(500); break;
        case 40023: hizAyarla(2000); break;
        case 40024: hizAyarla(4000); break;
        case 40025: hizAyarla(0); break;
        case 40021: {
            const QString hakkinda = xp::yerelBin("xp-hakkinda");
            if (!hakkinda.isEmpty()) QProcess::startDetached(hakkinda, {S(10025, "Görev Yöneticisi")});
            break;
        }
        // uygulamalar
        case 40044: gorevGecis(); break;
        case 40045: gorevlereUygula([this](const QModelIndex &ix) { gorevler->requestActivate(ix); }); break;
        case 40039: gorevlereUygula([this](const QModelIndex &ix) { if (!ix.data(TaskManager::AbstractTasksModel::IsMinimized).toBool()) gorevler->requestToggleMinimized(ix); }); break;
        case 40040: gorevlereUygula([this](const QModelIndex &ix) { if (!ix.data(TaskManager::AbstractTasksModel::IsMaximized).toBool()) gorevler->requestToggleMaximized(ix); }); break;
        case 40046: gorevSonlandir(); break;
        case 40047: {
            auto *it = gorevListe->currentItem();
            if (!it) break;
            const int pid = gorevIndeksi(it).data(TaskManager::AbstractTasksModel::AppPid).toInt();
            sekmeler->setCurrentIndex(1);
            tumKullanicilar->setChecked(true);
            for (int i = 0; i < islemListe->topLevelItemCount(); ++i)
                if (islemListe->topLevelItem(i)->data(0, Qt::UserRole).toInt() == pid) {
                    islemListe->setCurrentItem(islemListe->topLevelItem(i));
                    islemListe->scrollToItem(islemListe->topLevelItem(i));
                }
            break;
        }
        // işlemler
        case 40028: islemSonlandir(false); break;
        case 40105: islemSonlandir(true); break;
        case 40029: oncelikAta(-20); break;
        case 40031: oncelikAta(-10); break;
        case 40030: oncelikAta(-5); break;
        case 40032: oncelikAta(0); break;
        case 40033: oncelikAta(5); break;
        case 40034: oncelikAta(19); break;
        case 40055: benzesme(); break;
        // Bilgisayarı Kapat
        case 4004: QProcess::startDetached("systemctl", {"suspend"}); break;
        case 4002: QProcess::startDetached("systemctl", {"hibernate"}); break;
        case 4003: QProcess::startDetached("qdbus6", {"org.kde.Shutdown", "/Shutdown", "logoutAndShutdown"}); break;
        case 4005: QProcess::startDetached("qdbus6", {"org.kde.Shutdown", "/Shutdown", "logoutAndReboot"}); break;
        case 4006: QProcess::startDetached("qdbus6", {"org.kde.Shutdown", "/Shutdown", "logout"}); break;
        case 4007: QProcess::startDetached("qdbus6", {"org.kde.ksmserver", "/KSMServer", "openSwitchUserDialog"}); break;
        case 4010: QProcess::startDetached("loginctl", {"lock-session"}); break;
        default: break;
        }
    }
    void hizAyarla(int ms) {
        hiz = ms;
        if (ms > 0) zaman->start(ms);
        else zaman->stop();
        for (int id : {40022, 40023, 40024, 40025})
            if (QAction *a = eylemler.value(id)) a->setChecked((id == 40022 ? 500 : id == 40023 ? 2000 : id == 40024 ? 4000 : 0) == ms);
    }

    // ------------------------------------------------------------ yenile
    void guncelle() {
        olcum.yenile();
        gorevleriDoldur();
        islemleriDoldur();
        const int cpu = qRound(olcum.cpu);
        cpuGosterge->deger = olcum.cpu / 100.0;
        cpuGosterge->yazi = QString("%1 %").arg(cpu);
        cpuGosterge->update();
        cpuGecmis->ekle({olcum.cpu, olcum.cekirdek});
        cpuGecmis->seriler[1].v.fill(0);              // çekirdek zamanları: XP'de varsayılan kapalı
        const double kullanim = olcum.sinirK ? double(olcum.ayrilmisK) / olcum.sinirK : 0;
        bellekGosterge->deger = kullanim;
        bellekGosterge->yazi = olcum.ayrilmisK >= 1024 * 1024 ? TR.toString(olcum.ayrilmisK / 1024.0 / 1024.0, 'f', 2) + " GB"
                                                              : TR.toString(olcum.ayrilmisK / 1024) + " MB";
        bellekGosterge->update();
        bellekGecmis->ekle({kullanim * 100});
        toplamlar[0]->setText(TR.toString(olcum.isleyici));
        toplamlar[1]->setText(TR.toString(olcum.isParcacigi));
        toplamlar[2]->setText(TR.toString(olcum.islemler.size()));
        fiziksel[0]->setText(TR.toString(olcum.toplamK));
        fiziksel[1]->setText(TR.toString(olcum.kullanilabilirK));
        fiziksel[2]->setText(TR.toString(olcum.onbellekK));
        ayrilmis[0]->setText(TR.toString(olcum.ayrilmisK));
        ayrilmis[1]->setText(TR.toString(olcum.sinirK));
        ayrilmis[2]->setText(TR.toString(olcum.enYuksekK));
        cekirdek[0]->setText(TR.toString(olcum.cekirdekK));
        cekirdek[1]->setText(TR.toString(olcum.diskBellekliK));
        cekirdek[2]->setText(TR.toString(olcum.diskBelleksizK));

        // ağ
        const double sure = qMax(0.5, hizSn.restart() / 1000.0);
        for (int i = 0; i < adaptorler.size(); ++i) {
            Bagdastirici &b = adaptorler[i];
            const QString k = "/sys/class/net/" + b.arayuz + "/";
            const qint64 rx = oku(k + "statistics/rx_bytes").trimmed().toLongLong();
            const qint64 tx = oku(k + "statistics/tx_bytes").trimmed().toLongLong();
            int hizMbps = oku(k + "speed").trimmed().toInt();
            const bool bagli = oku(k + "operstate").trimmed() == "up";
            if (hizMbps <= 0) hizMbps = bagli ? 54 : 0;
            double gelen = 0, giden = 0;
            if (b.rx >= 0 && hizMbps > 0) {
                const double kapasite = hizMbps * 1e6 / 8.0 * sure;
                gelen = 100.0 * (rx - b.rx) / kapasite;
                giden = 100.0 * (tx - b.tx) / kapasite;
            }
            b.rx = rx;
            b.tx = tx;
            const double top = qMin(100.0, gelen + giden);
            // XP ölçeği: %1 / %5 / %10 / %25 / %50 / %100 — en büyük değere göre
            double enb = 1;
            for (const auto &s : b.grafik->seriler) for (double v : s.v) enb = qMax(enb, v);
            enb = qMax(enb, top);
            for (double o : {1.0, 5.0, 10.0, 25.0, 50.0, 100.0}) if (enb <= o) { b.grafik->olcek = o; break; }
            b.grafik->ekle({top, gelen, giden});
            if (auto *it = agListe->topLevelItem(i)) {
                it->setText(1, TR.toString(top, 'f', top < 1 ? 2 : 0) + " %");
                it->setText(2, hizMbps >= 1000 ? QString("%1 Gbps").arg(hizMbps / 1000) : QString("%1 Mbps").arg(hizMbps));
                it->setText(3, bagli ? S(21022, "Bağlı Olan") : S(21020, "Bağlantı kesildi"));
            }
        }

        durumIslem->setText(QString(S(30039, "İşlemler: %d")).replace("%d", QString::number(olcum.islemler.size())));
        durumCpu->setText(QString(S(30041, "CPU Kullanımı: %d%%")).replace("%d%%", QString::number(cpu) + "%"));
        durumBellek->setText(QString(S(30044, "Ayrılmış Bellek: %I64dM / %I64dM"))
                                 .replace(QRegularExpression("%I64dM / %I64dM"), QString("%1M / %2M").arg(olcum.ayrilmisK / 1024).arg(olcum.sinirK / 1024)));
        // bildirim alanı: 12 kademeli yeşil CPU göstergesi (taskmgr simgeleri 127..138)
        const int kademe = qBound(0, int(std::ceil(olcum.cpu / 100.0 * 11)), 11);
        if (kademe != tepsiKademe) {
            tepsiKademe = kademe;
            tepsi->setIcon(QIcon(TM.simge(127 + kademe, 16)));
        }
        tepsi->setToolTip(durumCpu->text());
        if (sekmeler->currentIndex() == 4 && ++kullaniciSayac % 5 == 0) kullanicilariDoldur();
    }

protected:
    void changeEvent(QEvent *e) override {
        QMainWindow::changeEvent(e);
        if (e->type() == QEvent::WindowStateChange && isMinimized() && gizle) QTimer::singleShot(0, this, &QWidget::hide);
    }

private:
    QTabWidget *sekmeler;
    QTreeWidget *gorevListe, *islemListe, *agListe, *kullaniciListe;
    QCheckBox *tumKullanicilar;
    TaskManager::TasksModel *gorevler;
    Gosterge *cpuGosterge, *bellekGosterge;
    Grafik *cpuGecmis, *bellekGecmis;
    QVector<QLabel *> toplamlar, fiziksel, ayrilmis, cekirdek;
    QVBoxLayout *agGrafikler;
    QVector<Bagdastirici> adaptorler;
    QLabel *durumIslem, *durumCpu, *durumBellek;
    QSystemTrayIcon *tepsi;
    QTimer *zaman;
    QElapsedTimer hizSn = [] { QElapsedTimer t; t.start(); return t; }();
    QHash<int, QAction *> eylemler;
    QAction *eylemSimge = nullptr;
    Olcum olcum;
    int hiz = 1000, tepsiKademe = -1, kullaniciSayac = 0;
    bool ustte = true, kucult = true, gizle = false;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-gorev");
    app.setQuitOnLastWindowClosed(true);
    TM = xp::Modul::yukle("taskmgr-exe");
    if (!TM.gecerli()) return QProcess::startDetached("plasma-systemmonitor", {}) ? 0 : 1;
    if (app.arguments().contains("--pencereler")) {      // tanı: Uygulamalar listesini yaz ve çık
        auto *m = new TaskManager::TasksModel(&app);
        m->setFilterByVirtualDesktop(false);
        m->setFilterByActivity(false);
        m->setFilterByScreen(false);
        m->setGroupMode(TaskManager::TasksModel::GroupDisabled);
        QTimer::singleShot(1500, &app, [m] {
            int n = 0;
            for (int i = 0; i < m->rowCount(); ++i)
                if (m->index(i, 0).data(TaskManager::AbstractTasksModel::IsWindow).toBool()) ++n;
            printf("pencere: %d\n", n);
            qApp->quit();
        });
        return app.exec();
    }
    // XP: Görev Yöneticisi tek örnek çalışırdı
    QLocalServer *tek = new QLocalServer(&app);
    const QString ad = "xp-gorev-" + qEnvironmentVariable("USER");
    QLocalSocket dene;
    dene.connectToServer(ad);
    if (dene.waitForConnected(300)) { dene.write("goster"); dene.flush(); dene.waitForBytesWritten(300); return 0; }
    QLocalServer::removeServer(ad);
    tek->listen(ad);
    GorevYoneticisi w;
    QObject::connect(tek, &QLocalServer::newConnection, &w, [&w, tek] {
        QLocalSocket *s = tek->nextPendingConnection();
        QObject::connect(s, &QLocalSocket::readyRead, &w, [&w, s] { s->readAll(); w.showNormal(); w.raise(); w.activateWindow(); s->deleteLater(); });
    });
    w.setWindowFlag(Qt::WindowStaysOnTopHint, true);     // XP varsayılanı: Her Zaman Üstte
    w.show();
    return app.exec();
}
