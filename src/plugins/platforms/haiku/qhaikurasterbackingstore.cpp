// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhaikurasterbackingstore.h"
#include "qhaikurasterwindow.h"

#include <Bitmap.h>
#include <View.h>

QT_BEGIN_NAMESPACE

QHaikuRasterBackingStore::QHaikuRasterBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_bitmap(nullptr)
{
}

QHaikuRasterBackingStore::~QHaikuRasterBackingStore()
{
    delete m_bitmap;
    m_bitmap = nullptr;
}

QPaintDevice *QHaikuRasterBackingStore::paintDevice()
{
    if (!m_bufferSize.isEmpty() && m_bitmap)
        return m_buffer.image();

    return nullptr;
}

void QHaikuRasterBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (!window)
        return;

    QHaikuRasterWindow *targetWindow = static_cast<QHaikuRasterWindow*>(window->handle());

    BView *view = targetWindow->nativeViewHandle();

    view->LockLooper();
    view->DrawBitmap(m_bitmap);
    view->UnlockLooper();
}

void QHaikuRasterBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    if (m_bufferSize == size)
        return;

    delete m_bitmap;
    m_bitmap = new BBitmap(BRect(0, 0, size.width(), size.height()), B_RGB32, false, true);
    m_buffer = QHaikuBuffer(m_bitmap);
    m_bufferSize = size;
}

QT_END_NAMESPACE
