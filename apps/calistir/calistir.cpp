// XPThemePlasma — XP "Çalıştır" penceresi (Başlat > Çalıştır..., Windows+R)
// Yerleşim ve metinler kendi XP kopyandaki shell32.dll'nin 1003 numaralı iletişim kutusundan.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../ortak/xpdlg.h"
#include "../ortak/xpkomut.h"

static QString gecmisDosyasi() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) + "/xpthemeplasma/calistir.txt";
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setDesktopFileName("xp-calistir");
    const xp::Modul shell = xp::Modul::yukle("shell32-dll");
    if (!shell.gecerli()) {
        // XP varlığı yok: KDE'nin çalıştırıcısına devret
        return QProcess::startDetached("krunner", {}) ? 0 : 1;
    }
    xp::Iletisim d(shell, 1003);
    d.setWindowIcon(QIcon(shell.simge(160, 16)));
    d.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    auto *s = d.sayfa;
    if (auto *ayri = s->k(12306)) ayri->hide();          // "Ayrı Bir Bellek Alanında Çalıştır": yalnızca 16 bit
    auto *kutu = s->k<QComboBox>(12298);
    auto *tamam = s->k<QPushButton>(1);
    auto *gozat = s->k<QPushButton>(12288);
    kutu->setInsertPolicy(QComboBox::NoInsert);
    kutu->setMaxCount(26);                                // XP: a-z MRU listesi
    QFile f(gecmisDosyasi());
    if (f.open(QIODevice::ReadOnly)) {
        const QStringList satirlar = QString::fromUtf8(f.readAll()).split('\n', Qt::SkipEmptyParts);
        kutu->addItems(satirlar);
    }
    kutu->setCurrentIndex(0);
    kutu->lineEdit()->selectAll();
    tamam->setEnabled(!kutu->currentText().isEmpty());
    QObject::connect(kutu, &QComboBox::editTextChanged, tamam, [tamam](const QString &t) { tamam->setEnabled(!t.trimmed().isEmpty()); });
    tamam->setDefault(true);

    QObject::connect(gozat, &QPushButton::clicked, &d, [&] {
        QString filtre = shell.dize(9014, "Programlar#*#Tüm Dosyalar#*#");
        QStringList p = filtre.split('#');
        QString qf;
        for (int i = 0; i + 1 < p.size(); i += 2)
            if (!p[i].isEmpty()) qf += (qf.isEmpty() ? "" : ";;") + p[i] + " (" + (i == 0 ? QString("*") : p[i + 1].replace(';', ' ')) + ")";
        const QString yol = QFileDialog::getOpenFileName(&d, shell.dize(9015, "Gözat"), QDir::homePath(), qf);
        if (!yol.isEmpty()) {
            kutu->setEditText(yol.contains(' ') ? '"' + yol + '"' : yol);
            tamam->setFocus();
        }
    });

    QObject::disconnect(tamam, nullptr, &d, nullptr);
    QObject::connect(tamam, &QPushButton::clicked, &d, [&] {
        const QString komut = kutu->currentText().trimmed();
        if (!xp::calistir(komut)) {
            QString ileti = shell.dize(6211, "Windows '%1!ls!' öğesini bulamıyor.");
            ileti.replace("%1!ls!", komut);
            QMessageBox::critical(&d, komut, ileti);
            kutu->lineEdit()->selectAll();
            kutu->setFocus();
            return;
        }
        QStringList gecmis{komut};
        for (int i = 0; i < kutu->count(); ++i)
            if (kutu->itemText(i).compare(komut, Qt::CaseInsensitive) != 0) gecmis << kutu->itemText(i);
        QDir().mkpath(QFileInfo(gecmisDosyasi()).path());
        QFile g(gecmisDosyasi());
        if (g.open(QIODevice::WriteOnly | QIODevice::Truncate)) g.write(gecmis.mid(0, 26).join('\n').toUtf8());
        d.accept();
    });

    // XP: ekranın sol altında, görev çubuğunun hemen üstünde açılırdı
    if (QScreen *ekran = QGuiApplication::primaryScreen()) {
        const QRect r = ekran->availableGeometry();
        d.move(r.left() + 6, r.bottom() - d.height() - 36);
    }
    d.show();
    kutu->setFocus();
    return d.exec() == QDialog::Accepted ? 0 : 1;
}
