// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SIMPLELIST_H_
#define SIMPLELIST_H_


#include "gvbwidget.h"
#include "listitem.h"
#include "listwidget.h"

class QGraphicsWidget;

class SimpleList : public GvbWidget
{
    Q_OBJECT

public:
    SimpleList(QGraphicsWidget *parent=0);
    virtual ~SimpleList();
    void addItem(ListItem *item);
    void insertItem(int index, ListItem *item);
    ListItem* takeItem(int row);
    ListItem* itemAt(int row);
    int itemCount() const;
    virtual void keyPressEvent(QKeyEvent *event);
    bool listItemCaching() const;
    void setListItemCaching(bool enable);

    void setTwoColumns(const bool twoColumns);
    bool twoColumns();

public slots:
    ScrollBar* verticalScrollBar() const;

private:
    Q_DISABLE_COPY(SimpleList)

    ListWidget *m_list;
};

#endif /* LISTTEST_H_ */
