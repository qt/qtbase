/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/private/qopenglcontext_p.h>
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
