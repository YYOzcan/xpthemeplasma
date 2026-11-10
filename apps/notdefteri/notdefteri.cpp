// XPThemePlasma — Windows XP "Not Defteri" (Notepad) benzeri basit metin düzenleyici
// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtWidgets>
#include <QtPrintSupport>

class NotDefteri : public QMainWindow {
public:
    NotDefteri() {
        ed = new QPlainTextEdit;
        ed->setFrameShape(QFrame::NoFrame);
        QFont f("Lucida Console", 10);
        f.setStyleHint(QFont::Monospace);
        ed->setFont(QSettings().value("yaziTipi", f).value<QFont>());
        setCentralWidget(ed);
        resize(QSettings().value("boyut", QSize(640, 480)).toSize());
        setWindowIcon(QIcon::fromTheme("accessories-text-editor"));

        durum = new QLabel;
        statusBar()->addPermanentWidget(durum);
        statusBar()->setVisible(QSettings().value("durumCubugu", false).toBool());

        auto *dosya = menuBar()->addMenu("&Dosya");
        eylem(dosya, "&Yeni", QKeySequence::New, [this] { if (sor()) { ed->clear(); ad.clear(); baslik(); } });
        eylem(dosya, "&Aç...", QKeySequence::Open, [this] { ac(); });
        eylem(dosya, "&Kaydet", QKeySequence::Save, [this] { kaydet(false); });
        eylem(dosya, "&Farklı Kaydet...", {}, [this] { kaydet(true); });
        dosya->addSeparator();
        eylem(dosya, "&Sayfa Yapısı...", {}, [this] { QPageSetupDialog(&yazici, this).exec(); });
        eylem(dosya, "Ya&zdır...", QKeySequence::Print, [this] {
            QPrintDialog d(&yazici, this);
            if (d.exec() == QDialog::Accepted) ed->print(&yazici);
        });
        dosya->addSeparator();
        eylem(dosya, "&Çıkış", {}, [this] { close(); });

        auto *duzen = menuBar()->addMenu("Dü&zen");
        geriAl = eylem(duzen, "&Geri Al", QKeySequence::Undo, [this] { ed->undo(); });
        duzen->addSeparator();
        kes = eylem(duzen, "&Kes", QKeySequence::Cut, [this] { ed->cut(); });
        kopyala = eylem(duzen, "K&opyala", QKeySequence::Copy, [this] { ed->copy(); });
        eylem(duzen, "Y&apıştır", QKeySequence::Paste, [this] { ed->paste(); });
        sil = eylem(duzen, "&Sil", QKeySequence::Delete, [this] { ed->textCursor().removeSelectedText(); });
        duzen->addSeparator();
        eylem(duzen, "&Bul...", QKeySequence::Find, [this] { bul(false); });
        eylem(duzen, "Son&rakini Bul", QKeySequence(Qt::Key_F3), [this] { bul(true); });
        eylem(duzen, "&Değiştir...", QKeySequence(Qt::CTRL | Qt::Key_H), [this] { degistir(); });
        git = eylem(duzen, "G&it...", QKeySequence(Qt::CTRL | Qt::Key_G), [this] { satiraGit(); });
        duzen->addSeparator();
        eylem(duzen, "&Tümünü Seç", QKeySequence::SelectAll, [this] { ed->selectAll(); });
        eylem(duzen, "Saat/Tari&h", QKeySequence(Qt::Key_F5), [this] {
            ed->insertPlainText(QDateTime::currentDateTime().toString("HH:mm dd.MM.yyyy"));
        });

        auto *bicim = menuBar()->addMenu("&Biçim");
        kaydir = eylem(bicim, "&Sözcük Kaydır", {}, [this] { sarmaAyarla(kaydir->isChecked()); });
        kaydir->setCheckable(true);
        eylem(bicim, "&Yazı Tipi...", {}, [this] {
            bool ok;
            QFont f = QFontDialog::getFont(&ok, ed->font(), this, "Yazı Tipi");
            if (ok) { ed->setFont(f); QSettings().setValue("yaziTipi", f); }
        });

        auto *gorunum = menuBar()->addMenu("&Görünüm");
        durumEylem = eylem(gorunum, "&Durum çubuğu", {}, [this] {
            statusBar()->setVisible(durumEylem->isChecked());
            QSettings().setValue("durumCubugu", durumEylem->isChecked());
        });
        durumEylem->setCheckable(true);
        durumEylem->setChecked(statusBar()->isVisible());

        auto *yardim = menuBar()->addMenu("&Yardım");
        eylem(yardim, "&Yardım Konuları", {}, [] { QDesktopServices::openUrl(QUrl("help:/")); });
        yardim->addSeparator();
        eylem(yardim, "Not Defteri &Hakkında", {}, [this] {
            QMessageBox::about(this, "Not Defteri Hakkında",
                               "<b>Not Defteri</b><br>XPThemePlasma<br><br>Windows XP Not Defteri benzeri metin düzenleyici.");
        });

        connect(ed->document(), &QTextDocument::modificationChanged, this, [this] { baslik(); });
        connect(ed, &QPlainTextEdit::cursorPositionChanged, this, [this] { konum(); });
        connect(duzen, &QMenu::aboutToShow, this, [this] {
            bool secim = ed->textCursor().hasSelection();
            kes->setEnabled(secim); kopyala->setEnabled(secim); sil->setEnabled(secim);
            geriAl->setEnabled(ed->document()->isUndoAvailable());
        });
        sarmaAyarla(QSettings().value("sozcukKaydir", false).toBool());
        baslik();
        konum();
    }

