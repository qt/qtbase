// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef VULKANSERVERBUFFERINTEGRATION_H
#define VULKANSERVERBUFFERINTEGRATION_H

#include <QtWaylandClient/private/qwayland-wayland.h>
#include "qwayland-qt-vulkan-server-buffer-unstable-v1.h"
#include <QtWaylandClient/private/qwaylandserverbufferintegration_p.h>

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtCore/QTextStream>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class VulkanServerBufferIntegration;

class VulkanServerBuffer : public QWaylandServerBuffer
{
public:
    VulkanServerBuffer(VulkanServerBufferIntegration *integration, struct ::qt_server_buffer *id, int32_t fd, uint32_t width, uint32_t height, uint32_t memory_size, uint32_t format);
    ~VulkanServerBuffer() override;
    QOpenGLTexture* toOpenGlTexture() override;

private:
    void import();

    VulkanServerBufferIntegration *m_integration = nullptr;
    struct ::qt_server_buffer *m_server_buffer = nullptr;
    QOpenGLTexture *m_texture = nullptr;
    int m_fd = -1;
    uint m_memorySize = 0;
    uint m_internalFormat = 0;
    GLuint m_memoryObject = 0;
};

class VulkanServerBufferIntegration
    : public QWaylandServerBufferIntegration
    , public QtWayland::zqt_vulkan_server_buffer_v1
{
public:
    void initialize(QWaylandDisplay *display) override;

    QWaylandServerBuffer *serverBuffer(struct qt_server_buffer *buffer) override;

    void deleteGLTextureWhenPossible(QOpenGLTexture *texture) { orphanedTextures << texture; }
    void deleteOrphanedTextures();

protected:
    void zqt_vulkan_server_buffer_v1_server_buffer_created(qt_server_buffer *id, int32_t fd, uint32_t width, uint32_t height, uint32_t memory_size, uint32_t format) override;

private:
    static void wlDisplayHandleGlobal(void *data, struct ::wl_registry *registry, uint32_t id,
                                      const QString &interface, uint32_t version);
    QWaylandDisplay *m_display = nullptr;
    QList<QOpenGLTexture *> orphanedTextures;
};

}

QT_END_NAMESPACE

#endif
