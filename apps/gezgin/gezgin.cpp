// XPThemePlasma — Windows XP "Windows Gezgini"
// Döşemeler/Simgeler/Liste/Ayrıntılar görünümleri, XP görev bölmesi, Klasörler ağacı,
// Bilgisayarım sayfası. Dosya işlemleri KDE'nin kioclient'ı ile (çöp kutusu, ilerleme pencereleri).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpkit.h"

static const QString BILGISAYARIM = QStringLiteral("xp:bilgisayarim");

static QString insanBoyut(qint64 b) {
    if (b < 1024) return QString("%1 bayt").arg(b);
    const char *b2[] = {"KB", "MB", "GB", "TB"};
    double v = b;
    int i = -1;
    while (v >= 1024 && i < 3) { v /= 1024; ++i; }
    return QLocale(QLocale::Turkish).toString(v, 'f', v < 10 ? 2 : (v < 100 ? 1 : 0)) + " " + b2[i];
}

static QString yerAdi(const QString &yol) {
    const QString h = QDir::homePath();
    if (yol == h) return QStringLiteral("Ana Klasör");
    if (yol == QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)) return QStringLiteral("Belgelerim");
    if (yol == QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)) return QStringLiteral("Resimlerim");
    if (yol == QStandardPaths::writableLocation(QStandardPaths::MusicLocation)) return QStringLiteral("Müziğim");
    if (yol == QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)) return QStringLiteral("Videolarım");
    if (yol == QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)) return QStringLiteral("Masaüstü");
    if (yol == "/") return QStringLiteral("Yerel Disk");
    return QFileInfo(yol).fileName();
}

// XP "Döşemeler" görünümü: 48 px simge, sağında ad / tür / boyut
class DoseDelegesi : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return QSize(240, 58); }
    void paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const override {
        p->save();
        const bool sec = o.state & QStyle::State_Selected;
        const QIcon ic = i.data(Qt::DecorationRole).value<QIcon>();
        QRect ir(o.rect.left() + 4, o.rect.top() + 5, 48, 48);
        ic.paint(p, ir, Qt::AlignCenter, sec ? QIcon::Selected : QIcon::Normal);
        auto *m = qobject_cast<const QFileSystemModel *>(i.model());
        QStringList satir{i.data().toString()};
        if (m) {
            QFileInfo fi = m->fileInfo(i);
            satir << m->type(i);
            if (fi.isFile()) satir << insanBoyut(fi.size());
        }
        QRect tr(ir.right() + 6, o.rect.top() + 8, o.rect.width() - 64, 44);
        QFont f("Tahoma", 8);
        p->setFont(f);
        QFontMetrics fm(f);
        int y = tr.top();
        for (int k = 0; k < satir.size(); ++k) {
            QString t = fm.elidedText(satir[k], Qt::ElideRight, tr.width() - 4);
            QRect lr(tr.left(), y, fm.horizontalAdvance(t) + 4, fm.height());
            if (k == 0 && sec) {
                p->fillRect(lr, o.palette.highlight());
                p->setPen(o.palette.highlightedText().color());
            } else {
                p->setPen(k == 0 ? Qt::black : QColor("#808080"));
            }
            p->drawText(lr.adjusted(2, 0, 0, 0), Qt::AlignVCenter, t);
            y += fm.height() + 1;
        }
        p->restore();
    }
};

