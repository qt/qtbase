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

#ifndef ABSTRACTSCROLLAREA_H
#define ABSTRACTSCROLLAREA_H

#include "gvbwidget.h"

class ScrollBar;
class QGraphicsGridLayout;

class AbstractScrollArea : public GvbWidget
{
    Q_OBJECT

public:

    AbstractScrollArea(QGraphicsWidget *parent = 0);
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
