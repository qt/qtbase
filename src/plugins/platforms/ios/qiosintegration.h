// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATION_UIKIT_H
#define QPLATFORMINTEGRATION_UIKIT_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/private/qfactoryloader_p.h>

#include "qiosapplicationstate.h"

#if !defined(Q_OS_TVOS) && !defined(Q_OS_VISIONOS)
#include "qiostextinputoverlay.h"
#endif

#if defined(Q_OS_VISIONOS)
#include <swift/bridging>
#endif

QT_BEGIN_NAMESPACE

using namespace QNativeInterface;

class QIOSServices;

class
#if defined(Q_OS_VISIONOS)
    SWIFT_IMMORTAL_REFERENCE
#endif
QIOSIntegration : public QPlatformNativeInterface, public QPlatformIntegration
#if defined(Q_OS_VISIONOS)
    , public QVisionOSApplication
#endif
{
    Q_OBJECT
public:
    QIOSIntegration();
    ~QIOSIntegration();

    void initialize() override;

    bool hasCapability(Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

#if QT_CONFIG(opengl)
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
#endif

    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;

    QPlatformFontDatabase *fontDatabase() const override;

#if QT_CONFIG(clipboard)
    QPlatformClipboard *clipboard() const override;
#endif

    QPlatformInputContext *inputContext() const override;
    QPlatformServices *services() const override;

    QVariant styleHint(StyleHint hint) const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformNativeInterface *nativeInterface() const override;

    QPointingDevice *touchDevice();
#if QT_CONFIG(accessibility)
    QPlatformAccessibility *accessibility() const override;
#endif

    void beep() const override;

    void setApplicationBadge(qint64 number) override;

    static QIOSIntegration *instance();

    // -- QPlatformNativeInterface --

    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;

    QFactoryLoader *optionalPlugins() { return m_optionalPlugins; }

    QIOSApplicationState applicationState;

#if defined(Q_OS_VISIONOS)
    void openImmersiveSpace() override;
    void dismissImmersiveSpace() override;

    using CompositorLayer = QVisionOSApplication::ImmersiveSpaceCompositorLayer;
    void setImmersiveSpaceCompositorLayer(CompositorLayer *layer) override;

    void configureCompositorLayer(cp_layer_renderer_capabilities_t, cp_layer_renderer_configuration_t);
    void renderCompositorLayer(cp_layer_renderer_t);
    void handleSpatialEvents(const char *jsonString);
#endif

private:
    QPlatformFontDatabase *m_fontDatabase;
#if QT_CONFIG(clipboard)
    QPlatformClipboard *m_clipboard;
#endif
    QPlatformInputContext *m_inputContext;
    QPointingDevice *m_touchDevice;
    QIOSServices *m_platformServices;
    mutable QPlatformAccessibility *m_accessibility;
    QFactoryLoader *m_optionalPlugins;
#if !defined(Q_OS_TVOS) && !defined(Q_OS_VISIONOS)
    QIOSTextInputOverlay m_textInputOverlay;
#endif

#if defined(Q_OS_VISIONOS)
    CompositorLayer *m_immersiveSpaceCompositorLayer = nullptr;
#endif
};

QT_END_NAMESPACE

#endif
