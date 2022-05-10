// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMFOREIGNWINDOW_H
#define QANDROIDPLATFORMFOREIGNWINDOW_H

#include "androidsurfaceclient.h"
#include "qandroidplatformwindow.h"

#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

class QAndroidPlatformForeignWindow : public QAndroidPlatformWindow
{
public:
    explicit QAndroidPlatformForeignWindow(QWindow *window, WId nativeHandle);
    ~QAndroidPlatformForeignWindow();
    void lower() override;
    void raise() override;
    void setGeometry(const QRect &rect) override;
    void setVisible(bool visible) override;
    void applicationStateChanged(Qt::ApplicationState state) override;
    void setParent(const QPlatformWindow *window) override;
    bool isForeignWindow() const override { return true; }

private:
    int m_surfaceId;
    QJniObject m_view;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMFOREIGNWINDOW_H
