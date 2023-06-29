// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "randomlistmodel.h"
#include <QRandomGenerator>

static constexpr int bufferSize(500);
static constexpr int lookAhead(100);
static constexpr int halfLookAhead(lookAhead / 2);

RandomListModel::RandomListModel(QObject *parent)
    : QAbstractListModel(parent), m_rows(bufferSize), m_count(10000)
{
}

int RandomListModel::rowCount(const QModelIndex &) const
{
    return m_count;
}

//! [0]
QVariant RandomListModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    int row = index.row();

    if (row > m_rows.lastIndex()) {
        if (row - m_rows.lastIndex() > lookAhead)
            cacheRows(row - halfLookAhead, qMin(m_count, row + halfLookAhead));
        else while (row > m_rows.lastIndex())
                m_rows.append(fetchRow(m_rows.lastIndex() + 1));
    } else if (row < m_rows.firstIndex()) {
        if (m_rows.firstIndex() - row > lookAhead)
            cacheRows(qMax(0, row - halfLookAhead), row + halfLookAhead);
        else while (row < m_rows.firstIndex())
                m_rows.prepend(fetchRow(m_rows.firstIndex() - 1));
    }

    return m_rows.at(row);
}

void RandomListModel::cacheRows(int from, int to) const
{
    for (int i = from; i <= to; ++i)
        m_rows.insert(i, fetchRow(i));
}
//![0]

//![1]
QString RandomListModel::fetchRow(int position) const
{
    return QString::number(QRandomGenerator::global()->bounded(++position));
}
//![1]
