// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include "qxcbwindow.h"
#include "qxcbvulkaninstance.h"

QT_BEGIN_NAMESPACE

class QXcbVulkanWindow : public QXcbWindow
{
public:
    QXcbVulkanWindow(QWindow *window);
    ~QXcbVulkanWindow();

    VkSurfaceKHR *surface();

protected:
    void resolveFormat(const QSurfaceFormat &format) override;

private:
    VkSurfaceKHR m_surface;
};

QT_END_NAMESPACE
