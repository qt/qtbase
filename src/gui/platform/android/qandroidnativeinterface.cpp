// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformoffscreensurface.h>
#include <qpa/qplatformintegration.h>

#include <QtGui/qoffscreensurface_platform.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtGui/qpa/qplatformscreen_p.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

/*!
    \class QNativeInterface::QAndroidOffscreenSurface
    \since 6.0
    \brief Native interface to a offscreen surface on Android.

    Accessed through QOffscreenSurface::nativeInterface().

    \inmodule QtGui
    \inheaderfile QOffscreenSurface
    \ingroup native-interfaces
    \ingroup native-interfaces-qoffscreensurface
*/

QT_DEFINE_NATIVE_INTERFACE(QAndroidOffscreenSurface);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QAndroidOffScreenIntegration);

QOffscreenSurface  *QNativeInterface::QAndroidOffscreenSurface::fromNative(ANativeWindow *nativeSurface)
{
    return QGuiApplicationPrivate::platformIntegration()->call<
            &QAndroidOffScreenIntegration::createOffscreenSurface>(nativeSurface);
}

/*!
    \class QNativeInterface::QAndroidScreen
    \since 6.7
    \brief Native interface to a screen.

    Accessed through QScreen::nativeInterface().
    \inmodule QtGui
    \ingroup native-interfaces
    \ingroup native-interfaces-qscreen
*/
/*!
    \fn int QNativeInterface::QAndroidScreen::displayId() const;
    \return the id of the underlying Android display.
*/
QT_DEFINE_NATIVE_INTERFACE(QAndroidScreen);

QT_END_NAMESPACE
