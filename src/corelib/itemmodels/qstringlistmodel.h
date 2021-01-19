/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSTRINGLISTMODEL_H
#define QSTRINGLISTMODEL_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qstringlist.h>

QT_REQUIRE_CONFIG(stringlistmodel);

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QStringListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit QStringListModel(QObject *parent = nullptr);
    explicit QStringListModel(const QStringList &strings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool clearItemData(const QModelIndex &index) override;
#endif

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;

    QMap<int, QVariant> itemData(const QModelIndex &index) const override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QStringList stringList() const;
    void setStringList(const QStringList &strings);

    Qt::DropActions supportedDropActions() const override;

private:
    Q_DISABLE_COPY(QStringListModel)
    QStringList lst;
};

QT_END_NAMESPACE

#endif // QSTRINGLISTMODEL_H
