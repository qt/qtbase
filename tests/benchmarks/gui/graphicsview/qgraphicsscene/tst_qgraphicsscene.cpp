/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

class tst_QGraphicsScene : public QObject
{
    Q_OBJECT

public:
    tst_QGraphicsScene();
    virtual ~tst_QGraphicsScene();

public slots:
    void init();
    void cleanup();

private slots:
    void construct();
    void addItem_data();
    void addItem();
    void itemAt_data();
    void itemAt();
    void initialShow();
};

tst_QGraphicsScene::tst_QGraphicsScene()
{
}

tst_QGraphicsScene::~tst_QGraphicsScene()
{
}

static inline void processEvents()
{
    QApplication::flush();
    QApplication::processEvents();
    QApplication::processEvents();
}

void tst_QGraphicsScene::init()
{
    processEvents();
}

void tst_QGraphicsScene::cleanup()
{
}

void tst_QGraphicsScene::construct()
{
    QBENCHMARK {
        QGraphicsScene scene;
    }
}

void tst_QGraphicsScene::addItem_data()
{
    QTest::addColumn<int>("indexMethod");
    QTest::addColumn<QRectF>("sceneRect");
    QTest::addColumn<int>("numItems_X");
    QTest::addColumn<int>("numItems_Y");
    QTest::addColumn<int>("itemType");
    QTest::addColumn<QRectF>("itemRect");

    QTest::newRow("null") << 0 << QRectF() << 0 << 0 << 0 << QRectF();
    QTest::newRow("0 QRectF() 10 x  10 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF() << 10 << 10 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 25 x  25 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF() << 25 << 25 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 100 x 100 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF() << 100 << 100 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 250 x 250 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF() << 250 << 250 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF()  10 x  10 QGraphicsEllipseItem (0,0,10,10)") << 0 << QRectF() << 10 << 10 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF()  25 x  25 QGraphicsEllipseItem (0,0,10,10)") << 0 << QRectF() << 25 << 25 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 100 x 100 QGraphicsEllipseItem (0,0,10,0)") << 0 << QRectF() << 100 << 100 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 250 x 250 QGraphicsEllipseItem (0,0,10,10)") << 0 << QRectF() << 250 << 250 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF()  10 x  10 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF(0, 0, 100, 100) << 10 << 10 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF()  25 x  25 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF(0, 0, 250, 250) << 25 << 25 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 100 x 100 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF(0, 0, 1000, 1000) << 100 << 100 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 250 x 250 QGraphicsRectItem (0,0,10,10)") << 0 << QRectF(0, 0, 2500, 2500) << 250 << 250 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF()  10 x  10 QGraphicsEllipseItem (0,0,10,10)") << 0 << QRectF(0, 0, 100, 100) << 10 << 10 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF()  25 x  25 QGraphicsEllipseItem (0,0,10,10)") << 0 << QRectF(0, 0, 250, 250) << 25 << 25 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 100 x 100 QGraphicsEllipseItem (0,0,10,10)") << 0 << QRectF(0, 0, 1000, 1000) << 100 << 100 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("0 QRectF() 250 x 250 QGraphicsEllipseItem (0,0,10,10)") << 0 << QRectF(0, 0, 2500, 2500) << 250 << 250 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 10 x  10 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF() << 10 << 10 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF()  25 x  25 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF() << 25 << 25 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 100 x 100 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF() << 100 << 100 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 250 x 250 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF() << 250 << 250 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF()  10 x  10 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF() << 10 << 10 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF()  25 x  25 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF() << 25 << 25 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 100 x 100 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF() << 100 << 100 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 250 x 250 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF() << 250 << 250 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF()  10 x  10 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF(0, 0, 100, 100) << 10 << 10 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF()  25 x  25 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF(0, 0, 250, 250) << 25 << 25 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 100 x 100 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF(0, 0, 1000, 1000) << 100 << 100 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 250 x 250 QGraphicsRectItem (0,0,10,10)") << 1 << QRectF(0, 0, 2500, 2500) << 250 << 250 << int(QGraphicsRectItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF()  10 x  10 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF(0, 0, 100, 100) << 10 << 10 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF()  25 x  25 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF(0, 0, 250, 250) << 25 << 25 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 100 x 100 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF(0, 0, 1000, 1000) << 100 << 100 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
    QTest::newRow("1 QRectF() 250 x 250 QGraphicsEllipseItem (0,0,10,10)") << 1 << QRectF(0, 0, 2500, 2500) << 250 << 250 << int(QGraphicsEllipseItem::Type) << QRectF(0, 0, 10, 10);
}

