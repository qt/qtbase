// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qshapedpixmapdndwindow_p.h"

#include "qplatformwindow.h"

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
    setFlags(Qt::FramelessWindowHint | Qt::BypassWindowManagerHint
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
            if (auto platformWindow = handle()) {
                const auto pixmapDpr = m_pixmap.devicePixelRatio();
                const auto winDpr = devicePixelRatio();
                const auto maskSize = (QSizeF(m_pixmap.size()) * winDpr / pixmapDpr).toSize();
                platformWindow->setMask(QBitmap::fromPixmap(mask.scaled(maskSize)));
            }
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
        size = m_pixmap.deviceIndependentSize().toSize();
    }
    setGeometry(QRect(pos - m_hotSpot, size));
}

QT_END_NAMESPACE

#include "moc_qshapedpixmapdndwindow_p.cpp"
