/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtSql module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSQLDRIVER_H
#define QSQLDRIVER_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtSql/qsql.h>
#ifdef QT3_SUPPORT
#include <QtSql/qsqlquery.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Sql)

class QSqlDatabase;
class QSqlDriverPrivate;
class QSqlError;
class QSqlField;
class QSqlIndex;
class QSqlRecord;
class QSqlResult;
class QVariant;

class Q_SQL_EXPORT QSqlDriver : public QObject
{
    friend class QSqlDatabase;
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSqlDriver)

public:
    enum DriverFeature { Transactions, QuerySize, BLOB, Unicode, PreparedQueries,
                         NamedPlaceholders, PositionalPlaceholders, LastInsertId,
                         BatchOperations, SimpleLocking, LowPrecisionNumbers,
                         EventNotifications, FinishQuery, MultipleResultSets };

    enum StatementType { WhereStatement, SelectStatement, UpdateStatement,
                         InsertStatement, DeleteStatement };

    enum IdentifierType { FieldName, TableName };

    explicit QSqlDriver(QObject *parent=0);
    ~QSqlDriver();
    virtual bool isOpen() const;
    bool isOpenError() const;

    virtual bool beginTransaction();
    virtual bool commitTransaction();
    virtual bool rollbackTransaction();
    virtual QStringList tables(QSql::TableType tableType) const;
    virtual QSqlIndex primaryIndex(const QString &tableName) const;
    virtual QSqlRecord record(const QString &tableName) const;
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT QSqlRecord record(const QSqlQuery& query) const
    { return query.record(); }
    inline QT3_SUPPORT QSqlRecord recordInfo(const QString& tablename) const
    { return record(tablename); }
    inline QT3_SUPPORT QSqlRecord recordInfo(const QSqlQuery& query) const
    { return query.record(); }
    inline QT3_SUPPORT QString nullText() const { return QLatin1String("NULL"); }
    inline QT3_SUPPORT QString formatValue(const QSqlField *field, bool trimStrings = false) const
    { return field ? formatValue(*field, trimStrings) : QString(); }
#endif
    virtual QString formatValue(const QSqlField& field, bool trimStrings = false) const;

    virtual QString escapeIdentifier(const QString &identifier, IdentifierType type) const;
    virtual QString sqlStatement(StatementType type, const QString &tableName,
                                 const QSqlRecord &rec, bool preparedStatement) const;

    QSqlError lastError() const;

    virtual QVariant handle() const;
    virtual bool hasFeature(DriverFeature f) const = 0;
    virtual void close() = 0;
    virtual QSqlResult *createResult() const = 0;

    virtual bool open(const QString& db,
                      const QString& user = QString(),
                      const QString& password = QString(),
                      const QString& host = QString(),
                      int port = -1,
                      const QString& connOpts = QString()) = 0;
    bool subscribeToNotification(const QString &name);	    // ### Qt 5: make virtual
    bool unsubscribeFromNotification(const QString &name);  // ### Qt 5: make virtual
    QStringList subscribedToNotifications() const;          // ### Qt 5: make virtual

    bool isIdentifierEscaped(const QString &identifier, IdentifierType type) const; // ### Qt 5: make virtual
    QString stripDelimiters(const QString &identifier, IdentifierType type) const;  // ### Qt 5: make virtual

    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
    QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;

Q_SIGNALS:
    void notification(const QString &name);

protected:
    virtual void setOpen(bool o);
    virtual void setOpenError(bool e);
    virtual void setLastError(const QSqlError& e);

protected Q_SLOTS:
    bool subscribeToNotificationImplementation(const QString &name);        // ### Qt 5: eliminate, see subscribeToNotification()
    bool unsubscribeFromNotificationImplementation(const QString &name);    // ### Qt 5: eliminate, see unsubscribeFromNotification()
    QStringList subscribedToNotificationsImplementation() const;            // ### Qt 5: eliminate, see subscribedNotifications()

    bool isIdentifierEscapedImplementation(const QString &identifier, IdentifierType type) const;   // ### Qt 5: eliminate, see isIdentifierEscaped()
    QString stripDelimitersImplementation(const QString &identifier, IdentifierType type) const;    // ### Qt 5: eliminate, see stripDelimiters()

private:
    Q_DISABLE_COPY(QSqlDriver)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSQLDRIVER_H
