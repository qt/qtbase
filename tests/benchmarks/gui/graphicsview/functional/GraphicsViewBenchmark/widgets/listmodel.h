/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <QAbstractListModel>

class RecycledListItem;
class ListItemCache;

class ListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    ListModel(QObject *parent = 0);
    ~ListModel();

public:

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole ) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    void insert(const int row, RecycledListItem *item);
    void appendRow(RecycledListItem *item);

    void clear();

    RecycledListItem *item(const int row) const;

    RecycledListItem *takeItem(const int row);

private:
    Q_DISABLE_COPY(ListModel)
    QList<RecycledListItem *> m_items;
};

#endif // LISTMODEL_H
