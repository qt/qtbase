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

#ifndef QEGLFSEMULATORINTEGRATION_H
#define QEGLFSEMULATORINTEGRATION_H

#include "private/qeglfsdeviceintegration_p.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QFunctionPointer>

typedef QByteArray (EGLAPIENTRYP PFNQGSGETDISPLAYSPROC) ();
typedef void (EGLAPIENTRYP PFNQGSSETDISPLAYPROC) (uint screen);

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsEmuDebug)

class QEglFSEmulatorIntegration : public QEglFSDeviceIntegration
{
public:
    QEglFSEmulatorIntegration();

    void platformInit() override;
    void platformDestroy() override;
    bool usesDefaultScreen() override;
    void screenInit() override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    PFNQGSGETDISPLAYSPROC getDisplays;
    PFNQGSSETDISPLAYPROC setDisplay;

    EGLNativeWindowType createNativeWindow(QPlatformWindow *platformWindow, const QSize &size, const QSurfaceFormat &format) override;
};

QT_END_NAMESPACE

#endif // QEGLFSEMULATORINTEGRATION_H
