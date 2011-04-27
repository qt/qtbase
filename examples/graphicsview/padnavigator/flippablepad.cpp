/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "flippablepad.h"

#include <QtGui/QtGui>

//! [0]
static QRectF boundsFromSize(const QSize &size)
{
    return QRectF((-size.width() / 2.0) * 150, (-size.height() / 2.0) * 150,
                  size.width() * 150, size.height() * 150);
}
//! [0]

//! [1]
static QPointF posForLocation(int column, int row, const QSize &size)
{
    return QPointF(column * 150, row * 150)
        - QPointF((size.width() - 1) * 75, (size.height() - 1) * 75);
}
//! [1]

//! [2]
FlippablePad::FlippablePad(const QSize &size, QGraphicsItem *parent)
    : RoundRectItem(boundsFromSize(size), QColor(226, 255, 92, 64), parent)
{
//! [2]
//! [3]
    int numIcons = size.width() * size.height();
    QList<QPixmap> pixmaps;
    QDirIterator it(":/images", QStringList() << "*.png");
    while (it.hasNext() && pixmaps.size() < numIcons)
        pixmaps << it.next();
//! [3]

//! [4]
    const QRectF iconRect(-54, -54, 108, 108);
    const QColor iconColor(214, 240, 110, 128);
    iconGrid.resize(size.height());
    int n = 0;

    for (int y = 0; y < size.height(); ++y) {
        iconGrid[y].resize(size.width());
        for (int x = 0; x < size.width(); ++x) {
            RoundRectItem *rect = new RoundRectItem(iconRect, iconColor, this);
            rect->setZValue(1);
            rect->setPos(posForLocation(x, y, size));
            rect->setPixmap(pixmaps.at(n++ % pixmaps.size()));
            iconGrid[y][x] = rect;
        }
    }
}
//! [4]

//! [5]
RoundRectItem *FlippablePad::iconAt(int column, int row) const
{
    return iconGrid[row][column];
}
//! [5]
