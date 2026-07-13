// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoswindowutils.h"

#include <QtCore/qvariant.h>
#include <QtGui/private/qohoswindowhints_p.h>
#include <QtGui/qcolor.h>
#include <QtGui/qpa/qplatformwindow_p.h>
#include <QtGui/qwindow.h>
#include <QtWidgets/qwidget.h>
#include <optional>

QT_BEGIN_NAMESPACE

/*!
    \namespace QtOhosAppKit::Window
    \inmodule QtOhosAppKit
    \brief Applies OpenHarmony-specific hints to a QWindow or QWidget.

    Each hint is recorded on the passed QWindow or QWidget and consumed by the
    platform plugin. A hint may be set on a QWidget before its window exists; it
    is carried over to the window once the widget is shown.
*/

namespace QtOhosAppKit {

namespace Window {

/*!
    Sets or unsets a given \a window as a floating window according to
    \a showAsFloatWindow. The OpenHarmony window type is fixed at creation, so
    this has to be called before the window is shown.

    \code
    auto window = std::make_unique<MainWindow>();
    QtOhosAppKit::Window::setShowWindowAsFloatWindowHint(window.get(), true);
    window->show();
    \endcode
*/
void setShowWindowAsFloatWindowHint(QWindow *window, bool showAsFloatWindow)
{
    window->setProperty(QOhosWindowHints::floatWindowKey, QVariant::fromValue(showAsFloatWindow));
}

/*! \overload */
void setShowWindowAsFloatWindowHint(QWidget *widget, bool showAsFloatWindow)
{
    widget->setProperty(QOhosWindowHints::floatWindowKey, QVariant::fromValue(showAsFloatWindow));
}

/*!
    Changes the privacy mode of \a window according to \a privacyModeEnabled. In
    privacy mode the window content cannot be captured or recorded. This requires
    the ohos.permission.PRIVACY_WINDOW permission to be declared in the
    application's module.json5.
*/
void setWindowPrivacyMode(QWindow *window, bool privacyModeEnabled)
{
    window->setProperty(QOhosWindowHints::privacyModeKey, QVariant::fromValue(privacyModeEnabled));
}

/*!
    Sets a given \a color as the background color of the \a window inner native
    layer.
*/
void setSurfaceBackgroundColor(QWindow *window, const QColor &color)
{
    window->setProperty(QOhosWindowHints::surfaceBackgroundColorKey, QVariant::fromValue(color));
}

/*!
    \overload

    Sets a given \a color as the background color of the \a widget inner native
    layer.
*/
void setSurfaceBackgroundColor(QWidget *widget, const QColor &color)
{
    widget->setProperty(QOhosWindowHints::surfaceBackgroundColorKey, QVariant::fromValue(color));
}

/*!
    Sets the corner \a radius of a \a window. The window has to be a sub or
    floating window (for example a window with the Qt::Window, Qt::Popup or
    Qt::Dialog flags, or one marked with setShowWindowAsFloatWindowHint()). The
    \a radius must not be negative; there is no upper limit, though OHOS visually
    clamps it to the maximum achievable value.
*/
void setWindowCornerRadius(QWindow *window, double radius)
{
    if (radius < 0.0) {
        qWarning("QtOhosAppKit::Window::setWindowCornerRadius: radius must not be negative");
        return;
    }
    window->setProperty(QOhosWindowHints::cornerRadiusKey, QVariant::fromValue(radius));
}

/*!
    \overload

    Sets the corner \a radius of a \a widget. The widget has to be a window on its
    own (not embedded in a parent widget). The \a radius must not be negative.
*/
void setWindowCornerRadius(QWidget *widget, double radius)
{
    if (radius < 0.0) {
        qWarning("QtOhosAppKit::Window::setWindowCornerRadius: radius must not be negative");
        return;
    }
    widget->setProperty(QOhosWindowHints::cornerRadiusKey, QVariant::fromValue(radius));
}

/*!
    Sets or unsets a given \a window to keep the screen on according to
    \a keepScreenOn.
*/
void setWindowKeepScreenOn(QWindow *window, bool keepScreenOn)
{
    window->setProperty(QOhosWindowHints::keepScreenOnKey, QVariant::fromValue(keepScreenOn));
}

/*!
    \overload

    Sets or unsets a given \a widget to keep the screen on according to
    \a keepScreenOn.
*/
void setWindowKeepScreenOn(QWidget *widget, bool keepScreenOn)
{
    widget->setProperty(QOhosWindowHints::keepScreenOnKey, QVariant::fromValue(keepScreenOn));
}

/*!
    Enables or disables resizing of \a window by dragging its edges according to
    \a dragResizable. Takes effect on sub windows and floating windows, but not on
    the application's main window.
*/
void setWindowDragResizable(QWindow *window, bool dragResizable)
{
    window->setProperty(QOhosWindowHints::dragResizableKey, QVariant::fromValue(dragResizable));
}

/*!
    \overload

    Enables or disables resizing of \a widget by dragging its edges according to
    \a dragResizable. The widget has to be a window on its own (not embedded in a
    parent widget). Takes effect on sub windows and floating windows, but not on
    the application's main window.
*/
void setWindowDragResizable(QWidget *widget, bool dragResizable)
{
    widget->setProperty(QOhosWindowHints::dragResizableKey, QVariant::fromValue(dragResizable));
}

}

std::optional<double> tryGetNativeWindowId(QWindow *window)
{
    auto *ohosWindow = window->nativeInterface<QNativeInterface::Private::QOhosWindow>();
    if (!ohosWindow)
        return {};

    return ohosWindow->windowId();
}

}

QT_END_NAMESPACE
