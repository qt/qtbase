// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformbackingstore.h"
#include "qandroidplatformscreen.h"
#include "qandroidplatformwindow.h"
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformBackingStore::QAndroidPlatformBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
    if (window->handle())
        setBackingStore(window);
}

QPaintDevice *QAndroidPlatformBackingStore::paintDevice()
{
    return &m_image;
}

void QAndroidPlatformBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(offset);

    if (!m_backingStoreSet)
        setBackingStore(window);

    (static_cast<QAndroidPlatformWindow *>(window->handle()))->repaint(region);
}

void QAndroidPlatformBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    if (m_image.size() != size)
        m_image = QImage(size, window()->screen()->handle()->format());
}

void QAndroidPlatformBackingStore::setBackingStore(QWindow *window)
{
    if (window->surfaceType() == QSurface::RasterSurface || window->surfaceType() == QSurface::RasterGLSurface) {
        (static_cast<QAndroidPlatformWindow *>(window->handle()))->setBackingStore(this);
        m_backingStoreSet = true;
    } else {
        qWarning("QAndroidPlatformBackingStore does not support OpenGL-only windows.");
    }
}

QT_END_NAMESPACE
