// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Giulio Camuffo.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandbuffer_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandBuffer::QWaylandBuffer()
{
}

QWaylandBuffer::~QWaylandBuffer()
{
    if (mBuffer)
        wl_buffer_destroy(mBuffer);
}

void QWaylandBuffer::init(wl_buffer *buf)
{
    mBuffer = buf;
    wl_buffer_add_listener(buf, &listener, this);
}

void QWaylandBuffer::release(void *data, wl_buffer *)
{
    QWaylandBuffer *self = static_cast<QWaylandBuffer *>(data);
    self->mBusy = false;
    self->mCommitted = false;
    if (self->mDeleteOnRelease)
        delete self;
}

void QWaylandBuffer::setDeleteOnRelease(bool deleteOnRelease)
{
    mDeleteOnRelease = deleteOnRelease;
}

const wl_buffer_listener QWaylandBuffer::listener = {
    QWaylandBuffer::release
};

}

QT_END_NAMESPACE
