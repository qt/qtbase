/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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
