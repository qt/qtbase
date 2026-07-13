// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWUTILS_H
#define QOHOSWINDOWUTILS_H

#include <QtGui/qcolor.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>
#include <QtWidgets/qwidget.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace Window {

Q_OHOSAPPKIT_EXPORT void setShowWindowAsFloatWindowHint(QWindow *window, bool showAsFloatWindow);
Q_OHOSAPPKIT_EXPORT void setShowWindowAsFloatWindowHint(QWidget *widget, bool showAsFloatWindow);

Q_OHOSAPPKIT_EXPORT void setWindowPrivacyMode(QWindow *window, bool privacyModeEnabled);

Q_OHOSAPPKIT_EXPORT void setSurfaceBackgroundColor(QWindow *window, const QColor &color);
Q_OHOSAPPKIT_EXPORT void setSurfaceBackgroundColor(QWidget *widget, const QColor &color);

Q_OHOSAPPKIT_EXPORT void setWindowCornerRadius(QWindow *window, double radius);
Q_OHOSAPPKIT_EXPORT void setWindowCornerRadius(QWidget *widget, double radius);

Q_OHOSAPPKIT_EXPORT void setWindowKeepScreenOn(QWindow *window, bool keepScreenOn);
Q_OHOSAPPKIT_EXPORT void setWindowKeepScreenOn(QWidget *widget, bool keepScreenOn);

Q_OHOSAPPKIT_EXPORT void setWindowDragResizable(QWindow *window, bool dragResizable);
Q_OHOSAPPKIT_EXPORT void setWindowDragResizable(QWidget *widget, bool dragResizable);

}

Q_OHOSAPPKIT_EXPORT std::optional<double> tryGetNativeWindowId(QWindow *window);

Q_OHOSAPPKIT_EXPORT std::optional<double> tryGetScreenDisplayId(QScreen *screen);

}

QT_END_NAMESPACE

#endif // QOHOSWINDOWUTILS_H
