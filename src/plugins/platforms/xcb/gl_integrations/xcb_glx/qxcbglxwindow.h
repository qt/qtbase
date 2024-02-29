// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
