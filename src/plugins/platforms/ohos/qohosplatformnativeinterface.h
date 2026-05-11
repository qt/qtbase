// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMNATIVEINTERFACE_H
#define QOHOSPLATFORMNATIVEINTERFACE_H
#include <QtCore/qglobal.h>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformNativeInterface : public QPlatformNativeInterface
{
public:
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;

protected:
    void customEvent(QEvent *event) override;

    QFunctionPointer platformFunction(const QByteArray &functionName) const override;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMNATIVEINTERFACE_H
