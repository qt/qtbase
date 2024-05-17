// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxglobal.h"

#include "qqnxbuffer.h"

#include <QtCore/QDebug>

#include <errno.h>
#include <sys/mman.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaScreenBuffer, "qt.qpa.screen.buffer");

QQnxBuffer::QQnxBuffer()
    : m_buffer(0)
{
    qCDebug(lcQpaScreenBuffer) << Q_FUNC_INFO << "Empty";
}

QQnxBuffer::QQnxBuffer(screen_buffer_t buffer)
    : m_buffer(buffer)
{
    qCDebug(lcQpaScreenBuffer) << Q_FUNC_INFO << "Normal";

    // Get size of buffer
    int size[2];
    Q_SCREEN_CRITICALERROR(screen_get_buffer_property_iv(buffer, SCREEN_PROPERTY_BUFFER_SIZE, size),
                        "Failed to query buffer size");

    // Get stride of buffer
    int stride;
    Q_SCREEN_CHECKERROR(screen_get_buffer_property_iv(buffer, SCREEN_PROPERTY_STRIDE, &stride),
                        "Failed to query buffer stride");

    // Get access to buffer's data
    errno = 0;
    uchar *dataPtr = nullptr;
    Q_SCREEN_CRITICALERROR(
            screen_get_buffer_property_pv(buffer, SCREEN_PROPERTY_POINTER, (void **)&dataPtr),
            "Failed to query buffer pointer");

    if (Q_UNLIKELY(!dataPtr))
        qFatal("QQNX: buffer pointer is NULL, errno=%d", errno);

    // Get format of buffer
    int screenFormat;
    Q_SCREEN_CHECKERROR(
            screen_get_buffer_property_iv(buffer, SCREEN_PROPERTY_FORMAT, &screenFormat),
            "Failed to query buffer format");

    // Convert screen format to QImage format
    QImage::Format imageFormat = QImage::Format_Invalid;
    switch (screenFormat) {
    case SCREEN_FORMAT_RGBX4444:
        imageFormat = QImage::Format_RGB444;
        break;
    case SCREEN_FORMAT_RGBA4444:
        imageFormat = QImage::Format_ARGB4444_Premultiplied;
        break;
    case SCREEN_FORMAT_RGBX5551:
        imageFormat = QImage::Format_RGB555;
        break;
    case SCREEN_FORMAT_RGB565:
        imageFormat = QImage::Format_RGB16;
        break;
    case SCREEN_FORMAT_RGBX8888:
        imageFormat = QImage::Format_RGB32;
        break;
    case SCREEN_FORMAT_RGBA8888:
        imageFormat = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        qFatal(lcQpaScreenBuffer, "QQNX: unsupported buffer format, format=%d", screenFormat);
    }

    // wrap buffer in an image
    m_image = QImage(dataPtr, size[0], size[1], stride, imageFormat);
}

QQnxBuffer::QQnxBuffer(const QQnxBuffer &other)
    : m_buffer(other.m_buffer),
      m_image(other.m_image)
{
    qCDebug(lcQpaScreenBuffer) << Q_FUNC_INFO << "Copy";
}

QQnxBuffer::~QQnxBuffer()
{
    qCDebug(lcQpaScreenBuffer) << Q_FUNC_INFO;
}

void QQnxBuffer::invalidateInCache()
{
    qCDebug(lcQpaScreenBuffer) << Q_FUNC_INFO;

    // Verify native buffer exists
    if (Q_UNLIKELY(!m_buffer))
        qFatal(lcQpaScreenBuffer, "QQNX: can't invalidate cache for null buffer");

    // Evict buffer's data from cache
    errno = 0;
    int result = msync(m_image.bits(), m_image.height() * m_image.bytesPerLine(), MS_INVALIDATE | MS_CACHE_ONLY);
    if (Q_UNLIKELY(result != 0))
        qFatal(lcQpaScreenBuffer, "QQNX: failed to invalidate cache, errno=%d", errno);
}

QT_END_NAMESPACE
