#!/usr/bin/env python3
"""XP kabuğunun Plasma sürümüne bağlı parçalarını kurulu Plasma'dan üretir.

    python3 tools/xp_kabuk.py <kurulu kabuk: ~/.local/share/plasma/shells/io.github.yyozcan.xpshell>

XP kabuğu yalnızca değiştirdiği dosyaları taşır; geri kalan her şey Plasma'nın kendi masaüstü
kabuğundan (org.kde.plasma.desktop) gelir, böylece KDE güncellemeleriyle kendiliğinden güncel kalır.
configuration/ dizini ise bir arada çalışan dosyalardan oluştuğu için kurulu Plasma'dan kopyalanır
ve ContainmentConfiguration.qml'e küçük bir ek yapılır: masaüstüne sağ tıklama > "Özellikler"
XP Görüntü Özellikleri'ni açar. Ek uygulanamazsa (Plasma dosyayı değiştirdiyse) stok dosya kalır.
"""
import shutil
import sys
from pathlib import Path

STOK = Path("/usr/share/plasma/shells/org.kde.plasma.desktop/contents")

EK = '''
    // XPThemePlasma: masaüstüne sağ tıklama > "Özellikler" Görüntü Özellikleri'ni açar
    P5Support.DataSource {
        id: xpKomut
        engine: "executable"
        connectedSources: []
        onNewData: source => disconnectSource(source)
    }
    Timer {                     // ayrı nesne: dosyanın kendi Component.onCompleted'ıyla çakışmasın
        interval: 0
        running: true
        onTriggered: {
        const masaustu = ["org.kde.plasma.folder", "org.kde.desktopcontainment"].indexOf(configDialog.containmentPlugin) !== -1;
        const yerel = StandardPaths.writableLocation(StandardPaths.HomeLocation).toString().replace("file://", "") + "/.local/bin";
        const gorunum = StandardPaths.findExecutable("xp-gorunum", [yerel]);
        if (masaustu && gorunum.toString() !== "") {
            xpKomut.connectSource("setsid -f \\"" + gorunum.toString().replace("file://", "") + "\\"");
            Qt.callLater(configDialog.close);
        }
        }
    }
'''


def main(hedef):
    hedef = Path(hedef) / "contents"
    kaynak = STOK / "configuration"
    yapilandirma = hedef / "configuration"
    if yapilandirma.exists():
        shutil.rmtree(yapilandirma)
    if not kaynak.is_dir():
        print("  stok kabuk yapılandırması yok; Plasma'nınki kullanılacak")
        return
    shutil.copytree(kaynak, yapilandirma)
    p = yapilandirma / "ContainmentConfiguration.qml"
    if not p.exists():
        return
    s = p.read_text()
    son = s.rstrip().rfind("}")
    if "AppletConfiguration {" not in s or son < 0:
        print("  ContainmentConfiguration.qml beklenen biçimde değil; ek atlandı")
        return
    ust = "import org.kde.plasma.plasma5support as P5Support\nimport QtCore\n"
    ilk = s.find("\nimport ")
    s = s[:ilk + 1] + ust + s[ilk + 1:]
    son = s.rstrip().rfind("}")
    s = s[:son] + EK + s[son:]
    p.write_text(s)
    print("  kabuk yapılandırması kurulu Plasma'dan üretildi")


if __name__ == "__main__":
    main(sys.argv[1])
