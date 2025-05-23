// Copyright (C) 2018 QNX Software Systems. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
