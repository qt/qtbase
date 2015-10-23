/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qshapedpixmapdndwindow_p.h"

#include <QtGui/QPainter>
#include <QtGui/QCursor>
#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>
#include <QtGui/QBitmap>

QT_BEGIN_NAMESPACE

QShapedPixmapWindow::QShapedPixmapWindow(QScreen *screen)
    : QWindow(screen),
      m_backingStore(0),
      m_useCompositing(true)
{
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setSurfaceType(RasterSurface);
    setFlags(Qt::ToolTip | Qt::FramelessWindowHint |
                   Qt::X11BypassWindowManagerHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
    create();
    m_backingStore = new QBackingStore(this);
}

QShapedPixmapWindow::~QShapedPixmapWindow()
{
    delete m_backingStore;
    m_backingStore = 0;
}

void QShapedPixmapWindow::render()
{
    QRect rect(QPoint(), geometry().size());

    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();

    {
        QPainter p(device);
        if (m_useCompositing)
            p.setCompositionMode(QPainter::CompositionMode_Source);
        else
            p.fillRect(rect, QGuiApplication::palette().base());
        p.drawPixmap(0, 0, m_pixmap);
    }

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

void QShapedPixmapWindow::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    if (!m_useCompositing)
        setMask(m_pixmap.mask());
}

void QShapedPixmapWindow::setHotspot(const QPoint &hotspot)
{
    m_hotSpot = hotspot;
}

void QShapedPixmapWindow::updateGeometry(const QPoint &pos)
{
    if (m_pixmap.isNull())
        m_backingStore->resize(QSize(1,1));
    else if (m_backingStore->size() != m_pixmap.size())
        m_backingStore->resize(m_pixmap.size());

    setGeometry(QRect(pos - m_hotSpot, m_backingStore->size()));
}

void QShapedPixmapWindow::exposeEvent(QExposeEvent *)
{
    render();
}

QT_END_NAMESPACE
