// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "shmserverbufferintegration.h"
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QDebug>
#include <QtOpenGL/QOpenGLTexture>
#include <QtGui/QOpenGLContext>
#include <QtGui/QImage>
#include <QtCore/QSharedMemory>

QT_BEGIN_NAMESPACE

static QOpenGLTexture *createTextureFromShm(const QString &key, int w, int h, int bpl, int format)
{
    QT_IGNORE_DEPRECATIONS(QSharedMemory shm(key);)
    bool ok;
    ok = shm.attach(QSharedMemory::ReadOnly);
    if (!ok) {
        qWarning() << "Could not attach to" << key;
        return nullptr;
    }
    ok = shm.lock();
    if (!ok) {
        qWarning() << "Could not lock" << key << "for reading";
        return nullptr;
    }

    QImage::Format imageFormat;
    switch (format) {
        case QtWayland::qt_shm_emulation_server_buffer::format_RGBA32:
            imageFormat = QImage::Format_RGBA8888;
            break;
        case QtWayland::qt_shm_emulation_server_buffer::format_A8:
            imageFormat = QImage::Format_Alpha8;
            break;
        default:
            qWarning() << "ShmServerBuffer: unknown format" << format;
            imageFormat = QImage::Format_RGBA8888;
            break;
    }

    QImage image(static_cast<const uchar*>(shm.constData()), w, h, bpl, imageFormat);

    if (!QOpenGLContext::currentContext())
        qWarning("ShmServerBuffer: creating texture with no current context");

    auto *tex = new QOpenGLTexture(image, QOpenGLTexture::DontGenerateMipMaps);
    shm.unlock();
    return tex;
}


namespace QtWaylandClient {

ShmServerBuffer::ShmServerBuffer(const QString &key, const QSize& size, int bytesPerLine, QWaylandServerBuffer::Format format)
    : m_key(key)
    , m_bpl(bytesPerLine)
{
    m_format = format;
    m_size = size;
}

ShmServerBuffer::~ShmServerBuffer()
{
}

QOpenGLTexture *ShmServerBuffer::toOpenGlTexture()
{
    if (!m_texture)
        m_texture = createTextureFromShm(m_key, m_size.width(), m_size.height(), m_bpl, m_format);

    return m_texture;
}

void ShmServerBufferIntegration::initialize(QWaylandDisplay *display)
{
    m_display = display;
    display->addRegistryListener(&wlDisplayHandleGlobal, this);
}

QWaylandServerBuffer *ShmServerBufferIntegration::serverBuffer(struct qt_server_buffer *buffer)
{
    return static_cast<QWaylandServerBuffer *>(qt_server_buffer_get_user_data(buffer));
}

void ShmServerBufferIntegration::wlDisplayHandleGlobal(void *data, ::wl_registry *registry, uint32_t id, const QString &interface, uint32_t version)
{
    Q_UNUSED(version);
    if (interface == "qt_shm_emulation_server_buffer") {
        auto *integration = static_cast<ShmServerBufferIntegration *>(data);
        integration->QtWayland::qt_shm_emulation_server_buffer::init(registry, id, 1);
    }
}


void QtWaylandClient::ShmServerBufferIntegration::shm_emulation_server_buffer_server_buffer_created(qt_server_buffer *id, const QString &key, int32_t width, int32_t height, int32_t bytes_per_line, int32_t format)
{
    QSize size(width, height);
    auto fmt = QWaylandServerBuffer::Format(format);
    auto *server_buffer = new ShmServerBuffer(key, size, bytes_per_line, fmt);
    qt_server_buffer_set_user_data(id, server_buffer);
}

}

QT_END_NAMESPACE
