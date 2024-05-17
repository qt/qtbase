// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LISTITEMCONTAINER_H
#define LISTITEMCONTAINER_H

#include <QGraphicsWidget>
#include <QColor>

#include "abstractitemcontainer.h"

class QGraphicsLinearLayout;
class AbstractViewItem;
class ListItemCache;
class ListItem;
class ItemRecyclingList;

class ListItemContainer : public AbstractItemContainer
{
    Q_OBJECT

public:
    ListItemContainer(int bufferSize, ItemRecyclingList *view, QGraphicsWidget *parent=0);
    virtual ~ListItemContainer();

    virtual void setTwoColumns(const bool twoColumns);

    bool listItemCaching() const;
    void setListItemCaching(const bool enabled);
    virtual void setListItemCaching(const bool enabled, const int index);

protected:

   virtual void addItemToVisibleLayout(int index, AbstractViewItem *item);
   virtual void removeItemFromVisibleLayout(AbstractViewItem *item);

   virtual void adjustVisibleContainerSize(const QSizeF &size);
   virtual int maxItemCountInItemBuffer() const;

private:
    Q_DISABLE_COPY(ListItemContainer)

    ItemRecyclingList *m_view;
    QGraphicsLinearLayout *m_layout;

    void setListItemCaching(const bool enabled, ListItem *listItem);
    bool m_listItemCaching;
};

#endif // LISTITEMCONTAINER_H
