// XPThemePlasma — XP uygulamaları için ortak parçalar: görev bölmesi (ExplorerBar),
// XP araç çubuğu düğmeleri, adres çubuğu. Gerçek XP varlıkları yoksa düz çizime döner.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtWidgets>
#include <functional>

namespace xp {

inline QString varlik(const QString &ad) {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/xpthemeplasma/varlik/" + ad + ".png";
}

inline QPixmap pix(const QString &ad) {
    static QHash<QString, QPixmap> cache;
    if (!cache.contains(ad)) cache.insert(ad, QPixmap(varlik(ad)));
    return cache.value(ad);
}

// uxtheme "stretch" 9 dilim (l, r, t, b)
inline void dokuz(QPainter &p, const QRect &r, const QPixmap &src, int l, int rr, int t, int b) {
    if (src.isNull()) return;
    const int w = src.width(), h = src.height();
    const int xs[4] = {0, l, w - rr, w}, ys[4] = {0, t, h - b, h};
    const int xd[4] = {r.left(), r.left() + l, r.right() + 1 - rr, r.right() + 1};
    const int yd[4] = {r.top(), r.top() + t, r.bottom() + 1 - b, r.bottom() + 1};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            QRect s(xs[i], ys[j], xs[i + 1] - xs[i], ys[j + 1] - ys[j]);
            QRect d(xd[i], yd[j], xd[i + 1] - xd[i], yd[j + 1] - yd[j]);
            if (s.width() > 0 && s.height() > 0 && d.width() > 0 && d.height() > 0) p.drawPixmap(d, src, s);
        }
}

inline QIcon simge(const QString &ad) { return QIcon::fromTheme(ad, QIcon::fromTheme("application-x-executable")); }

// ---------------------------------------------------------------- görev bölmesi
class Baglanti : public QLabel {
public:
    Baglanti(const QIcon &ic, const QString &t, std::function<void()> f, const QColor &c, QWidget *p = nullptr)
        : QLabel(p), metin(t), eylem(std::move(f)), renk(c) {
        setCursor(Qt::PointingHandCursor);
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(0, 1, 0, 1);
        lay->setSpacing(6);
        if (!ic.isNull()) {
            auto *ikon = new QLabel;
            ikon->setPixmap(ic.pixmap(16, 16));
            ikon->setFixedSize(16, 16);
            lay->addWidget(ikon, 0, Qt::AlignTop);
        }
        yazi = new QLabel(t);
        yazi->setWordWrap(true);
        yazi->setFont(QFont("Tahoma", 8));
        lay->addWidget(yazi, 1);
        boya(false);
    }
protected:
    void enterEvent(QEnterEvent *) override { boya(true); }
    void leaveEvent(QEvent *) override { boya(false); }
    void mouseReleaseEvent(QMouseEvent *e) override { if (e->button() == Qt::LeftButton && eylem) eylem(); }
private:
    void boya(bool ust) {
        QColor c = ust ? renk.lighter(140) : renk;
        yazi->setStyleSheet(QString("color:%1;%2").arg(c.name(), ust ? "text-decoration:underline;" : ""));
    }
    QString metin;
    QLabel *yazi;
    std::function<void()> eylem;
    QColor renk;
};

class GorevGrubu : public QWidget {
public:
    GorevGrubu(const QString &baslik, bool ozel, QWidget *p = nullptr) : QWidget(p), baslikMetni(baslik), ozel(ozel) {
        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);
        bas = new QWidget;
        bas->setFixedHeight(25);
        bas->installEventFilter(this);
        bas->setCursor(Qt::PointingHandCursor);
        lay->addWidget(bas);
        govde = new QWidget;
        govdeLay = new QVBoxLayout(govde);
        govdeLay->setContentsMargins(12, 8, 10, 10);
        govdeLay->setSpacing(4);
        govde->installEventFilter(this);
        lay->addWidget(govde);
    }
    void baglanti(const QString &ikon, const QString &t, std::function<void()> f) {
        govdeLay->addWidget(new Baglanti(ikon.isEmpty() ? QIcon() : simge(ikon), t, std::move(f), QColor("#215DC6")));
    }
    void bilgi(QWidget *w) { govdeLay->addWidget(w); }
