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

#ifndef QBBINTEGRATION_H
#define QBBINTEGRATION_H

#include <QtGui/qplatformintegration_qpa.h>

#include <QtCore/qmutex.h>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QBBEventThread;
class QBBInputContext;
class QBBNavigatorThread;
class QBBWindow;

#ifndef QT_NO_CLIPBOARD
class QBBClipboard;
#endif

template<class K, class V> class QHash;
typedef QHash<screen_window_t, QWindow *> QBBWindowMapper;

class QBBIntegration : public QPlatformIntegration
{
public:
    QBBIntegration();
    virtual ~QBBIntegration();

    virtual bool hasCapability(QPlatformIntegration::Capability cap) const;

    virtual QPlatformWindow *createPlatformWindow(QWindow *window) const;
    virtual QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;
    virtual QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;

    virtual QPlatformInputContext *inputContext() const;

    virtual QList<QPlatformScreen *> screens() const;
    virtual void moveToScreen(QWindow *window, int screen);

    virtual QAbstractEventDispatcher *guiThreadEventDispatcher() const;

    virtual QPlatformFontDatabase *fontDatabase() const { return m_fontDatabase; }

#ifndef QT_NO_CLIPBOARD
    virtual QPlatformClipboard *clipboard() const;
#endif

    virtual QVariant styleHint(StyleHint hint) const;

    bool paintUsingOpenGL() const { return m_paintUsingOpenGL; }

    static QWindow *window(screen_window_t qnxWindow);

private:
    static void addWindow(screen_window_t qnxWindow, QWindow *window);
    static void removeWindow(screen_window_t qnxWindow);

    screen_context_t m_screenContext;
    QBBEventThread *m_eventThread;
    QBBNavigatorThread *m_navigatorThread;
    QBBInputContext *m_inputContext;
    QPlatformFontDatabase *m_fontDatabase;
    bool m_paintUsingOpenGL;
    QAbstractEventDispatcher *m_eventDispatcher;
#ifndef QT_NO_CLIPBOARD
    mutable QBBClipboard* m_clipboard;
#endif

    static QBBWindowMapper ms_windowMapper;
    static QMutex ms_windowMapperMutex;

    friend class QBBWindow;
};

QT_END_NAMESPACE

#endif // QBBINTEGRATION_H
