// Copyright (C) 2016 LG Electronics Inc, author: <mikko.levonmaa@lge.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include <QtWaylandClient/private/qwaylandshm_p.h>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>

#include "qwaylandsharedmemoryformathelper_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandShm::QWaylandShm(QWaylandDisplay *display, int version, uint32_t id)
    : QtWayland::wl_shm(display->wl_registry(), id, qMin(version, 2))
{
}

QWaylandShm::~QWaylandShm()
{
    if (version() < WL_SHM_RELEASE_SINCE_VERSION) {
        wl_shm_destroy(object());
    } else {
        wl_shm_release(object());
    }
}

void QWaylandShm::shm_format(uint32_t format)
{
    m_formats << format;
}

bool QWaylandShm::formatSupported(wl_shm_format format) const
{
    return m_formats.contains(format);
}

bool QWaylandShm::formatSupported(QImage::Format format) const
{
    wl_shm_format fmt = formatFrom(format);
    return formatSupported(fmt);
}

wl_shm_format QWaylandShm::formatFrom(QImage::Format format)
{
    return QWaylandSharedMemoryFormatHelper::fromQImageFormat(format);
}

QImage::Format QWaylandShm::formatFrom(wl_shm_format format)
{
    return QWaylandSharedMemoryFormatHelper::fromWaylandShmFormat(format);
}

}

QT_END_NAMESPACE
