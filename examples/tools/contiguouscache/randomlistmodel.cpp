/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/
#include "randomlistmodel.h"
#include <stdlib.h>

static const int bufferSize(500);
static const int lookAhead(100);
static const int halfLookAhead(lookAhead/2);

RandomListModel::RandomListModel(QObject *parent)
: QAbstractListModel(parent), m_rows(bufferSize), m_count(10000)
{
}

RandomListModel::~RandomListModel()
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
            cacheRows(row-halfLookAhead, qMin(m_count, row+halfLookAhead));
        else while (row > m_rows.lastIndex())
            m_rows.append(fetchRow(m_rows.lastIndex()+1));
    } else if (row < m_rows.firstIndex()) {
        if (m_rows.firstIndex() - row > lookAhead)
            cacheRows(qMax(0, row-halfLookAhead), row+halfLookAhead);
        else while (row < m_rows.firstIndex())
            m_rows.prepend(fetchRow(m_rows.firstIndex()-1));
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
    return QString::number(rand() % ++position);
}
//![1]
