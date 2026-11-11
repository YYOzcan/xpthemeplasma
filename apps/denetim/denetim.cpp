// XPThemePlasma — Windows XP "Denetim Masası" (Kategori ve Klasik Görünüm)
// Her simge ilgili KDE ayar modülünü (kcmshell6) ya da programı ayrı pencerede açar.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpkit.h"

struct Oge {
    QString ad, ikon, komut;   // komut: "kcm:ad" ya da program satırı
};
struct Kategori {
    QString ad, ikon;
    QList<QPair<QString, QString>> gorevler;   // (metin, komut)
    QList<Oge> ogeler;
};

static bool kcmVar(const QString &ad) {
    for (const auto &d : {QStringLiteral("/usr/lib/qt6/plugins/plasma/kcms/systemsettings/"),
                          QStringLiteral("/usr/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets/")})
        if (QFile::exists(d + ad + ".so")) return true;
    return false;
}

// PATH'te ~/.local/bin olmasa da XP programları bulunsun
static QString bul(const QString &prog) {
    QString p = QStandardPaths::findExecutable(prog);
    if (p.isEmpty()) p = QStandardPaths::findExecutable(prog, {QDir::homePath() + "/.local/bin"});
    return p;
}

static bool calisabilir(const QString &komut) {
    if (komut.startsWith("kcm:")) return kcmVar(komut.mid(4));
    return !bul(komut.split(' ').first()).isEmpty();
}

static void calistir(const QString &komut) {
    if (komut.startsWith("kcm:")) {
        QProcess::startDetached("kcmshell6", {komut.mid(4)});
    } else {
        QStringList p = QProcess::splitCommand(komut);
        const QString prog = p.takeFirst();
        QProcess::startDetached(bul(prog).isEmpty() ? prog : bul(prog), p);
    }
}

static QString paketYoneticisi() {
    for (const auto &k : {"plasma-discover", "octopi", "pamac-manager", "gnome-software"})
        if (!QStandardPaths::findExecutable(k).isEmpty()) return k;
    return QString();
}

