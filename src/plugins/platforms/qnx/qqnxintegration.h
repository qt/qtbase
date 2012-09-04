/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQNXINTEGRATION_H
#define QQNXINTEGRATION_H

#include <qpa/qplatformintegration.h>

#include <QtCore/qmutex.h>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxBpsEventFilter;
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

#if defined(QQNX_PPS)
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
    QQnxIntegration();
    ~QQnxIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const;

    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;

#if !defined(QT_NO_OPENGL)
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
#endif

#if defined(QQNX_PPS)
    QPlatformInputContext *inputContext() const;
#endif

    void moveToScreen(QWindow *window, int screen);

    QAbstractEventDispatcher *guiThreadEventDispatcher() const;

    QPlatformFontDatabase *fontDatabase() const { return m_fontDatabase; }

    QPlatformNativeInterface *nativeInterface() const;

#if !defined(QT_NO_CLIPBOARD)
    QPlatformClipboard *clipboard() const;
#endif

    QVariant styleHint(StyleHint hint) const;

    QPlatformServices *services() const;

#if defined(Q_OS_BLACKBERRY)
    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const;
    QQnxBpsEventFilter *bpsEventFilter() const { return m_bpsEventFilter; }
#endif

    static QWindow *window(screen_window_t qnxWindow);

private:
    void createDisplays();
    void destroyDisplays();
    QQnxScreen *primaryDisplay() const;

    static void addWindow(screen_window_t qnxWindow, QWindow *window);
    static void removeWindow(screen_window_t qnxWindow);

    screen_context_t m_screenContext;
    QQnxScreenEventThread *m_screenEventThread;
    QQnxNavigatorEventHandler *m_navigatorEventHandler;
    QQnxAbstractVirtualKeyboard *m_virtualKeyboard;
#if defined(QQNX_PPS)
    QQnxNavigatorEventNotifier *m_navigatorEventNotifier;
    QQnxInputContext *m_inputContext;
    QQnxButtonEventNotifier *m_buttonsNotifier;
#endif
    QQnxServices *m_services;
    QPlatformFontDatabase *m_fontDatabase;
    QAbstractEventDispatcher *m_eventDispatcher;
#if defined(Q_OS_BLACKBERRY)
    QQnxBpsEventFilter *m_bpsEventFilter;
#endif
    QQnxNativeInterface *m_nativeInterface;
    QList<QQnxScreen*> m_screens;
    QQnxScreenEventHandler *m_screenEventHandler;
#if !defined(QT_NO_CLIPBOARD)
    mutable QQnxClipboard* m_clipboard;
#endif

    QQnxAbstractNavigator *m_navigator;

    static QQnxWindowMapper ms_windowMapper;
    static QMutex ms_windowMapperMutex;

    friend class QQnxWindow;
};

QT_END_NAMESPACE

#endif // QQNXINTEGRATION_H
