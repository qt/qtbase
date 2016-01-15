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

#include "itemrecyclinglistview.h"

ItemRecyclingListView::ItemRecyclingListView(QGraphicsWidget * parent)
    : AbstractItemView(parent), m_rootIndex()
{
}

/*virtual*/
ItemRecyclingListView::~ItemRecyclingListView()
{
}
void ItemRecyclingListView::setCurrentRow(const int row)
{
    setCurrentIndex(model()->index(row,0));
}

int ItemRecyclingListView::rows() const
{
    if (m_model)
        return m_model->rowCount();
    return 0;
}

/*virtual*/
void ItemRecyclingListView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if (parent == m_rootIndex) {
        AbstractItemView::rowsInserted(parent, start, end);
    }
}

/*virtual*/
void ItemRecyclingListView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if (parent == m_rootIndex) {
        AbstractItemView::rowsRemoved(parent, start, end);
    }
}
