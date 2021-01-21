/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#ifndef QWINDOWSINTEGRATION_H
#define QWINDOWSINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <QtCore/qscopedpointer.h>
#include <QtFontDatabaseSupport/private/qwindowsfontdatabase_p.h>

QT_BEGIN_NAMESPACE

struct QWindowsIntegrationPrivate;
struct QWindowsWindowData;
class QWindowsWindow;
class QWindowsStaticOpenGLContext;

class QWindowsIntegration : public QPlatformIntegration
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
        DontUseWMPointer = 0x400,
        DetectAltGrModifier = 0x800,
        RtlEnabled = 0x1000,
        DarkModeWindowFrames = 0x2000,
        DarkModeStyle = 0x4000
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

    Qt::KeyboardModifiers queryKeyboardModifiers() const override;
    QList<int> possibleKeys(const QKeyEvent *e) const override;

    static QWindowsIntegration *instance() { return m_instance; }

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
