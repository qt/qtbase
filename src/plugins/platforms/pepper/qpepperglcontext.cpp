/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include <qglobal.h>

#ifndef QT_NO_OPENGL

#include "qpepperglcontext.h"
#include "qpepperinstance.h"

#include <ppapi/gles2/gl2ext_ppapi.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/graphics_3d_client.h>
#include <qdebug.h>

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_GLCONTEXT, "qt.platform.pepper.glcontext")

QPepperGLContext::QPepperGLContext()
    :m_pendingFlush(false)
    ,m_callbackFactory(this)
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "QPepperGLContext";

}

QPepperGLContext::~QPepperGLContext()
{

}

bool QPepperGLContext::makeCurrent(QPlatformSurface *surface)
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "makeCurrent";

    if (m_context.is_null())
        initGl();

    QSize newSize = QPepperInstance::get()->geometry().size();
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

void QPepperGLContext::swapBuffers(QPlatformSurface *surface)
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "swapBuffers";

    if (m_pendingFlush) {
        qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "swapBuffers overflush";
        return;
    }

    m_pendingFlush = true;
    m_context.SwapBuffers(m_callbackFactory.NewCallback(&QPepperGLContext::flushCallback));
}

void QPepperGLContext::flushCallback(int32_t)
{
    m_pendingFlush = false;
}

//    virtual void (*getProcAddress(const QByteArray&));

QFunctionPointer QPepperGLContext::getProcAddress(const QByteArray& procName)
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

QSurfaceFormat QPepperGLContext::format() const
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "format";

    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    return format;
}

bool QPepperGLContext::initGl()
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "initGl";
    if (!glInitializePPAPI(pp::Module::Get()->get_browser_interface())) {
      qWarning("Unable to initialize GL PPAPI!\n");
      return false;
    }
    QPepperInstance *instance = QPepperInstance::get();
    m_currentSize = instance->geometry().size();

    const int32_t attrib_list[] = {
      PP_GRAPHICS3DATTRIB_ALPHA_SIZE, 8,
      PP_GRAPHICS3DATTRIB_DEPTH_SIZE, 24,
      PP_GRAPHICS3DATTRIB_WIDTH, m_currentSize.width(),
      PP_GRAPHICS3DATTRIB_HEIGHT, m_currentSize.height(),
      PP_GRAPHICS3DATTRIB_NONE
    };

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
