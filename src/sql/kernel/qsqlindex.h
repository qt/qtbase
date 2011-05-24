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

#ifndef QSQLINDEX_H
#define QSQLINDEX_H

#include <QtSql/qsqlrecord.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Sql)

class Q_SQL_EXPORT QSqlIndex : public QSqlRecord
{
public:
    QSqlIndex(const QString &cursorName = QString(), const QString &name = QString());
    QSqlIndex(const QSqlIndex &other);
    ~QSqlIndex();
    QSqlIndex &operator=(const QSqlIndex &other);
    void setCursorName(const QString &cursorName);
    inline QString cursorName() const { return cursor; }
    void setName(const QString& name);
    inline QString name() const { return nm; }

    void append(const QSqlField &field);
    void append(const QSqlField &field, bool desc);

    bool isDescending(int i) const;
    void setDescending(int i, bool desc);

#ifdef QT3_SUPPORT
    QT3_SUPPORT QString toString(const QString &prefix = QString(),
                               const QString &sep = QLatin1String(","),
                               bool verbose = true) const;
    QT3_SUPPORT QStringList toStringList(const QString& prefix = QString(),
                                       bool verbose = true) const;
#endif

private:
    QString createField(int i, const QString& prefix, bool verbose) const;
    QString cursor;
    QString nm;
    QList<bool> sorts;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSQLINDEX_H
