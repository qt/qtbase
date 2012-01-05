/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
    ItemRecyclingList(const int itemBuffer = 4, QGraphicsWidget * parent = 0);
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
