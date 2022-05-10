// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractviewitem.h"

AbstractViewItem::AbstractViewItem(QGraphicsWidget *parent)
    : GvbWidget(parent),
    m_index(),
    m_itemView(0),
    m_prototype(0)
{
}

/*virtual*/
AbstractViewItem::~AbstractViewItem()
{
}

QModelIndex AbstractViewItem::modelIndex() const
{
    return m_index;
}

AbstractViewItem *AbstractViewItem::prototype() const
{
    return m_prototype;
}

AbstractItemView *AbstractViewItem::itemView() const
{
    return m_itemView;
}

void AbstractViewItem::setItemView(AbstractItemView *itemView)
{
    m_itemView = itemView;
}

void AbstractViewItem::setModelIndex(const QModelIndex &index)
{
    if (m_index != index) {
        m_index = index;
        updateItemContents();
    }
}

/*virtual*/
QSizeF AbstractViewItem::effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    return GvbWidget::effectiveSizeHint(which, constraint);
}

/*virtual*/
bool AbstractViewItem::event(QEvent *e)
{
    return QGraphicsWidget::event(e);
}

/*virtual*/
void AbstractViewItem::updateItemContents()
{
    ; // No impl yet
}

/*virtual*/
void AbstractViewItem::themeChange()
{
    ; // No impl yet
}

/*virtual*/
void AbstractViewItem::setSubtreeCacheEnabled(bool enabled)
{
    Q_UNUSED(enabled);
    ; // No impl
}

