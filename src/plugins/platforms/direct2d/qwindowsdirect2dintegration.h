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

#ifndef QWINDOWSDIRECT2DINTEGRATION_H
#define QWINDOWSDIRECT2DINTEGRATION_H

#include "qwindowsintegration.h"

#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DContext;
class QWindowsDirect2DIntegrationPrivate;

class QWindowsDirect2DIntegration : public QWindowsIntegration
{
public:
    static QWindowsDirect2DIntegration *create(const QStringList &paramList);

    virtual ~QWindowsDirect2DIntegration();

    static QWindowsDirect2DIntegration *instance();

    QPlatformNativeInterface *nativeInterface() const override;
    QPlatformPixmap *createPlatformPixmap(QPlatformPixmap::PixelType type) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QWindowsDirect2DContext *direct2DContext() const;

protected:
    QWindowsWindow *createPlatformWindowHelper(QWindow *window, const QWindowsWindowData &) const override;

private:
    explicit QWindowsDirect2DIntegration(const QStringList &paramList);
    bool init();

    QScopedPointer<QWindowsDirect2DIntegrationPrivate> d;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DINTEGRATION_H
