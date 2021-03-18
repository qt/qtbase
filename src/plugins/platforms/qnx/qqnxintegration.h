/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QQNXINTEGRATION_H
#define QQNXINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <private/qtguiglobal_p.h>
#include <QtCore/qmutex.h>

#include <screen/screen.h>

#if QT_CONFIG(opengl)
#include <EGL/egl.h>
#endif

QT_BEGIN_NAMESPACE

class QQnxScreenEventThread;
class QQnxFileDialogHelper;
class QQnxNativeInterface;
class QQnxWindow;
class QQnxScreen;
class QQnxScreenEventHandler;
class QQnxNavigatorEventHandler;
class QQnxAbstractNavigator;
class QQnxAbstractVirtualKeyboard;
class QQnxServices;

class QSimpleDrag;

#if QT_CONFIG(qqnx_pps)
class QQnxInputContext;
class QQnxNavigatorEventNotifier;
class QQnxButtonEventNotifier;
#endif

#if !defined(QT_NO_CLIPBOARD)
class QQnxClipboard;
#endif

template<class K, class V> class QHash;
typedef QHash<screen_window_t, QWindow *> QQnxWindowMapper;

class QQnxIntegration : public QPlatformIntegration
{
public:
    enum Option { // Options to be passed on command line.
        NoOptions = 0x0,
        FullScreenApplication = 0x1,
        RootWindow = 0x2,
        AlwaysFlushScreenContext = 0x4,
        SurfacelessEGLContext = 0x8
    };
    Q_DECLARE_FLAGS(Options, Option)
    explicit QQnxIntegration(const QStringList &paramList);
    ~QQnxIntegration();

    static QQnxIntegration *instance() { return ms_instance; }

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

#if QT_CONFIG(opengl)
    EGLDisplay eglDisplay() const { return m_eglDisplay; }
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
#endif

#if QT_CONFIG(qqnx_pps)
    QPlatformInputContext *inputContext() const override;
#endif

    void moveToScreen(QWindow *window, int screen);

    bool supportsNavigatorEvents() const;

    QAbstractEventDispatcher *createEventDispatcher() const override;

    QPlatformFontDatabase *fontDatabase() const override { return m_fontDatabase; }

    QPlatformNativeInterface *nativeInterface() const override;

#if !defined(QT_NO_CLIPBOARD)
    QPlatformClipboard *clipboard() const override;
#endif
#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif
    QVariant styleHint(StyleHint hint) const override;

    QPlatformServices *services() const override;

    QWindow *window(screen_window_t qnxWindow) const;

    QQnxScreen *screenForNative(screen_display_t qnxScreen) const;

    void createDisplay(screen_display_t display, bool isPrimary);
    void removeDisplay(QQnxScreen *screen);
    QQnxScreen *primaryDisplay() const;
    Options options() const;
    screen_context_t screenContext();
    QByteArray screenContextId();

    QQnxNavigatorEventHandler *navigatorEventHandler();

private:
    void createDisplays();
    void destroyDisplays();

    void addWindow(screen_window_t qnxWindow, QWindow *window);
    void removeWindow(screen_window_t qnxWindow);
    QList<screen_display_t *> sortDisplays(screen_display_t *displays,
                                          int displayCount);

    screen_context_t m_screenContext;
    QByteArray m_screenContextId;
    QQnxScreenEventThread *m_screenEventThread;
    QQnxNavigatorEventHandler *m_navigatorEventHandler;
    QQnxAbstractVirtualKeyboard *m_virtualKeyboard;
#if QT_CONFIG(qqnx_pps)
    QQnxNavigatorEventNotifier *m_navigatorEventNotifier;
    QQnxInputContext *m_inputContext;
    QQnxButtonEventNotifier *m_buttonsNotifier;
#endif
    QPlatformInputContext *m_qpaInputContext;
    QQnxServices *m_services;
    QPlatformFontDatabase *m_fontDatabase;
    mutable QAbstractEventDispatcher *m_eventDispatcher;
    QQnxNativeInterface *m_nativeInterface;
    QList<QQnxScreen*> m_screens;
    QQnxScreenEventHandler *m_screenEventHandler;
#if !defined(QT_NO_CLIPBOARD)
    mutable QQnxClipboard* m_clipboard;
#endif
    QQnxAbstractNavigator *m_navigator;
#if QT_CONFIG(draganddrop)
    QSimpleDrag *m_drag;
#endif
    QQnxWindowMapper m_windowMapper;
    mutable QMutex m_windowMapperMutex;

    Options m_options;

#if QT_CONFIG(opengl)
    EGLDisplay m_eglDisplay;
    void createEglDisplay();
    void destroyEglDisplay();
#endif

    static QQnxIntegration *ms_instance;

    friend class QQnxWindow;
};

QT_END_NAMESPACE

#endif // QQNXINTEGRATION_H
