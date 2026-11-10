// XPThemePlasma: ilk açılış düzeni — XP görev çubuğu + Bliss benzeri duvar kağıdı
loadTemplate("io.github.yyozcan.xptaskbar")

var desktopsArray = desktopsForActivity(currentActivity());
for (var j = 0; j < desktopsArray.length; j++) {
    var d = desktopsArray[j];
    d.wallpaperPlugin = 'org.kde.image';
    d.currentConfigGroup = ["Wallpaper", "org.kde.image", "General"];
    d.writeConfig("Image", "file://" + userDataPath() + "/.local/share/wallpapers/XPThemePlasma/");
    d.writeConfig("FillMode", "2");
    d.currentConfigGroup = ["General"];
    d.writeConfig("arrangement", "1");
    d.writeConfig("alignment", "0");
    d.writeConfig("iconSize", "2");
}