void tst_QGraphicsScene::addItem()
{
    QFETCH(int, indexMethod);
    QFETCH(QRectF, sceneRect);
    QFETCH(int, numItems_X);
    QFETCH(int, numItems_Y);
    QFETCH(int, itemType);
    QFETCH(QRectF, itemRect);

    QGraphicsScene scene;
    scene.setItemIndexMethod(indexMethod ? QGraphicsScene::BspTreeIndex : QGraphicsScene::NoIndex);
    if (!sceneRect.isNull())
        scene.setSceneRect(sceneRect);

    processEvents();

    QBENCHMARK {
        QGraphicsItem *item = 0;
        for (int y = 0; y < numItems_Y; ++y) {
            for (int x = 0; x < numItems_X; ++x) {
                switch (itemType) {
                case QGraphicsRectItem::Type:
                    item = new QGraphicsRectItem(itemRect);
                    break;
                case QGraphicsEllipseItem::Type:
                default:
                    item = new QGraphicsEllipseItem(itemRect);
                    break;
                }
                item->setPos(x * itemRect.width(), y * itemRect.height());
                scene.addItem(item);
            }
        }
        scene.itemAt(0, 0);
    }
    //let QGraphicsScene::_q_polishItems be called so ~QGraphicsItem doesn't spend all his time cleaning the unpolished list
    qApp->processEvents();
}

void tst_QGraphicsScene::itemAt_data()
{
    QTest::addColumn<int>("bspTreeDepth");
    QTest::addColumn<QRectF>("sceneRect");
    QTest::addColumn<int>("numItems_X");
    QTest::addColumn<int>("numItems_Y");
    QTest::addColumn<QRectF>("itemRect");

    QTest::newRow("null") << 0 << QRectF() << 0 << 0 << QRectF();
    QTest::newRow("NoIndex 10x10") << -1 << QRectF() << 10 << 10 << QRectF(-10, -10, 20, 20);
    QTest::newRow("NoIndex 25x25") << -1 << QRectF() << 25 << 25 << QRectF(-10, -10, 20, 20);
    QTest::newRow("NoIndex 100x100") << -1 << QRectF() << 100 << 100 << QRectF(-10, -10, 20, 20);
    QTest::newRow("NoIndex 250x250") << -1 << QRectF() << 250 << 250 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=auto 10x10") << 0 << QRectF() << 10 << 10 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=auto 25x25") << 0 << QRectF() << 25 << 25 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=auto 100x100") << 0 << QRectF() << 100 << 100 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=auto 250x250") << 0 << QRectF() << 250 << 250 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=16 10x10") << 16 << QRectF() << 10 << 10 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=16 25x25") << 16 << QRectF() << 25 << 25 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=16 100x100") << 16 << QRectF() << 100 << 100 << QRectF(-10, -10, 20, 20);
    QTest::newRow("BspTreeIndex depth=16 250x250") << 16 << QRectF() << 250 << 250 << QRectF(-10, -10, 20, 20);
}

void tst_QGraphicsScene::itemAt()
{
    QFETCH(int, bspTreeDepth);
    QFETCH(QRectF, sceneRect);
    QFETCH(int, numItems_X);
    QFETCH(int, numItems_Y);
    QFETCH(QRectF, itemRect);

    QGraphicsScene scene;
    scene.setItemIndexMethod(bspTreeDepth >= 0 ? QGraphicsScene::BspTreeIndex : QGraphicsScene::NoIndex);
    if (bspTreeDepth > 0)
        scene.setBspTreeDepth(bspTreeDepth);
    if (!sceneRect.isNull())
        scene.setSceneRect(sceneRect);

    for (int y = 0; y < numItems_Y; ++y) {
        for (int x = 0; x < numItems_X; ++x) {
            QGraphicsRectItem *item = new QGraphicsRectItem(itemRect);
            item->setPos((x - numItems_X/2) * itemRect.width(), (y - numItems_Y/2) * itemRect.height());
            scene.addItem(item);
        }
    }

    scene.itemAt(0, 0); // triggers indexing
    processEvents();

    QGraphicsItem *item = 0;
    QBENCHMARK {
        item = scene.itemAt(0, 0);
    }

    //let QGraphicsScene::_q_polishItems be called so ~QGraphicsItem doesn't spend all his time cleaning the unpolished list
    qApp->processEvents();
}

void tst_QGraphicsScene::initialShow()
{
    QGraphicsScene scene;

    QBENCHMARK {
        for (int y = 0; y < 30000; ++y) {
            QGraphicsRectItem *item = new QGraphicsRectItem(0, 0, 50, 50);
            item->setPos((y/2) * item->rect().width(), (y/2) * item->rect().height());
            scene.addItem(item);
        }
        scene.itemAt(0, 0); // triggers indexing
        //This call polish the items so we bench their processing too.
        qApp->processEvents();
    }
}

QTEST_MAIN(tst_QGraphicsScene)
#include "tst_qgraphicsscene.moc"
