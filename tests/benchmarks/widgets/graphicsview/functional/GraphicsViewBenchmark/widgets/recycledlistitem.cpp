// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QGraphicsGridLayout>
#include <QDebug>

#include "recycledlistitem.h"
#include "listmodel.h"

static const int MinItemHeight = 70;
static const int MinItemWidth = 276;

RecycledListItem::RecycledListItem(QGraphicsWidget *parent)
    : AbstractViewItem(parent),
    m_item(new ListItem(this)),
    m_item2(0),
    m_model(0),
    m_layout(new QGraphicsGridLayout())
{
    m_item->setMinimumWidth(MinItemWidth);
    setContentsMargins(0,0,0,0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_layout->addItem(m_item, 0, 0);
    setLayout(m_layout);
    m_layout->setContentsMargins(0,0,0,0);
    m_layout->setSpacing(0);
    m_layout->setHorizontalSpacing(0.0);
    m_layout->setVerticalSpacing(0.0);
}

RecycledListItem::~RecycledListItem()
{
}

void RecycledListItem::setModel(QAbstractItemModel *model)
{
    m_model = model;
}

/*virtual*/
AbstractViewItem *RecycledListItem::newItemInstance()
{
    RecycledListItem* item = new RecycledListItem();
    return item;
}

QSizeF RecycledListItem::effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    QSizeF s = m_item->effectiveSizeHint(which,constraint);
    if (m_item2)
        s.setWidth(s.width()*2);
    if (s.height()<MinItemHeight)
        s.setHeight(MinItemHeight);
    return s;
}

QVariant RecycledListItem::data(int role) const
{
    if (m_item && role == Qt::DisplayRole)
        return m_item->data();

    return QVariant();
}

void RecycledListItem::setData(const QVariant &value, int role)
{
    if (m_item && role == Qt::DisplayRole) {
        m_item->setData(value);
        if (m_item2) {
            m_item2->setData(value);
        }
    }
}

/*virtual*/
void RecycledListItem::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    AbstractViewItem::resizeEvent(event);
}

void RecycledListItem::updateItemContents()
{
    AbstractViewItem::updateItemContents();
    if (m_model && m_index.isValid())
        setData(m_model->data(m_index,Qt::DisplayRole), Qt::DisplayRole);
}

void RecycledListItem::setTwoColumns(const bool enabled)
{
    if (m_item2 && enabled)
        return;

    if (enabled) {
        m_item2 = new ListItem();
        m_item2->setMinimumWidth(MinItemWidth);
        m_layout->addItem(m_item2, 0, 1);
        updateItemContents();
    }
    else {
        if (m_layout->count() > 1) {
            m_layout->removeAt(1);
        }
        delete m_item2;
        m_item2 = 0;
    }

    if (!m_layout->isActivated())
        m_layout->activate();
}
