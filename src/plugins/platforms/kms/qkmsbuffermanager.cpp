/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qkmsbuffermanager.h"
#include "qkmsscreen.h"
#include "qkmscontext.h"
#include "qkmsdevice.h"

QT_BEGIN_NAMESPACE

QKmsBufferManager::QKmsBufferManager(QKmsScreen *screen) :
    m_screen(screen),
    m_frameBufferObject(0),
    m_renderTarget(0),
    m_displayCanidate(0),
    m_currentDisplay(0)
{
}

QKmsBufferManager::~QKmsBufferManager()
{
    clearBuffers();
    glDeleteFramebuffers(1, &m_frameBufferObject);
}

void QKmsBufferManager::setupBuffersForMode(const drmModeModeInfo &mode, int numBuffers)
{
    eglMakeCurrent(m_screen->device()->eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, m_screen->device()->eglContext());
    m_screen->bindFramebuffer();


    if (m_frameBufferObject) {
        clearBuffers();
    } else {
        //Setup Framebuffer Object
        glGenFramebuffers(1, &m_frameBufferObject);
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferObject);
    }

    //Setup shared Depth/Stencil buffer
    glGenRenderbuffers(1, &m_depthAndStencilBufferObject);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthAndStencilBufferObject);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
                          mode.hdisplay, mode.vdisplay);

    //Setup "numBuffer" many rendering targets
    for (int i = 0; i < numBuffers; i++) {
        QKmsFramebuffer *buffer = new QKmsFramebuffer();

        glGenRenderbuffers(1, &buffer->renderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, buffer->renderBuffer);

        buffer->graphicsBufferObject = gbm_bo_create(m_screen->device()->gbmDevice(),
                                                     mode.hdisplay, mode.vdisplay,
                                                     GBM_BO_FORMAT_XRGB8888,
                                                     GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        buffer->eglImage = eglCreateImageKHR(m_screen->device()->eglDisplay(), 0, EGL_NATIVE_PIXMAP_KHR,
                                             buffer->graphicsBufferObject, 0);
        glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, buffer->eglImage);

        quint32 stride = gbm_bo_get_pitch(buffer->graphicsBufferObject);
        quint32 handle = gbm_bo_get_handle(buffer->graphicsBufferObject).u32;

        int status = drmModeAddFB(m_screen->device()->fd(), mode.hdisplay, mode.vdisplay,
                                  32, 32, stride, handle, &buffer->framebufferId);
        //Todo: IF this returns true, then this is one less buffer that we use
        //Not so fatal, but not handled at the moment.
        if (status)
            qFatal("failed to add framebuffer");
        m_framebuffers.append(buffer);
    }
    //Attach the Depth and Stencil buffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              m_depthAndStencilBufferObject);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              m_depthAndStencilBufferObject);
    //Attach  renderbuffer as Color Attachment.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER,
                              this->renderTargetBuffer());

    eglMakeCurrent(m_screen->device()->eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void QKmsBufferManager::clearBuffers()
{
    //Make sure that the FBO is binded
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferObject);
    //Detach the Color/Depth/Stencil Attachments.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER,
                              0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    //Delete the shared Depth/Stencil buffer
    glDeleteRenderbuffers(1, &m_depthAndStencilBufferObject);

    //Delete each renderbuffer object
    //Delete each EGLImage
    //Remove each drm Framebuffer
    foreach (QKmsFramebuffer *buffer, m_framebuffers) {
        glDeleteRenderbuffers(1, &buffer->renderBuffer);
        eglDestroyImageKHR(m_screen->device()->eglDisplay(), buffer->eglImage);
        drmModeRmFB(m_screen->device()->fd(), buffer->framebufferId);
        delete buffer;
    }
    m_framebuffers.clear();
}

GLuint QKmsBufferManager::renderTargetBuffer()
{
    //TODO: Handle more senarios than assuming at least 2 buffers
    if (!m_renderTarget) {
        m_renderTarget = m_framebuffers.at(1);
    }
    return m_renderTarget->renderBuffer;
}

quint32 QKmsBufferManager::displayFramebufferId()
{
    if (!m_currentDisplay) {
        m_currentDisplay = m_framebuffers.at(0);
        m_currentDisplay->available = false;
        return m_currentDisplay->framebufferId;
    }

    if (!m_displayCanidate)
        return m_currentDisplay->framebufferId;

    m_currentDisplay->available = true;
    m_displayCanidate->available = false;
    m_currentDisplay = m_displayCanidate;
    return m_currentDisplay->framebufferId;

}

bool QKmsBufferManager::nextBuffer()
{
    m_displayCanidate = m_renderTarget;
    foreach (QKmsFramebuffer *buffer, m_framebuffers) {
        if (buffer->available && buffer != m_displayCanidate) {
            m_renderTarget = buffer;
            return true;
        }
    }
    return false;
}

QT_BEGIN_NAMESPACE
