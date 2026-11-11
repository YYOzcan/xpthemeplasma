// XPThemePlasma — XP iletişim kutusu kaynaklarından (gen_luna_dlg.py'nin JSON'u) Qt pencereleri kurar.
//
// Kendi XP kopyandaki DIALOG şablonu birebir yerleştirilir: iletişim birimi → piksel (MS Shell Dlg 8 =
// Tahoma 8, 96 dpi: x*6/4, y*13/8), Win32 denetim sınıfı → Qt parçacığı. Program kodu denetimlere
// XP'deki kimlikleriyle erişir (sayfa->k<QPushButton>(1) = "Tamam"). Menüler ve dizeler de aynı JSON'dan.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtWidgets>
#include <functional>

namespace xp {

inline QString dlgDizini() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/xpthemeplasma/varlik/dlg/";
}

inline int dluX(int v) { return (v * 6 + 2) / 4; }    // MulDiv(v, 6, 4)
inline int dluY(int v) { return (v * 13 + 4) / 8; }   // MulDiv(v, 13, 8)

// ---------------------------------------------------------------- modül (JSON)
struct Modul {
    QString kok;          // ör. "sysdm-cpl"
    QJsonObject veri;

    static Modul yukle(const QString &ad) {
        Modul m;
        m.kok = ad;
        QFile f(dlgDizini() + ad + ".json");
        if (f.open(QIODevice::ReadOnly)) m.veri = QJsonDocument::fromJson(f.readAll()).object();
        return m;
    }
    bool gecerli() const { return !veri.isEmpty(); }
    QString dize(int id, const QString &yedek = {}) const {
        const QString s = veri.value("strings").toObject().value(QString::number(id)).toString();
        return s.isEmpty() ? yedek : s;
    }
    QJsonObject dialog(int id) const { return veri.value("dialogs").toObject().value(QString::number(id)).toObject(); }
    QJsonArray menu(int id) const { return veri.value("menus").toObject().value(QString::number(id)).toArray(); }
    QPixmap simge(int id, int boyut = 32) const {
        return QPixmap(dlgDizini() + kok + (boyut <= 16 ? "-ico16-" : "-ico-") + QString::number(id) + ".png");
    }
    QPixmap bitmap(int id) const { return QPixmap(dlgDizini() + kok + "-bmp-" + QString::number(id) + ".png"); }
};

// Win32 "&Aç:" → Qt "&Aç:" ("&&" aynı); sekme sonrası hızlandırıcı metni menüde kalır
inline QString metin(const QString &s) { return s; }
inline QString sade(QString s) {       // mnemonik işaretsiz düz metin
    s.replace("&&", "\x01");
    s.remove('&');
    s.replace("\x01", "&");
    return s;
}

