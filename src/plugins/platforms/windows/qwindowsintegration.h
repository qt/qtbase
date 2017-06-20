/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#ifndef QWINDOWSINTEGRATION_H
#define QWINDOWSINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <QtCore/QScopedPointer>
#include <QtFontDatabaseSupport/private/qwindowsfontdatabase_p.h>

QT_BEGIN_NAMESPACE

struct QWindowsIntegrationPrivate;
struct QWindowsWindowData;
class QWindowsWindow;
class QWindowsStaticOpenGLContext;

class QWindowsIntegration : public QPlatformIntegration
{
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
        NoNativeMenus = 0x200
    };

    explicit QWindowsIntegration(const QStringList &paramList);
    virtual ~QWindowsIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext::OpenGLModuleType openGLModuleType() override;
    static QWindowsStaticOpenGLContext *staticOpenGLContext();
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
#ifndef QT_NO_ACCESSIBILITY
    QPlatformAccessibility *accessibility() const override;
#endif
    QPlatformFontDatabase *fontDatabase() const override;
    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QPlatformServices *services() const override;
    QVariant styleHint(StyleHint hint) const override;

    Qt::KeyboardModifiers queryKeyboardModifiers() const override;
    QList<int> possibleKeys(const QKeyEvent *e) const override;

    static QWindowsIntegration *instance() { return m_instance; }

    inline void emitScreenAdded(QPlatformScreen *s, bool isPrimary = false) { screenAdded(s, isPrimary); }
    inline void emitDestroyScreen(QPlatformScreen *s) { destroyScreen(s); }

    unsigned options() const;

    void beep() const override;

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
};

QT_END_NAMESPACE

#endif
