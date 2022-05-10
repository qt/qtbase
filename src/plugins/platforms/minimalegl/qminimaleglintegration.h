// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMINIMALEGLINTEGRATION_H
#define QMINIMALEGLINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QMinimalEglIntegration : public QPlatformIntegration
{
public:
    QMinimalEglIntegration();
    ~QMinimalEglIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
#endif
    QPlatformFontDatabase *fontDatabase() const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;

    QVariant styleHint(QPlatformIntegration::StyleHint hint) const override;

private:
    QPlatformFontDatabase *mFontDb;
    QPlatformScreen *mScreen;
};

QT_END_NAMESPACE

#endif // QMINIMALEGLINTEGRATION_H
