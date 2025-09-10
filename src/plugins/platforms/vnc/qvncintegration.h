// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVNCINTEGRATION_H
#define QVNCINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QVncScreen;
class QVncServer;

class QVncIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    QVncIntegration(const QStringList &paramList);
    ~QVncIntegration();

    void initialize() override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;

    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformServices *services() const override;
    QPlatformInputContext *inputContext() const override { return m_inputContext; }

    QPlatformNativeInterface *nativeInterface() const override;

    QList<QPlatformScreen *> screens() const;

private:
    mutable QVncServer *m_server;
    QVncScreen *m_primaryScreen;
    QPlatformInputContext *m_inputContext;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    mutable QScopedPointer<QPlatformServices> m_services;
    QScopedPointer<QPlatformNativeInterface> m_nativeInterface;
};

QT_END_NAMESPACE

#endif // QVNCINTEGRATION_H
