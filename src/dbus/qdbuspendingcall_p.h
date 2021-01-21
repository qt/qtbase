/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSPENDINGCALL_P_H
#define QDBUSPENDINGCALL_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include <qshareddata.h>
#include <qpointer.h>
#include <qvector.h>
#include <qmutex.h>
#include <qwaitcondition.h>

#include "qdbusmessage.h"
#include "qdbus_symbols_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusPendingCall;
class QDBusPendingCallWatcher;
class QDBusPendingCallWatcherHelper;
class QDBusConnectionPrivate;

class QDBusPendingCallPrivate: public QSharedData
{
public:
    // {
    //     set only during construction:
    const QDBusMessage sentMessage;
    QDBusConnectionPrivate * const connection;

    // for the callback mechanism (see setReplyCallback and QDBusConnectionPrivate::sendWithReplyAsync)
    QPointer<QObject> receiver;
    QVector<int> metaTypes;
    int methodIdx;

    // }

    mutable QMutex mutex;
    QWaitCondition waitForFinishedCondition;

    // {
    //    protected by the mutex above:
    QDBusPendingCallWatcherHelper *watcherHelper;
    QDBusMessage replyMessage;
    DBusPendingCall *pending;
    QString expectedReplySignature;
    // }

    QDBusPendingCallPrivate(const QDBusMessage &sent, QDBusConnectionPrivate *connection)
        : sentMessage(sent), connection(connection), watcherHelper(nullptr), pending(nullptr)
    { }
    ~QDBusPendingCallPrivate();
    bool setReplyCallback(QObject *target, const char *member);
    void waitForFinished();
    void setMetaTypes(int count, const int *types);
    void checkReceivedSignature();

    static QDBusPendingCall fromMessage(const QDBusMessage &msg);
};

class QDBusPendingCallWatcherHelper: public QObject
{
    Q_OBJECT
public:
    void add(QDBusPendingCallWatcher *watcher);

    void emitSignals(const QDBusMessage &replyMessage, const QDBusMessage &sentMessage)
    {
        if (replyMessage.type() == QDBusMessage::ReplyMessage)
            emit reply(replyMessage);
        else
            emit error(QDBusError(replyMessage), sentMessage);
        emit finished();
    }

Q_SIGNALS:
    void finished();
    void reply(const QDBusMessage &msg);
    void error(const QDBusError &error, const QDBusMessage &msg);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
