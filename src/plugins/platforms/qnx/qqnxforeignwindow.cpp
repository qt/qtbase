// Copyright (C) 2018 QNX Software Systems. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxforeignwindow.h"
#include "qqnxintegration.h"

QT_BEGIN_NAMESPACE

QQnxForeignWindow::QQnxForeignWindow(QWindow *window,
                                     screen_context_t context,
                                     screen_window_t screenWindow)
    : QQnxWindow(window, context, screenWindow)
{
    initWindow();
}

bool QQnxForeignWindow::isForeignWindow() const
{
    return true;
}

int QQnxForeignWindow::pixelFormat() const
{
    int result = SCREEN_FORMAT_RGBA8888;
    screen_get_window_property_iv(nativeHandle(), SCREEN_PROPERTY_FORMAT, &result);
    return result;
}

QT_END_NAMESPACE
