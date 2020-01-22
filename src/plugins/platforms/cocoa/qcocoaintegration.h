/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMINTEGRATION_COCOA_H
#define QPLATFORMINTEGRATION_COCOA_H

#include <AppKit/AppKit.h>

#include "qcocoacursor.h"
#include "qcocoawindow.h"
#include "qcocoanativeinterface.h"
#include "qcocoainputcontext.h"
#include "qcocoaaccessibility.h"
#include "qcocoaclipboard.h"
#include "qcocoadrag.h"
#include "qcocoaservices.h"
#include "qcocoakeymapper.h"
#if QT_CONFIG(vulkan)
#include "qcocoavulkaninstance.h"
#endif

#include <QtCore/QScopedPointer>
#include <qpa/qplatformintegration.h>
#include <QtFontDatabaseSupport/private/qcoretextfontdatabase_p.h>

QT_BEGIN_NAMESPACE

class QCocoaIntegration : public QObject, public QPlatformIntegration
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
#ifndef QT_NO_ACCESSIBILITY
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

    Qt::KeyboardModifiers queryKeyboardModifiers() const override;
    QList<int> possibleKeys(const QKeyEvent *event) const override;

    void setToolbar(QWindow *window, NSToolbar *toolbar);
    NSToolbar *toolbar(QWindow *window) const;
    void clearToolbars();

    void pushPopupWindow(QCocoaWindow *window);
    QCocoaWindow *popPopupWindow();
    QCocoaWindow *activePopupWindow() const;
    QList<QCocoaWindow *> *popupWindowStack();

    void setApplicationIcon(const QIcon &icon) const override;

    void beep() const override;

    void closePopups(QWindow *forWindow = nullptr);

private Q_SLOTS:
    void focusWindowChanged(QWindow *);

private:
    static QCocoaIntegration *mInstance;
    Options mOptions;

    QScopedPointer<QCoreTextFontDatabase> mFontDb;

    QScopedPointer<QPlatformInputContext> mInputContext;
#ifndef QT_NO_ACCESSIBILITY
    QScopedPointer<QCocoaAccessibility> mAccessibility;
#endif
    QScopedPointer<QPlatformTheme> mPlatformTheme;
#ifndef QT_NO_CLIPBOARD
    QCocoaClipboard  *mCocoaClipboard;
#endif
    QScopedPointer<QCocoaDrag> mCocoaDrag;
    QScopedPointer<QCocoaNativeInterface> mNativeInterface;
    QScopedPointer<QCocoaServices> mServices;
    QScopedPointer<QCocoaKeyMapper> mKeyboardMapper;

#if QT_CONFIG(vulkan)
    mutable QCocoaVulkanInstance *mCocoaVulkanInstance = nullptr;
#endif
    QHash<QWindow *, NSToolbar *> mToolbars;
    QList<QCocoaWindow *> m_popupWindowStack;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QCocoaIntegration::Options)

QT_END_NAMESPACE

#endif

