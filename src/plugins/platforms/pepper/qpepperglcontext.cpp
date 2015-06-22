/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qglobal.h>

#ifndef QT_NO_OPENGL
#include "qpepperglcontext.h"

#include "qpepperinstance_p.h"

#include <QtCore/QDebug>

#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/graphics_3d_client.h>
#include <ppapi/gles2/gl2ext_ppapi.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_GLCONTEXT, "qt.platform.pepper.glcontext")

QPepperGLContext::QPepperGLContext()
    : m_pendingFlush(false)
    , m_callbackFactory(this)
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "QPepperGLContext";
}

QPepperGLContext::~QPepperGLContext() {}

QSurfaceFormat QPepperGLContext::format() const
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "format";

    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setAlphaBufferSize(8);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    return format;
}

void QPepperGLContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "swapBuffers";

    if (m_pendingFlush) {
        qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "swapBuffers overflush";
        return;
    }

    m_pendingFlush = true;
    m_context.SwapBuffers(m_callbackFactory.NewCallback(&QPepperGLContext::flushCallback));
    bool qtOnSecondaryThread = QPepperInstancePrivate::get()->m_runQtOnThread;
    while (qtOnSecondaryThread && m_pendingFlush) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
}

bool QPepperGLContext::makeCurrent(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "makeCurrent";

    if (m_context.is_null())
        initGl();

    QSize newSize = QPepperInstancePrivate::get()->deviceGeometry().size();
    if (newSize != m_currentSize) {
        int32_t result = m_context.ResizeBuffers(newSize.width(), newSize.height());
        if (result < 0) {
            qWarning() << "Unable to resize buffer to" << newSize;
            return false;
        }
        m_currentSize = newSize;
    }

    glSetCurrentContextPPAPI(m_context.pp_resource());
    return true;
}

void QPepperGLContext::doneCurrent()
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "doneCurrent";
    glSetCurrentContextPPAPI(0);
}

QFunctionPointer QPepperGLContext::getProcAddress(const QByteArray &procName)
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "getProcAddress" << procName;

    qWarning("UNIMPLEMENTED: QPepperGLContext::getProcAddress");

    // const PPB_OpenGLES2_Dev* functionPointers = glGetInterfacePPAPI();
    // if (procName == QByteArrayLiteral("glIsRenderbufferEXT")) {
    //   return reinterpret_cast<void *>(functionPointers->IsRenderbuffer);
    // }
    // ### ... and so on for all functions.

    return 0;
}

void QPepperGLContext::flushCallback(int32_t)
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "swapBuffers callback";
    m_pendingFlush = false;
}

bool QPepperGLContext::initGl()
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "initGl";
    if (!glInitializePPAPI(pp::Module::Get()->get_browser_interface())) {
        qWarning("Unable to initialize GL PPAPI!\n");
        return false;
    }
    m_currentSize = QPepperInstancePrivate::get()->geometry().size();
    QSurfaceFormat f = format();

    const int32_t attrib_list[] = {
      PP_GRAPHICS3DATTRIB_ALPHA_SIZE, f.alphaBufferSize(),
      PP_GRAPHICS3DATTRIB_DEPTH_SIZE, f.depthBufferSize(),
      PP_GRAPHICS3DATTRIB_STENCIL_SIZE, f.stencilBufferSize(),
      PP_GRAPHICS3DATTRIB_WIDTH, m_currentSize.width(),
      PP_GRAPHICS3DATTRIB_HEIGHT, m_currentSize.height(),
      PP_GRAPHICS3DATTRIB_NONE
    };

    pp::Instance *instance = QPepperInstancePrivate::getPPInstance();
    m_context = pp::Graphics3D(instance, attrib_list);
    if (!instance->BindGraphics(m_context)) {
        qWarning("Unable to bind 3d context!\n");
        m_context = pp::Graphics3D();
        glSetCurrentContextPPAPI(0);
        return false;
    }

    return true;
}

#endif
