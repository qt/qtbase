/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
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

#include <qtest.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>

class tst_QGraphicsItem : public QObject
{
    Q_OBJECT

public:
    tst_QGraphicsItem();
    virtual ~tst_QGraphicsItem();

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    void setParentItem();
    void setParentItem_deep();
    void setParentItem_deep_reversed();
    void deleteItemWithManyChildren();
    void setPos_data();
    void setPos();
    void setTransform_data();
    void setTransform();
    void rotate();
    void scale();
    void shear();
    void translate();
};

tst_QGraphicsItem::tst_QGraphicsItem()
{
}

tst_QGraphicsItem::~tst_QGraphicsItem()
{
}

static inline void processEvents()
{
    QApplication::flush();
    QApplication::processEvents();
    QApplication::processEvents();
}

void tst_QGraphicsItem::initTestCase()
{
    QApplication::flush();
    QTest::qWait(1500);
    processEvents();
}

void tst_QGraphicsItem::init()
{
    processEvents();
}

void tst_QGraphicsItem::cleanup()
{
}

void tst_QGraphicsItem::setParentItem()
{
    QBENCHMARK {
        QGraphicsRectItem rect;
        QGraphicsRectItem *childRect = new QGraphicsRectItem;
        childRect->setParentItem(&rect);
    }
}

void tst_QGraphicsItem::setParentItem_deep()
{
    QBENCHMARK {
        QGraphicsRectItem rect;
        QGraphicsRectItem *lastRect = &rect;
        for (int i = 0; i < 10; ++i) {
            QGraphicsRectItem *childRect = new QGraphicsRectItem;
            childRect->setParentItem(lastRect);
            lastRect = childRect;
        }
        QGraphicsItem *first = rect.children().first();
        first->setParentItem(0);
    }
}

void tst_QGraphicsItem::setParentItem_deep_reversed()
{
    QBENCHMARK {
        QGraphicsRectItem *lastRect = new QGraphicsRectItem;
        for (int i = 0; i < 100; ++i) {
            QGraphicsRectItem *parentRect = new QGraphicsRectItem;
            lastRect->setParentItem(parentRect);
            lastRect = parentRect;
        }
        delete lastRect;
    }
}

void tst_QGraphicsItem::deleteItemWithManyChildren()
{
    QBENCHMARK {
        QGraphicsRectItem *rect = new QGraphicsRectItem;
        for (int i = 0; i < 1000; ++i)
            new QGraphicsRectItem(rect);
        delete rect;
    }
}

void tst_QGraphicsItem::setPos_data()
{
    QTest::addColumn<QPointF>("pos");

    QTest::newRow("0, 0") << QPointF(0, 0);
    QTest::newRow("10, 10") << QPointF(10, 10);
    QTest::newRow("-10, -10") << QPointF(-10, -10);
}

void tst_QGraphicsItem::setPos()
{
    QFETCH(QPointF, pos);

    QGraphicsScene scene;
    QGraphicsRectItem *rect = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    QBENCHMARK {
        rect->setPos(10, 10);
    }
}

void tst_QGraphicsItem::setTransform_data()
{
    QTest::addColumn<QTransform>("transform");

    QTest::newRow("rotate 45z") << QTransform().rotate(45);
    QTest::newRow("scale 2x2") << QTransform().scale(2, 2);
    QTest::newRow("translate 100, 100") << QTransform().translate(100, 100);
    QTest::newRow("rotate 45x 45y 45z") << QTransform().rotate(45, Qt::XAxis)
        .rotate(45, Qt::YAxis).rotate(45, Qt::ZAxis);
}

void tst_QGraphicsItem::setTransform()
{
    QFETCH(QTransform, transform);

    QGraphicsScene scene;
    QGraphicsRectItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    QBENCHMARK {
        item->setTransform(transform);
    }
}

void tst_QGraphicsItem::rotate()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    QBENCHMARK {
        item->rotate(45);
    }
}

void tst_QGraphicsItem::scale()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    QBENCHMARK {
        item->scale(2, 2);
    }
}

void tst_QGraphicsItem::shear()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    QBENCHMARK {
        item->shear(1.5, 1.5);
    }
}

void tst_QGraphicsItem::translate()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    QBENCHMARK {
        item->translate(100, 100);
    }
}

QTEST_MAIN(tst_QGraphicsItem)
#include "tst_qgraphicsitem.moc"
