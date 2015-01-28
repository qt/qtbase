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

#ifndef ABSTRACTVIEWITEM_H
#define ABSTRACTVIEWITEM_H

#include <QModelIndex>

#include "gvbwidget.h"
#include "abstractitemview.h"
#include "listitem.h"

class QGraphicsWidget;

class AbstractViewItem : public GvbWidget
{
    Q_OBJECT
public:
    AbstractViewItem(QGraphicsWidget *parent = 0);
    virtual ~AbstractViewItem();

    virtual AbstractViewItem *newItemInstance() = 0;

    QModelIndex modelIndex() const;

    void setModelIndex(const QModelIndex &index);

    AbstractViewItem *prototype() const;
    AbstractItemView *itemView() const;
    void setItemView(AbstractItemView *itemView) ;
    virtual void updateItemContents();
    virtual void themeChange();

    virtual void setSubtreeCacheEnabled(bool enabled);

    virtual QSizeF effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

    virtual void setModel(QAbstractItemModel *model) = 0;
    virtual QVariant data(int role) const = 0;
    virtual void setData(const QVariant &value, int role = Qt::DisplayRole) = 0;
    virtual void setTwoColumns(const bool enabled) = 0;

protected:
    virtual bool event(QEvent *e);

    QPersistentModelIndex m_index;

private:
    Q_DISABLE_COPY(AbstractViewItem)

    AbstractItemView *m_itemView;
    AbstractViewItem *m_prototype;

};

#endif // ABSTRACTVIEWITEM_H