class Gezgin : public QMainWindow {
public:
    explicit Gezgin(const QString &baslangic) {
        setWindowIcon(xp::simge("system-file-manager"));
        resize(QSettings().value("boyut", QSize(860, 600)).toSize());
        k = xp::kabukKur(this, {});
        menuleriKur();

        model = new QFileSystemModel(this);
        model->setRootPath("/");
        model->setReadOnly(false);
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        model->setResolveSymlinks(true);

        agacModel = new QFileSystemModel(this);
        agacModel->setRootPath("/");
        agacModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

        // sol: görev bölmesi ya da klasör ağacı
        solYigin = new QStackedWidget;
        solYigin->setFixedWidth(220);
        k.govde->removeWidget(k.bolme);
        solYigin->addWidget(k.bolme);
        k.bolme->setFixedWidth(220);
        auto *agacKap = new QWidget;
        auto *av = new QVBoxLayout(agacKap);
        av->setContentsMargins(0, 0, 0, 0);
        av->setSpacing(0);
        auto *agacBas = new QWidget;
        auto *ah = new QHBoxLayout(agacBas);
        ah->setContentsMargins(6, 2, 2, 2);
        ah->addWidget(new QLabel("Klasörler"), 1);
        auto *kapa = new QToolButton;
        kapa->setText("✕");
        kapa->setAutoRaise(true);
        connect(kapa, &QToolButton::clicked, this, [this] { k.klasorler->setChecked(false); solYigin->setCurrentIndex(0); });
        ah->addWidget(kapa);
        av->addWidget(agacBas);
        agac = new QTreeView;
        agac->setModel(agacModel);
        agac->setHeaderHidden(true);
        for (int c = 1; c < 4; ++c) agac->hideColumn(c);
        agac->setRootIndex(agacModel->index("/"));
        connect(agac, &QTreeView::clicked, this, [this](const QModelIndex &i) { git(agacModel->filePath(i)); });
        av->addWidget(agac, 1);
        solYigin->addWidget(agacKap);
        k.govde->insertWidget(0, solYigin);
        connect(k.klasorler, &QToolButton::toggled, this, [this](bool on) { solYigin->setCurrentIndex(on ? 1 : 0); });

        // sağ: görünümler
        sagYigin = new QStackedWidget;
        liste = new QListView;
        liste->setModel(model);
        liste->setSelectionMode(QAbstractItemView::ExtendedSelection);
        liste->setEditTriggers(QAbstractItemView::EditKeyPressed);
        liste->setResizeMode(QListView::Adjust);
        liste->setMovement(QListView::Static);
        liste->setUniformItemSizes(true);
        liste->setFrameShape(QFrame::NoFrame);
        liste->setContextMenuPolicy(Qt::CustomContextMenu);
        doseDelegesi = new DoseDelegesi(liste);
        varsayilanDelege = liste->itemDelegate();
        ayrinti = new QTreeView;
        ayrinti->setModel(model);
        ayrinti->setRootIsDecorated(false);
        ayrinti->setSelectionMode(QAbstractItemView::ExtendedSelection);
        ayrinti->setEditTriggers(QAbstractItemView::EditKeyPressed);
        ayrinti->setSortingEnabled(true);
        ayrinti->setFrameShape(QFrame::NoFrame);
        ayrinti->setContextMenuPolicy(Qt::CustomContextMenu);
        ayrinti->header()->setStretchLastSection(false);
        ayrinti->setColumnWidth(0, 260);
        bilgisayarim = new QListWidget;
        bilgisayarim->setViewMode(QListView::IconMode);
        bilgisayarim->setIconSize(QSize(48, 48));
        bilgisayarim->setGridSize(QSize(240, 64));
        bilgisayarim->setMovement(QListView::Static);
        bilgisayarim->setResizeMode(QListView::Adjust);
        bilgisayarim->setFrameShape(QFrame::NoFrame);
        sagYigin->addWidget(liste);
        sagYigin->addWidget(ayrinti);
        sagYigin->addWidget(bilgisayarim);
        k.govde->addWidget(sagYigin, 1);

        for (QAbstractItemView *v : {static_cast<QAbstractItemView *>(liste), static_cast<QAbstractItemView *>(ayrinti)}) {
            connect(v, &QAbstractItemView::activated, this, [this](const QModelIndex &i) { ac(model->filePath(i)); });
            connect(v, &QWidget::customContextMenuRequested, this, [this, v](const QPoint &pt) { sagTik(v, pt); });
        }
        connect(liste->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] { bolmeyiKur(); durumYaz(); });
        connect(ayrinti->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] { bolmeyiKur(); durumYaz(); });
        connect(model, &QFileSystemModel::directoryLoaded, this, [this] { durumYaz(); });
        connect(bilgisayarim, &QListWidget::itemActivated, this, [this](QListWidgetItem *it) { git(it->data(Qt::UserRole).toString()); });

        connect(k.geri, &QToolButton::clicked, this, [this] { if (!geriYol.isEmpty()) { ileriYol.append(yol); git(geriYol.takeLast(), false); } });
        connect(k.ileri, &QToolButton::clicked, this, [this] { if (!ileriYol.isEmpty()) { geriYol.append(yol); git(ileriYol.takeLast(), false); } });
        connect(k.yukari, &QToolButton::clicked, this, [this] {
            if (yol == BILGISAYARIM) return;
            QDir d(yol);
            if (yol == "/" ) git(BILGISAYARIM); else if (d.cdUp()) git(d.absolutePath());
        });
        connect(k.ara, &QToolButton::clicked, this, [this] {
            if (!QStandardPaths::findExecutable("kfind").isEmpty()) QProcess::startDetached("kfind", {yol == BILGISAYARIM ? "/" : yol});
            else QProcess::startDetached("krunner", {});
        });
        connect(k.adres->lineEdit(), &QLineEdit::returnPressed, this, [this] {
            QString t = k.adres->currentText().trimmed();
            if (t == "Bilgisayarım") git(BILGISAYARIM);
            else if (QFileInfo(t).isDir()) git(t);
            else if (QFileInfo::exists(t)) ac(t);
            else QMessageBox::warning(this, yerAdi(yol), QString("'%1' bulunamıyor. Yazımı denetleyin ve yeniden deneyin.").arg(t));
        });

        auto *gm = new QMenu(this);
        auto *grup = new QActionGroup(this);
        for (auto [ad, kip] : std::initializer_list<std::pair<const char *, int>>{{"Döşemeler", 0}, {"Simgeler", 1}, {"Liste", 2}, {"Ayrıntılar", 3}}) {
            QAction *a = gm->addAction(ad, this, [this, kip] { gorunum(kip); });
            a->setCheckable(true);
            grup->addAction(a);
            gorunumEylem[kip] = a;
        }
        k.gorunumler->setMenu(gm);
        gorunumMenusu->addActions(grup->actions());

        statusBar()->show();
        gorunum(QSettings().value("gorunum", 0).toInt());
        git(baslangic, false);
    }

