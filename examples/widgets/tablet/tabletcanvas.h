/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef TABLETCANVAS_H
#define TABLETCANVAS_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QTabletEvent>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QPoint>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QString;
QT_END_NAMESPACE

//! [0]
class TabletCanvas : public QWidget
{
    Q_OBJECT

public:
    enum AlphaChannelType { AlphaPressure, AlphaTilt, NoAlpha };
    enum ColorSaturationType { SaturationVTilt, SaturationHTilt,
                               SaturationPressure, NoSaturation };
    enum LineWidthType { LineWidthPressure, LineWidthTilt, NoLineWidth };

    TabletCanvas();

    bool saveImage(const QString &file);
    bool loadImage(const QString &file);
    void setAlphaChannelType(AlphaChannelType type)
        { alphaChannelType = type; }
    void setColorSaturationType(ColorSaturationType type)
        { colorSaturationType = type; }
    void setLineWidthType(LineWidthType type)
        { lineWidthType = type; }
    void setColor(const QColor &color)
        { myColor = color; }
    QColor color() const
        { return myColor; }
    void setTabletDevice(QTabletEvent::TabletDevice device)
        { myTabletDevice = device; }
    int maximum(int a, int b)
        { return a > b ? a : b; }

protected:
    void tabletEvent(QTabletEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    void initPixmap();
    void paintPixmap(QPainter &painter, QTabletEvent *event);
    Qt::BrushStyle brushPattern(qreal value);
    void updateBrush(QTabletEvent *event);

    AlphaChannelType alphaChannelType;
    ColorSaturationType colorSaturationType;
    LineWidthType lineWidthType;
    QTabletEvent::PointerType pointerType;
    QTabletEvent::TabletDevice myTabletDevice;
    QColor myColor;

    QPixmap pixmap;
    QBrush myBrush;
    QPen myPen;
    bool deviceDown;
    QPoint polyLine[3];
};
//! [0]

#endif
