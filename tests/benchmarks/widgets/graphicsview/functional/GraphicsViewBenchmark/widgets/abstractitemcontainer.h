// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTITEMCONTAINER_H
#define ABSTRACTITEMCONTAINER_H

#include <QModelIndex>

#include "gvbwidget.h"

class QGraphicsWidget;
class AbstractItemView;
class AbstractViewItem;

class AbstractItemContainer : public GvbWidget
{
  Q_OBJECT
public:
    AbstractItemContainer(int bufferSize, QGraphicsWidget *parent=0);
    virtual ~AbstractItemContainer();

    virtual void addItem(const QModelIndex &index);
    virtual void removeItem(const QModelIndex &index);

    virtual void setItemView(AbstractItemView *view);
    virtual void setItemPrototype(AbstractViewItem *ptype);
    virtual void reset();
    virtual int itemCount() const;
    virtual AbstractViewItem* itemAt(const int row) const;
    AbstractViewItem* findItemByIndex(const QModelIndex &index) const;
    AbstractViewItem *prototype();
    AbstractViewItem *firstItem();
    void updateContent();
    void themeChange();
    int bufferSize() const;
    virtual void setTwoColumns(const bool enabled);
    bool twoColumns();

    void setSubtreeCacheEnabled(const bool enabled);
    virtual void setListItemCaching(const bool enabled, const int index) = 0;

protected:
    virtual void adjustVisibleContainerSize(const QSizeF &size) = 0;
    virtual void addItemToVisibleLayout(int index, AbstractViewItem *item) = 0;
    virtual void removeItemFromVisibleLayout(AbstractViewItem *item) = 0;

    virtual bool event(QEvent *e);
    virtual bool eventFilter(QObject *obj, QEvent *event);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual int maxItemCountInItemBuffer() const;
    bool itemVisibleInView(AbstractViewItem* item, const QRectF &viewRect, bool fullyVisible = true) const;

protected:
    void updateItemBuffer();
    void findFirstAndLastVisibleBufferIndex(int &firstVisibleBufferIndex,
                                            int &lastVisibleBufferIndex,
                                            const QRectF &viewRect,
                                            bool fullyVisible) const;
    QList<AbstractViewItem*>  m_items;
    AbstractItemView *m_itemView;
    AbstractViewItem *m_prototype;
    int m_bufferSize;

private:
    void insertItem(int pos, const QModelIndex &index);
    bool m_twoColumns;

    Q_DISABLE_COPY(AbstractItemContainer)
};

#endif // ABSTRACTITEMCONTAINER_H
