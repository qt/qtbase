
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformcursor.h"
#include "qohospointerstyle.h"
#include "qohosfloatingwindow.h"
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

void QOhosPlatformCursor::changeCursor(QCursor *cursor, QWindow *window)
{
    if (cursor == nullptr)
        return;

    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(window);
    if (platformWindow != nullptr)
        platformWindow->setCursor(*cursor);
}

QPoint QOhosPlatformCursor::pos() const
{
    return QHighDpi::toNativePixels(QPlatformCursor::pos(), QGuiApplication::primaryScreen());
}

QT_END_NAMESPACE
