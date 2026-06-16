// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMINTERATION_H
#define QOHOSPLATFORMINTERATION_H

#include <QtGui/qtguiglobal.h>


#include <qohosdisplayinfo.h>
#include <qohosscreenmanager.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformoffscreensurface.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformwindow_p.h>
#if QT_CONFIG(vulkan)
#include <qpa/qplatformvulkaninstance.h>
#endif
#include <EGL/egl.h>
#include <QtCore/qpoint.h>
#include <QtCore/qscopedpointer.h>
#include <QInputDevice>
#include <qohosplatformclipboard.h>
#include <render/qxcomponent.h>
#include <memory>

QT_BEGIN_NAMESPACE
class QPlatformOffscreenSurface;
class QOffscreenSurface;
class QThread;
class QOhosInputMethodEventHandler;
class QOhosPlatformScreen;
class QOhosPlatformFontDatabase;
class QOhosPlatformNativeInterface;
class QOhosSystemLocale;
class QOhosPlatformServices;

class QOhosPlatformIntegration : public QPlatformIntegration
                               , QNativeInterface::Private::QEGLIntegration
                               , public QNativeInterface::Private::QOhosIntegration
{

public:
    enum class WindowGeometryPersistencePolicy
    {
        Disabled,
        Enabled,
        FollowSystemSetting,
    };

    static QOhosPlatformIntegration *instance();

    QOhosPlatformIntegration(const QStringList &paramList);

    static void setDefaultDisplayMetrics(const QOhosDisplayInfo &m_displayInfo);

    Qt::WindowState defaultWindowState(Qt::WindowFlags flags) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *, WId) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QVariant styleHint(StyleHint hint) const override;
    QPlatformNativeInterface *nativeInterface() const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const override;
#endif
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformOffscreenSurface
                        *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;

    QPlatformFontDatabase *fontDatabase() const override;

#ifndef QT_NO_CLIPBOARD
    QOhosPlatformClipboard *clipboard() const override;
#endif
#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif

    bool hasCapability(Capability cap) const override;

    QPlatformInputContext *inputContext() const override;
    void initialize() override;
    QPlatformServices *services() const override;

#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
#endif

    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QStringList themeNames() const override;

    WId windowHandle(ArkUI_NodeHandle content) override;

    static QOhosSystemLocale *systemLocale();
    static void setSystemLocale(QOhosSystemLocale *systemLocale);

    static void setMainWindowGeometryPersistencePolicy(WindowGeometryPersistencePolicy policy);
    static WindowGeometryPersistencePolicy getMainWindowGeometryPersistencePolicy();

    QOhosInputMethodEventHandler *inputMethodEventHandler() const;
    QOhosScreenManager *screenManager() const;

private:
    std::shared_ptr<EGLDisplay> m_eglDisplay;

    QThread *m_mainThread;
    std::unique_ptr<QOhosPlatformFontDatabase> m_ohosFDB;
    std::unique_ptr<QOhosPlatformServices> m_ohosPlatformServices;

    std::unique_ptr<QOhosScreenManager> m_screenManager;

    QScopedPointer<QOhosPlatformNativeInterface> m_ohosPlatformNativeInterface;

    QScopedPointer<QPlatformInputContext> m_platformInputContext;
    std::unique_ptr<QOhosInputMethodEventHandler> m_ohosInputMethodEventHandler;

#ifndef QT_NO_CLIPBOARD
    std::unique_ptr<QOhosPlatformClipboard> m_platformClipboard;
#endif
#if QT_CONFIG(draganddrop)
    std::unique_ptr<QPlatformDrag> m_drag;
#endif

    static QScopedPointer<QOhosSystemLocale> m_systemLocale;
    static WindowGeometryPersistencePolicy m_mainWindowPersistencePolicy;

    std::shared_ptr<void> m_applicationStateTracker;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMINTERATION_H
