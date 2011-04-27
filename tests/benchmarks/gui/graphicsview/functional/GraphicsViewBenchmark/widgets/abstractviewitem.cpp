/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#if (QT_VERSION >= 0x040600)
/*virtual*/
void AbstractViewItem::setSubtreeCacheEnabled(bool enabled)
{
    Q_UNUSED(enabled)
    ; // No impl
}
#endif

