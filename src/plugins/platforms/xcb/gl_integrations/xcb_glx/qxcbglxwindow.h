// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "qxcbglxintegration.h"
#include "qxcbwindow.h"

QT_BEGIN_NAMESPACE

class QXcbGlxWindow : public QXcbWindow
{
public:
    QXcbGlxWindow(QWindow *window);
    ~QXcbGlxWindow();

protected:
    const xcb_visualtype_t *createVisual() override;
};

QT_END_NAMESPACE
