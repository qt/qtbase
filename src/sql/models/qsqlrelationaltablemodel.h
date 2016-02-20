/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSQLRELATIONALTABLEMODEL_H
#define QSQLRELATIONALTABLEMODEL_H

#include <QtSql/qtsqlglobal.h>
#include <QtSql/qsqltablemodel.h>

#include <QtCore/qtypeinfo.h>

QT_BEGIN_NAMESPACE


class Q_SQL_EXPORT QSqlRelation
{
public:
    QSqlRelation() {}
    QSqlRelation(const QString &aTableName, const QString &indexCol,
               const QString &displayCol)
        : tName(aTableName), iColumn(indexCol), dColumn(displayCol) {}

    void swap(QSqlRelation &other) Q_DECL_NOTHROW
    {
        qSwap(tName, other.tName);
        qSwap(iColumn, other.iColumn);
        qSwap(dColumn, other.dColumn);
    }

    inline QString tableName() const
    { return tName; }
    inline QString indexColumn() const
    { return iColumn; }
    inline QString displayColumn() const
    { return dColumn; }
    bool isValid() const Q_DECL_NOTHROW
    { return !(tName.isEmpty() || iColumn.isEmpty() || dColumn.isEmpty()); }
private:
    QString tName, iColumn, dColumn;
};
Q_DECLARE_SHARED_NOT_MOVABLE_UNTIL_QT6(QSqlRelation)

class QSqlRelationalTableModelPrivate;

class Q_SQL_EXPORT QSqlRelationalTableModel: public QSqlTableModel
{
    Q_OBJECT

public:
    enum JoinMode {
        InnerJoin,
        LeftJoin
    };

    explicit QSqlRelationalTableModel(QObject *parent = Q_NULLPTR,
                                      QSqlDatabase db = QSqlDatabase());
    virtual ~QSqlRelationalTableModel();

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &item, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;

    void clear() Q_DECL_OVERRIDE;
    bool select() Q_DECL_OVERRIDE;

    void setTable(const QString &tableName) Q_DECL_OVERRIDE;
    virtual void setRelation(int column, const QSqlRelation &relation);
    QSqlRelation relation(int column) const;
    virtual QSqlTableModel *relationModel(int column) const;
    void setJoinMode( QSqlRelationalTableModel::JoinMode joinMode );

public Q_SLOTS:
    void revertRow(int row) Q_DECL_OVERRIDE;

protected:
    QString selectStatement() const Q_DECL_OVERRIDE;
    bool updateRowInTable(int row, const QSqlRecord &values) Q_DECL_OVERRIDE;
    bool insertRowIntoTable(const QSqlRecord &values) Q_DECL_OVERRIDE;
    QString orderByClause() const Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(QSqlRelationalTableModel)
};

QT_END_NAMESPACE

#endif // QSQLRELATIONALTABLEMODEL_H