static QList<Kategori> kategoriler() {
    QList<Kategori> l;
    l.append({"Görünüm ve Temalar", "preferences-desktop-theme",
              {{"Bilgisayarın temasını değiştir", "xp-gorunum ,,0"},
               {"Masaüstü arka planını değiştir", "xp-gorunum ,,1"},
               {"Ekran koruyucu seçin", "xp-gorunum ,,2"},
               {"Ekran çözünürlüğünü değiştir", "xp-gorunum ,,4"}},
              {{"Görüntü", "preferences-desktop-display", "xp-gorunum"},
               {"Görev Çubuğu ve Başlat Menüsü", "preferences-desktop-taskbar", "kcm:kcm_workspace"},
               {"Yazı Tipleri", "preferences-desktop-font", "kcm:kcm_fonts"},
               {"Pencere Çerçeveleri", "preferences-desktop-theme", "kcm:kcm_kwindecoration"},
               {"Renkler", "preferences-desktop-color", "kcm:kcm_colors"},
               {"Simgeler", "preferences-desktop-icons", "kcm:kcm_icons"},
               {"Fare İşaretçileri", "input-mouse", "kcm:kcm_cursortheme"}}});
    l.append({"Ağ ve Internet Bağlantıları", "preferences-system-network",
              {{"Ağ bağlantılarını görüntüleyin veya oluşturun", "kcm:kcm_networkmanagement"},
               {"Internet ayarlarınızı değiştirin", "kcm:kcm_proxy"}},
              {{"Ağ Bağlantıları", "network-workgroup", "kcm:kcm_networkmanagement"},
               {"Internet Seçenekleri", "internet-web-browser", "kcm:kcm_proxy"},
               {"Windows Güvenlik Duvarı", "security-high", "kcm:kcm_firewall"},
               {"Bluetooth Aygıtları", "preferences-system-bluetooth", "kcm:kcm_bluetooth"}}});
    l.append({"Program Ekle veya Kaldır", "system-software-install", {}, {}});
    l.append({"Sesler, Konuşma ve Ses Aygıtları", "preferences-desktop-sound",
              {{"Bilgisayarın ses düzeyini ayarlayın", "kcm:kcm_pulseaudio"},
               {"Ses düzenini değiştirin", "kcm:kcm_soundtheme"},
               {"Hoparlör ayarlarını değiştirin", "kcm:kcm_pulseaudio"}},
              {{"Sesler ve Ses Aygıtları", "audio-card", "kcm:kcm_pulseaudio"},
               {"Ses Düzeni", "preferences-desktop-sound", "kcm:kcm_soundtheme"},
               {"Bildirimler", "preferences-desktop-notification-bell", "kcm:kcm_notifications"}}});
    l.append({"Performans ve Bakım", "preferences-system-performance",
              {{"Bu bilgisayarla ilgili bilgileri görün", "kinfocenter"},
               {"Sabit diskte yer açın", "filelight"},
               {"Güç seçeneklerini ayarlayın", "kcm:kcm_powerdevilprofilesconfig"}},
              {{"Sistem", "computer", "xp-sistem"},
               {"Güç Seçenekleri", "preferences-system-power-management", "kcm:kcm_powerdevilprofilesconfig"},
               {"Yönetimsel Araçlar", "utilities-system-monitor", "xp-gorev"},
               {"Başlangıç Programları", "system-run", "kcm:kcm_autostart"},
               {"Oturum Yönetimi", "system-log-out", "kcm:kcm_smserver"}}});
    l.append({"Yazıcılar ve Diğer Donanım", "preferences-desktop-peripherals",
              {{"Fare ayarlarını değiştirin", "kcm:kcm_mouse"},
               {"Klavye ayarlarını değiştirin", "kcm:kcm_keyboard"}},
              {{"Fare", "input-mouse", "kcm:kcm_mouse"},
               {"Klavye", "input-keyboard", "kcm:kcm_keyboard"},
               {"Oyun Denetleyicileri", "input-gaming", "kcm:kcm_gamecontroller"},
               {"Dokunmatik Yüzey", "input-touchpad", "kcm:kcm_touchpad"},
               {"Yazıcılar ve Fakslar", "printer", "kcm:kcm_printer_manager"},
               {"Tablet", "input-tablet", "kcm:kcm_tablet"},
               {"Klavye Kısayolları", "input-keyboard", "kcm:kcm_keys"},
               {"Otomatik Bağlama", "drive-removable-media", "kcm:kcm_device_automounter"}}});
    l.append({"Kullanıcı Hesapları", "system-users",
              {{"Bir hesabı değiştirin", "kcm:kcm_users"}, {"Yeni hesap oluşturun", "kcm:kcm_users"}},
              {{"Kullanıcı Hesapları", "system-users", "kcm:kcm_users"}}});
    l.append({"Tarih, Saat, Dil ve Bölge Seçenekleri", "preferences-system-time",
              {{"Tarihi ve saati değiştirin", "xp-tarihsaat"},
               {"Sayıların, tarihlerin ve saatlerin biçimini değiştirin", "kcm:kcm_regionandlang"}},
              {{"Tarih ve Saat", "preferences-system-time", "xp-tarihsaat"},
               {"Bölge ve Dil Seçenekleri", "preferences-desktop-locale", "kcm:kcm_regionandlang"}}});
    l.append({"Erişilebilirlik Seçenekleri", "preferences-desktop-accessibility",
              {{"Erişilebilirlik ayarlarını yapın", "kcm:kcm_access"}},
              {{"Erişilebilirlik Seçenekleri", "preferences-desktop-accessibility", "kcm:kcm_access"},
               {"Ekran Klavyesi", "input-keyboard-virtual", "kcm:kcm_virtualkeyboard"}}});
    l.append({"Güvenlik Merkezi", "security-high",
              {{"Güvenlik duvarı ayarlarını değiştirin", "kcm:kcm_firewall"}},
              {{"Windows Güvenlik Duvarı", "security-high", "kcm:kcm_firewall"}}});
    // kurulu olmayanları ayıkla
    for (auto &k : l) {
        QList<Oge> o;
        for (const auto &x : k.ogeler) if (calisabilir(x.komut)) o.append(x);
        k.ogeler = o;
        QList<QPair<QString, QString>> g;
        for (const auto &x : k.gorevler) if (calisabilir(x.second)) g.append(x);
        k.gorevler = g;
    }
    return l;
}