// ---------------------------------------------------------------- sayfa: bir DIALOG şablonu
class Sayfa : public QWidget {
public:
    Sayfa(const Modul &m, int dlgId, QWidget *ebeveyn = nullptr) : QWidget(ebeveyn), modul(m) {
        const QJsonObject d = m.dialog(dlgId);
        baslik = d.value("title").toString();
        boyut = QSize(dluX(d.value("w").toInt()), dluY(d.value("h").toInt()));
        setFixedSize(boyut);
        QButtonGroup *grup = nullptr;
        QLabel *bekleyenEtiket = nullptr;
        QList<QWidget *> kutular;
        for (const QJsonValue &v : d.value("controls").toArray()) {
            const QJsonObject c = v.toObject();
            const QString sinif = c.value("class").toString();
            const int id = c.value("id").toInt();
            const quint32 st = quint32(c.value("style").toDouble());
            const quint32 exst = quint32(c.value("exstyle").toDouble());
            const QString t = c.value("text").toString();
            QRect r(dluX(c.value("x").toInt()), dluY(c.value("y").toInt()), dluX(c.value("w").toInt()), dluY(c.value("h").toInt()));
            QWidget *w = nullptr;
            const QString s = sinif.toUpper();
            if (s == "BUTTON") {
                const int tur = st & 0xF;
                if (tur == 0 || tur == 1) {
                    auto *b = new QPushButton(t, this);
                    b->setDefault(tur == 1);
                    b->setAutoDefault(false);
                    w = b;
                } else if (tur == 2 || tur == 3 || tur == 5 || tur == 6) {
                    auto *b = new QCheckBox(t, this);
                    b->setTristate(tur == 5 || tur == 6);
                    w = b;
                } else if (tur == 4 || tur == 9) {
                    auto *b = new QRadioButton(t, this);
                    if (!grup || (st & 0x00020000)) { grup = new QButtonGroup(this); }   // WS_GROUP yeni grup başlatır
                    grup->addButton(b);
                    w = b;
                } else if (tur == 7) {
                    auto *g = new QGroupBox(t, this);
                    kutular << g;
                    w = g;
                } else {                                        // BS_OWNERDRAW vb.
                    w = new QWidget(this);
                }
                if (tur != 4 && tur != 9 && (st & 0x00020000)) grup = nullptr;
            } else if (s == "EDIT") {
                if (st & 0x4) {                                 // ES_MULTILINE
                    auto *e = new QPlainTextEdit(this);
                    e->setReadOnly(st & 0x800);
                    e->setPlainText(t);
                    w = e;
                } else {
                    auto *e = new QLineEdit(t, this);
                    e->setReadOnly(st & 0x800);
                    if (st & 0x20) e->setEchoMode(QLineEdit::Password);
                    if (st & 0x2) e->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    else if (st & 0x1) e->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                    if (!(st & 0x00800000) && !(exst & 0x200)) {   // çerçevesiz (ör. Görev Yöneticisi sayıları)
                        e->setFrame(false);
                        e->setStyleSheet("QLineEdit { border: none; background: transparent; }");
                    }
                    w = e;
                }
            } else if (s == "STATIC") {
                const int tur = st & 0x1F;
                if (tur == 0x3 || tur == 0xE) {
                    auto *l = new QLabel(this);
                    if (t.startsWith('#')) {
                        const int rid = t.mid(1).toInt();
                        QPixmap p = tur == 0x3 ? m.simge(rid) : m.bitmap(rid);
                        l->setPixmap(p);
                        if (tur == 0x3 && !p.isNull()) r.setSize(p.size());
                        else if (!p.isNull() && !(st & 0x40)) r.setSize(p.size());
                    }
                    w = l;
                } else if (tur == 0x10 || tur == 0x11 || tur == 0x12 || (tur >= 4 && tur <= 9)) {
                    auto *f = new QFrame(this);
                    if (tur == 0x10) { f->setFrameShape(QFrame::HLine); f->setFrameShadow(QFrame::Sunken); r.setHeight(2); }
                    else if (tur == 0x11) { f->setFrameShape(QFrame::VLine); f->setFrameShadow(QFrame::Sunken); r.setWidth(2); }
                    else if (tur == 0x12) { f->setFrameShape(QFrame::Box); f->setFrameShadow(QFrame::Sunken); }
                    else f->setFrameShape(QFrame::Box);
                    w = f;
                } else if (tur == 0xD) {
                    w = new QWidget(this);
                } else {
                    auto *l = new QLabel(this);
                    const bool onekYok = st & 0x80;             // SS_NOPREFIX
                    l->setText(onekYok ? t : t);
                    l->setTextFormat(Qt::PlainText);
                    l->setWordWrap(tur != 0xB && tur != 0xC);
                    l->setAlignment((tur == 1 ? Qt::AlignHCenter : tur == 2 ? Qt::AlignRight : Qt::AlignLeft)
                                    | ((st & 0x200) ? Qt::AlignVCenter : Qt::AlignTop));   // SS_CENTERIMAGE
                    if (st & 0x1000) { l->setFrameShape(QFrame::Panel); l->setFrameShadow(QFrame::Sunken); }
                    if (!onekYok && t.contains('&')) bekleyenEtiket = l;
                    else l->setText(onekYok ? t : sade(t));
                    w = l;
                }
            } else if (s == "COMBOBOX" || s == "COMBOBOXEX32") {
                auto *cb = new QComboBox(this);
                cb->setEditable((st & 3) == 2 || (st & 3) == 1);
                r.setHeight(qMax(21, cb->fontMetrics().height() + 8));   // XP açılır kutusu: 21 px
                w = cb;
            } else if (s == "LISTBOX") {
                w = new QListWidget(this);
            } else if (s == "SCROLLBAR") {
                w = new QScrollBar((st & 1) ? Qt::Vertical : Qt::Horizontal, this);
            } else if (s == "SYSLISTVIEW32") {
                if ((st & 3) == 1) {                            // LVS_REPORT
                    auto *tw = new QTreeWidget(this);
                    tw->setRootIsDecorated(false);
                    tw->setUniformRowHeights(true);
                    tw->setHeaderHidden(st & 0x4000);             // LVS_NOCOLUMNHEADER
                    w = tw;
                } else {
                    auto *lw = new QListWidget(this);
                    lw->setViewMode((st & 3) == 0 ? QListView::IconMode : QListView::ListMode);
                    w = lw;
                }
            } else if (s == "SYSTREEVIEW32") {
                auto *tw = new QTreeWidget(this);
                tw->setHeaderHidden(true);
                w = tw;
            } else if (s == "SYSTABCONTROL32") {
                w = new QTabWidget(this);
            } else if (s == "MSCTLS_TRACKBAR32") {
                auto *sl = new QSlider((st & 2) ? Qt::Vertical : Qt::Horizontal, this);
                sl->setTickPosition((st & 1) ? QSlider::NoTicks : QSlider::TicksBelow);
                w = sl;
            } else if (s == "MSCTLS_PROGRESS32") {
                auto *pb = new QProgressBar(this);
                pb->setTextVisible(false);
                w = pb;
            } else if (s == "SYSMONTHCAL32") {
                w = new QCalendarWidget(this);
            } else if (s == "SYSDATETIMEPICK32") {
                auto *dt = (st & 0x9) == 0x9 ? new QTimeEdit(this) : new QDateTimeEdit(this);
                w = dt;
            } else if (s == "SYSLINK" || s == "LINK WINDOW") {
                auto *l = new QLabel(this);
                QString h = t;
                h.replace("<A>", "<a href=\"#\">", Qt::CaseInsensitive).replace("</A>", "</a>", Qt::CaseInsensitive);
                l->setText(h);
                l->setWordWrap(true);
                w = l;
            } else if (s.startsWith("RICHEDIT")) {
                auto *tb = new QTextBrowser(this);
                tb->setPlainText(t);
                w = tb;
            } else if (s == "MSCTLS_UPDOWN32") {
                w = new QWidget(this);                          // ortak eşi (buddy) program kodu düzenler
                w->hide();
            } else if (s == "MSCTLS_HOTKEY32") {
                w = new QKeySequenceEdit(this);
            } else {
                auto *f = new QFrame(this);                     // özel sınıf: program kodu doldurur
                f->setObjectName(sinif);
                w = f;
            }
            w->setGeometry(r);
            if (!(st & 0x10000000)) w->hide();                  // WS_VISIBLE yok
            if (st & 0x08000000) w->setEnabled(false);          // WS_DISABLED
            w->setProperty("xpId", id);
            w->setProperty("xpSinif", sinif);
            if (bekleyenEtiket && bekleyenEtiket != w) {        // "&Aç:" → sonraki denetim eşi
                bekleyenEtiket->setText(bekleyenEtiket->text());
                bekleyenEtiket->setBuddy(w);
                bekleyenEtiket = nullptr;
            }
            if (id != -1 && id != 0xFFFF && !kontrol.contains(id)) kontrol.insert(id, w);
            tumu << w;
        }
        if (bekleyenEtiket) bekleyenEtiket->setText(sade(bekleyenEtiket->text()));
        for (QWidget *g : kutular) g->lower();                  // grup kutuları içeriğin arkasında kalsın
    }

