// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBSDFBINTEGRATION_H
#define QBSDFBINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QBsdFbScreen;
class QFbVtHandler;

class QBsdFbIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    explicit QBsdFbIntegration(const QStringList &paramList);
    ~QBsdFbIntegration() override;

    void initialize() override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;

    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformServices *services() const override;
    QPlatformInputContext *inputContext() const override { return m_inputContext.data(); }

    QPlatformNativeInterface *nativeInterface() const override;

    QList<QPlatformScreen *> screens() const;

private:
    void createInputHandlers();

    QScopedPointer<QBsdFbScreen> m_primaryScreen;
    QScopedPointer<QPlatformInputContext> m_inputContext;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    mutable QScopedPointer<QPlatformServices> m_services;
    QScopedPointer<QFbVtHandler> m_vtHandler;
    QScopedPointer<QPlatformNativeInterface> m_nativeInterface;
};

QT_END_NAMESPACE

#endif // QBSDFBINTEGRATION_H
