
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformcursor.h"
#include "qohosplatformintegration.h"
#include "qohosfloatingwindow.h"
#include "qohosinputmethodeventhandler.h"
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
    auto *platformIntegration = QOhosPlatformIntegration::instance();
    auto *inputMethodEventHandler = platformIntegration->inputMethodEventHandler();
    return inputMethodEventHandler->cursorPosition();
}

QT_END_NAMESPACE
