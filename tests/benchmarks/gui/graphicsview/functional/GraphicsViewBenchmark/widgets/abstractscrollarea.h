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