protected:
    bool eventFilter(QObject *o, QEvent *e) override {
        if (o == bas && e->type() == QEvent::Paint) {
            QPainter p(bas);
            QPixmap h = pix(ozel ? "eb-special-head" : "eb-normal-head");
            if (!h.isNull()) dokuz(p, bas->rect(), h, 3, ozel ? 106 : 106, 3, 1);
            else p.fillRect(bas->rect(), ozel ? QColor("#0248B2") : QColor("#FFFFFF"));
            QFont f("Tahoma", 8);
            f.setBold(true);
            p.setFont(f);
            p.setPen(ozel ? (ustte ? QColor("#428EFF") : Qt::white) : (ustte ? QColor("#428EFF") : QColor("#215DC6")));
            if (ozel && !ustte) p.setPen(Qt::white);
            p.drawText(bas->rect().adjusted(12, 0, -28, 0), Qt::AlignVCenter | Qt::AlignLeft, baslikMetni);
            QString d = QString("eb-%1-%2-%3").arg(ozel ? "special" : "normal", acik ? "collapse" : "expand").arg(ustte ? 1 : 0);
            QPixmap b = pix(d);
            if (!b.isNull()) p.drawPixmap(bas->width() - b.width() - 3, (bas->height() - b.height()) / 2 + 1, b);
            return true;
        }
        if (o == govde && e->type() == QEvent::Paint) {
            QPainter p(govde);
            QPixmap g = pix(ozel ? "eb-special-bg" : "eb-normal-bg");
            if (!g.isNull()) dokuz(p, govde->rect(), g, 3, 3, 3, 3);
            else { p.fillRect(govde->rect(), QColor("#D6DFF7")); p.setPen(Qt::white); p.drawRect(govde->rect().adjusted(0, 0, -1, -1)); }
            return false;
        }
        if (o == bas && e->type() == QEvent::Enter) { ustte = true; bas->update(); }
        if (o == bas && e->type() == QEvent::Leave) { ustte = false; bas->update(); }
        if (o == bas && e->type() == QEvent::MouseButtonRelease) { acik = !acik; govde->setVisible(acik); bas->update(); }
        return QWidget::eventFilter(o, e);
    }
private:
    QString baslikMetni;
    bool ozel, acik = true, ustte = false;
    QWidget *bas, *govde;
    QVBoxLayout *govdeLay;
};

class GorevBolmesi : public QScrollArea {
public:
    explicit GorevBolmesi(QWidget *p = nullptr) : QScrollArea(p) {
        setFrameShape(QFrame::NoFrame);
        setWidgetResizable(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFixedWidth(210);
        ic = new QWidget;
        ic->setAutoFillBackground(false);
        lay = new QVBoxLayout(ic);
        lay->setContentsMargins(12, 12, 12, 12);
        lay->setSpacing(15);
        lay->addStretch();
        setWidget(ic);
        viewport()->setAutoFillBackground(false);
        ic->installEventFilter(this);
    }
    GorevGrubu *grup(const QString &baslik, bool ozel = false) {
        auto *g = new GorevGrubu(baslik, ozel);
        lay->insertWidget(lay->count() - 1, g);
        return g;
    }
    void temizle() {
        while (lay->count() > 1) {
            auto *it = lay->takeAt(0);
            delete it->widget();
            delete it;
        }
    }
protected:
    bool eventFilter(QObject *o, QEvent *e) override {
        if (o == ic && e->type() == QEvent::Paint) {
            QPainter p(ic);
            QLinearGradient g(0, 0, 0, ic->height());
            g.setColorAt(0, QColor("#7BA2E7"));
            g.setColorAt(1, QColor("#6375D6"));
            p.fillRect(ic->rect(), g);
        }
        return false;
    }
private:
    QWidget *ic;
    QVBoxLayout *lay;
};

// ---------------------------------------------------------------- araç çubuğu
// shell32 214 şeridi (24 px): 0 Geri, 1 İleri, 17 Ara, 28 Yukarı, 32 Görünümler, 43 Klasörler
inline QIcon seritSimge(int i, const QString &yedek) {
    QPixmap n = pix("tb-gezgin"), h = pix("tb-gezgin-hot");
    if (n.isNull()) return simge(yedek);
    QIcon ic;
    ic.addPixmap(n.copy(i * 24, 0, 24, 24), QIcon::Normal);
    if (!h.isNull()) ic.addPixmap(h.copy(i * 24, 0, 24, 24), QIcon::Active);
    QImage gri = n.copy(i * 24, 0, 24, 24).toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < gri.height(); ++y)
        for (int x = 0; x < gri.width(); ++x) {
            QColor c = gri.pixelColor(x, y);
            int v = qGray(c.rgb());
            gri.setPixelColor(x, y, QColor(v, v, v, c.alpha() / 2));
        }
    ic.addPixmap(QPixmap::fromImage(gri), QIcon::Disabled);
    return ic;
}

