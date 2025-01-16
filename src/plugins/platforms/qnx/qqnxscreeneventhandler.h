// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXSCREENEVENTHANDLER_H
#define QQNXSCREENEVENTHANDLER_H

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/QBasicTimer>
#include <QtCore/QLoggingCategory>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaScreenEvents);

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
    void handleManagerEvent(screen_event_t event);

private:
    enum {
        MaximumTouchPoints = 10
    };

    QQnxIntegration *m_qnxIntegration;
    QPoint m_lastGlobalMousePoint;
    QPoint m_lastLocalMousePoint;
    Qt::MouseButtons m_lastButtonState;
    screen_window_t m_lastMouseWindow;
    QPointingDevice *m_touchDevice;
    QPointingDevice *m_mouseDevice;
    QWindowSystemInterface::TouchPoint m_touchPoints[MaximumTouchPoints];
    QList<QQnxScreenEventFilter*> m_eventFilters;
    QQnxScreenEventThread *m_eventThread;
    QBasicTimer m_focusLostTimer;
};

QT_END_NAMESPACE

#endif // QQNXSCREENEVENTHANDLER_H
