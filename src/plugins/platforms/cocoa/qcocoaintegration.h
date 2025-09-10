// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATION_COCOA_H
#define QPLATFORMINTEGRATION_COCOA_H

#include "qcocoacursor.h"
#include "qcocoawindow.h"
#include "qcocoanativeinterface.h"
#include "qcocoainputcontext.h"
#include "qcocoaaccessibility.h"
#include "qcocoaclipboard.h"
#include "qcocoadrag.h"
#include "qcocoaservices.h"
#if QT_CONFIG(vulkan)
#include "qcocoavulkaninstance.h"
#endif
#include "qcocoawindowmanager.h"

#include <QtCore/QScopedPointer>
#include <qpa/qplatformintegration.h>
#include <QtGui/private/qcoretextfontdatabase_p.h>
#ifndef QT_NO_OPENGL
#  include <QtGui/private/qopenglcontext_p.h>
#endif
#include <QtGui/private/qapplekeymapper_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(NSToolbar);

QT_BEGIN_NAMESPACE

class QCocoaIntegration : public QObject, public QPlatformIntegration
#ifndef QT_NO_OPENGL
    , public QNativeInterface::Private::QCocoaGLIntegration
#endif
{
    Q_OBJECT
public:
    enum Option {
        UseFreeTypeFontEngine = 0x1
    };
    Q_DECLARE_FLAGS(Options, Option)

    QCocoaIntegration(const QStringList &paramList);
    ~QCocoaIntegration();

    static QCocoaIntegration *instance();
    Options options() const;

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext *createOpenGLContext(NSOpenGLContext *, QOpenGLContext *shareContext) const override;
#endif
    QPlatformBackingStore *createPlatformBackingStore(QWindow *widget) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;

#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
    QCocoaVulkanInstance *getCocoaVulkanInstance() const;
#endif

#if QT_CONFIG(sessionmanager)
    QPlatformSessionManager *createPlatformSessionManager(const QString &id, const QString &key) const override;
#endif

    QCoreTextFontDatabase *fontDatabase() const override;
    QCocoaNativeInterface *nativeInterface() const override;
    QPlatformInputContext *inputContext() const override;
#if QT_CONFIG(accessibility)
    QCocoaAccessibility *accessibility() const override;
#endif
#ifndef QT_NO_CLIPBOARD
    QCocoaClipboard *clipboard() const override;
#endif
    QCocoaDrag *drag() const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QCocoaServices *services() const override;
    QVariant styleHint(StyleHint hint) const override;

    QPlatformKeyMapper *keyMapper() const override;

    void setApplicationIcon(const QIcon &icon) const override;
    void setApplicationBadge(qint64 number) override;

    void beep() const override;
    void quit() const override;

private Q_SLOTS:
    void focusWindowChanged(QWindow *);

private:
    static QCocoaIntegration *mInstance;
    Options mOptions;

    QScopedPointer<QCoreTextFontDatabase> mFontDb;

    QScopedPointer<QPlatformInputContext> mInputContext;
#if QT_CONFIG(accessibility)
    QScopedPointer<QCocoaAccessibility> mAccessibility;
#endif
    QScopedPointer<QPlatformTheme> mPlatformTheme;
#ifndef QT_NO_CLIPBOARD
    QCocoaClipboard  *mCocoaClipboard;
#endif
    QScopedPointer<QCocoaDrag> mCocoaDrag;
    QScopedPointer<QCocoaNativeInterface> mNativeInterface;
    mutable QScopedPointer<QCocoaServices> mServices;
    QScopedPointer<QAppleKeyMapper> mKeyboardMapper;

#if QT_CONFIG(vulkan)
    mutable QCocoaVulkanInstance *mCocoaVulkanInstance = nullptr;
#endif

    QCocoaWindowManager m_windowManager;

    QMacNotificationObserver m_menuTrackingObserver;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QCocoaIntegration::Options)

QT_END_NAMESPACE

#endif

