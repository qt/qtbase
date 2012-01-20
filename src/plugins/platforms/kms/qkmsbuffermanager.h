/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QKMSBUFFERMANAGER_H
#define QKMSBUFFERMANAGER_H

#include <QObject>
#include <QList>

#define EGL_EGLEXT_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES 1

extern "C" {
#include <gbm.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
}

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

QT_BEGIN_NAMESPACE

class QKmsScreen;

class QKmsFramebuffer
{
public:
    QKmsFramebuffer() : available(true) {}
    gbm_bo *graphicsBufferObject;
    GLuint renderBuffer;
    EGLImageKHR eglImage;
    quint32 framebufferId;
    bool available;
};


class QKmsBufferManager
{
public:
    explicit QKmsBufferManager(QKmsScreen *screen);
    ~QKmsBufferManager();
    void setupBuffersForMode(const drmModeModeInfo &mode, int numBuffers = 3);
    GLuint framebufferObject() const { return m_frameBufferObject; }
    quint32 displayFramebufferId();
    GLuint renderTargetBuffer();
    bool nextBuffer();

private:
    void clearBuffers();

    QKmsScreen *m_screen;
    QList<QKmsFramebuffer*> m_framebuffers;
    GLuint m_frameBufferObject;
    GLuint m_depthAndStencilBufferObject;

    QKmsFramebuffer *m_renderTarget;
    QKmsFramebuffer *m_displayCanidate;
    QKmsFramebuffer *m_currentDisplay;

};

QT_END_NAMESPACE

#endif // QKMSBUFFERMANAGER_H
