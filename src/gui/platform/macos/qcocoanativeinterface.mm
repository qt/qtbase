// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtgui-config.h>
#ifndef QT_NO_OPENGL
#  include <QtGui/private/qopenglcontext_p.h>
#endif

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformwindow_p.h>
#include <qpa/qplatformmenu_p.h>

#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

/*!
    \class QNativeInterface::Private::QCocoaWindow
    \since 6.0
    \internal
    \brief Native interface for QPlatformWindow on \macos.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QCocoaWindow);


/*!
    \class QNativeInterface::Private::QCocoaMenu
    \since 6.0
    \internal
    \brief Native interface for QPlatformMenu on \macos.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QCocoaMenu);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QCocoaMenuBar);

#ifndef QT_NO_OPENGL

/*!
    \class QNativeInterface::QCocoaGLContext
    \since 6.0
    \brief Native interface to an NSOpenGLContext on \macos.

    Accessed through QOpenGLContext::nativeInterface().

    \inmodule QtGui
    \inheaderfile QOpenGLContext
    \ingroup native-interfaces
    \ingroup native-interfaces-qopenglcontext
*/

/*!
    \fn QOpenGLContext *QNativeInterface::QCocoaGLContext::fromNative(NSOpenGLContext *context, QOpenGLContext *shareContext = nullptr)

    \brief Adopts an NSOpenGLContext.

    The adopted NSOpenGLContext \a context is retained. Ownership of the
    created QOpenGLContext \a shareContext is transferred to the caller.
*/

/*!
    \fn NSOpenGLContext *QNativeInterface::QCocoaGLContext::nativeContext() const

    \return the underlying NSOpenGLContext.
*/

QT_DEFINE_NATIVE_INTERFACE(QCocoaGLContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QCocoaGLIntegration);

QOpenGLContext *QNativeInterface::QCocoaGLContext::fromNative(NSOpenGLContext *nativeContext, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QCocoaGLIntegration::createOpenGLContext>(nativeContext, shareContext);
}

#endif // QT_NO_OPENGL

QT_END_NAMESPACE
