// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTextDocument>

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
    void createTextItem();
    void createTextItemNoLayouting();
};

tst_QGraphicsItem::tst_QGraphicsItem()
{
}

tst_QGraphicsItem::~tst_QGraphicsItem()
{
}

static inline void processEvents()
{
    QApplication::processEvents();
    QApplication::processEvents();
}

void tst_QGraphicsItem::initTestCase()
{
    processEvents();
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
        QGraphicsItem *first = rect.childItems().first();
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
        rect->setPos(pos);
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

    const QTransform rotate(QTransform().rotate(45));
    QBENCHMARK {
        item->setTransform(rotate, true);
    }
}

void tst_QGraphicsItem::scale()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    const QTransform scale(QTransform::fromScale(2, 2));
    QBENCHMARK {
        item->setTransform(scale, true);
    }
}

void tst_QGraphicsItem::shear()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    const QTransform shear = QTransform().shear(1.5, 1.5);
    QBENCHMARK {
        item->setTransform(shear, true);
    }
}

void tst_QGraphicsItem::translate()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
    processEvents();

    const QTransform translate = QTransform::fromTranslate(100, 100);
    QBENCHMARK {
        item->setTransform(translate, true);
    }
}

void tst_QGraphicsItem::createTextItem()
{
    // Ensure QFontDatabase loaded the font beforehand
    QFontInfo(qApp->font()).family();
    const QString text = "This is some text";
    QBENCHMARK {
        QGraphicsTextItem item(text);
    }
}

void tst_QGraphicsItem::createTextItemNoLayouting()
{
    // Ensure QFontDatabase loaded the font beforehand
    QFontInfo(qApp->font()).family();
    const QString text = "This is some text";
    QBENCHMARK {
        QGraphicsTextItem item;
        item.document()->setLayoutEnabled(false);
        // Prepare everything
        item.setPlainText(text);
        QTextOption option = item.document()->defaultTextOption();
        option.setAlignment(Qt::AlignHCenter);
        item.document()->setDefaultTextOption(option);
        // And (in a real app) enable layouting here
    }
}

QTEST_MAIN(tst_QGraphicsItem)
#include "tst_qgraphicsitem.moc"
