// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSWINDOWCLASSDESCRIPTION_H
#define QWINDOWSWINDOWCLASSDESCRIPTION_H

#include "qtwindowsglobal.h"

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QWindow;

struct QWindowsWindowClassDescription
{
    enum class WindowStyleOption
    {
        None = 0x00,
        GLSurface = 0x01,
        DropShadow = 0x02
    };
    Q_DECLARE_FLAGS(WindowStyleOptions, WindowStyleOption)

    static QWindowsWindowClassDescription fromName(QString name, WNDPROC procedure);
    static QWindowsWindowClassDescription fromWindow(const QWindow *window, WNDPROC procedure);

    QString name;
    WNDPROC procedure{ DefWindowProc };
    unsigned int style{ 0 };
    HBRUSH brush{ nullptr };
    bool hasIcon{ false };
    bool shouldAddPrefix{ true };

private:
    static QString classNameSuffix(Qt::WindowFlags type, unsigned int style, bool hasIcon);
    static bool computeHasIcon(Qt::WindowFlags flags, Qt::WindowFlags type);
    static unsigned int computeWindowStyles(Qt::WindowFlags flags, Qt::WindowFlags type, WindowStyleOptions options);

    friend QDebug operator<<(QDebug dbg, const QWindowsWindowClassDescription &description);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsWindowClassDescription::WindowStyleOptions)

QT_END_NAMESPACE

#endif // QWINDOWSWINDOWCLASSDESCRIPTION_H
