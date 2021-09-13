/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "rasterwindow.h"

RasterWindow::RasterWindow()
:m_eventCount(0)
,m_timeoutCount(0)
,m_pressed(false)
{
    qDebug() << "RasterWindow()";

    // disable alpha; saves filling the window with transparent pixels
    QSurfaceFormat format;
    format.setAlphaBufferSize(0);
    setFormat(format);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this](){
        ++m_timeoutCount;
        update();
    });
    timer->start(1000);
}

void RasterWindow::paintEvent(QPaintEvent * event)
{
    QRect r = event->rect();
    qDebug() << "RasterWindow::paintEvent" << r;

    QPainter p(this);

    QColor fillColor(0, 102, 153);
    QColor fillColor2(0, 85, 123);

    int tileSize = 40;
    for (int i = -tileSize * 2; i < r.width() + tileSize * 2; i += tileSize) {
        for (int j = -tileSize * 2; j < r.height() + tileSize * 2; j += tileSize) {
            QRect rect(i + (m_offset.x() % tileSize * 2), j + (m_offset.y() % tileSize * 2), tileSize, tileSize);
            int colorIndex = abs((i/tileSize - j/tileSize) % 2);
            p.fillRect(rect, colorIndex == 0 ? fillColor : fillColor2);
        }
    }

    QRect g = geometry();
    QRect sg = this->screen()->geometry();
    QString text;
    text += QString("Window Geometry: %1 %2 %3 %4\n").arg(g.x()).arg(g.y()).arg(g.width()).arg(g.height());
    text += QString("Window devicePixelRatio: %1\n").arg(devicePixelRatio());
    text += QString("Screen Geometry: %1 %2 %3 %4\n").arg(sg.x()).arg(sg.y()).arg(sg.width()).arg(sg.height());
    text += QString("Received Events: %1\n").arg(m_eventCount);
    text += QString("Received Timers: %1\n").arg(m_timeoutCount);

    p.drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, text);
}
void RasterWindow::exposeEvent(QExposeEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__ << ev->region();
    QRasterWindow::exposeEvent(ev);
    incrementEventCount();
}

void RasterWindow::focusInEvent(QFocusEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::focusInEvent(ev);
    incrementEventCount();
}

void RasterWindow::focusOutEvent(QFocusEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::focusOutEvent(ev);
    incrementEventCount();
}

void RasterWindow::hideEvent(QHideEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::hideEvent(ev);
    incrementEventCount();
}

void RasterWindow::keyPressEvent(QKeyEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::keyPressEvent(ev);
    incrementEventCount();
}

void RasterWindow::keyReleaseEvent(QKeyEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::keyReleaseEvent(ev);
    incrementEventCount();
}

void RasterWindow::mouseDoubleClickEvent(QMouseEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::mouseDoubleClickEvent(ev);
    incrementEventCount();
}

void RasterWindow::mouseMoveEvent(QMouseEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::mouseMoveEvent(ev);
    incrementEventCount();

    if (m_pressed)
        m_offset += ev->localPos().toPoint() - m_lastPos;
    m_lastPos = ev->localPos().toPoint();
}

void RasterWindow::mousePressEvent(QMouseEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::mousePressEvent(ev);
    incrementEventCount();
    m_pressed = true;
}

void RasterWindow::mouseReleaseEvent(QMouseEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::mouseReleaseEvent(ev);
    incrementEventCount();
    m_pressed = false;
}

void RasterWindow::moveEvent(QMoveEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::moveEvent(ev);
    incrementEventCount();
}

void RasterWindow::resizeEvent(QResizeEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::resizeEvent(ev);
    incrementEventCount();
}

void RasterWindow::showEvent(QShowEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::showEvent(ev);
    incrementEventCount();
}

void RasterWindow::tabletEvent(QTabletEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::tabletEvent(ev);
    incrementEventCount();
}

void RasterWindow::touchEvent(QTouchEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::touchEvent(ev);
    incrementEventCount();
}

void RasterWindow::wheelEvent(QWheelEvent * ev)
{
    qDebug() << __PRETTY_FUNCTION__;
    QRasterWindow::wheelEvent(ev);
    incrementEventCount();
}

void RasterWindow::incrementEventCount()
{
    ++m_eventCount;
    update();
}
