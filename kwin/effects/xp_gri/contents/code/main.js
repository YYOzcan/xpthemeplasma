/*
    XPThemePlasma — "Bilgisayarı Kapat" / "Oturumu Kapat" penceresi açılınca, XP'deki gibi
    arkadaki masaüstü birkaç saniyede renksizleşir; pencere kapanınca renkler hemen geri gelir.
    SPDX-License-Identifier: GPL-2.0-or-later
*/
"use strict";

var xpGri = {
    acik: false,
    greeterMi: function (w) {
        return w.windowClass === "ksmserver-logout-greeter ksmserver-logout-greeter";
    },
    soldur: function (w) {
        if (!w.visible || w.popupWindow || xpGri.greeterMi(w) || w.xpGri) {
            return;
        }
        w.xpGri = set({
            window: w,
            curve: QEasingCurve.Linear,
            duration: animationTime(2400),
            keepAlive: false,
            animations: [{ type: Effect.Saturation, to: 0.0 }]
        });
    },
    renklendir: function (w) {
        if (w.xpGri) {
            cancel(w.xpGri);
            delete w.xpGri;
        }
    },
    eklendi: function (w) {
        if (xpGri.greeterMi(w)) {
            xpGri.acik = true;
            effects.stackingOrder.forEach(xpGri.soldur);
        } else if (xpGri.acik) {
            xpGri.soldur(w);
        }
    },
    kapandi: function (w) {
        if (!xpGri.greeterMi(w)) {
            return;
        }
        xpGri.acik = false;
        effects.stackingOrder.forEach(xpGri.renklendir);
    },
    init: function () {
        effects.windowAdded.connect(xpGri.eklendi);
        effects.windowClosed.connect(xpGri.kapandi);
    }
};
xpGri.init();
