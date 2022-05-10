// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SIMPLELISTVIEW_H
#define SIMPLELISTVIEW_H

#include "scrollbar.h"
#include "abstractscrollarea.h"

class SimpleListViewPrivate;

class SimpleListView : public AbstractScrollArea
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SimpleListView)

public:

    SimpleListView(QGraphicsWidget *parent = nullptr);
    virtual ~SimpleListView();

public:

    void addItem(QGraphicsWidget *item);
    void insertItem(int index, QGraphicsWidget *item);
    QGraphicsWidget* takeItem(int row);
    QGraphicsWidget* itemAt(int row);
    int itemCount();
    void updateListContents();

    void setTwoColumns(const bool twoColumns);
    bool twoColumns();

public slots:

    void themeChange();
    bool listItemCaching() const;
    void setListItemCaching(bool enabled);

protected:

    virtual void scrollContentsBy(qreal dx, qreal dy);
    void resizeEvent(QGraphicsSceneResizeEvent *event);
    QSizeF sizeHint(Qt::SizeHint which,
                    const QSizeF & constraint) const;

private:

    SimpleListViewPrivate *d_ptr;
};

#endif
