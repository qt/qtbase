// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtGui/qwindow.h>
#include <QtGui/qpa/qplatformscreen_p.h>
#include <QtGui/qpa/qplatformwindow_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

/*!
    \class QNativeInterface::Private::QOhosScreen
    \since 6.12
    \internal
    \brief Native interface to a screen on OpenHarmony.
    \inmodule QtGui
    \ingroup native-interfaces
*/
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QOhosScreen);

/*!
    \class QNativeInterface::Private::QOhosWindow
    \since 6.12
    \internal
    \brief Native interface to a window on OpenHarmony.
    \inmodule QtGui
    \ingroup native-interfaces
*/
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QOhosWindow);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QOhosIntegration);

/*!
    \class QNativeInterface::QHarmonyWindow
    \brief Native interface to a window on HarmonyOS.

    Accessed through QWindow::nativeInterface().

    On HarmonyOS, an ArkTS \c UIAbility instance shows a top-level QWindow
    in a \c window.Window. Platform APIs that are bound to a window need
    these objects, for example a picker that opens over the window, or an
    ability started from it. This interface provides them as
    QHarmonyJsObjectRef handles, which you dereference on the JS thread.
    Only top-level windows are shown this way. For a child window, both
    functions return null references. Use the interface of its top-level
    window instead.

    Both functions return at once, without waiting for the JS thread. The
    objects are available when QWindow::show() returns and stay the same
    while the window stays visible and top-level. After the window is
    hidden or reparented, the functions may return null references.
    Showing the window again, or recreating it with QWindow::destroy()
    and show(), may give different objects.

    This interface does not track what the platform does with the
    objects. If the platform destroys the ability or the
    \c window.Window, the functions may still return references to the
    destroyed objects, and code that uses the references on the JS
    thread must handle that.

    \inmodule QtGui
    \inheaderfile QWindow
    \ingroup native-interfaces
    \ingroup native-interfaces-qwindow

    \sa {Qt for HarmonyOS Platform API}
*/
QT_DEFINE_NATIVE_INTERFACE(QHarmonyWindow);

/*!
    \fn QHarmonyJsObjectRef QNativeInterface::QHarmonyWindow::ability() const

    Returns the \c UIAbility instance that shows the window. Returns a null
    reference if no ability shows the window, for example if the platform
    has not attached the window to an ability yet.
*/

/*!
    \fn QHarmonyJsObjectRef QNativeInterface::QHarmonyWindow::jsWindow() const

    Returns the \c window.Window that the window is shown in, or a null
    reference if the window is not shown in one.
*/

QWindow *QOhosWindow::fromNative(ArkUI_NodeHandle content)
{
    if (!content)
        return nullptr;

    auto *integration = QGuiApplicationPrivate::platformIntegration();
    const WId id = integration->call<&QOhosIntegration::windowHandle>(content);
    return id ? QWindow::fromWinId(id) : nullptr;
}

QT_END_NAMESPACE
