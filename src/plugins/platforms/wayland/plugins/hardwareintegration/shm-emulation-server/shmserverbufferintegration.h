// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QtWaylandClient/private/qwayland-wayland.h>
#include "qwayland-shm-emulation-server-buffer.h"
#include <QtWaylandClient/private/qwaylandserverbufferintegration_p.h>

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtCore/QTextStream>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class ShmServerBufferIntegration;

class ShmServerBuffer : public QWaylandServerBuffer
{
public:
    ShmServerBuffer(const QString &key, const QSize &size, int bytesPerLine, QWaylandServerBuffer::Format format);
    ~ShmServerBuffer() override;
    QOpenGLTexture* toOpenGlTexture() override;
private:
    QOpenGLTexture *m_texture = nullptr;
    QString m_key;
    int m_bpl;
};

class ShmServerBufferIntegration
    : public QWaylandServerBufferIntegration
    , public QtWayland::qt_shm_emulation_server_buffer
{
public:
    void initialize(QWaylandDisplay *display) override;

    QWaylandServerBuffer *serverBuffer(struct qt_server_buffer *buffer) override;

protected:
    void shm_emulation_server_buffer_server_buffer_created(qt_server_buffer *id, const QString &key, int32_t width, int32_t height, int32_t bytes_per_line, int32_t format) override;

private:
    static void wlDisplayHandleGlobal(void *data, struct ::wl_registry *registry, uint32_t id,
                                      const QString &interface, uint32_t version);
    QWaylandDisplay *m_display = nullptr;
};

}

QT_END_NAMESPACE
