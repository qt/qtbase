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

#include "qwindowsdirect2dbackingstore.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dintegration.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2ddevicecontext.h"
#include "qwindowsdirect2dwindow.h"

#include "qwindowscontext.h"

#include <QtGui/QPainter>
#include <QtGui/QWindow>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsDirect2DBackingStore
    \brief Backing store for windows.
    \internal
    \ingroup qt-lighthouse-win
*/

static inline QWindowsDirect2DPlatformPixmap *platformPixmap(QPixmap *p)
{
    return static_cast<QWindowsDirect2DPlatformPixmap *>(p->handle());
}

static inline QWindowsDirect2DBitmap *bitmap(QPixmap *p)
{
    return platformPixmap(p)->bitmap();
}

static inline QWindowsDirect2DWindow *nativeWindow(QWindow *window)
{
    return static_cast<QWindowsDirect2DWindow *>(window->handle());
}

QWindowsDirect2DBackingStore::QWindowsDirect2DBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QWindowsDirect2DBackingStore::~QWindowsDirect2DBackingStore()
{
}

void QWindowsDirect2DBackingStore::beginPaint(const QRegion &region)
{
    QPixmap *pixmap = nativeWindow(window())->pixmap();
    bitmap(pixmap)->deviceContext()->begin();

    QPainter painter(pixmap);
    QColor clear(Qt::transparent);

    painter.setCompositionMode(QPainter::CompositionMode_Source);

    for (const QRect &r : region)
        painter.fillRect(r, clear);
}

void QWindowsDirect2DBackingStore::endPaint()
{
    bitmap(nativeWindow(window())->pixmap())->deviceContext()->end();
}

QPaintDevice *QWindowsDirect2DBackingStore::paintDevice()
{
    return nativeWindow(window())->pixmap();
}

void QWindowsDirect2DBackingStore::flush(QWindow *targetWindow, const QRegion &region, const QPoint &offset)
{
    if (targetWindow != window()) {
        QSharedPointer<QWindowsDirect2DBitmap> copy(nativeWindow(window())->copyBackBuffer());
        nativeWindow(targetWindow)->flush(copy.data(), region, offset);
    }

    nativeWindow(targetWindow)->present(region);
}

void QWindowsDirect2DBackingStore::resize(const QSize &size, const QRegion &region)
{
    QPixmap old = nativeWindow(window())->pixmap()->copy();

    nativeWindow(window())->resizeSwapChain(size);
    QPixmap *newPixmap = nativeWindow(window())->pixmap();

    if (!old.isNull()) {
        for (const QRect &rect : region)
            platformPixmap(newPixmap)->copy(old.handle(), rect);
    }
}

QImage QWindowsDirect2DBackingStore::toImage() const
{
    return nativeWindow(window())->pixmap()->toImage();
}

QT_END_NAMESPACE
