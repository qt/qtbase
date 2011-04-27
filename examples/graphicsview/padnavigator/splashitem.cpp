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

#include "splashitem.h"

#include <QtGui/QtGui>

//! [0]
SplashItem::SplashItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    text = tr("Welcome to the Pad Navigator Example. You can use the"
              " keyboard arrows to navigate the icons, and press enter"
              " to activate an item. Press any key to begin.");
    setCacheMode(DeviceCoordinateCache);
}
//! [0]

//! [1]
QRectF SplashItem::boundingRect() const
{
    return QRectF(0, 0, 400, 175);
}
//! [1]

//! [2]
void SplashItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(QColor(245, 245, 255, 220));
    painter->setClipRect(boundingRect());
    painter->drawRoundRect(3, -100 + 3, 400 - 6, 250 - 6);

    QRectF textRect = boundingRect().adjusted(10, 10, -10, -10);
    int flags = Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap;

    QFont font;
    font.setPixelSize(18);
    painter->setPen(Qt::black);
    painter->setFont(font);
    painter->drawText(textRect, flags, text);
}
//! [2]