inline QToolButton *aracDugmesi(const QIcon &ic, const QString &metin, bool yaziYaninda) {
    auto *b = new QToolButton;
    b->setIcon(ic);
    b->setIconSize(QSize(24, 24));
    b->setText(metin);
    b->setToolTip(metin);
    b->setAutoRaise(true);
    b->setToolButtonStyle(yaziYaninda ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    return b;
}

// XP Gezgini kabuğu: menü + araç çubuğu + adres çubuğu + sol görev bölmesi + içerik
struct Kabuk {
    QToolButton *geri, *ileri, *yukari, *ara, *klasorler, *gorunumler;
    QComboBox *adres;
    QLabel *adresIkon;
    GorevBolmesi *bolme;
    QWidget *icerik;
    QHBoxLayout *govde;
};

inline Kabuk kabukKur(QMainWindow *w, const QStringList &menuler) {
    Kabuk k;
    for (const auto &m : menuler) w->menuBar()->addMenu(m);
    auto *tb = new QToolBar("Standart Düğmeler");
    tb->setMovable(false);
    tb->setIconSize(QSize(24, 24));
    k.geri = aracDugmesi(seritSimge(0, "go-previous"), "Geri", true);
    k.ileri = aracDugmesi(seritSimge(1, "go-next"), "İleri", false);
    k.yukari = aracDugmesi(seritSimge(28, "go-up"), "Yukarı", false);
    k.ara = aracDugmesi(seritSimge(17, "system-search"), "Ara", true);
    k.klasorler = aracDugmesi(seritSimge(43, "view-sidetree"), "Klasörler", true);
    k.gorunumler = aracDugmesi(seritSimge(32, "view-choose"), "Görünümler", false);
    k.gorunumler->setPopupMode(QToolButton::InstantPopup);
    k.klasorler->setCheckable(true);
    tb->addWidget(k.geri);
    tb->addWidget(k.ileri);
    tb->addWidget(k.yukari);
    tb->addSeparator();
    tb->addWidget(k.ara);
    tb->addWidget(k.klasorler);
    tb->addSeparator();
    tb->addWidget(k.gorunumler);
    w->addToolBar(tb);
    w->addToolBarBreak();
    auto *ab = new QToolBar("Adres Çubuğu");
    ab->setMovable(false);
    auto *aw = new QWidget;
    auto *al = new QHBoxLayout(aw);
    al->setContentsMargins(4, 1, 4, 1);
    al->setSpacing(4);
    auto *lbl = new QLabel("Adres");
    lbl->setStyleSheet("color:#7F7C73;");
    al->addWidget(lbl);
    k.adres = new QComboBox;
    k.adres->setEditable(true);
    k.adres->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    al->addWidget(k.adres, 1);
    auto *git = new QToolButton;
    git->setText("Git");
    git->setIcon(simge("go-next"));
    git->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    git->setAutoRaise(true);
    QObject::connect(git, &QToolButton::clicked, k.adres->lineEdit(), &QLineEdit::returnPressed);
    al->addWidget(git);
    ab->addWidget(aw);
    w->addToolBar(ab);
    k.adresIkon = nullptr;
    auto *merkez = new QWidget;
    k.govde = new QHBoxLayout(merkez);
    k.govde->setContentsMargins(0, 0, 0, 0);
    k.govde->setSpacing(0);
    k.bolme = new GorevBolmesi;
    k.govde->addWidget(k.bolme);
    k.icerik = nullptr;
    w->setCentralWidget(merkez);
    return k;
}

}  // namespace xp
