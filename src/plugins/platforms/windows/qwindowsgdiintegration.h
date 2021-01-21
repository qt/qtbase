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
