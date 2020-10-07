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

#include <QtGui/private/qtguiglobal_p.h>

#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen_p.h>
#include <qpa/qplatformwindow_p.h>

#include <QtGui/private/qkeymapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

#ifndef QT_NO_OPENGL

#if defined(Q_OS_LINUX)
QT_DEFINE_NATIVE_INTERFACE(QGLXContext, QOpenGLContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QGLXIntegration);

QOpenGLContext *QNativeInterface::QGLXContext::fromNative(GLXContext configBasedContext, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QGLXIntegration::createOpenGLContext>(configBasedContext, nullptr, shareContext);
}

QOpenGLContext *QNativeInterface::QGLXContext::fromNative(GLXContext visualBasedContext, void *visualInfo, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QGLXIntegration::createOpenGLContext>(visualBasedContext, visualInfo, shareContext);
}
#endif

#if QT_CONFIG(egl)
QT_DEFINE_NATIVE_INTERFACE(QEGLContext, QOpenGLContext);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QEGLIntegration);

QOpenGLContext *QNativeInterface::QEGLContext::fromNative(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
        &QEGLIntegration::createOpenGLContext>(context, display, shareContext);
}
#endif

#endif // QT_NO_OPENGL

#if QT_CONFIG(xcb)
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QXcbScreen);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QXcbWindow);
#endif

#if QT_CONFIG(vsp2)
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QVsp2Screen);
#endif

#if QT_CONFIG(evdev)
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QEvdevKeyMapper);

template <>
QEvdevKeyMapper *QKeyMapper::nativeInterface<QEvdevKeyMapper>() const
{
    return dynamic_cast<QEvdevKeyMapper*>(QGuiApplicationPrivate::platformIntegration());
}
#endif

QT_END_NAMESPACE
