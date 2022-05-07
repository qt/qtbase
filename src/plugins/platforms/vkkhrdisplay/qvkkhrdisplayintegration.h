/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
