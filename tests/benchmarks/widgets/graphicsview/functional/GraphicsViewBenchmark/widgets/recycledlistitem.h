// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RECYCLEDLISTITEM_H
#define RECYCLEDLISTITEM_H

#include "abstractviewitem.h"

class ListItem;
class QGraphicsWidget;
class QGraphicsGridLayout;

class RecycledListItem : public AbstractViewItem
{
    Q_OBJECT
public:
    RecycledListItem(QGraphicsWidget *parent=0);
    virtual ~RecycledListItem();

    virtual void setModel(QAbstractItemModel *model);

    virtual AbstractViewItem *newItemInstance();
    virtual void updateItemContents();

    virtual QVariant data(int role) const;
    virtual void setData(const QVariant &value, int role = Qt::DisplayRole);

    ListItem *item() { return m_item; }

    void setTwoColumns(const bool enabled);

protected:
    virtual QSizeF effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
    virtual void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    Q_DISABLE_COPY(RecycledListItem)

    ListItem *m_item;
    ListItem *m_item2;
    QAbstractItemModel *m_model;
    QGraphicsGridLayout *m_layout;
};

#endif // RECYCLEDLISTITEM_H