// mavi, üzerine gelince altı çizilen bağlantı (simge + metin)
class KategoriBaglanti : public QWidget {
public:
    KategoriBaglanti(const QString &ikon, const QString &metin, int boyut, std::function<void()> f, bool kalin = false)
        : eylem(std::move(f)) {
        setCursor(Qt::PointingHandCursor);
        auto *l = new QHBoxLayout(this);
        l->setContentsMargins(4, 4, 4, 4);
        l->setSpacing(8);
        auto *ic = new QLabel;
        ic->setPixmap(xp::simge(ikon).pixmap(boyut, boyut));
        ic->setFixedSize(boyut, boyut);
        l->addWidget(ic);
        yazi = new QLabel(metin);
        yazi->setWordWrap(true);
        QFont fn("Tahoma", 8);
        fn.setBold(kalin);
        yazi->setFont(fn);
        l->addWidget(yazi, 1);
        boya(false);
    }
protected:
    void enterEvent(QEnterEvent *) override { boya(true); }
    void leaveEvent(QEvent *) override { boya(false); }
    void mouseReleaseEvent(QMouseEvent *e) override { if (e->button() == Qt::LeftButton) eylem(); }
private:
    void boya(bool u) { yazi->setStyleSheet(QString("color:%1;%2").arg(u ? "#428EFF" : "#215DC6", u ? "text-decoration:underline;" : "")); }
    QLabel *yazi;
    std::function<void()> eylem;
};

class Denetim : public QMainWindow {
public:
    Denetim() {
        setWindowTitle("Denetim Masası");
        setWindowIcon(xp::simge("preferences-system"));
        resize(QSettings().value("boyut", QSize(780, 560)).toSize());
        k = xp::kabukKur(this, {"&Dosya", "Dü&zen", "&Görünüm", "&Sık Kullanılanlar", "&Araçlar", "&Yardım"});
        liste = kategoriler();
        yigin = new QStackedWidget;
        yigin->setAutoFillBackground(true);
        QPalette pal = yigin->palette();
        pal.setColor(QPalette::Window, Qt::white);
        yigin->setPalette(pal);
        k.govde->addWidget(yigin, 1);
        k.geri->setEnabled(false);
        k.ileri->setEnabled(false);
        connect(k.geri, &QToolButton::clicked, this, [this] { if (!gecmis.isEmpty()) { git(gecmis.takeLast(), false); } });
        connect(k.yukari, &QToolButton::clicked, this, [this] { git(-1); });
        connect(k.ara, &QToolButton::clicked, this, [] { QProcess::startDetached("krunner", {}); });
        auto *gm = new QMenu(this);
        gm->addAction("Kategori Görünümü", this, [this] { klasik = false; git(-1); });
        gm->addAction("Klasik Görünüm", this, [this] { klasik = true; git(-1); });
        k.gorunumler->setMenu(gm);
        klasik = QSettings().value("klasik", false).toBool();
        git(-1, false);
    }
protected:
    void closeEvent(QCloseEvent *e) override {
        QSettings().setValue("boyut", size());
        QSettings().setValue("klasik", klasik);
        e->accept();
    }
private:
    xp::Kabuk k;
    QList<Kategori> liste;
    QStackedWidget *yigin;
    QList<int> gecmis;
    int simdiki = -2;
    bool klasik = false;

    void git(int kat, bool kaydet = true) {
        if (kaydet && simdiki != -2 && simdiki != kat) gecmis.append(simdiki);
        k.geri->setEnabled(!gecmis.isEmpty());
        k.yukari->setEnabled(kat >= 0);
        simdiki = kat;
        while (yigin->count()) { auto *w = yigin->widget(0); yigin->removeWidget(w); w->deleteLater(); }
        k.bolme->temizle();
        auto *g = k.bolme->grup("Denetim Masası", true);
        if (kat < 0) {
            g->baglanti(klasik ? "view-choose" : "view-list-icons", klasik ? "Kategori Görünümüne Geç" : "Klasik Görünüme Geç",
                        [this] { klasik = !klasik; git(-1, false); });
        } else {
            g->baglanti("view-choose", "Kategori Görünümüne Geç", [this] { klasik = false; git(-1); });
        }
        auto *b = k.bolme->grup("Ayrıca Bkz.");
        b->baglanti("system-software-update", "Windows Update", [] { QProcess::startDetached("kcmshell6", {"kcm_kded"}); });
        b->baglanti("help-browser", "Yardım ve Destek", [] { QProcess::startDetached("khelpcenter", {}); });
        if (kat < 0) b->baglanti("preferences-system", "Diğer Denetim Masası Seçenekleri", [] { QProcess::startDetached("systemsettings", {}); });
        k.adres->clear();
        k.adres->addItem(xp::simge(kat < 0 ? "preferences-system" : liste[kat].ikon), kat < 0 ? "Denetim Masası" : liste[kat].ad);
        setWindowTitle(kat < 0 ? "Denetim Masası" : liste[kat].ad);
        yigin->addWidget(kat < 0 ? (klasik ? klasikSayfa() : kategoriSayfasi()) : kategoriDetay(kat));
    }

