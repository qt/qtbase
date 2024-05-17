// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    ItemRecyclingList(const int itemBuffer = 4, QGraphicsWidget * parent = nullptr);
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
