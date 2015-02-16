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

#ifndef QPLATFORMGLCONTEXT_PEPPER_H
#define QPLATFORMGLCONTEXT_PEPPER_H

#include <qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtCore/qloggingcategory.h>
#include <qpa/qplatformopenglcontext.h>

#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/graphics_3d_client.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/utility/completion_callback_factory.h>

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_GLCONTEXT)

class QPepperInstance;
class QPepperGLContext : public QPlatformOpenGLContext
{
public:
    explicit QPepperGLContext();
    virtual ~QPepperGLContext();

    virtual bool makeCurrent(QPlatformSurface *);
    virtual void doneCurrent();
    virtual void swapBuffers(QPlatformSurface *);
    void flushCallback(int32_t);
    virtual QFunctionPointer getProcAddress(const QByteArray &);

    virtual QSurfaceFormat format() const;

private:
    bool initGl();
    QSize m_currentSize;
    pp::Graphics3D m_context;
    bool m_pendingFlush;
    pp::CompletionCallbackFactory<QPepperGLContext> m_callbackFactory;
};

#endif

#endif
