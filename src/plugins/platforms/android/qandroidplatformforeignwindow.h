// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMFOREIGNWINDOW_H
#define QANDROIDPLATFORMFOREIGNWINDOW_H

#include "qandroidplatformwindow.h"

#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(View, "android/view/View")

class QAndroidPlatformForeignWindow : public QAndroidPlatformWindow
{
public:
    explicit QAndroidPlatformForeignWindow(QWindow *window, WId nativeHandle);
    void initialize() override;
    ~QAndroidPlatformForeignWindow();
    void setVisible(bool visible) override;
    void applicationStateChanged(Qt::ApplicationState state) override;
    bool isForeignWindow() const override { return true; }

    WId winId() const override;

private:
    void addViewToWindow();

    QtJniTypes::View m_view;
    bool m_nativeViewInserted;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMFOREIGNWINDOW_H
