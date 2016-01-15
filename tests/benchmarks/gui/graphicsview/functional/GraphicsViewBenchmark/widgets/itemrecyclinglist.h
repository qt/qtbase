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

#ifndef ITEMRECYCLINGLIST_H
#define ITEMRECYCLINGLIST_H

#include "listitem.h"
#include "abstractitemview.h"
#include "listmodel.h"
#include "itemrecyclinglistview.h"
#include "recycledlistitem.h"

class QGraphicsWidget;

class ItemRecyclingList : public ItemRecyclingListView
{
    Q_OBJECT

public:
    ItemRecyclingList(const int itemBuffer = 4, QGraphicsWidget * parent = 0);
    virtual ~ItemRecyclingList();

    virtual void insertItem(int index, RecycledListItem *item);
    virtual void addItem(RecycledListItem *item);
    virtual void clear();
    virtual AbstractViewItem *takeItem(const int row);
    virtual void setItemPrototype(AbstractViewItem* prototype);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual bool listItemCaching() const;
    virtual void setListItemCaching(bool enabled);

    void setTwoColumns(const bool enabled);
    bool twoColumns();

public slots:
    void themeChange();

private:
    void updateListItemBackgrounds(int index);

private:
    Q_DISABLE_COPY(ItemRecyclingList)

    ListModel *m_listModel;
};

#endif // ITEMRECYCLINGLIST_H
