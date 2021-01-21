/***************************************************************************
**
** Copyright (C) 2018 QNX Software Systems. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
