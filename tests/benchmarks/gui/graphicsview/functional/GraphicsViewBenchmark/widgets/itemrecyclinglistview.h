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

#ifndef ITEMRECYCLINGLISTVIEW_H
#define ITEMRECYCLINGLISTVIEW_H

#include "abstractitemview.h"

class ItemRecyclingListView : public AbstractItemView
{
public:
    ItemRecyclingListView(QGraphicsWidget * parent = 0);
    virtual ~ItemRecyclingListView();
    void setCurrentRow(const int row);
    int rows() const;
    virtual bool listItemCaching() const = 0;
    virtual void setListItemCaching(bool enabled) = 0;

protected:
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent,int start,int end);

private:
    QModelIndex m_rootIndex;
};

#endif // ITEMRECYCLINGLISTVIEW_H
