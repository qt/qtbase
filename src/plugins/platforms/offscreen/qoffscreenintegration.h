// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOFFSCREENINTEGRATION_H
#define QOFFSCREENINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

#include <qscopedpointer.h>
#include <qjsonobject.h>

QT_BEGIN_NAMESPACE

class QOffscreenBackendData;
class QOffscreenScreen;

class QOffscreenIntegration : public QPlatformIntegration
{
public:
    QOffscreenIntegration(const QStringList& paramList);
    ~QOffscreenIntegration();

    QJsonObject defaultConfiguration() const;
    std::optional<QJsonObject> resolveConfigFileConfiguration(const QStringList& paramList) const;
    void setConfiguration(const QJsonObject &configuration);
    QJsonObject configuration() const;

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

    static QOffscreenIntegration *createOffscreenIntegration(const QStringList& paramList);

    QList<QOffscreenScreen *> screens() const;
protected:
    QScopedPointer<QPlatformFontDatabase> m_fontDatabase;
#if QT_CONFIG(draganddrop)
    QScopedPointer<QPlatformDrag> m_drag;
#endif
    QScopedPointer<QPlatformInputContext> m_inputContext;
    mutable QScopedPointer<QPlatformServices> m_services;
    mutable QScopedPointer<QPlatformNativeInterface> m_nativeInterface;
    QList<QOffscreenScreen *> m_screens;
    bool m_windowFrameMarginsEnabled = true;
    QJsonObject m_configuration;
};

QT_END_NAMESPACE

#endif
