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

#ifndef QPLATFORMINTEGRATION_UIKIT_H
#define QPLATFORMINTEGRATION_UIKIT_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/private/qfactoryloader_p.h>

#include "qiosapplicationstate.h"
#ifndef Q_OS_TVOS
#include "qiostextinputoverlay.h"
#endif

QT_BEGIN_NAMESPACE

class QIOSServices;

class QIOSIntegration : public QPlatformNativeInterface, public QPlatformIntegration
{
    Q_OBJECT
public:
    QIOSIntegration();
    ~QIOSIntegration();

    void initialize() override;

    bool hasCapability(Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;

    QPlatformFontDatabase *fontDatabase() const override;
#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *clipboard() const override;
#endif
    QPlatformInputContext *inputContext() const override;
    QPlatformServices *services() const override;

    QVariant styleHint(StyleHint hint) const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformNativeInterface *nativeInterface() const override;

    QTouchDevice *touchDevice();
#ifndef QT_NO_ACCESSIBILITY
    QPlatformAccessibility *accessibility() const override;
#endif

    void beep() const override;

    static QIOSIntegration *instance();

    // -- QPlatformNativeInterface --

    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;

    QFactoryLoader *optionalPlugins() { return m_optionalPlugins; }

    QIOSApplicationState applicationState;

private:
    QPlatformFontDatabase *m_fontDatabase;
#ifndef Q_OS_TVOS
    QPlatformClipboard *m_clipboard;
#endif
    QPlatformInputContext *m_inputContext;
    QTouchDevice *m_touchDevice;
    QIOSServices *m_platformServices;
    mutable QPlatformAccessibility *m_accessibility;
    QFactoryLoader *m_optionalPlugins;
#ifndef Q_OS_TVOS
    QIOSTextInputOverlay m_textInputOverlay;
#endif
};

QT_END_NAMESPACE

#endif
