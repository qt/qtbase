// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXNAVIGATOREVENTNOTIFIER_H
#define QQNXNAVIGATOREVENTNOTIFIER_H

#include <QObject>

QT_BEGIN_NAMESPACE

class QQnxNavigatorEventHandler;
class QSocketNotifier;

class QQnxNavigatorEventNotifier : public QObject
{
    Q_OBJECT
public:
    explicit QQnxNavigatorEventNotifier(QQnxNavigatorEventHandler *eventHandler, QObject *parent = nullptr);
    ~QQnxNavigatorEventNotifier();

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void readData();

private:
    void parsePPS(const QByteArray &ppsData, QByteArray &msg, QByteArray &dat, QByteArray &id);
    void replyPPS(const QByteArray &res, const QByteArray &id, const QByteArray &dat);
    void handleMessage(const QByteArray &msg, const QByteArray &dat, const QByteArray &id);

    int m_fd;
    QSocketNotifier *m_readNotifier;
    QQnxNavigatorEventHandler *m_eventHandler;

    static const char *navigatorControlPath;
    static const size_t ppsBufferSize;
};

QT_END_NAMESPACE

#endif // QQNXNAVIGATOREVENTNOTIFIER_H
