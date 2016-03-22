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
    : m_useCompositing(true)
{
    setScreen(screen);
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint
             | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
}

QShapedPixmapWindow::~QShapedPixmapWindow()
{
}

void QShapedPixmapWindow::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    if (!m_useCompositing) {
        const QBitmap mask = m_pixmap.mask();
        if (!mask.isNull()) {
            if (!handle())
                create();
            setMask(mask);
        }
    }
}

void QShapedPixmapWindow::setHotspot(const QPoint &hotspot)
{
    m_hotSpot = hotspot;
}

void QShapedPixmapWindow::paintEvent(QPaintEvent *)
{
    if (!m_pixmap.isNull()) {
        const QRect rect(QPoint(0, 0), size());
        QPainter painter(this);
        if (m_useCompositing)
            painter.setCompositionMode(QPainter::CompositionMode_Source);
        else
            painter.fillRect(rect, QGuiApplication::palette().base());
        painter.drawPixmap(rect, m_pixmap);
    }
}

void QShapedPixmapWindow::updateGeometry(const QPoint &pos)
{
    QSize size(1, 1);
    if (!m_pixmap.isNull()) {
        size = qFuzzyCompare(m_pixmap.devicePixelRatio(), qreal(1.0))
            ? m_pixmap.size()
            : (QSizeF(m_pixmap.size()) / m_pixmap.devicePixelRatio()).toSize();
    }
    setGeometry(QRect(pos - m_hotSpot, size));
}

QT_END_NAMESPACE
