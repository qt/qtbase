// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMINTERATION_H
#define QANDROIDPLATFORMINTERATION_H

#include "qandroidinputcontext.h"
#include "qandroidplatformscreen.h"

#include <QtGui/qtguiglobal.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformmenu.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformoffscreensurface.h>
#include <qpa/qplatformtheme.h>
#include <private/qflatmap_p.h>
#include <QtCore/qvarlengtharray.h>

#include <EGL/egl.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QAndroidPlatformServices;
class QAndroidSystemLocale;
class QPlatformAccessibility;

struct AndroidStyle;
class QAndroidPlatformNativeInterface: public QPlatformNativeInterface
{
public:
    void *nativeResourceForIntegration(const QByteArray &resource) override;
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
    std::shared_ptr<AndroidStyle> m_androidStyle;

protected:
    void customEvent(QEvent *event) override;
};

class QAndroidPlatformIntegration : public QPlatformIntegration
#if QT_CONFIG(egl)
                                  , QNativeInterface::Private::QEGLIntegration
                                  , QNativeInterface::Private::QAndroidOffScreenIntegration
#endif
{
    friend class QAndroidPlatformScreen;

public:
    QAndroidPlatformIntegration(const QStringList &paramList);
    ~QAndroidPlatformIntegration();

    void initialize() override;

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
#if QT_CONFIG(egl)
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const override;
#endif
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QAndroidPlatformScreen *screen() { return m_primaryScreen; }
#if QT_CONFIG(egl)
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
    QOffscreenSurface *createOffscreenSurface(ANativeWindow *nativeSurface) const override;
#endif

    void setAvailableGeometry(const QRect &availableGeometry);
    void setPhysicalSize(int width, int height);
    void setScreenSize(int width, int height);
    void setRefreshRate(qreal refreshRate);
    bool isVirtualDesktop() { return true; }

    QPlatformFontDatabase *fontDatabase() const override;

    void handleScreenAdded(int displayId);
    void handleScreenChanged(int displayId);
    void handleScreenRemoved(int displayId);

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *clipboard() const override;
#endif

    QPlatformInputContext *inputContext() const override;
    QPlatformNativeInterface *nativeInterface() const override;
    QPlatformServices *services() const override;

#if QT_CONFIG(accessibility)
    virtual QPlatformAccessibility *accessibility() const override;
#endif

    QVariant styleHint(StyleHint hint) const override;
    Qt::WindowState defaultWindowState(Qt::WindowFlags flags) const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    static void setScreenOrientation(Qt::ScreenOrientation currentOrientation,
                                     Qt::ScreenOrientation nativeOrientation);

    QPointingDevice *touchDevice() const { return m_touchDevice; }
    void setTouchDevice(QPointingDevice *touchDevice) { m_touchDevice = touchDevice; }

    void flushPendingUpdates();

    static void updateColorScheme(Qt::ColorScheme colorScheme);
    static Qt::ColorScheme colorScheme() { return m_colorScheme; }
#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
#endif

private:
    EGLDisplay m_eglDisplay;
    QPointingDevice *m_touchDevice;

    QAndroidPlatformScreen *m_primaryScreen;

    QThread *m_mainThread;

    static Qt::ColorScheme m_colorScheme;

    static Qt::ScreenOrientation m_orientation;
    static Qt::ScreenOrientation m_nativeOrientation;
    static bool m_showPasswordEnabled;

    QPlatformFontDatabase *m_androidFDB;
    QAndroidPlatformNativeInterface *m_androidPlatformNativeInterface;
    QScopedPointer<QAndroidPlatformServices> m_androidPlatformServices;

    // Handling the multiple screens connected. Every display is identified
    // with an unique (autoincremented) displayID. The values of this ID will
    // not repeat during the OS runtime. We use this value as the key in the
    // storage of screens.
    QFlatMap<int, QAndroidPlatformScreen *, std::less<int>
            , QVarLengthArray<int, 10>
            , QVarLengthArray<QAndroidPlatformScreen *, 10> > m_screens;
    // ID of the primary display, in documentation it is said to be always 0,
    // but nevertheless it is retrieved
    int m_primaryDisplayId = 0;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *m_androidPlatformClipboard;
#endif

    QAndroidSystemLocale *m_androidSystemLocale;
#if QT_CONFIG(accessibility)
    mutable QPlatformAccessibility *m_accessibility;
#endif

    QScopedPointer<QPlatformInputContext> m_inputContext;
};

QT_END_NAMESPACE

#endif
