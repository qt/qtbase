/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QQNXSCREENEVENTHANDLER_H
#define QQNXSCREENEVENTHANDLER_H

#include <qpa/qwindowsysteminterface.h>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxIntegration;
class QQnxScreenEventFilter;
class QQnxScreenEventThread;

class QQnxScreenEventHandler : public QObject
{
    Q_OBJECT
public:
    explicit QQnxScreenEventHandler(QQnxIntegration *integration);

    void addScreenEventFilter(QQnxScreenEventFilter *filter);
    void removeScreenEventFilter(QQnxScreenEventFilter *filter);

    bool handleEvent(screen_event_t event);
    bool handleEvent(screen_event_t event, int qnxType);

    static void injectKeyboardEvent(int flags, int sym, int mod, int scan, int cap);

    void setScreenEventThread(QQnxScreenEventThread *eventThread);

Q_SIGNALS:
    void newWindowCreated(void *window);
    void windowClosed(void *window);

protected:
    void timerEvent(QTimerEvent *event) override;

private Q_SLOTS:
    void processEvents();

private:
    void handleKeyboardEvent(screen_event_t event);
    void handlePointerEvent(screen_event_t event);
    void handleTouchEvent(screen_event_t event, int qnxType);
    void handleCloseEvent(screen_event_t event);
    void handleCreateEvent(screen_event_t event);
    void handleDisplayEvent(screen_event_t event);
    void handlePropertyEvent(screen_event_t event);
    void handleKeyboardFocusPropertyEvent(screen_window_t window);
    void handleGeometryPropertyEvent(screen_window_t window);

private:
    enum {
        MaximumTouchPoints = 10
    };

    QQnxIntegration *m_qnxIntegration;
    QPoint m_lastGlobalMousePoint;
    QPoint m_lastLocalMousePoint;
    Qt::MouseButtons m_lastButtonState;
    screen_window_t m_lastMouseWindow;
    QTouchDevice *m_touchDevice;
    QWindowSystemInterface::TouchPoint m_touchPoints[MaximumTouchPoints];
    QList<QQnxScreenEventFilter*> m_eventFilters;
    QQnxScreenEventThread *m_eventThread;
    int m_focusLostTimer;
};

QT_END_NAMESPACE

#endif // QQNXSCREENEVENTHANDLER_H
