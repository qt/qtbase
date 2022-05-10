// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTSCROLLAREA_H
#define ABSTRACTSCROLLAREA_H

#include "gvbwidget.h"

class ScrollBar;
class QGraphicsGridLayout;

class AbstractScrollArea : public GvbWidget
{
    Q_OBJECT

public:

    AbstractScrollArea(QGraphicsWidget *parent = nullptr);
    ~AbstractScrollArea();

public:

    void setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy policy);
    void setVerticalScrollBarPolicy(Qt::ScrollBarPolicy policy);
    Qt::ScrollBarPolicy verticalScrollBarPolicy() const;
    Qt::ScrollBarPolicy horizontalScrollBarPolicy() const;

    QGraphicsWidget *viewport() const;
    void setViewport(QGraphicsWidget *viewport);

    ScrollBar *verticalScrollBar() const;
    ScrollBar *horizontalScrollBar() const;

signals:

    void viewportChanged(QGraphicsWidget *viewport);

protected:

    virtual bool event(QEvent *e);
    virtual void scrollContentsBy(qreal dx, qreal dy);

private slots:

    void verticalScrollStart();
    void verticalScroll(qreal);
    void horizontalScrollStart();
    void horizontalScroll(qreal);

private:

    void adjustScrollBars();

    QGraphicsWidget *m_viewport;
    ScrollBar *m_horizontalScrollBar;
    ScrollBar *m_verticalScrollBar;
    Qt::ScrollBarPolicy m_verticalScrollBarPolicy;
    Qt::ScrollBarPolicy m_horizontalScrollBarPolicy;
    qreal m_prevHorizontalValue;
    qreal m_prevVerticalValue;
};

#endif // ABSTRACTSCROLLAREA_H
