// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LISTWIDGET_H
#define LISTWIDGET_H

#include <QGraphicsWidget>
#include "simplelistview.h"
#include "scroller.h"
#include "gvbwidget.h"

class AbstractViewItem;
class QGraphicsSceneResizeEvent;
class QGraphicsGridLayout;
class QGraphicsLinearLayout;

class ListWidget : public GvbWidget
{
    Q_OBJECT

public:
    ListWidget(QGraphicsWidget * parent = nullptr);
    virtual ~ListWidget();
    void addItem(QGraphicsWidget *item);
    void insertItem(int index, QGraphicsWidget *item);
    QGraphicsWidget* takeItem(int row);
    QGraphicsWidget* itemAt(int row);
    int itemCount() const;
    bool listItemCaching() const;
    void setListItemCaching(bool enable);
    ScrollBar* verticalScrollBar() const;

    void setTwoColumns(const bool twoColumns);
    bool twoColumns();

protected:
    virtual void resizeEvent( QGraphicsSceneResizeEvent * event );

private:
    Q_DISABLE_COPY(ListWidget)

    QGraphicsLinearLayout *m_layout;
    SimpleListView *m_listView;
    Scroller m_scroller;
};

#endif
