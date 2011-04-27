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

#include "dbscreen.h"
#include <QApplication>

//! [0]
bool DBScreen::initDevice()
{
    if (!QLinuxFbScreen::initDevice())
        return false;

    QScreenCursor::initSoftwareCursor();
    image = new QImage(deviceWidth(), deviceHeight(), pixelFormat());
    painter = new QPainter(image);

    return true;
}
//! [0]

//! [1]
void DBScreen::shutdownDevice()
{
    QLinuxFbScreen::shutdownDevice();
    delete painter;
    delete image;
}
//! [1]

//! [2]
void DBScreen::solidFill(const QColor &color, const QRegion &region)
{
    QVector<QRect> rects = region.rects();
    for (int i = 0; i  < rects.size(); i++)
        painter->fillRect(rects.at(i), color);
}
//! [2]

//! [3]
void DBScreen::blit(const QImage &image, const QPoint &topLeft, const QRegion &region)
{
    QVector<QRect> rects = region.rects();
    for (int i = 0; i < rects.size(); i++) {
        QRect destRect = rects.at(i);
        QRect srcRect(destRect.x()-topLeft.x(), destRect.y()-topLeft.y(), destRect.width(), destRect.height());
        painter->drawImage(destRect.topLeft(), image, srcRect);
    }
}
//! [3]

//! [4]
void DBScreen::exposeRegion(QRegion region, int changing)
{
    QLinuxFbScreen::exposeRegion(region, changing);
    QLinuxFbScreen::blit(*image, QPoint(0, 0), region);
}
//! [4]

