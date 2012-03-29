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

#ifndef QQNXSCREENEVENTHANDLER_H
#define QQNXSCREENEVENTHANDLER_H

#include <QWindowSystemInterface>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxScreenEventHandler : public QObject
{
    Q_OBJECT
public:
    QQnxScreenEventHandler();

    bool handleEvent(screen_event_t event);
    bool handleEvent(screen_event_t event, int qnxType);

    static void injectKeyboardEvent(int flags, int sym, int mod, int scan, int cap);

Q_SIGNALS:
    void newWindowCreated(void *window);
    void windowClosed(void *window);

private:
    void handleKeyboardEvent(screen_event_t event);
    void handlePointerEvent(screen_event_t event);
    void handleTouchEvent(screen_event_t event, int qnxType);
    void handleCloseEvent(screen_event_t event);
    void handleCreateEvent(screen_event_t event);

private:
    enum {
        MaximumTouchPoints = 10
    };

    QPoint m_lastGlobalMousePoint;
    QPoint m_lastLocalMousePoint;
    Qt::MouseButtons m_lastButtonState;
    screen_window_t m_lastMouseWindow;
    QTouchDevice *m_touchDevice;
    QWindowSystemInterface::TouchPoint m_touchPoints[MaximumTouchPoints];
};

QT_END_NAMESPACE

#endif // QQNXSCREENEVENTHANDLER_H
