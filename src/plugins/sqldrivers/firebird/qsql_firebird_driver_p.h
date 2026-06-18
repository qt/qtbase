// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QSQL_FIREBIRD_DRIVER_P_H
#define QSQL_FIREBIRD_DRIVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsql_firebird_p.h"
#include "qsql_firebird_helpers_p.h"

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/private/qsqldriver_p.h>

#include <firebird/Interface.h>

#include <mutex>

QT_BEGIN_NAMESPACE

class QFirebirdEventSubscription;

//  Private driver state

class QFirebirdDriverPrivate : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QFirebirdDriver)
public:
    QFirebirdDriverPrivate()
        : QSqlDriverPrivate(QSqlDriver::FirebirdSQL)
    {
        master = Firebird::fb_get_master_interface();
        iStatus = master->getStatus();
    }

    ~QFirebirdDriverPrivate()
    {
        if (iStatus)
            iStatus->dispose();
    }

    Firebird::IMaster *master = nullptr;
    Firebird::IStatus *iStatus = nullptr;
    Firebird::IAttachment *iAtt = nullptr;
    Firebird::ITransaction *iTrans = nullptr;   // explicit user-managed transaction

    /*! \internal
        Guards the iAtt lifecycle (published in open(), withdrawn in close())
        against cancelQuery(), which reads and uses the attachment from a
        foreign thread. All other iAtt access is owner-thread-only.
    */
    std::mutex attMutex;

    /*! \internal
        Opt-in (connection option FB_SCROLLABLE_CACHE=1): when set, scrollable
        (non-forward-only) SELECT results buffer their rows client-side as they
        are first read, so random seek()s become in-memory lookups instead of a
        server fetchAbsolute round-trip each. Trades memory for random-access
        speed; off by default to preserve the driver's low-memory streaming model.
    */
    bool cacheScrollableResults = false;

    QHash<QString, QFirebirdEventSubscription *> eventSubscriptions;

    /*! \internal
        Results created from this driver. Tracked so they can be neutralised if
        the driver object is destroyed while a QSqlQuery is still alive — see
        ~QFirebirdDriver and QFirebirdResultPrivate::cleanup().
    */
    QList<QFirebirdResultPrivate *> activeResults;

    void setError(const QString &msg,
                  QSqlError::ErrorType type, const QString &code = {})
    {
        Q_Q(QFirebirdDriver);
        q->setLastError(QSqlError(msg, {}, type, code));
    }

    void setFbError(const QString &context,
                    QSqlError::ErrorType type = QSqlError::UnknownError);

    /*! \internal
        Finish the active user transaction by committing or rolling back. On a
        failed commit/rollback the handle is released and the member cleared so
        no caller sees a stale pointer. Shared by commitTransaction()/
        rollbackTransaction(), which differ only in the verb.
    */
    bool finishTransaction(TxnEnd end);
};

QT_END_NAMESPACE

#endif // QSQL_FIREBIRD_DRIVER_P_H