    void dosyaYukle(const QString &yol) {
        QFile f(yol);
        if (!f.open(QIODevice::ReadOnly)) {
            if (!QFileInfo::exists(yol)) {
                auto c = QMessageBox::question(this, "Not Defteri",
                    QString("%1 dosyası bulunamıyor.\n\nYeni bir dosya yaratmak ister misiniz?").arg(QFileInfo(yol).fileName()),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                if (c == QMessageBox::Yes) { ad = yol; baslik(); }
                return;
            }
            QMessageBox::warning(this, "Not Defteri", QString("%1 dosyası açılamıyor.").arg(yol));
            return;
        }
        ed->setPlainText(QString::fromUtf8(f.readAll()));
        ed->document()->setModified(false);
        ad = yol;
        baslik();
    }

protected:
    void closeEvent(QCloseEvent *e) override {
        if (sor()) { QSettings().setValue("boyut", size()); e->accept(); } else e->ignore();
    }

private:
    QPlainTextEdit *ed;
    QLabel *durum;
    QString ad;
    QString aranan;
    QPrinter yazici;
    QAction *geriAl, *kes, *kopyala, *sil, *git, *kaydir, *durumEylem;

    template <typename F>
    QAction *eylem(QMenu *m, const QString &t, const QKeySequence &k, F f) {
        auto *a = m->addAction(t);
        if (!k.isEmpty()) a->setShortcut(k);
        connect(a, &QAction::triggered, this, f);
        return a;
    }

    QString kisaAd() const { return ad.isEmpty() ? QStringLiteral("Adsız") : QFileInfo(ad).fileName(); }
    void baslik() { setWindowTitle(kisaAd() + " - Not Defteri"); }

    void konum() {
        auto c = ed->textCursor();
        durum->setText(QString("   St %1, Stn %2  ").arg(c.blockNumber() + 1).arg(c.positionInBlock() + 1));
    }

    void sarmaAyarla(bool on) {
        // XP: Sözcük Kaydır açıkken Git ve Durum çubuğu devre dışıdır
        kaydir->setChecked(on);
        ed->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
        git->setEnabled(!on);
        durumEylem->setEnabled(!on);
        statusBar()->setVisible(!on && durumEylem->isChecked());
        QSettings().setValue("sozcukKaydir", on);
    }

    bool sor() {
        if (!ed->document()->isModified()) return true;
        QMessageBox m(QMessageBox::Warning, "Not Defteri",
                      QString("%1 dosyasındaki metin değişmiş.\n\nDeğişiklikleri kaydetmek istiyor musunuz?").arg(kisaAd()),
                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
        m.button(QMessageBox::Yes)->setText("&Evet");
        m.button(QMessageBox::No)->setText("&Hayır");
        m.button(QMessageBox::Cancel)->setText("İptal");
        int r = m.exec();
        if (r == QMessageBox::Yes) return kaydet(false);
        return r == QMessageBox::No;
    }

    void ac() {
        if (!sor()) return;
        QString y = QFileDialog::getOpenFileName(this, "Aç", QFileInfo(ad).path(),
                                                 "Metin Belgeleri (*.txt);;Tüm Dosyalar (*)");
        if (!y.isEmpty()) dosyaYukle(y);
    }

    bool kaydet(bool farkli) {
        QString y = ad;
        if (farkli || y.isEmpty()) {
            y = QFileDialog::getSaveFileName(this, "Farklı Kaydet",
                                             ad.isEmpty() ? QDir::homePath() + "/Adsız.txt" : ad,
                                             "Metin Belgeleri (*.txt);;Tüm Dosyalar (*)");
            if (y.isEmpty()) return false;
        }
        QSaveFile f(y);
        if (!f.open(QIODevice::WriteOnly) || f.write(ed->toPlainText().toUtf8()) < 0 || !f.commit()) {
            QMessageBox::warning(this, "Not Defteri",
                                 QString("%1 dosyası yaratılamıyor.\n\nYolun ve dosya adının doğru olduğundan emin olun.").arg(y));
            return false;
        }
        ad = y;
        ed->document()->setModified(false);
        baslik();
        return true;
    }

    void bul(bool sonraki) {
        if (!sonraki || aranan.isEmpty()) {
            bool ok;
            QString t = QInputDialog::getText(this, "Bul", "&Aranan:", QLineEdit::Normal, aranan, &ok);
            if (!ok || t.isEmpty()) return;
            aranan = t;
        }
        if (!ed->find(aranan)) {
            QMessageBox::information(this, "Not Defteri", QString("\"%1\" bulunamıyor").arg(aranan));
        }
    }

    void degistir() {
        QDialog d(this);
        d.setWindowTitle("Değiştir");
        auto *aranacak = new QLineEdit(aranan), *yeni = new QLineEdit;
        auto *form = new QFormLayout;
        form->addRow("&Aranan:", aranacak);
        form->addRow("Ye&ni değer:", yeni);
        auto *btn = new QDialogButtonBox;
        auto *tek = btn->addButton("&Değiştir", QDialogButtonBox::ActionRole);
        auto *hepsi = btn->addButton("&Tümünü Değiştir", QDialogButtonBox::ActionRole);
        btn->addButton("İptal", QDialogButtonBox::RejectRole);
        auto *lay = new QVBoxLayout(&d);
        lay->addLayout(form);
        lay->addWidget(btn);
        connect(btn, &QDialogButtonBox::rejected, &d, &QDialog::reject);
        connect(tek, &QPushButton::clicked, &d, [&] {
            aranan = aranacak->text();
            if (ed->textCursor().selectedText() == aranan) ed->insertPlainText(yeni->text());
            if (!ed->find(aranan))
                QMessageBox::information(&d, "Not Defteri", QString("\"%1\" bulunamıyor").arg(aranan));
        });
        connect(hepsi, &QPushButton::clicked, &d, [&] {
            aranan = aranacak->text();
            if (aranan.isEmpty()) return;
            ed->moveCursor(QTextCursor::Start);
            auto c = ed->textCursor();
            c.beginEditBlock();
            while (ed->find(aranan)) ed->insertPlainText(yeni->text());
            c.endEditBlock();
        });
        d.exec();
    }

    void satiraGit() {
        bool ok;
        int n = QInputDialog::getInt(this, "Satıra Git", "&Satır numarası:",
                                     ed->textCursor().blockNumber() + 1, 1, ed->blockCount(), 1, &ok);
        if (!ok) return;
        QTextCursor c(ed->document()->findBlockByNumber(n - 1));
        ed->setTextCursor(c);
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("XPThemePlasma");
    QApplication::setApplicationName("xp-notdefteri");
    QApplication::setApplicationDisplayName("Not Defteri");
    QGuiApplication::setDesktopFileName("xp-notdefteri");
    NotDefteri w;
    if (argc > 1) w.dosyaYukle(QString::fromLocal8Bit(argv[1]));
    w.show();
    return app.exec();
}
