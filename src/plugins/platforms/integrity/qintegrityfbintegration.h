// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QINTEGRITYFBINTEGRATION_H
#define QINTEGRITYFBINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QIntegrityFbScreen;

class QIntegrityFbIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    QIntegrityFbIntegration(const QStringList &paramList);
    ~QIntegrityFbIntegration();

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
    void createInputHandlers();

    QIntegrityFbScreen *m_primaryScreen;
    QPlatformInputContext *m_inputContext;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    QScopedPointer<QPlatformServices> m_services;
    QScopedPointer<QPlatformNativeInterface> m_nativeInterface;
};

QT_END_NAMESPACE

#endif // QINTEGRITYFBINTEGRATION_H
