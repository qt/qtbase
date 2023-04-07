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