    QWidget *k(int id) const { return kontrol.value(id); }
    template <class T> T *k(int id) const { return qobject_cast<T *>(kontrol.value(id)); }

    // özel sınıflı bir denetimin yerine program parçacığı koy
    void yerlestir(int id, QWidget *yeni) {
        QWidget *eski = kontrol.value(id);
        if (!eski) return;
        yeni->setParent(this);
        yeni->setGeometry(eski->geometry());
        yeni->setVisible(!eski->isHidden());
        yeni->stackUnder(eski);
        eski->hide();
        kontrol.insert(id, yeni);
    }

    const Modul modul;
    QString baslik;
    QSize boyut;
    QHash<int, QWidget *> kontrol;
    QList<QWidget *> tumu;
};

// ---------------------------------------------------------------- menü
inline QHash<int, QAction *> menuKur(QWidget *hedef, const QJsonArray &ogeler, const std::function<void(int)> &tetik) {
    QHash<int, QAction *> eylemler;
    std::function<void(QWidget *, const QJsonArray &)> doldur = [&](QWidget *m, const QJsonArray &a) {
        for (const QJsonValue &v : a) {
            const QJsonObject o = v.toObject();
            const QString t = o.value("text").toString();
            if (o.contains("items")) {
                QMenu *alt = nullptr;
                if (auto *mb = qobject_cast<QMenuBar *>(m)) alt = mb->addMenu(t);
                else alt = static_cast<QMenu *>(m)->addMenu(t);
                alt->setEnabled(!o.value("grayed").toBool());
                doldur(alt, o.value("items").toArray());
            } else if (o.value("sep").toBool()) {
                if (auto *mn = qobject_cast<QMenu *>(m)) mn->addSeparator();
            } else {
                const int id = o.value("id").toInt();
                QAction *ac = new QAction(t, m);
                ac->setEnabled(!o.value("grayed").toBool());
                if (o.value("checked").toBool()) { ac->setCheckable(true); ac->setChecked(true); }
                QObject::connect(ac, &QAction::triggered, m, [tetik, id] { tetik(id); });
                m->addAction(ac);
                eylemler.insert(id, ac);
            }
        }
    };
    doldur(hedef, ogeler);
    return eylemler;
}

