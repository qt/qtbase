/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
