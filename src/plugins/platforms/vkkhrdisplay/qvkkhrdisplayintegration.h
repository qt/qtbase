// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATION_VKKHRDISPLAY_H
#define QPLATFORMINTEGRATION_VKKHRDISPLAY_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QVkKhrDisplayScreen;
class QVkKhrDisplayVulkanInstance;
class QFbVtHandler;

class QVkKhrDisplayIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    explicit QVkKhrDisplayIntegration(const QStringList &parameters);
    ~QVkKhrDisplayIntegration();

    void initialize() override;

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformServices *services() const override;
    QPlatformInputContext *inputContext() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QPlatformNativeInterface *nativeInterface() const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;

    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;

private:
    static void handleInstanceCreated(QVkKhrDisplayVulkanInstance *, void *);
    void createInputHandlers();

    mutable QPlatformFontDatabase *m_fontDatabase = nullptr;
    mutable QPlatformServices *m_services = nullptr;
    QPlatformInputContext *m_inputContext = nullptr;
    QFbVtHandler *m_vtHandler = nullptr;
    QVkKhrDisplayScreen *m_primaryScreen = nullptr;
};

QT_END_NAMESPACE

#endif