// ---------------------------------------------------------------- özellik sayfası (Tamam / İptal / Uygula)
class Ozellikler : public QDialog {
public:
    QTabWidget *sekmeler;
    QPushButton *tamam, *iptal, *uygula;
    std::function<bool()> uygulaIslevi;      // false = kapanma

    explicit Ozellikler(const QString &baslik, QWidget *ebeveyn = nullptr) : QDialog(ebeveyn) {
        setWindowTitle(baslik);
        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(dluX(4), dluY(4), dluX(4), dluY(4));
        lay->setSpacing(dluY(4));
        sekmeler = new QTabWidget;
        sekmeler->tabBar()->setUsesScrollButtons(false);   // XP: tüm sekmeler görünür (gerekirse pencere genişler)
        lay->addWidget(sekmeler);
        auto *alt = new QHBoxLayout;
        alt->setSpacing(dluX(4) - 2);
        alt->addStretch();
        tamam = new QPushButton("Tamam");
        iptal = new QPushButton("İptal");
        uygula = new QPushButton("&Uygula");
        for (auto *b : {tamam, iptal, uygula}) {
            b->setFixedSize(dluX(50), dluY(14));
            alt->addWidget(b);
        }
        tamam->setDefault(true);
        uygula->setEnabled(false);
        lay->addLayout(alt);
        connect(tamam, &QPushButton::clicked, this, [this] {
            if (!uygula->isEnabled() || !uygulaIslevi || uygulaIslevi()) accept();
        });
        connect(iptal, &QPushButton::clicked, this, &QDialog::reject);
        connect(uygula, &QPushButton::clicked, this, [this] {
            if (!uygulaIslevi || uygulaIslevi()) uygula->setEnabled(false);
        });
    }
    Sayfa *ekle(const Modul &m, int dlgId, const QString &baslik = {}) {
        auto *s = new Sayfa(m, dlgId);
        auto *kap = new QWidget;
        auto *l = new QVBoxLayout(kap);
        l->setContentsMargins(1, 1, 4, 4);
        l->addWidget(s, 0, Qt::AlignLeft | Qt::AlignTop);
        sekmeler->addTab(kap, baslik.isEmpty() ? s->baslik : baslik);
        return s;
    }
    void degisti() { uygula->setEnabled(true); }
    // sayfadaki tüm giriş denetimlerinin değişimi "Uygula"yı etkinleştirsin
    void izle(Sayfa *s) {
        for (QWidget *w : s->tumu) {
            if (auto *b = qobject_cast<QAbstractButton *>(w); b && b->isCheckable()) connect(b, &QAbstractButton::toggled, this, &Ozellikler::degisti);
            else if (auto *e = qobject_cast<QLineEdit *>(w)) connect(e, &QLineEdit::textEdited, this, &Ozellikler::degisti);
            else if (auto *c = qobject_cast<QComboBox *>(w)) connect(c, &QComboBox::activated, this, &Ozellikler::degisti);
            else if (auto *sl = qobject_cast<QSlider *>(w)) connect(sl, &QSlider::valueChanged, this, &Ozellikler::degisti);
        }
    }
};

// XP iletişim kutusu olarak tek sayfa (Çalıştır gibi)
class Iletisim : public QDialog {
public:
    Sayfa *sayfa;
    Iletisim(const Modul &m, int dlgId, QWidget *ebeveyn = nullptr) : QDialog(ebeveyn) {
        sayfa = new Sayfa(m, dlgId, this);
        setWindowTitle(sayfa->baslik);
        setFixedSize(sayfa->boyut);
        if (auto *t = sayfa->k<QPushButton>(1)) connect(t, &QPushButton::clicked, this, &QDialog::accept);
        if (auto *i = sayfa->k<QPushButton>(2)) connect(i, &QPushButton::clicked, this, &QDialog::reject);
    }
};

}  // namespace xp
