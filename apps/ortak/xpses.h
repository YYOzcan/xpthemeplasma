// XPThemePlasma — ses aygıtları ve akışları (PulseAudio / PipeWire, pactl üzerinden)
// pactl'ın JSON çıktısı (pactl -f json) KDE'den bağımsız ve yıllardır kararlıdır; XP ses programları
// (Ses Ayarı Denetimi, Ses ve Ses Aygıtları Özellikleri) yalnızca bunu kullanır.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtCore>

namespace xp::ses {

struct Aygit {
    int dizin = -1;
    QString ad, aciklama, uygulama, simge;
    bool sessiz = false;
    int sol = 0, sag = 0;            // yüzde
    int kanal = 2;
    int yuzde() const { return qMax(sol, sag); }
    int denge() const {              // -100 (sol) .. 100 (sağ)
        const int v = yuzde();
        if (v == 0 || kanal < 2) return 0;
        return sol >= sag ? -qRound(100.0 * (v - sag) / v) : qRound(100.0 * (v - sol) / v);
    }
};

inline QByteArray pactl(const QStringList &a, int ms = 4000) {
    QProcess p;
    QProcessEnvironment e = QProcessEnvironment::systemEnvironment();
    e.insert("LC_ALL", "C");
    p.setProcessEnvironment(e);
    p.start("pactl", a);
    p.waitForFinished(ms);
    return p.readAllStandardOutput();
}

inline Aygit coz(const QJsonObject &o) {
    Aygit a;
    a.dizin = o.value("index").toInt();
    a.ad = o.value("name").toString();
    a.aciklama = o.value("description").toString();
    a.sessiz = o.value("mute").toBool();
    const QJsonObject oz = o.value("properties").toObject();
    a.uygulama = oz.value("application.name").toString();
    a.simge = oz.value("application.icon_name").toString();
    if (a.aciklama.isEmpty()) a.aciklama = oz.value("media.name").toString();
    const QJsonObject v = o.value("volume").toObject();
    QList<int> yuzdeler;
    QStringList anahtarlar = v.keys();
    for (const QString &k : anahtarlar) yuzdeler << v.value(k).toObject().value("value_percent").toString().remove('%').trimmed().toInt();
    a.kanal = yuzdeler.size();
    if (yuzdeler.isEmpty()) return a;
    // front-left / front-right; tek kanal ise ikisi aynı
    const int il = anahtarlar.indexOf("front-left"), ir = anahtarlar.indexOf("front-right");
    a.sol = yuzdeler.value(il >= 0 ? il : 0);
    a.sag = yuzdeler.value(ir >= 0 ? ir : yuzdeler.size() - 1);
    return a;
}

inline QList<Aygit> liste(const QString &tur) {        // sinks | sources | sink-inputs
    QList<Aygit> l;
    for (const QJsonValue &v : QJsonDocument::fromJson(pactl({"-f", "json", "list", tur})).array()) {
        const Aygit a = coz(v.toObject());
        if (tur == "sources" && a.ad.endsWith(".monitor")) continue;     // XP'de "monitör" girişleri yoktu
        l << a;
    }
    return l;
}

inline QString varsayilan(bool cikis) {
    return QString::fromUtf8(pactl({cikis ? "get-default-sink" : "get-default-source"})).trimmed();
}
inline Aygit varsayilanAygit(bool cikis = true) {
    const QString ad = varsayilan(cikis);
    for (const Aygit &a : liste(cikis ? "sinks" : "sources")) if (a.ad == ad) return a;
    return {};
}

// tür: sink | source | sink-input ; hedef: ad ya da dizin
inline void sesAyarla(const QString &tur, const QString &hedef, int yuzde, int denge = 0, int kanal = 2) {
    yuzde = qBound(0, yuzde, 100);
    const int sol = denge > 0 ? qRound(yuzde * (100 - denge) / 100.0) : yuzde;
    const int sag = denge < 0 ? qRound(yuzde * (100 + denge) / 100.0) : yuzde;
    QStringList a{"set-" + tur + "-volume", hedef};
    if (kanal >= 2) a << QString::number(sol) + "%" << QString::number(sag) + "%";
    else a << QString::number(yuzde) + "%";
    QProcess::startDetached("pactl", a);
}
inline void sessizAyarla(const QString &tur, const QString &hedef, bool s) {
    QProcess::startDetached("pactl", {"set-" + tur + "-mute", hedef, s ? "1" : "0"});
}

// pactl subscribe: ses sunucusunda bir şey değişince bildirir (yoklamadan)
class Izleyici : public QObject {
public:
    std::function<void()> degisti;
    Izleyici(QObject *e = nullptr) : QObject(e) {
        p.setProcessEnvironment([] { auto e = QProcessEnvironment::systemEnvironment(); e.insert("LC_ALL", "C"); return e; }());
        connect(&p, &QProcess::readyReadStandardOutput, this, [this] { p.readAll(); zaman.start(120); });
        zaman.setSingleShot(true);
        connect(&zaman, &QTimer::timeout, this, [this] { if (degisti) degisti(); });
        p.start("pactl", {"subscribe"});
    }
    ~Izleyici() override { p.kill(); p.waitForFinished(500); }
private:
    QProcess p;
    QTimer zaman;
};

inline void bip() {
    const QString f = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/sounds/Windows XP/stereo/bell.wav";
    if (QFileInfo::exists(f)) QProcess::startDetached("pw-play", {f});
}

}  // namespace xp::ses
