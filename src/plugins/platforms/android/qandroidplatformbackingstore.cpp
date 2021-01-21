/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
