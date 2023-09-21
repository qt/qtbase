// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBINTEGRATION_H
#define QXCBINTEGRATION_H

#include <QtGui/private/qtguiglobal_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformopenglcontext.h>

#include "qxcbexport.h"

#include <xcb/xcb.h>

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QAbstractEventDispatcher;
class QXcbNativeInterface;

class Q_XCB_EXPORT QXcbIntegration : public QPlatformIntegration
#ifndef QT_NO_OPENGL
# if QT_CONFIG(xcb_glx_plugin)
    , public QNativeInterface::Private::QGLXIntegration
# endif
# if QT_CONFIG(egl)
    , public QNativeInterface::Private::QEGLIntegration
# endif
#endif
{
public:
    QXcbIntegration(const QStringList &parameters, int &argc, char **argv);
    ~QXcbIntegration();

    QPlatformPixmap *createPlatformPixmap(QPlatformPixmap::PixelType type) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
# if QT_CONFIG(xcb_glx_plugin)
    QOpenGLContext *createOpenGLContext(GLXContext context, void *visualInfo, QOpenGLContext *shareContext) const override;
# endif
# if QT_CONFIG(egl)
    QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const override;
# endif
#endif
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;

    bool hasCapability(Capability cap) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    void initialize() override;

    void moveToScreen(QWindow *window, int screen);

    QPlatformFontDatabase *fontDatabase() const override;

    QPlatformNativeInterface *nativeInterface()const override;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *clipboard() const override;
#endif
#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif

    QPlatformInputContext *inputContext() const override;

#if QT_CONFIG(accessibility)
    QPlatformAccessibility *accessibility() const override;
#endif

    QPlatformServices *services() const override;

    QPlatformKeyMapper *keyMapper() const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QVariant styleHint(StyleHint hint) const override;

    bool hasConnection() const { return m_connection; }
    QXcbConnection *connection() const { return m_connection; }

    QByteArray wmClass() const;

#if QT_CONFIG(xcb_sm)
    QPlatformSessionManager *createPlatformSessionManager(const QString &id, const QString &key) const override;
#endif

    void sync() override;

    void beep() const override;

    bool nativePaintingEnabled() const;

#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
#endif

    static QXcbIntegration *instance() { return m_instance; }

    void setApplicationBadge(qint64 number) override;

private:
    QXcbConnection *m_connection = nullptr;

    QScopedPointer<QPlatformFontDatabase> m_fontDatabase;
    QScopedPointer<QXcbNativeInterface> m_nativeInterface;

    QScopedPointer<QPlatformInputContext> m_inputContext;

#if QT_CONFIG(accessibility)
    mutable QScopedPointer<QPlatformAccessibility> m_accessibility;
#endif

    QScopedPointer<QPlatformServices> m_services;

    mutable QByteArray m_wmClass;
    const char *m_instanceName;
    bool m_canGrab;
    xcb_visualid_t m_defaultVisualId;

    static QXcbIntegration *m_instance;
};

QT_END_NAMESPACE

#endif