protected:
    void closeEvent(QCloseEvent *e) override {
        QSettings().setValue("boyut", size());
        QSettings().setValue("gorunum", kip);
        e->accept();
    }

private:
    xp::Kabuk k;
    QFileSystemModel *model, *agacModel;
    QStackedWidget *solYigin, *sagYigin;
    QTreeView *agac, *ayrinti;
    QListView *liste;
    QListWidget *bilgisayarim;
    QAbstractItemDelegate *doseDelegesi, *varsayilanDelege;
    QMenu *gorunumMenusu = nullptr;
    QAction *gorunumEylem[4] = {};
    QString yol;
    QStringList geriYol, ileriYol;
    int kip = 0;

    QAbstractItemView *etkinGorunum() const { return kip == 3 ? static_cast<QAbstractItemView *>(ayrinti) : liste; }

    QStringList secili() const {
        QStringList l;
        if (yol == BILGISAYARIM) return l;
        for (const auto &i : etkinGorunum()->selectionModel()->selectedIndexes())
            if (i.column() == 0) l << model->filePath(i);
        return l;
    }

    void menuleriKur() {
        auto *dosya = menuBar()->addMenu("&Dosya");
        auto *yeni = dosya->addMenu("&Yeni");
        yeni->addAction(xp::simge("folder"), "&Klasör", this, [this] { yeniKlasor(); });
        yeni->addAction(xp::simge("text-x-generic"), "Metin Belgesi", this, [this] { yeniMetin(); });
        dosya->addSeparator();
        dosya->addAction("&Sil", QKeySequence::Delete, this, [this] { sil(); });
        dosya->addAction("&Yeniden Adlandır", QKeySequence(Qt::Key_F2), this, [this] { yenidenAdlandir(); });
        dosya->addAction("Ö&zellikler", QKeySequence(Qt::ALT | Qt::Key_Return), this, [this] { ozellikler(); });
        dosya->addSeparator();
        dosya->addAction("&Kapat", this, [this] { close(); });
        auto *duzen = menuBar()->addMenu("Dü&zen");
        duzen->addAction(xp::simge("edit-cut"), "&Kes", QKeySequence::Cut, this, [this] { pano(true); });
        duzen->addAction(xp::simge("edit-copy"), "K&opyala", QKeySequence::Copy, this, [this] { pano(false); });
        duzen->addAction(xp::simge("edit-paste"), "&Yapıştır", QKeySequence::Paste, this, [this] { yapistir(); });
        duzen->addSeparator();
        duzen->addAction("&Tümünü Seç", QKeySequence::SelectAll, this, [this] { etkinGorunum()->selectAll(); });
        auto *gor = menuBar()->addMenu("&Görünüm");
        auto *cubuk = gor->addMenu("&Gezgin Çubuğu");
        cubuk->addAction("&Klasörler", this, [this] { k.klasorler->toggle(); });
        auto *dc = gor->addAction("&Durum Çubuğu");
        dc->setCheckable(true);
        dc->setChecked(true);
        connect(dc, &QAction::toggled, statusBar(), &QWidget::setVisible);
        gor->addSeparator();
        gorunumMenusu = gor;
        gor->addSeparator();
        auto *gizli = gor->addAction("Gizli dosyaları göster");
        gizli->setCheckable(true);
        connect(gizli, &QAction::toggled, this, [this](bool on) {
            model->setFilter(on ? (QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden) : (QDir::AllEntries | QDir::NoDotAndDotDot));
        });
        gor->addAction("&Yenile", QKeySequence(Qt::Key_F5), this, [this] { git(yol, false); });
        auto *sik = menuBar()->addMenu("&Sık Kullanılanlar");
        sik->addAction(xp::simge("folder-documents"), "Belgelerim", this, [this] { git(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)); });
        sik->addAction(xp::simge("folder-download"), "İndirilenler", this, [this] { git(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)); });
        auto *araclar = menuBar()->addMenu("&Araçlar");
        araclar->addAction("Komut İstemini Burada Aç", this, [this] {
            QProcess::startDetached("konsole", {"--workdir", yol == BILGISAYARIM ? QDir::homePath() : yol});
        });
        araclar->addAction("&Klasör Seçenekleri...", this, [] { QProcess::startDetached("kcmshell6", {"kcm_filetypes"}); });
        auto *yardim = menuBar()->addMenu("&Yardım");
        yardim->addAction("Yardım ve Destek Merkezi", this, [] { QProcess::startDetached("khelpcenter", {}); });
        yardim->addSeparator();
        yardim->addAction("&Windows Gezgini Hakkında", this, [this] {
            const QString h = QDir::homePath() + "/.local/bin/xp-hakkinda";
            if (QFileInfo(h).isExecutable()) QProcess::startDetached(h, {"Windows Gezgini"});
            else QMessageBox::about(this, "Windows Gezgini Hakkında", "<b>Windows Gezgini</b><br>XPThemePlasma");
        });
    }

    void gorunum(int yeni) {
        kip = yeni;
        if (gorunumEylem[yeni]) gorunumEylem[yeni]->setChecked(true);
        if (yol == BILGISAYARIM) return;
        if (yeni == 3) {
            sagYigin->setCurrentWidget(ayrinti);
            return;
        }
        sagYigin->setCurrentWidget(liste);
        if (yeni == 0) {                       // Döşemeler
            liste->setViewMode(QListView::IconMode);
            liste->setFlow(QListView::LeftToRight);
            liste->setWrapping(true);
            liste->setItemDelegate(doseDelegesi);
            liste->setIconSize(QSize(48, 48));
            liste->setGridSize(QSize(246, 62));
        } else if (yeni == 1) {                // Simgeler
            liste->setItemDelegate(varsayilanDelege);
            liste->setViewMode(QListView::IconMode);
            liste->setIconSize(QSize(32, 32));
            liste->setGridSize(QSize(84, 72));
            liste->setWordWrap(true);
        } else {                               // Liste
            liste->setItemDelegate(varsayilanDelege);
            liste->setViewMode(QListView::ListMode);
            liste->setFlow(QListView::TopToBottom);
            liste->setWrapping(true);
            liste->setIconSize(QSize(16, 16));
            liste->setGridSize(QSize());
        }
    }

    void git(const QString &hedef, bool kaydet = true) {
        if (kaydet && !yol.isEmpty() && yol != hedef) { geriYol.append(yol); ileriYol.clear(); }
        yol = hedef;
        k.geri->setEnabled(!geriYol.isEmpty());
        k.ileri->setEnabled(!ileriYol.isEmpty());
        k.yukari->setEnabled(yol != BILGISAYARIM);
        k.adres->clear();
        if (yol == BILGISAYARIM) {
            setWindowTitle("Bilgisayarım");
            k.adres->addItem(xp::simge("computer"), "Bilgisayarım");
            bilgisayarimiDoldur();
            sagYigin->setCurrentWidget(bilgisayarim);
        } else {
            setWindowTitle(yerAdi(yol));
            k.adres->addItem(xp::simge("folder-open"), yol);
            QModelIndex r = model->setRootPath(yol);
            model->sort(0, Qt::AscendingOrder);      // XP: önce klasörler, A→Z
            liste->setRootIndex(r);
            ayrinti->setRootIndex(r);
            gorunum(kip);
            QModelIndex ai = agacModel->index(yol);
            agac->setCurrentIndex(ai);
            agac->scrollTo(ai);
        }
        bolmeyiKur();
        durumYaz();
    }

    void bilgisayarimiDoldur() {
        bilgisayarim->clear();
        auto ekle = [this](const QString &ikon, const QString &ad, const QString &yolu) {
            auto *it = new QListWidgetItem(xp::simge(ikon), ad, bilgisayarim);
            it->setData(Qt::UserRole, yolu);
        };
        ekle("folder-documents", "Belgelerim", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        ekle("user-home", "Ana Klasör", QDir::homePath());
        for (const QStorageInfo &s : QStorageInfo::mountedVolumes()) {
            if (!s.isValid() || !s.isReady()) continue;
            const QString r = s.rootPath();
            if (r != "/" && !r.startsWith("/run/media") && !r.startsWith("/media") && !r.startsWith("/mnt") && !r.startsWith("/seagate")
                && !(r.count('/') == 1 && !r.startsWith("/boot") && s.fileSystemType() != "tmpfs"))
                continue;
            if (s.fileSystemType() == "tmpfs" || (s.fileSystemType() == "vfat" && r.startsWith("/boot"))) continue;
            QString ad = s.displayName().isEmpty() || s.displayName() == r ? (r == "/" ? "Yerel Disk" : QFileInfo(r).fileName()) : s.displayName();
            ad += QString("  (%1 boş / %2)").arg(insanBoyut(s.bytesAvailable()), insanBoyut(s.bytesTotal()));
            ekle(r.startsWith("/run/media") || r.startsWith("/media") ? "drive-removable-media" : "drive-harddisk", ad, r);
        }
    }

    void ac(const QString &yolu) {
        QFileInfo fi(yolu);
        if (fi.isDir()) git(fi.absoluteFilePath());
        else QDesktopServices::openUrl(QUrl::fromLocalFile(yolu));
    }

    void bolmeyiKur() {
        k.bolme->temizle();
        const QStringList s = secili();
        if (yol == BILGISAYARIM) {
            auto *g = k.bolme->grup("Sistem Görevleri", true);
            g->baglanti("computer", "Sistem bilgilerini görüntüle", [] { QProcess::startDetached("kinfocenter", {}); });
            g->baglanti("system-software-install", "Program ekle veya kaldır", [] { QProcess::startDetached("xp-denetim", {}); });
            g->baglanti("preferences-system", "Bir ayarı değiştir", [] { QProcess::startDetached("xp-denetim", {}); });
        } else {
            auto *g = k.bolme->grup(s.isEmpty() ? "Dosya ve Klasör Görevleri" : (s.size() == 1 ? (QFileInfo(s[0]).isDir() ? "Dosya ve Klasör Görevleri" : "Dosya ve Klasör Görevleri") : "Dosya ve Klasör Görevleri"), true);
            if (s.isEmpty()) {
                g->baglanti("folder-new", "Yeni klasör oluştur", [this] { yeniKlasor(); });
                g->baglanti("text-x-generic", "Yeni metin belgesi oluştur", [this] { yeniMetin(); });
            } else {
                const bool dir = s.size() == 1 && QFileInfo(s[0]).isDir();
                const QString ne = s.size() > 1 ? "seçili öğeleri" : (dir ? "bu klasörü" : "bu dosyayı");
                if (s.size() == 1) g->baglanti("edit-rename", QString("%1 yeniden adlandır").arg(dir ? "Bu klasörü" : "Bu dosyayı"), [this] { yenidenAdlandir(); });
                g->baglanti("edit-move", QString("%1 taşı").arg(ne.left(1).toUpper() + ne.mid(1)), [this] { tasiKopyala(true); });
                g->baglanti("edit-copy", QString("%1 kopyala").arg(ne.left(1).toUpper() + ne.mid(1)), [this] { tasiKopyala(false); });
                g->baglanti("edit-delete", QString("%1 sil").arg(ne.left(1).toUpper() + ne.mid(1)), [this] { sil(); });
            }
        }
        auto *d = k.bolme->grup("Diğer Yerler");
        auto yer = [this, d](const QString &ikon, const QString &ad, const QString &hedef) {
            if (hedef != yol) d->baglanti(ikon, ad, [this, hedef] { git(hedef); });
        };
        yer("user-desktop", "Masaüstü", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        yer("folder-documents", "Belgelerim", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        yer("folder-pictures", "Resimlerim", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        yer("folder-music", "Müziğim", QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
        yer("computer", "Bilgisayarım", BILGISAYARIM);
        d->baglanti("user-trash", "Geri Dönüşüm Kutusu", [] { QProcess::startDetached("kioclient", {"exec", "trash:/"}); });
        d->baglanti("network-workgroup", "Ağ Bağlantılarım", [] { QProcess::startDetached("kioclient", {"exec", "remote:/"}); });

        auto *a = k.bolme->grup("Ayrıntılar");
        auto *etiket = new QLabel;
        etiket->setWordWrap(true);
        etiket->setFont(QFont("Tahoma", 8));
        if (yol == BILGISAYARIM) {
            etiket->setText("<b>Bilgisayarım</b><br>Sistem Klasörü");
        } else if (s.size() == 1) {
            QFileInfo fi(s[0]);
            QString t = QString("<b>%1</b><br>%2").arg(fi.fileName().toHtmlEscaped(), model->type(model->index(s[0])).toHtmlEscaped());
            t += "<br>Değiştirilme Tarihi: " + QLocale(QLocale::Turkish).toString(fi.lastModified(), "dd MMMM yyyy, HH:mm");
            if (fi.isFile()) t += "<br>Boyut: " + insanBoyut(fi.size());
            etiket->setText(t);
        } else if (s.size() > 1) {
            etiket->setText(QString("<b>%1 öğe seçildi.</b>").arg(s.size()));
        } else {
            etiket->setText(QString("<b>%1</b><br>Dosya Klasörü").arg(yerAdi(yol).toHtmlEscaped()));
        }
        a->bilgi(etiket);
    }

    void durumYaz() {
        if (yol == BILGISAYARIM) { statusBar()->showMessage(QString("%1 nesne").arg(bilgisayarim->count())); return; }
        const QStringList s = secili();
        if (!s.isEmpty()) {
            qint64 t = 0;
            for (const auto &f : s) t += QFileInfo(f).isFile() ? QFileInfo(f).size() : 0;
            statusBar()->showMessage(QString("%1 nesne seçildi").arg(s.size()) + (t ? "     " + insanBoyut(t) : ""));
        } else {
            statusBar()->showMessage(QString("%1 nesne").arg(model->rowCount(liste->rootIndex())));
        }
    }

    void sagTik(QAbstractItemView *v, const QPoint &pt) {
        QMenu m;
        const QModelIndex i = v->indexAt(pt);
        if (i.isValid()) {
            const QString f = model->filePath(i);
            m.addAction("&Aç", this, [this, f] { ac(f); })->setFont([] { QFont x; x.setBold(true); return x; }());
            m.addAction("Birlikte Aç...", this, [f] { QProcess::startDetached("kioclient", {"openProperties", f}); });
            m.addSeparator();
            m.addAction("&Kes", this, [this] { pano(true); });
            m.addAction("K&opyala", this, [this] { pano(false); });
            m.addSeparator();
            m.addAction("&Sil", this, [this] { sil(); });
            m.addAction("&Yeniden Adlandır", this, [this] { yenidenAdlandir(); });
            m.addSeparator();
            m.addAction("Ö&zellikler", this, [this] { ozellikler(); });
        } else {
            auto *gm = m.addMenu("&Görünüm");
            for (auto *a : gorunumEylem) if (a) gm->addAction(a);
            m.addAction("&Yenile", this, [this] { git(yol, false); });
            m.addSeparator();
            m.addAction("&Yapıştır", this, [this] { yapistir(); })->setEnabled(QGuiApplication::clipboard()->mimeData()->hasUrls());
            m.addSeparator();
            auto *yeni = m.addMenu("&Yeni");
            yeni->addAction(xp::simge("folder"), "&Klasör", this, [this] { yeniKlasor(); });
            yeni->addAction(xp::simge("text-x-generic"), "Metin Belgesi", this, [this] { yeniMetin(); });
            m.addSeparator();
            m.addAction("Ö&zellikler", this, [this] { QProcess::startDetached("kioclient", {"openProperties", yol}); });
        }
        m.exec(v->viewport()->mapToGlobal(pt));
    }

    void pano(bool kes) {
        const QStringList s = secili();
        if (s.isEmpty()) return;
        auto *md = new QMimeData;
        QList<QUrl> urls;
        for (const auto &f : s) urls << QUrl::fromLocalFile(f);
        md->setUrls(urls);
        md->setData("application/x-kde-cutselection", kes ? "1" : "0");
        QByteArray gn = (kes ? "cut\n" : "copy\n");
        for (const auto &u : urls) gn += u.toEncoded() + "\n";
        md->setData("x-special/gnome-copied-files", gn);
        QGuiApplication::clipboard()->setMimeData(md);
    }

    void yapistir() {
        const QMimeData *md = QGuiApplication::clipboard()->mimeData();
        if (!md || !md->hasUrls() || yol == BILGISAYARIM) return;
        const bool kes = md->data("application/x-kde-cutselection") == "1";
        QStringList arg{kes ? "move" : "copy"};
        for (const auto &u : md->urls()) arg << u.toString();
        arg << QUrl::fromLocalFile(yol).toString();
        QProcess::startDetached("kioclient", arg);
        if (kes) QGuiApplication::clipboard()->clear();
    }

    void sil() {
        const QStringList s = secili();
        if (s.isEmpty()) return;
        QString soru = s.size() == 1
            ? QString("'%1' öğesini Geri Dönüşüm Kutusu'na göndermek istediğinizden emin misiniz?").arg(QFileInfo(s[0]).fileName())
            : QString("Bu %1 öğeyi Geri Dönüşüm Kutusu'na göndermek istediğinizden emin misiniz?").arg(s.size());
        QMessageBox mb(QMessageBox::Question, s.size() == 1 ? "Dosya Silmeyi Onayla" : "Birden Çok Dosya Silmeyi Onayla",
                       soru, QMessageBox::Yes | QMessageBox::No, this);
        mb.button(QMessageBox::Yes)->setText("&Evet");
        mb.button(QMessageBox::No)->setText("&Hayır");
        if (mb.exec() != QMessageBox::Yes) return;
        QStringList arg{"move"};
        for (const auto &f : s) arg << QUrl::fromLocalFile(f).toString();
        arg << "trash:/";
        QProcess::startDetached("kioclient", arg);
    }

    void tasiKopyala(bool tasi) {
        const QStringList s = secili();
        if (s.isEmpty()) return;
        QString h = QFileDialog::getExistingDirectory(this, tasi ? "Öğeleri Taşı" : "Öğeleri Kopyala", yol);
        if (h.isEmpty()) return;
        QStringList arg{tasi ? "move" : "copy"};
        for (const auto &f : s) arg << QUrl::fromLocalFile(f).toString();
        arg << QUrl::fromLocalFile(h).toString();
        QProcess::startDetached("kioclient", arg);
    }

    void yenidenAdlandir() {
        QModelIndexList l = etkinGorunum()->selectionModel()->selectedIndexes();
        for (const auto &i : l) if (i.column() == 0) { etkinGorunum()->edit(i); return; }
    }

    void ozellikler() {
        const QStringList s = secili();
        QProcess::startDetached("kioclient", {"openProperties", s.isEmpty() ? yol : s.first()});
    }

    void yeniKlasor() {
        if (yol == BILGISAYARIM) return;
        QString ad = "Yeni Klasör";
        for (int n = 2; QFileInfo::exists(yol + "/" + ad); ++n) ad = QString("Yeni Klasör (%1)").arg(n);
        QModelIndex i = model->mkdir(model->index(yol), ad);
        if (i.isValid()) { etkinGorunum()->setCurrentIndex(i); etkinGorunum()->edit(i); }
    }

    void yeniMetin() {
        if (yol == BILGISAYARIM) return;
        QString ad = "Yeni Metin Belgesi.txt";
        for (int n = 2; QFileInfo::exists(yol + "/" + ad); ++n) ad = QString("Yeni Metin Belgesi (%1).txt").arg(n);
        QFile f(yol + "/" + ad);
        if (f.open(QIODevice::WriteOnly)) f.close();
        QTimer::singleShot(300, this, [this, ad] {
            QModelIndex i = model->index(yol + "/" + ad);
            if (i.isValid()) { etkinGorunum()->setCurrentIndex(i); etkinGorunum()->edit(i); }
        });
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("XPThemePlasma");
    QApplication::setApplicationName("xp-gezgin");
    QGuiApplication::setDesktopFileName("xp-gezgin");
    QString hedef = QDir::homePath();
    if (argc > 1) {
        QString a = QString::fromLocal8Bit(argv[1]);
        if (a == "--bilgisayarim") hedef = BILGISAYARIM;
        else {
            QUrl u = QUrl::fromUserInput(a, QDir::currentPath());
            hedef = u.isLocalFile() ? u.toLocalFile() : a;
            if (QFileInfo(hedef).isFile()) { QDesktopServices::openUrl(QUrl::fromLocalFile(hedef)); return 0; }
        }
    }
    Gezgin w(hedef);
    w.show();
    return app.exec();
}
