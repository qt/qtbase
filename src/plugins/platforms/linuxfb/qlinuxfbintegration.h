// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLINUXFBINTEGRATION_H
#define QLINUXFBINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/private/qkeymapper_p.h>

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QFbScreen;
class QFbVtHandler;
class QEvdevKeyboardManager;

class QLinuxFbIntegration : public QPlatformIntegration, public QPlatformNativeInterface
#if QT_CONFIG(evdev)
    , public QNativeInterface::Private::QEvdevKeyMapper
#endif
{
public:
    QLinuxFbIntegration(const QStringList &paramList);
    ~QLinuxFbIntegration();

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

    QFunctionPointer platformFunction(const QByteArray &function) const override;

#if QT_CONFIG(evdev)
    void loadKeymap(const QString &filename) override;
    void switchLang() override;
#endif

private:
    void createInputHandlers();

    QFbScreen *m_primaryScreen;
    QPlatformInputContext *m_inputContext;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    QScopedPointer<QPlatformServices> m_services;
    QScopedPointer<QFbVtHandler> m_vtHandler;

    QEvdevKeyboardManager *m_kbdMgr;
};

QT_END_NAMESPACE

#endif // QLINUXFBINTEGRATION_H
