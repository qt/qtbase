/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
