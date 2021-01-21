/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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
    explicit QQnxNavigatorEventNotifier(QQnxNavigatorEventHandler *eventHandler, QObject *parent = 0);
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
};

QT_END_NAMESPACE

#endif // QQNXNAVIGATOREVENTNOTIFIER_H
