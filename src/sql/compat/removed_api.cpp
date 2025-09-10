// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_SQL_BUILD_REMOVED_API

#include "qtsqlglobal.h"

QT_USE_NAMESPACE

#if QT_SQL_REMOVED_SINCE(6, 4)

#endif // QT_SQL_REMOVED_SINCE(6, 4)

#if QT_SQL_REMOVED_SINCE(6, 5)

#if QT_CONFIG(sqlmodel)

#include "qsqlquerymodel.h"
#include "qsqlquery.h"

QSqlQuery QSqlQueryModel::query() const
{
    QT_IGNORE_DEPRECATIONS(return query(QT6_CALL_NEW_OVERLOAD);)
}

#include "qsqltablemodel.h"

void QSqlTableModel::setQuery(const QSqlQuery &query)
{
    QT_IGNORE_DEPRECATIONS(QSqlQueryModel::setQuery(query);)
}

#endif // QT_CONFIG(sqlmodel)

#endif // QT_SQL_REMOVED_SINCE(6, 5)

#if QT_SQL_REMOVED_SINCE(6, 6)

#include "qsqlresult.h"
#include <QtSql/private/qsqlresult_p.h>

// #include <qotherheader.h>
// // implement removed functions from qotherheader.h
// order sections alphabetically to reduce chances of merge conflicts

QList<QVariant> &QSqlResult::boundValues() const
{
    Q_D(const QSqlResult);
    return const_cast<QSqlResultPrivate *>(d)->values;
}

#endif // QT_SQL_REMOVED_SINCE(6, 6)

#if QT_SQL_REMOVED_SINCE(6, 8)

#include "qsqlrecord.h"
#include "qsqlfield.h"

// #include <qotherheader.h>
// // implement removed functions from qotherheader.h
// order sections alphabetically to reduce chances of merge conflicts

bool QSqlRecord::contains(const QString &name) const
{
    return contains(QStringView(name));
}

QSqlField QSqlRecord::field(const QString &name) const
{
    return field(QStringView(name));
}

int QSqlRecord::indexOf(const QString &name) const
{
    return indexOf(QStringView(name));
}

bool QSqlRecord::isGenerated(const QString &name) const
{
    return isGenerated(QStringView(name));
}

bool QSqlRecord::isNull(const QString &name) const
{
    return isNull(QStringView(name));
}

void QSqlRecord::setGenerated(const QString &name, bool generated)
{
    setGenerated(QStringView(name), generated);
}

void QSqlRecord::setNull(const QString &name)
{
    setNull(QStringView(name));
}

void QSqlRecord::setValue(const QString &name, const QVariant &val)
{
    setValue(QStringView(name), val);
}

QVariant QSqlRecord::value(const QString &name) const
{
    return value(QStringView(name));
}


#include "qsqlquery.h"

bool QSqlQuery::isNull(const QString &name) const
{
    return isNull(QStringView(name));
}

QVariant QSqlQuery::value(const QString &name) const
{
    return value(QStringView(name));
}

#endif // QT_SQL_REMOVED_SINCE(6, 8)
