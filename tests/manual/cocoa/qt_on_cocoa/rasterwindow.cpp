/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rasterwindow.h"

//#include <private/qguiapplication_p.h>

#include <QBackingStore>
#include <QPainter>
#include <QtWidgets>

static int colorIndexId = 0;

QColor colorTable[] =
{
    QColor("#f09f8f"),
    QColor("#a2bff2"),
    QColor("#c0ef8f")
};

RasterWindow::RasterWindow(QRasterWindow *parent)
    : QRasterWindow(parent)
    , m_backgroundColorIndex(colorIndexId++)
{
    initialize();
}

void RasterWindow::initialize()
{
    if (parent())
        setGeometry(QRect(160, 120, 320, 240));
    else {
        setGeometry(QRect(10, 10, 640, 480));

        setSizeIncrement(QSize(10, 10));
        setBaseSize(QSize(640, 480));
        setMinimumSize(QSize(240, 160));
        setMaximumSize(QSize(800, 600));
    }

    create();
    m_backingStore = new QBackingStore(this);

    m_image = QImage(geometry().size(), QImage::Format_RGB32);
    m_image.fill(colorTable[m_backgroundColorIndex % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

    m_lastPos = QPoint(-1, -1);
    m_renderTimer = 0;
}

void RasterWindow::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
}

void RasterWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->pos());
        m_lastPos = event->pos();
    }

    scheduleRender();
}

void RasterWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_lastPos != QPoint(-1, -1)) {
        QPainter p(&m_image);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(m_lastPos, event->pos());
        m_lastPos = QPoint(-1, -1);
    }

    scheduleRender();
}

void RasterWindow::exposeEvent(QExposeEvent *)
{
    scheduleRender();
}

void RasterWindow::resizeEvent(QResizeEvent *)
{
    QImage old = m_image;

    //qDebug() << "RasterWindow::resizeEvent" << width << height;

    int width = qMax(geometry().width(), old.width());
    int height = qMax(geometry().height(), old.height());

    if (width > old.width() || height > old.height()) {
        m_image = QImage(width, height, QImage::Format_RGB32);
        m_image.fill(colorTable[(m_backgroundColorIndex) % (sizeof(colorTable) / sizeof(colorTable[0]))].rgba());

        QPainter p(&m_image);
        p.drawImage(0, 0, old);
    }

    render();
}

void RasterWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Backspace:
        m_text.chop(1);
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        m_text.append('\n');
        break;
    default:
        m_text.append(event->text());
        break;
    }
    scheduleRender();
}

void RasterWindow::scheduleRender()
{
    if (!m_renderTimer)
        m_renderTimer = startTimer(1);
}

void RasterWindow::timerEvent(QTimerEvent *)
{
    render();
    killTimer(m_renderTimer);
    m_renderTimer = 0;
}

void RasterWindow::render()
{
    QRect rect(QPoint(), geometry().size());

    m_backingStore->resize(rect.size());

    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();

    QPainter p(device);
    p.drawImage(0, 0, m_image);

    QFont font;
    font.setPixelSize(32);

    p.setFont(font);
    p.drawText(rect, 0, m_text);

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}


