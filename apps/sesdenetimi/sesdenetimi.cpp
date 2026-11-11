// XPThemePlasma — XP "Ses Ayarı Denetimi" (sndvol32; bildirim alanındaki hoparlöre çift tıklama)
// Sütunlar kendi XP kopyandaki sndvol32.exe'nin iletişim kutularından: ilki varsayılan çıkış aygıtı
// (Denge, Ses Ayarı, Tümü sessiz), ardından ses çalan her program (Denge, Ses Ayarı, Sessiz).
// Ses sunucusu pactl ile denetlenir; değişiklikler pactl subscribe ile anında yansır.
//   xp-sesdenetimi          çalma denetimleri       xp-sesdenetimi --kayit    kayıt denetimleri
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"
#include "../ortak/xpses.h"

static xp::Modul SV;
static QString S(int id, const char *y) { return SV.dize(id, QString::fromUtf8(y)); }

class Sutun : public QWidget {
public:
    QString tur, hedef;              // sink | source | sink-input
    xp::ses::Aygit aygit;
    xp::Sayfa *sayfa;
    QSlider *denge, *ses;
    QCheckBox *sessiz;
    QLabel *ad;
    bool guncelleniyor = false;

    Sutun(const xp::ses::Aygit &a, const QString &t, bool ana, const QString &etiket) : tur(t), aygit(a) {
        hedef = t == "sink-input" ? QString::number(a.dizin) : a.ad;
        sayfa = new xp::Sayfa(SV, ana ? 101 : 102, this);
        setFixedSize(sayfa->boyut);
        ad = sayfa->k<QLabel>(1004);
        ad->setText(etiket);
        ad->setToolTip(etiket);
        if (auto *l = sayfa->k<QLabel>(1026)) l->setPixmap(SV.simge(301, 16));
        if (auto *l = sayfa->k<QLabel>(1027)) l->setPixmap(SV.simge(302, 16));
        if (auto *w = sayfa->k(1005)) w->hide();                  // VU ölçer
        denge = sayfa->k<QSlider>(1002);
        ses = sayfa->k<QSlider>(1001);
        sessiz = sayfa->k<QCheckBox>(1000);
        denge->setRange(-100, 100);
        denge->setPageStep(25);
        denge->setTickInterval(25);
        denge->setTickPosition(QSlider::TicksBelow);
        ses->setOrientation(Qt::Vertical);
        ses->setRange(0, 100);
        ses->setPageStep(10);
        ses->setTickInterval(10);
        ses->setTickPosition(QSlider::TicksBothSides);
        denge->setEnabled(a.kanal >= 2);
        goster(a);
        auto uygula = [this] {
            if (guncelleniyor) return;
            xp::ses::sesAyarla(tur, hedef, ses->value(), denge->value(), aygit.kanal);
        };
        connect(ses, &QSlider::valueChanged, this, uygula);
        connect(denge, &QSlider::valueChanged, this, uygula);
        connect(ses, &QSlider::sliderReleased, this, [] { xp::ses::bip(); });
        connect(sessiz, &QCheckBox::toggled, this, [this](bool s) { if (!guncelleniyor) xp::ses::sessizAyarla(tur, hedef, s); });
    }
    void goster(const xp::ses::Aygit &a) {
        if (ses->isSliderDown() || denge->isSliderDown()) return;
        guncelleniyor = true;
        aygit = a;
        ses->setValue(a.yuzde());
        denge->setValue(a.denge());
        sessiz->setChecked(a.sessiz);
        guncelleniyor = false;
    }
};

class Karistirici : public QMainWindow {
public:
    explicit Karistirici(bool kayit) : kayit(kayit) {
        setWindowTitle(S(203, "Ses Ayarı Denetimi"));
        setWindowIcon(QIcon(SV.simge(300, 16)));
        auto *orta = new QWidget;
        satir = new QHBoxLayout(orta);
        satir->setContentsMargins(0, 0, 0, 0);
        satir->setSpacing(0);
        setCentralWidget(orta);
        durum = new QLabel;
        statusBar()->addWidget(durum, 1);
        statusBar()->setSizeGripEnabled(false);
        xp::menuKur(menuBar(), SV.menu(150), [this](int id) {
            if (id == 5) close();
            else if (id == 21) QProcess::startDetached(QDir::homePath() + "/.local/bin/xp-ses", {"2"});
            else if (id == 11) QProcess::startDetached(QDir::homePath() + "/.local/bin/xp-hakkinda", {S(203, "Ses Ayarı Denetimi")});
        });
        for (QAction *a : menuBar()->actions())
            for (QAction *b : a->menu() ? a->menu()->actions() : QList<QAction *>()) if (b->text().contains("Gelişmiş") || b->text().contains("Yardım Konuları")) b->setEnabled(false);
        izle = new xp::ses::Izleyici(this);
        izle->degisti = [this] { yenile(); };
        yenile();
    }

private:
    void yenile() {
        const xp::ses::Aygit ana = xp::ses::varsayilanAygit(!kayit);
        QList<xp::ses::Aygit> akislar = kayit ? QList<xp::ses::Aygit>() : xp::ses::liste("sink-inputs");
        QStringList anahtarlar{ana.ad};
        for (const auto &a : akislar) anahtarlar << QString::number(a.dizin);
        if (anahtarlar != oncekiAnahtarlar) {
            // sütunlar değişti: yeniden kur
            while (QLayoutItem *i = satir->takeAt(0)) { if (i->widget()) i->widget()->deleteLater(); delete i; }
            sutunlar.clear();
            if (!ana.ad.isEmpty()) {
                auto *s = new Sutun(ana, kayit ? "source" : "sink", true, kayit ? S(206, "Kayıt") : S(203, "Ses Ayarı Denetimi"));
                satir->addWidget(s);
                sutunlar << s;
            }
            for (const auto &a : akislar) {
                QString et = a.uygulama.isEmpty() ? a.aciklama : a.uygulama;
                auto *s = new Sutun(a, "sink-input", false, et);
                satir->addWidget(s);
                sutunlar << s;
            }
            oncekiAnahtarlar = anahtarlar;
            durum->setText(ana.aciklama);
            adjustSize();
            setFixedSize(sizeHint());
        } else {
            if (!sutunlar.isEmpty()) sutunlar[0]->goster(ana);
            for (int i = 0; i < akislar.size() && i + 1 < sutunlar.size(); ++i) sutunlar[i + 1]->goster(akislar[i]);
        }
    }

    bool kayit;
    QHBoxLayout *satir;
    QLabel *durum;
    QList<Sutun *> sutunlar;
    QStringList oncekiAnahtarlar;
    xp::ses::Izleyici *izle;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-sesdenetimi");
    SV = xp::Modul::yukle("sndvol32-exe");
    if (!SV.gecerli()) return QProcess::startDetached("kcmshell6", {"kcm_pulseaudio"}) ? 0 : 1;
    Karistirici k(app.arguments().contains("--kayit"));
    k.show();
    return app.exec();
}
