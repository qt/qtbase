// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QWINDOWSGDIINTEGRATION_H
#define QWINDOWSGDIINTEGRATION_H

#include "qwindowsintegration.h"

QT_BEGIN_NAMESPACE

class QWindowsGdiIntegrationPrivate;
class QWindowsGdiIntegration : public QWindowsIntegration
{
public:
    explicit QWindowsGdiIntegration(const QStringList &paramList);
    ~QWindowsGdiIntegration() override;

    QPlatformNativeInterface *nativeInterface() const override;
    QPlatformPixmap *createPlatformPixmap(QPlatformPixmap::PixelType type) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

private:
    QScopedPointer<QWindowsGdiIntegrationPrivate> d;
};

QT_END_NAMESPACE

#endif // QWINDOWSGDIINTEGRATION_H
