/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QGraphicsLinearLayout>
#include <QFont>
#include <QTime>

#include "simplelist.h"
static const int MinItemWidth = 276;

SimpleList::SimpleList(QGraphicsWidget *parent)
  : GvbWidget(parent),
    m_list(new ListWidget(this))
{
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout();
    layout->setContentsMargins(0,0,0,0);
    setContentsMargins(0,0,0,0);
    setLayout(layout);
    layout->addItem(m_list);
    setObjectName("SimpleList");
}

/*virtual*/
SimpleList::~SimpleList()
{
}

void SimpleList::addItem(ListItem *item)
{
    item->setMinimumWidth(MinItemWidth);
    m_list->addItem(item);
}

void SimpleList::insertItem(int index, ListItem *item)
{
    item->setMinimumWidth(MinItemWidth);
    m_list->insertItem(index, item);
}

ListItem* SimpleList::takeItem(int row)
{
    return static_cast<ListItem*>(m_list->takeItem(row));
}

ListItem* SimpleList::itemAt(int row)
{
    return static_cast<ListItem*>(m_list->itemAt(row));
}

int SimpleList::itemCount() const
{
    if (m_list)
        return m_list->itemCount();
    return 0;
}

ScrollBar* SimpleList::verticalScrollBar() const
{
    if (m_list)
        return m_list->verticalScrollBar();
    return 0;
}

bool SimpleList::listItemCaching() const
{
    return m_list->listItemCaching();
}

void SimpleList::setListItemCaching(bool enable)
{
    m_list->setListItemCaching(enable);
}

void SimpleList::keyPressEvent(QKeyEvent *event)
{
    static QTime keyPressInterval = QTime::currentTime();
    static qreal step = 0.0;
    static bool repeat = false;
    int interval = keyPressInterval.elapsed();

    ScrollBar* sb = verticalScrollBar();
    qreal currentValue = sb->sliderPosition();

    if(interval < 250 ) {
        if(!repeat) step = 0.0;
        step = step + 2.0;
        if(step > 100) step = 100;
        repeat = true;
    }
    else {
        step = 1.0;
        if(itemAt(0))
            step = itemAt(0)->size().height();
        repeat = false;
    }

    if(event->key() == Qt::Key_Up ) { //Up Arrow
        sb->setSliderPosition(currentValue - step);
    }

    if(event->key() == Qt::Key_Down ) { //Down Arrow
        sb->setSliderPosition(currentValue + step);
    }
    keyPressInterval.start();
}


void SimpleList::setTwoColumns(const bool twoColumns)
{
    m_list->setTwoColumns(twoColumns);
}

bool SimpleList::twoColumns()
{
    return m_list->twoColumns();
}

