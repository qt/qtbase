/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QANDROIDPLATFORMFOREIGNWINDOW_H
#define QANDROIDPLATFORMFOREIGNWINDOW_H

#include "androidsurfaceclient.h"
#include "qandroidplatformwindow.h"
#include <QtCore/private/qjni_p.h>

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
    QJNIObjectPrivate m_view;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMFOREIGNWINDOW_H
