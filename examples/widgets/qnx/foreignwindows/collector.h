// Copyright (C) 2018 QNX Software Systems. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef COLLECTOR_H_
#define COLLECTOR_H_

#include <QAbstractNativeEventFilter>
#include <QList>
#include <QWidget>

#include <screen/screen.h>

class Collector : public QWidget, public QAbstractNativeEventFilter
{
public:
    explicit Collector(QWidget *parent = nullptr);
    ~Collector() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    struct Collectible
    {
        QWindow *window;
        QWidget *widget;
    };
    QList<Collectible> m_collectibles;

    bool filterQnxScreenEvent(screen_event_t event);
    bool filterQnxScreenWindowEvent(screen_event_t event);
    bool filterQnxScreenWindowCreateEvent(screen_window_t window, screen_event_t event);
    bool filterQnxScreenWindowCloseEvent(screen_window_t window, screen_event_t event);
    bool filterQnxScreenWindowManagerEvent(screen_window_t window, screen_event_t event);
    bool filterQnxScreenWindowManagerNameEvent(screen_window_t window,
                                               screen_event_t event);
};


#endif /* COLLECTOR_H_ */
