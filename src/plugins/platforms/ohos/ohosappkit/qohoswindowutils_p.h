// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWUTILS_P_H
#define QOHOSWINDOWUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qcolor.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtOhosAppKit/private/qtohosappkitglobal_p.h>
#include <QtWidgets/qwidget.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace Window {

Q_NAMESPACE

enum class WindowGeometryPersistenceHint
{
    Disabled,
    Enabled,
    FollowSystemSetting,
};
Q_ENUM_NS(WindowGeometryPersistenceHint)

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

Q_OHOSAPPKIT_EXPORT void setMainWindowGeometryPersistenceHint(WindowGeometryPersistenceHint hint);

}

Q_OHOSAPPKIT_EXPORT std::optional<double> tryGetNativeWindowId(QWindow *window);

Q_OHOSAPPKIT_EXPORT std::optional<double> tryGetScreenDisplayId(QScreen *screen);

}

QT_END_NAMESPACE

#endif // QOHOSWINDOWUTILS_P_H
