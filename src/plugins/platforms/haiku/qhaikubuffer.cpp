// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhaikubuffer.h"

#include <Bitmap.h>
#include <Rect.h>

QT_BEGIN_NAMESPACE

QHaikuBuffer::QHaikuBuffer()
    : m_buffer(nullptr)
{
}

QHaikuBuffer::QHaikuBuffer(BBitmap *buffer)
    : m_buffer(buffer)
{
    // wrap buffer in an image
    m_image = QImage(static_cast<uchar*>(m_buffer->Bits()), m_buffer->Bounds().right, m_buffer->Bounds().bottom, m_buffer->BytesPerRow(), QImage::Format_RGB32);
}

BBitmap* QHaikuBuffer::nativeBuffer() const
{
    return m_buffer;
}

const QImage *QHaikuBuffer::image() const
{
    return (m_buffer != nullptr) ? &m_image : nullptr;
}

QImage *QHaikuBuffer::image()
{
    return (m_buffer != nullptr) ? &m_image : nullptr;
}

QRect QHaikuBuffer::rect() const
{
    return m_image.rect();
}

QT_END_NAMESPACE
