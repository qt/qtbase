// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>

namespace src_gui_image_qpixmapcache {

void wrapper0(QPainter *painter) {
//! [1]
QPixmap pm;
if (!QPixmapCache::find("my_big_image", &pm)) {
    pm.load("bigimage.png");
    QPixmapCache::insert("my_big_image", pm);
}
painter->drawPixmap(0, 0, pm);
//! [1]

} // wrapper0
} // src_gui_image_qpixmapcache
