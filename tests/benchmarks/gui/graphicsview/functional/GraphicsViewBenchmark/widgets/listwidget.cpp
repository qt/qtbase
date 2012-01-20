/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGraphicsSceneResizeEvent>
#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QTimer>
#include "listwidget.h"

ListWidget::ListWidget(QGraphicsWidget * parent)
  : GvbWidget(parent),
    m_layout(new QGraphicsLinearLayout(Qt::Vertical)), 
    m_listView(new SimpleListView(this))
{ 
    //listView->setViewport(listView->content());
    //listView->content()->setParentItem(listView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0,0,0,0);
    m_layout->setContentsMargins(0,0,0,0); 
    m_listView->setContentsMargins(0,0,0,0);
    m_layout->addItem(m_listView);
    setLayout(m_layout);

    m_scroller.setScrollable(m_listView);
    m_listView->installEventFilter(&m_scroller);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ListWidget::~ListWidget() 
{

}

void ListWidget::addItem(QGraphicsWidget *item)
{
    m_listView->addItem(item);
}

void ListWidget::insertItem(int index, QGraphicsWidget *item)
{
    m_listView->insertItem(index, item);
}

QGraphicsWidget* ListWidget::takeItem(int row)
{
    return m_listView->takeItem(row);
}

QGraphicsWidget* ListWidget::itemAt(int row)
{
    return m_listView->itemAt(row);
}

/* virtual */
void ListWidget::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    QGraphicsWidget::resizeEvent(event);
}

int ListWidget::itemCount() const
{
    if (m_listView)
        return m_listView->itemCount();
    return 0;
}

ScrollBar* ListWidget::verticalScrollBar() const
{
    if (m_listView)
        return m_listView->verticalScrollBar();
    return 0;
}

bool ListWidget::listItemCaching() const
{
    return m_listView->listItemCaching();
}

void ListWidget::setListItemCaching(bool enable)
{
    m_listView->setListItemCaching(enable);
}

void ListWidget::setTwoColumns(const bool twoColumns)
{
    m_listView->setTwoColumns(twoColumns);
}

bool ListWidget::twoColumns()
{
    return m_listView->twoColumns();
}
