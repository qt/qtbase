// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QMINIMALEGLWINDOW_H
#define QMINIMALEGLWINDOW_H

#include "qminimaleglintegration.h"

#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

class QMinimalEglWindow : public QPlatformWindow
{
public:
    QMinimalEglWindow(QWindow *w);

    void setGeometry(const QRect &) override;
    WId winId() const override;

private:
    WId m_winid;
};
QT_END_NAMESPACE
#endif // QMINIMALEGLWINDOW_H
