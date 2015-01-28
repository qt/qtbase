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
