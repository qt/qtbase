// Copyright (C) 2018 QNX Software Systems. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXFOREIGNWINDOW_H
#define QQNXFOREIGNWINDOW_H

#include "qqnxwindow.h"

QT_BEGIN_NAMESPACE

class QQnxForeignWindow : public QQnxWindow
{
public:
    QQnxForeignWindow(QWindow *window,
                      screen_context_t context,
                      screen_window_t screenWindow);

    bool isForeignWindow() const override;
    int pixelFormat() const override;
    void resetBuffers() override {}
};

QT_END_NAMESPACE

#endif // QQNXFOREIGNWINDOW_H
