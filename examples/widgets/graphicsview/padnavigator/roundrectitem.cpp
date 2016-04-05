/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "roundrectitem.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>

//! [0]
RoundRectItem::RoundRectItem(const QRectF &bounds, const QColor &color,
                             QGraphicsItem *parent)
    : QGraphicsObject(parent), fillRect(false), bounds(bounds)
{
    gradient.setStart(bounds.topLeft());
    gradient.setFinalStop(bounds.bottomRight());
    gradient.setColorAt(0, color);
    gradient.setColorAt(1, color.dark(200));
    setCacheMode(ItemCoordinateCache);
}
//! [0]

//! [1]
QPixmap RoundRectItem::pixmap() const
{
    return pix;
}
void RoundRectItem::setPixmap(const QPixmap &pixmap)
{
    pix = pixmap;
    update();
}
//! [1]

//! [2]
QRectF RoundRectItem::boundingRect() const
{
    return bounds.adjusted(0, 0, 2, 2);
}
//! [2]

//! [3]
void RoundRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 64));
    painter->drawRoundRect(bounds.translated(2, 2));
//! [3]
//! [4]
    if (fillRect)
        painter->setBrush(QApplication::palette().brush(QPalette::Window));
    else
        painter->setBrush(gradient);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawRoundRect(bounds);
//! [4]
//! [5]
    if (!pix.isNull()) {
        painter->scale(1.95, 1.95);
        painter->drawPixmap(-pix.width() / 2, -pix.height() / 2, pix);
    }
}
//! [5]

//! [6]
bool RoundRectItem::fill() const
{
    return fillRect;
}
void RoundRectItem::setFill(bool fill)
{
    fillRect = fill;
    update();
}
//! [6]
