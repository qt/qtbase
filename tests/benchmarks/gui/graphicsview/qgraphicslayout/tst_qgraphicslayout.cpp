/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtTest/QtTest>
#include <QtGui/qgraphicslayout.h>
#include <QtGui/qgraphicslinearlayout.h>
#include <QtGui/qgraphicswidget.h>
#include <QtGui/qgraphicsview.h>

class tst_QGraphicsLayout : public QObject
{
    Q_OBJECT
public:
    tst_QGraphicsLayout() {}
    ~tst_QGraphicsLayout() {}

private slots:
    void invalidate();
};


class RectWidget : public QGraphicsWidget
{
public:
    RectWidget(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0) : QGraphicsWidget(parent, wFlags), setGeometryCalls(0) {}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->drawRoundRect(rect());
        painter->drawLine(rect().topLeft(), rect().bottomRight());
        painter->drawLine(rect().bottomLeft(), rect().topRight());
    }

    void setGeometry(const QRectF &rect)
    {
        //qDebug() << "setGeometry():" << this->data(0).toString();
        setGeometryCalls->insert(this, rect);
        QGraphicsWidget::setGeometry(rect);
    }

    void callUpdateGeometry() {
        QGraphicsWidget::updateGeometry();
    }

    QMap<RectWidget*, QRectF> *setGeometryCalls;
};

/**
 * Test to see how much time is needed to resize all widgets in a
 * layout-widget-layout-widget-.... hierarchy from the point where a
 * leaf widget changes its size hint. (updateGeometry() is called).
 *
 * If you run the test for 4.7 you'll get some really high numbers, but
 * that's because they also include painting (and possible processing of
 * some other events).
 */
void tst_QGraphicsLayout::invalidate()
{
    QGraphicsLayout::setInstantInvalidatePropagation(true);
    QGraphicsScene scene;
    QGraphicsView *view = new QGraphicsView(&scene);
    QMap<RectWidget*, QRectF> setGeometryCalls;

    RectWidget *window = new RectWidget(0, Qt::Window);
    window->setGeometryCalls = &setGeometryCalls;
    window->setData(0, QString(QChar('a')));

    scene.addItem(window);
    RectWidget *leaf = 0;
    const int depth = 100;
    RectWidget *parent = window;
    for (int i = 1; i < depth; ++i) {
        QGraphicsLinearLayout *l = new QGraphicsLinearLayout(parent);
        l->setContentsMargins(0,0,0,0);
        RectWidget *child = new RectWidget;
        child->setData(0, QString(QChar('a' + i)));
        child->setGeometryCalls = &setGeometryCalls;
        l->addItem(child);
        parent = child;
    }
    leaf = parent;
    leaf->setMinimumSize(QSizeF(1,1));

    view->show();

    QTest::qWaitForWindowShown(view);

    // ...then measure...

    int pass = 1;

    // should be as small as possible, to reduce overhead of painting
    QSizeF size(1, 1);
    setGeometryCalls.clear();
    QBENCHMARK {
        leaf->setMinimumSize(size);
        leaf->setMaximumSize(size);
        while (setGeometryCalls.count() < depth) {
            QApplication::sendPostedEvents();
        }
        // force a resize on each widget, this will ensure
        // that each iteration will resize all 50 widgets
        int w = int(size.width());
        w^=2;
        size.setWidth(w);
    }
    delete view;
    QGraphicsLayout::setInstantInvalidatePropagation(false);
}

QTEST_MAIN(tst_QGraphicsLayout)

#include "tst_qgraphicslayout.moc"
