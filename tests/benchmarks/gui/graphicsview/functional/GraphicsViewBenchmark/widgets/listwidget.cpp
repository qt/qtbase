/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
