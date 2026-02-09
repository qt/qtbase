// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandserverbufferintegration_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandServerBuffer::QWaylandServerBuffer()
{
}

QWaylandServerBuffer::~QWaylandServerBuffer()
{
}

QWaylandServerBuffer::Format QWaylandServerBuffer::format() const
{
    return m_format;
}

QSize QWaylandServerBuffer::size() const
{
    return m_size;
}

void QWaylandServerBuffer::setUserData(void *userData)
{
    m_user_data = userData;
}

void *QWaylandServerBuffer::userData() const
{
    return m_user_data;
}

QWaylandServerBufferIntegration::QWaylandServerBufferIntegration()
{
}
QWaylandServerBufferIntegration::~QWaylandServerBufferIntegration()
{
}

}

QT_END_NAMESPACE