    QLabel *baslik(const QString &t, int boy = 15) {
        auto *l = new QLabel(t);
        QFont f("Trebuchet MS", boy);
        l->setFont(f);
        l->setStyleSheet("color:#4A6EC6;");
        return l;
    }

    QWidget *sar(QWidget *ic) {
        auto *sa = new QScrollArea;
        sa->setFrameShape(QFrame::NoFrame);
        sa->setWidgetResizable(true);
        sa->setWidget(ic);
        ic->setAutoFillBackground(true);
        QPalette p = ic->palette();
        p.setColor(QPalette::Window, Qt::white);
        ic->setPalette(p);
        return sa;
    }

    QWidget *kategoriSayfasi() {
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);
        v->setContentsMargins(30, 24, 30, 24);
        v->setSpacing(18);
        v->addWidget(baslik("Bir kategori seçin", 17));
        auto *grid = new QGridLayout;
        grid->setHorizontalSpacing(30);
        grid->setVerticalSpacing(10);
        for (int i = 0; i < liste.size(); ++i) {
            auto *b = new KategoriBaglanti(liste[i].ikon, liste[i].ad, 32, [this, i] {
                if (liste[i].ad == "Program Ekle veya Kaldır") {
                    QString pm = paketYoneticisi();
                    if (!pm.isEmpty()) QProcess::startDetached(pm, {});
                    else QMessageBox::information(this, "Program Ekle veya Kaldır", "Paket yöneticisi bulunamadı (Discover, Octopi, Pamac).");
                    return;
                }
                git(i);
            });
            grid->addWidget(b, i / 2, i % 2);
        }
        v->addLayout(grid);
        v->addStretch();
        return sar(w);
    }

    QWidget *kategoriDetay(int i) {
        const Kategori &c = liste[i];
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);
        v->setContentsMargins(30, 20, 30, 20);
        v->setSpacing(10);
        auto *bas = new QHBoxLayout;
        auto *ic = new QLabel;
        ic->setPixmap(xp::simge(c.ikon).pixmap(48, 48));
        bas->addWidget(ic);
        bas->addWidget(baslik(c.ad, 17), 1);
        v->addLayout(bas);
        if (!c.gorevler.isEmpty()) {
            v->addSpacing(8);
            v->addWidget(baslik("Bir görev seçin...", 12));
            for (const auto &g : c.gorevler) {
                QString kmt = g.second;
                v->addWidget(new KategoriBaglanti("go-next", g.first, 16, [kmt] { calistir(kmt); }));
            }
        }
        if (!c.ogeler.isEmpty()) {
            v->addSpacing(12);
            v->addWidget(baslik("veya bir Denetim Masası simgesi seçin", 12));
            auto *grid = new QGridLayout;
            grid->setHorizontalSpacing(20);
            for (int j = 0; j < c.ogeler.size(); ++j) {
                Oge o = c.ogeler[j];
                grid->addWidget(new KategoriBaglanti(o.ikon, o.ad, 32, [o] { calistir(o.komut); }), j / 2, j % 2);
            }
            v->addLayout(grid);
        }
        v->addStretch();
        return sar(w);
    }

    QWidget *klasikSayfa() {
        auto *lw = new QListWidget;
        lw->setViewMode(QListView::IconMode);
        lw->setIconSize(QSize(32, 32));
        lw->setGridSize(QSize(96, 72));
        lw->setResizeMode(QListView::Adjust);
        lw->setMovement(QListView::Static);
        lw->setWordWrap(true);
        lw->setFrameShape(QFrame::NoFrame);
        QMap<QString, Oge> hepsi;
        for (const auto &c : liste) for (const auto &o : c.ogeler) hepsi.insert(o.ad, o);
        for (const auto &o : hepsi) {
            auto *it = new QListWidgetItem(xp::simge(o.ikon), o.ad, lw);
            it->setData(Qt::UserRole, o.komut);
        }
        connect(lw, &QListWidget::itemActivated, this, [](QListWidgetItem *it) { calistir(it->data(Qt::UserRole).toString()); });
        return lw;
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("XPThemePlasma");
    QApplication::setApplicationName("xp-denetim");
    QGuiApplication::setDesktopFileName("xp-denetim");
    Denetim w;
    w.show();
    return app.exec();
}
