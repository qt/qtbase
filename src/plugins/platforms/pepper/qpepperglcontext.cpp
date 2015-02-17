/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include <qglobal.h>

#ifndef QT_NO_OPENGL

#include "qpepperglcontext.h"
#include "qpepperinstance.h"
#include "qpepperinstance_p.h"

#include <qdebug.h>

#include <ppapi/gles2/gl2ext_ppapi.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/graphics_3d_client.h>

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_GLCONTEXT, "qt.platform.pepper.glcontext")

QPepperGLContext::QPepperGLContext()
    : m_pendingFlush(false)
    , m_callbackFactory(this)
{
    qCDebug(QT_PLATFORM_PEPPER_GLCONTEXT) << "QPepperGLContext";
}

QPepperGLContext::~QPepperGLContext() {}

bool QPepperGLContext::makeCurrent(QPlatformSurface *surface)
{
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

void QPepperGLContext::flushCallback(int32_t) { m_pendingFlush = false; }

//    virtual void (*getProcAddress(const QByteArray&));

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

    QPepperInstance *instance = QPepperInstancePrivate::getInstance();
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
