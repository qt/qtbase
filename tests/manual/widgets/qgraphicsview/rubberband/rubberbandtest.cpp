/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

class MyGraphicsItem : public QGraphicsRectItem
{
public:
    MyGraphicsItem() : QGraphicsRectItem()
    {
        setFlags(QGraphicsItem::ItemIsSelectable);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option*/, QWidget * /*widget*/)
    {
        if (isSelected())
            painter->fillRect(rect(), QColor(255, 0, 0));
        else
            painter->fillRect(rect(), QColor(0, 255, 0));
    }
};

class MyGraphicsView : public QGraphicsView
{

public:
    MyGraphicsView() : QGraphicsView()
    {
        setDragMode(QGraphicsView::RubberBandDrag);
    }
protected:
    void mouseMoveEvent(QMouseEvent *event)
    {
        int rightmostInView = viewport()->mapToGlobal(viewport()->geometry().topRight()).x();
        int xglobal = event->globalX();
        if (xglobal > rightmostInView)
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + 10);
        QGraphicsView::mouseMoveEvent(event);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MyGraphicsView v;

    QGraphicsScene s(0.0, 0.0, 10000.0, 100.0);
    v.setScene(&s);
    v.setInteractive(true);
    v.setRubberBandSelectionMode(Qt::IntersectsItemBoundingRect);
    s.addRect( (qreal) 0.0, 0.0, 1000.0, 50.0, QPen(),QBrush(QColor(0,0,255)));

    for (int u = 0; u < 100; ++u) {
        MyGraphicsItem *item = new MyGraphicsItem();
        item->setRect(QRectF(u * 100, 50.0, 50.0, 20.0));
        s.addItem(item);
    }
    v.show();
    app.exec();
    return 0;
}
