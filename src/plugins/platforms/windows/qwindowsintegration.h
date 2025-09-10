// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSINTEGRATION_H
#define QWINDOWSINTEGRATION_H

#include "qwindowsapplication.h"

#include <qpa/qplatformintegration.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/private/qwindowsfontdatabase_p.h>
#ifndef QT_NO_OPENGL
#include <QtGui/private/qopenglcontext_p.h>
#endif
#include <qpa/qplatformopenglcontext.h>

QT_BEGIN_NAMESPACE

struct QWindowsIntegrationPrivate;
struct QWindowsWindowData;
class QWindowsWindow;
class QWindowsStaticOpenGLContext;

class QWindowsIntegration : public QPlatformIntegration
#ifndef QT_NO_OPENGL
    , public QNativeInterface::Private::QWindowsGLIntegration
#endif
    , public QWindowsApplication
{
    Q_DISABLE_COPY_MOVE(QWindowsIntegration)
public:
    enum Options { // Options to be passed on command line.
        FontDatabaseFreeType = 0x1,
        FontDatabaseNative = 0x2,
        DisableArb = 0x4,
        NoNativeDialogs = 0x8,
        XpNativeDialogs = 0x10,
        DontPassOsMouseEventsSynthesizedFromTouch = 0x20, // Do not pass OS-generated mouse events from touch.
        // Keep in sync with QWindowsFontDatabase::FontOptions
        DontUseDirectWriteFonts = QWindowsFontDatabase::DontUseDirectWriteFonts,
        DontUseColorFonts = QWindowsFontDatabase::DontUseColorFonts,
        AlwaysUseNativeMenus = 0x100,
        NoNativeMenus = 0x200,
        DetectAltGrModifier = 0x400,
        RtlEnabled = 0x0800,
        FontDatabaseGDI = 0x1000
    };

    explicit QWindowsIntegration(const QStringList &paramList);
    ~QWindowsIntegration() override;

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext::OpenGLModuleType openGLModuleType() override;
    static QWindowsStaticOpenGLContext *staticOpenGLContext();

    HMODULE openGLModuleHandle() const override;
    QOpenGLContext *createOpenGLContext(HGLRC context, HWND window,
                                        QOpenGLContext *shareContext) const override;
#endif
    QAbstractEventDispatcher *createEventDispatcher() const override;
    void initialize() override;
#if QT_CONFIG(clipboard)
    QPlatformClipboard *clipboard() const override;
#  if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#  endif
#endif // !QT_NO_CLIPBOARD
    QPlatformInputContext *inputContext() const override;
#if QT_CONFIG(accessibility)
    QPlatformAccessibility *accessibility() const override;
#endif
    QPlatformFontDatabase *fontDatabase() const override;
    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QPlatformServices *services() const override;
    QVariant styleHint(StyleHint hint) const override;

    QPlatformKeyMapper *keyMapper() const override;

    static QWindowsIntegration *instance() { return m_instance; }

    unsigned options() const;

    void beep() const override;

    void setApplicationBadge(qint64 number) override;
    void setApplicationBadge(const QImage &image);
    void updateApplicationBadge();

#if QT_CONFIG(sessionmanager)
    QPlatformSessionManager *createPlatformSessionManager(const QString &id, const QString &key) const override;
#endif

#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
#endif

protected:
    virtual QWindowsWindow *createPlatformWindowHelper(QWindow *window, const QWindowsWindowData &) const;

private:
    QScopedPointer<QWindowsIntegrationPrivate> d;

    static QWindowsIntegration *m_instance;

    qint64 m_applicationBadgeNumber = 0;
};

QT_END_NAMESPACE

#endif
