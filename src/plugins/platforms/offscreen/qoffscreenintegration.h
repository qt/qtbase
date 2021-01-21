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

#ifndef QOFFSCREENINTEGRATION_H
#define QOFFSCREENINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

#include <qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QOffscreenBackendData;

class QOffscreenIntegration : public QPlatformIntegration
{
public:
    QOffscreenIntegration();
    ~QOffscreenIntegration();

    void initialize() override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif

    QPlatformInputContext *inputContext() const override;
    QPlatformServices *services() const override;

    QPlatformFontDatabase *fontDatabase() const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;

    QPlatformNativeInterface *nativeInterface() const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    static QOffscreenIntegration *createOffscreenIntegration();

protected:
    QScopedPointer<QPlatformFontDatabase> m_fontDatabase;
#if QT_CONFIG(draganddrop)
    QScopedPointer<QPlatformDrag> m_drag;
#endif
    QScopedPointer<QPlatformInputContext> m_inputContext;
    QScopedPointer<QPlatformServices> m_services;
    mutable QScopedPointer<QPlatformNativeInterface> m_nativeInterface;
};

QT_END_NAMESPACE

#endif
