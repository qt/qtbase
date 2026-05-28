// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "vulkanserverbufferintegration.h"
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QDebug>
#include <QtOpenGL/QOpenGLTexture>
#include <QtGui/QOpenGLContext>
#include <QtGui/qopengl.h>
#include <QtGui/QImage>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

static constexpr bool sbiExtraDebug =
#ifdef VULKAN_SERVER_BUFFER_EXTRA_DEBUG
    true;
#else
    false;
#endif

#define DECL_GL_FUNCTION(name, type) \
    type name

#define FIND_GL_FUNCTION(name, type) \
    do { \
        name = reinterpret_cast<type>(glContext->getProcAddress(#name)); \
        if (!name) {                                                    \
            qWarning() << "ERROR in GL proc lookup. Could not find " #name; \
            return false;                                               \
        } \
    } while (0)

struct VulkanServerBufferGlFunctions
{
    DECL_GL_FUNCTION(glCreateMemoryObjectsEXT, PFNGLCREATEMEMORYOBJECTSEXTPROC);
    DECL_GL_FUNCTION(glImportMemoryFdEXT, PFNGLIMPORTMEMORYFDEXTPROC);
    DECL_GL_FUNCTION(glTextureStorageMem2DEXT, PFNGLTEXTURESTORAGEMEM2DEXTPROC);
    DECL_GL_FUNCTION(glTexStorageMem2DEXT, PFNGLTEXSTORAGEMEM2DEXTPROC);
    DECL_GL_FUNCTION(glDeleteMemoryObjectsEXT, PFNGLDELETEMEMORYOBJECTSEXTPROC);

    bool init(QOpenGLContext *glContext)
    {
        FIND_GL_FUNCTION(glCreateMemoryObjectsEXT, PFNGLCREATEMEMORYOBJECTSEXTPROC);
        FIND_GL_FUNCTION(glImportMemoryFdEXT, PFNGLIMPORTMEMORYFDEXTPROC);
        FIND_GL_FUNCTION(glTextureStorageMem2DEXT, PFNGLTEXTURESTORAGEMEM2DEXTPROC);
        FIND_GL_FUNCTION(glTexStorageMem2DEXT, PFNGLTEXSTORAGEMEM2DEXTPROC);
        FIND_GL_FUNCTION(glDeleteMemoryObjectsEXT, PFNGLDELETEMEMORYOBJECTSEXTPROC);

        return true;
    }
    static bool create(QOpenGLContext *glContext);
};

static VulkanServerBufferGlFunctions *funcs = nullptr;

bool VulkanServerBufferGlFunctions::create(QOpenGLContext *glContext)
{
    if (funcs)
        return true;
    funcs = new VulkanServerBufferGlFunctions;
    if (!funcs->init(glContext)) {
        delete funcs;
        funcs = nullptr;
        return false;
    }
    return true;
}

VulkanServerBuffer::VulkanServerBuffer(VulkanServerBufferIntegration *integration, struct ::qt_server_buffer *id,
                                       int32_t fd, uint32_t width, uint32_t height, uint32_t memory_size, uint32_t format)
    : m_integration(integration)
    , m_server_buffer(id)
    , m_fd(fd)
    , m_memorySize(memory_size)
    , m_internalFormat(format)
{
    m_size = QSize(width, height);
}

VulkanServerBuffer::~VulkanServerBuffer()
{
    if (QCoreApplication::closingDown())
        return; // can't trust anything at this point

    if (m_texture) { //only do gl cleanup if import has been called
        m_integration->deleteGLTextureWhenPossible(m_texture);

        if (sbiExtraDebug) qDebug() << "glDeleteMemoryObjectsEXT" << m_memoryObject;
        funcs->glDeleteMemoryObjectsEXT(1, &m_memoryObject);
    }
    qt_server_buffer_release(m_server_buffer);
    qt_server_buffer_destroy(m_server_buffer);
}

void VulkanServerBuffer::import()
{
    if (m_texture)
        return;

    if (sbiExtraDebug) qDebug() << "importing" << m_fd << Qt::hex << glGetError();

    auto *glContext = QOpenGLContext::currentContext();
    if (!glContext)
        return;

    if (!funcs && !VulkanServerBufferGlFunctions::create(glContext))
        return;

    funcs->glCreateMemoryObjectsEXT(1, &m_memoryObject);
    if (sbiExtraDebug) qDebug() << "glCreateMemoryObjectsEXT" << Qt::hex << glGetError();
    funcs->glImportMemoryFdEXT(m_memoryObject, m_memorySize, GL_HANDLE_TYPE_OPAQUE_FD_EXT, m_fd);
    if (sbiExtraDebug) qDebug() << "glImportMemoryFdEXT" << Qt::hex << glGetError();


    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->create();

    if (sbiExtraDebug) qDebug() << "created texture" << m_texture->textureId() << Qt::hex << glGetError();

    m_texture->bind();
    if (sbiExtraDebug) qDebug() << "bound texture" << Qt::hex << glGetError();
    funcs->glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, m_internalFormat, m_size.width(), m_size.height(), m_memoryObject, 0 );
    if (sbiExtraDebug) qDebug() << "glTexStorageMem2DEXT" << Qt::hex << glGetError();
    if (sbiExtraDebug) qDebug() << "format" << Qt::hex  << m_internalFormat;
}

QOpenGLTexture *VulkanServerBuffer::toOpenGlTexture()
{
    m_integration->deleteOrphanedTextures();
    if (!m_texture)
        import();
    return m_texture;
}

void VulkanServerBufferIntegration::initialize(QWaylandDisplay *display)
{
    m_display = display;
    display->addRegistryListener(&wlDisplayHandleGlobal, this);
}

QWaylandServerBuffer *VulkanServerBufferIntegration::serverBuffer(struct qt_server_buffer *buffer)
{
    return static_cast<QWaylandServerBuffer *>(qt_server_buffer_get_user_data(buffer));
}

void VulkanServerBufferIntegration::wlDisplayHandleGlobal(void *data, ::wl_registry *registry, uint32_t id, const QString &interface, uint32_t version)
{
    Q_UNUSED(version);
    if (interface == "zqt_vulkan_server_buffer_v1") {
        auto *integration = static_cast<VulkanServerBufferIntegration *>(data);
        integration->QtWayland::zqt_vulkan_server_buffer_v1::init(registry, id, 1);
    }
}

void VulkanServerBufferIntegration::zqt_vulkan_server_buffer_v1_server_buffer_created(qt_server_buffer *id, int32_t fd, uint32_t width, uint32_t height, uint32_t memory_size, uint32_t format)
{
    if (sbiExtraDebug) qDebug() << "vulkan_server_buffer_server_buffer_created" << fd;
    auto *server_buffer = new VulkanServerBuffer(this, id, fd, width, height, memory_size, format);
    qt_server_buffer_set_user_data(id, server_buffer);
}

void VulkanServerBufferIntegration::deleteOrphanedTextures()
{
    if (!QOpenGLContext::currentContext()) {
        qWarning("VulkanServerBufferIntegration::deleteOrphanedTextures with no current context!");
        return;
    }
    qDeleteAll(orphanedTextures);
    orphanedTextures.clear();
}

}

QT_END_NAMESPACE
