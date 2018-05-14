/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtGui>
#include <QtTest/QtTest>
#include <QTest>
#include <QMetaType>
#include <QtWidgets/qgraphicsanchorlayout.h>
#include <private/qgraphicsanchorlayout_p.h>

#define TEST_COMPLEX_CASES


//---------------------- AnchorLayout helper class ----------------------------
class TheAnchorLayout : public QGraphicsAnchorLayout
{
public:
    TheAnchorLayout() : QGraphicsAnchorLayout()
    {
        setContentsMargins( 0,0,0,0 );
        setSpacing( 0 );
    }

    bool isValid()
    {
        return !QGraphicsAnchorLayoutPrivate::get(this)->hasConflicts();
    }

    void setAnchor(
        QGraphicsLayoutItem *startItem,
        Qt::AnchorPoint startEdge,
        QGraphicsLayoutItem *endItem,
        Qt::AnchorPoint endEdge,
        qreal value)
        {
            QGraphicsAnchor *anchor = addAnchor( startItem, startEdge, endItem, endEdge);
            if (anchor)
                anchor->setSpacing(value);
        }

    int indexOf(const QGraphicsLayoutItem* item) const
    {
        for ( int i=0; i< count(); i++) {
            if ( itemAt(i) == item ) {
                return i;
            }
        }
        return -1;
    }

    void removeItem(QGraphicsLayoutItem* item)
    {
        removeAt(indexOf(item));
    }

    void removeAnchor(
        QGraphicsLayoutItem *startItem,
        Qt::AnchorPoint startEdge,
        QGraphicsLayoutItem *endItem,
        Qt::AnchorPoint endEdge)
        {
            delete QGraphicsAnchorLayout::anchor(startItem, startEdge, endItem, endEdge);
        }
};
//-----------------------------------------------------------------------------


struct BasicLayoutTestData
{
    inline BasicLayoutTestData(
        int index1, Qt::AnchorPoint edge1,
        int index2, Qt::AnchorPoint edge2,
        qreal distance)
        : firstIndex(index1), firstEdge(edge1),
          secondIndex(index2), secondEdge(edge2),
          spacing(distance)
        {
        }

    int firstIndex;
    Qt::AnchorPoint firstEdge;
    int secondIndex;
    Qt::AnchorPoint secondEdge;
    qreal spacing;
};

struct AnchorItemSizeHint
{
    inline AnchorItemSizeHint(
        qreal hmin, qreal hpref, qreal hmax,
        qreal vmin, qreal vpref, qreal vmax )
        : hmin(hmin), hpref(hpref), hmax(hmax), vmin(vmin), vpref(vpref), vmax(vmax)
        {
        }
    qreal hmin, hpref, hmax;
    qreal vmin, vpref, vmax;
};

// some test results

struct BasicLayoutTestResult
{
    inline BasicLayoutTestResult(
        int resultIndex, const QRectF& resultRect )
        : index(resultIndex), rect(resultRect)
        {
        }

    int index;
    QRectF rect;
};

typedef QList<BasicLayoutTestData> BasicLayoutTestDataList;
Q_DECLARE_METATYPE(BasicLayoutTestDataList)

typedef QList<BasicLayoutTestResult> BasicLayoutTestResultList;
Q_DECLARE_METATYPE(BasicLayoutTestResultList)

typedef QList<AnchorItemSizeHint> AnchorItemSizeHintList;
Q_DECLARE_METATYPE(AnchorItemSizeHintList)


//---------------------- Test Widget used on all tests ------------------------
class TestWidget : public QGraphicsWidget
{
public:
    inline TestWidget(QGraphicsItem *parent = 0, const QString &name = QString())
        : QGraphicsWidget(parent)
        {
            setContentsMargins( 0,0,0,0 );
            if (name.isEmpty())
                setData(0, QLatin1Char('w') + QString::number(quintptr(this)));
            else
                setData(0, name);
        }
    ~TestWidget()
        {
        }

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
};

QSizeF TestWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    Q_UNUSED( constraint );
    if (which == Qt::MinimumSize) {
        return QSizeF(5,5);
    }

    if (which == Qt::PreferredSize) {
        return QSizeF(50,50);
    }

    return QSizeF(500,500);
}
//-----------------------------------------------------------------------------



//----------------------------- Test class ------------------------------------
class tst_QGraphicsAnchorLayout1 : public QObject
{
    Q_OBJECT

private slots:
    void testCount();

    void testRemoveAt();
    void testRemoveItem();

    void testItemAt();
    void testIndexOf();

    void testAddAndRemoveAnchor();
    void testIsValid();
    void testSpecialCases();

    void testBasicLayout_data();
    void testBasicLayout();

    void testNegativeSpacing_data();
    void testNegativeSpacing();

    void testMixedSpacing_data();
    void testMixedSpacing();

    void testMulti_data();
    void testMulti();

    void testCenterAnchors_data();
    void testCenterAnchors();

    void testRemoveCenterAnchor_data();
    void testRemoveCenterAnchor();

    void testSingleSizePolicy_data();
    void testSingleSizePolicy();

    void testDoubleSizePolicy_data();
    void testDoubleSizePolicy();

    void testSizeDistribution_data();
    void testSizeDistribution();

    void testSizeHint();

#ifdef TEST_COMPLEX_CASES
    void testComplexCases_data();
    void testComplexCases();
#endif
};


void tst_QGraphicsAnchorLayout1::testCount()
{
    QGraphicsWidget *widget = new QGraphicsWidget;

    TheAnchorLayout *layout = new TheAnchorLayout();
    QVERIFY( layout->count() == 0 );

    TestWidget *widget1 = new TestWidget();
    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 1);
    QCOMPARE( layout->count(), 1 );

    // adding one more anchor for already added widget should not increase the count
    layout->setAnchor(layout, Qt::AnchorRight, widget1, Qt::AnchorRight, 1);
    QCOMPARE( layout->count(), 1 );

    // create one more widget and attach with anchor layout
    TestWidget *widget2 = new TestWidget();
    layout->setAnchor(layout, Qt::AnchorLeft, widget2, Qt::AnchorLeft, 1);
    QCOMPARE( layout->count(), 2 );

    widget->setLayout(layout);
    delete widget;
}

void tst_QGraphicsAnchorLayout1::testRemoveAt()
{
    TheAnchorLayout *layout = new TheAnchorLayout();
    QVERIFY( layout->count() == 0 );

    TestWidget *widget1 = new TestWidget();
    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 2);
    QVERIFY( layout->count() == 1 );

    TestWidget *widget2 = new TestWidget();
    layout->setAnchor(widget2, Qt::AnchorLeft, layout, Qt::AnchorLeft, 0.1);
    QVERIFY( layout->count() == 2 );

    layout->removeAt(0);
    QVERIFY( layout->count() == 1 );

    layout->removeAt(-55);
    layout->removeAt(55);
    QVERIFY( layout->count() == 1 );

    layout->removeAt(0);
    QVERIFY( layout->count() == 0 );

    delete layout;
    delete widget1;
    delete widget2;
}

void tst_QGraphicsAnchorLayout1::testRemoveItem()
{
    TheAnchorLayout *layout = new TheAnchorLayout();
    QCOMPARE( layout->count(), 0 );

    TestWidget *widget1 = new TestWidget();
    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 2);
    QCOMPARE( layout->count(), 1 );

    TestWidget *widget2 = new TestWidget();
    layout->setAnchor(layout, Qt::AnchorLeft, widget2, Qt::AnchorLeft, 0.1);
    QCOMPARE( layout->count(), 2 );

    layout->removeItem(0);
    QCOMPARE( layout->count(), 2 );

    layout->removeItem(widget1);
    QCOMPARE( layout->count(), 1 );
    QCOMPARE( layout->indexOf(widget1), -1 );
    QCOMPARE( layout->indexOf(widget2), 0 );

    layout->removeItem(widget1);
    QCOMPARE( layout->count(), 1 );

    layout->removeItem(widget2);
    QVERIFY( layout->count() == 0 );

    delete layout;
    delete widget1;
    delete widget2;
}

void tst_QGraphicsAnchorLayout1::testItemAt()
{
    QGraphicsWidget *widget = new QGraphicsWidget;

    TheAnchorLayout *layout = new TheAnchorLayout();

    TestWidget *widget1 = new TestWidget();
    TestWidget *widget2 = new TestWidget();
    TestWidget *widget3 = new TestWidget();
    TestWidget *widget4 = new TestWidget();

    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorLeft, widget2, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorLeft, widget3, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorLeft, widget4, Qt::AnchorLeft, 0.1);

    QVERIFY( layout->itemAt(0) == widget1 );

    layout->removeAt(0);

    QVERIFY( layout->itemAt(0) == widget2 );

    delete widget1;

    widget->setLayout(layout);
    delete widget;
}

void tst_QGraphicsAnchorLayout1::testIndexOf()
{
    QGraphicsWidget *widget = new QGraphicsWidget;

    TheAnchorLayout *layout = new TheAnchorLayout();

    TestWidget *widget1 = new TestWidget();
    TestWidget *widget2 = new TestWidget();
    TestWidget *widget3 = new TestWidget();
    TestWidget *widget4 = new TestWidget();

    QCOMPARE( layout->indexOf(widget1), -1 );

    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorLeft, widget2, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorLeft, widget3, Qt::AnchorLeft, 0.1);

    QCOMPARE( layout->indexOf(widget4), -1 );
    layout->setAnchor(layout, Qt::AnchorLeft, widget4, Qt::AnchorLeft, 0.1);

    QCOMPARE( layout->count(), 4 );
    for (int i = 0; i < layout->count(); ++i) {
        QCOMPARE(layout->indexOf(layout->itemAt(i)), i);
    }

    QCOMPARE( layout->indexOf(0), -1 );
    widget->setLayout(layout);
    delete widget;
}

void tst_QGraphicsAnchorLayout1::testAddAndRemoveAnchor()
{
    QGraphicsWidget *widget = new QGraphicsWidget;

    TheAnchorLayout *layout = new TheAnchorLayout();

    TestWidget *widget1 = new TestWidget();
    TestWidget *widget2 = new TestWidget();
    TestWidget *widget3 = new TestWidget();
    TestWidget *widget4 = new TestWidget();
    TestWidget *widget5 = new TestWidget();

    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorLeft, widget2, Qt::AnchorLeft, 0.5);
    layout->setAnchor(layout, Qt::AnchorLeft, widget3, Qt::AnchorLeft, 10);
    layout->setAnchor(layout, Qt::AnchorLeft, widget4, Qt::AnchorLeft, 0.1);
    QCOMPARE( layout->count(), 4 );

    // test setting invalid anchors
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor NULL items");
    layout->setAnchor(0, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 1);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor NULL items");
    layout->setAnchor(layout, Qt::AnchorLeft, 0, Qt::AnchorLeft, 1);
    QCOMPARE( layout->count(), 4 );

    // test removing invalid anchors
    layout->removeAnchor(widget4, Qt::AnchorRight, widget1, Qt::AnchorRight);

    // anchor one horizontal edge with vertical edge. it should not add this widget as a child
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor edges of different orientations");
    layout->setAnchor(layout, Qt::AnchorLeft, widget5, Qt::AnchorTop, 10);
    QCOMPARE( layout->count(), 4 );

    // anchor two edges of a widget (to define width / height)
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
    layout->setAnchor(widget5, Qt::AnchorLeft, widget5, Qt::AnchorRight, 10);
    // QCOMPARE( layout->count(), 5 );
    QCOMPARE( layout->count(), 4 );

    // anchor yet new widget properly
    layout->setAnchor(layout, Qt::AnchorRight, widget5, Qt::AnchorRight, 20 );
    QCOMPARE( layout->count(), 5 );

    // remove anchor for widget1. widget1 should be removed from layout since the
    // last anchor was removed.
    layout->removeAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft);

    QCOMPARE( layout->count(), 4 );
    QVERIFY( !widget1->parentLayoutItem() );

    // test that item is not removed from layout if other anchors remain set
    layout->setAnchor(widget2, Qt::AnchorLeft, widget3, Qt::AnchorRight, 10);
    layout->removeAnchor(layout, Qt::AnchorLeft, widget2, Qt::AnchorLeft);
    QCOMPARE( layout->count(), 4 );

    // remove all the anchors
    layout->removeAnchor(widget2, Qt::AnchorLeft, widget3, Qt::AnchorRight);
    layout->removeAnchor(layout, Qt::AnchorLeft, widget3, Qt::AnchorLeft);
    layout->removeAnchor(layout, Qt::AnchorLeft, widget4, Qt::AnchorLeft);
    layout->removeAnchor(widget5, Qt::AnchorLeft, widget5, Qt::AnchorRight);
    layout->removeAnchor(layout, Qt::AnchorRight, widget5, Qt::AnchorRight);

    QCOMPARE( layout->count(), 0 );

    // set one anchor "another way round" to get full coverage for "removeAnchor"
    layout->setAnchor(widget1, Qt::AnchorLeft, layout, Qt::AnchorLeft, 0.1);
    layout->removeAnchor(widget1, Qt::AnchorLeft, layout, Qt::AnchorLeft);

    QCOMPARE( layout->count(), 0 );

    delete widget1;
    delete widget2;
    delete widget3;
    delete widget4;
    delete widget5;

    widget->setLayout(layout);
    delete widget;
}

void tst_QGraphicsAnchorLayout1::testIsValid()
{
    // Empty, valid
    {
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();
    widget->setLayout(layout);
    widget->setGeometry(QRectF(0,0,100,100));

    QCOMPARE(layout->isValid(), true);
    delete widget;
    }

    // One widget, valid
    {
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();

    TestWidget *widget1 = new TestWidget();

    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorTop, widget1, Qt::AnchorTop, 0.1);
    layout->setAnchor(widget1, Qt::AnchorRight, layout, Qt::AnchorRight, 0.1);
    layout->setAnchor(widget1, Qt::AnchorBottom, layout, Qt::AnchorBottom, 0.1);

    widget->setLayout(layout);

    widget->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(layout->isValid(), true);
    delete widget;
    }

    // Overconstrained one widget, invalid
    // ### Our understanding is that this case is valid. What happens though,
    //     is that the layout minimum and maximum vertical size hints become
    //     the same, 10.1. That means its height is fixed.
    //     What will "fail" then is the "setGeometry(0, 0, 100, 100)" call,
    //     after which the layout geometry will be (0, 0, 100, 10.1).

    {
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();

    TestWidget *widget1 = new TestWidget();

    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorTop, widget1, Qt::AnchorTop, 0.1);
    layout->setAnchor(widget1, Qt::AnchorRight, layout, Qt::AnchorRight, 0.1);
    layout->setAnchor(widget1, Qt::AnchorBottom, layout, Qt::AnchorBottom, 0.1);

    layout->setAnchor(widget1, Qt::AnchorTop, layout, Qt::AnchorBottom, 10);

    widget->setLayout(layout);

    widget->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(layout->isValid(), true);
    delete widget;
    }

    // Underconstrained two widgets, valid
    {
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();

    TestWidget *widget1 = new TestWidget();
    TestWidget *widget2 = new TestWidget();

    // Vertically the layout has floating items. Therefore, we have a conflict
    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 0.1);
    layout->setAnchor(layout, Qt::AnchorRight, widget1, Qt::AnchorRight, -0.1);

    // Horizontally the layout has floating items. Therefore, we have a conflict
    layout->setAnchor(layout, Qt::AnchorTop, widget2, Qt::AnchorTop, 0.1);
    layout->setAnchor(layout, Qt::AnchorBottom, widget2, Qt::AnchorBottom, -0.1);

    widget->setLayout(layout);

    widget->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(layout->isValid(), false);
    delete widget;
    }
}

void tst_QGraphicsAnchorLayout1::testSpecialCases()
{
    // One widget, setLayout before defining layouts
    {
    if (QLibraryInfo::isDebugBuild())
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsLayout::addChildLayoutItem: QGraphicsWidget \"\""
                                           " in wrong parent; moved to correct parent");
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();
    widget->setLayout(layout);

    TestWidget *widget1 = new TestWidget();

    layout->setAnchor(layout, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 1);
    layout->setAnchor(layout, Qt::AnchorTop, widget1, Qt::AnchorTop, 1);
    layout->setAnchor(widget1, Qt::AnchorRight, layout, Qt::AnchorRight, 1);
    layout->setAnchor(widget1, Qt::AnchorBottom, layout, Qt::AnchorBottom, 1);
    widget->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(widget1->geometry(), QRectF(1,1,98,98));
    delete widget1;
    delete widget;
    }

    // One widget, layout inside layout, layout inside layout inside layout
    {
    if (QLibraryInfo::isDebugBuild())
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsLayout::addChildLayoutItem: QGraphicsWidget \"\""
                                           " in wrong parent; moved to correct parent");
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();
    widget->setLayout(layout);

    TheAnchorLayout *layout1 = new TheAnchorLayout();
    TestWidget *widget1 = new TestWidget();
    layout1->setAnchor(layout1, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 1);
    layout1->setAnchor(layout1, Qt::AnchorTop, widget1, Qt::AnchorTop, 1);
    layout1->setAnchor(widget1, Qt::AnchorRight, layout1, Qt::AnchorRight, 1);
    layout1->setAnchor(widget1, Qt::AnchorBottom, layout1, Qt::AnchorBottom, 1);

    TheAnchorLayout *layout2 = new TheAnchorLayout();
    TestWidget *widget2 = new TestWidget();
    layout2->setAnchor(layout2, Qt::AnchorLeft, widget2, Qt::AnchorLeft, 1);
    layout2->setAnchor(layout2, Qt::AnchorTop, widget2, Qt::AnchorTop, 1);
    layout2->setAnchor(widget2, Qt::AnchorRight, layout2, Qt::AnchorRight, 1);
    layout2->setAnchor(widget2, Qt::AnchorBottom, layout2, Qt::AnchorBottom, 1);

    layout1->setAnchor(layout1, Qt::AnchorLeft, layout2, Qt::AnchorLeft, 1);
    layout1->setAnchor(layout1, Qt::AnchorTop, layout2, Qt::AnchorTop, 1);
    layout1->setAnchor(layout2, Qt::AnchorRight, layout1, Qt::AnchorRight, 1);
    layout1->setAnchor(layout2, Qt::AnchorBottom, layout1, Qt::AnchorBottom, 1);

    layout->setAnchor(layout, Qt::AnchorLeft, layout1, Qt::AnchorLeft, 1);
    layout->setAnchor(layout, Qt::AnchorTop, layout1, Qt::AnchorTop, 1);
    layout->setAnchor(layout1, Qt::AnchorRight, layout, Qt::AnchorRight, 1);
    layout->setAnchor(layout1, Qt::AnchorBottom, layout, Qt::AnchorBottom, 1);

    // remove and add again to improve test coverage.
    layout->removeItem(layout1);

    layout->setAnchor(layout, Qt::AnchorLeft, layout1, Qt::AnchorLeft, 1);
    layout->setAnchor(layout, Qt::AnchorTop, layout1, Qt::AnchorTop, 1);
    layout->setAnchor(layout1, Qt::AnchorRight, layout, Qt::AnchorRight, 1);
    layout->setAnchor(layout1, Qt::AnchorBottom, layout, Qt::AnchorBottom, 1);

    widget->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(widget1->geometry(), QRectF(2,2,96,96));
    QCOMPARE(widget2->geometry(), QRectF(3,3,94,94));
    delete widget;
    }

    // One widget, layout inside layout, setLayout after layout definition
    {
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();

    TheAnchorLayout *layout1 = new TheAnchorLayout();

    TestWidget *widget1 = new TestWidget();
    layout1->setAnchor(layout1, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 1);
    layout1->setAnchor(layout1, Qt::AnchorTop, widget1, Qt::AnchorTop, 1);
    layout1->setAnchor(widget1, Qt::AnchorRight, layout1, Qt::AnchorRight, 1);
    layout1->setAnchor(widget1, Qt::AnchorBottom, layout1, Qt::AnchorBottom, 1);

    layout->setAnchor(layout, Qt::AnchorLeft, layout1, Qt::AnchorLeft, 1);
    layout->setAnchor(layout, Qt::AnchorTop, layout1, Qt::AnchorTop, 1);
    layout->setAnchor(layout1, Qt::AnchorRight, layout, Qt::AnchorRight, 1);
    layout->setAnchor(layout1, Qt::AnchorBottom, layout, Qt::AnchorBottom, 1);

    widget->setLayout(layout);
    widget->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(widget1->geometry(), QRectF(2,2,96,96));
    delete widget;
    }

    // One widget, layout inside layout, setLayout after layout definition, widget transferred from
    // one layout to another
    {
    QGraphicsWidget *widget = new QGraphicsWidget;
    TheAnchorLayout *layout = new TheAnchorLayout();
    widget->setLayout(layout);

    TheAnchorLayout *layout1 = new TheAnchorLayout();
    TestWidget *widget1 = new TestWidget();

    // Additional layout + widget to improve coverage.
    TheAnchorLayout *layout0 = new TheAnchorLayout();
    TestWidget *widget0 = new TestWidget();

    // widget0 to layout0
    layout0->setAnchor(layout0, Qt::AnchorLeft, widget0, Qt::AnchorLeft, 1);
    layout0->setAnchor(layout0, Qt::AnchorTop, widget0, Qt::AnchorTop, 1);
    layout0->setAnchor(widget0, Qt::AnchorRight, layout0, Qt::AnchorRight, 1);
    layout0->setAnchor(widget0, Qt::AnchorBottom, layout0, Qt::AnchorBottom, 1);

    // layout0 to layout
    layout->setAnchor(layout, Qt::AnchorLeft, layout0, Qt::AnchorLeft, 1);
    layout->setAnchor(layout, Qt::AnchorTop, layout0, Qt::AnchorTop, 1);
    layout->setAnchor(layout0, Qt::AnchorRight, layout, Qt::AnchorRight, 50);
    layout->setAnchor(layout0, Qt::AnchorBottom, layout, Qt::AnchorBottom, 1);

    // widget1 to layout1
    layout1->setAnchor(layout1, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 1);
    layout1->setAnchor(layout1, Qt::AnchorTop, widget1, Qt::AnchorTop, 1);
    layout1->setAnchor(widget1, Qt::AnchorRight, layout1, Qt::AnchorRight, 1);
    layout1->setAnchor(widget1, Qt::AnchorBottom, layout1, Qt::AnchorBottom, 1);

    // layout1 to layout
    layout->setAnchor(layout, Qt::AnchorLeft, layout1, Qt::AnchorLeft, 1);
    layout->setAnchor(layout, Qt::AnchorTop, layout1, Qt::AnchorTop, 1);
    layout->setAnchor(layout1, Qt::AnchorRight, layout, Qt::AnchorRight, 50);
    layout->setAnchor(layout1, Qt::AnchorBottom, layout, Qt::AnchorBottom, 1);

    TheAnchorLayout *layout2 = new TheAnchorLayout();

    // layout2 to layout
    layout->setAnchor(layout, Qt::AnchorLeft, layout2, Qt::AnchorLeft, 50);
    layout->setAnchor(layout, Qt::AnchorTop, layout2, Qt::AnchorTop, 1);
    layout->setAnchor(layout2, Qt::AnchorRight, layout, Qt::AnchorRight, 1);
    layout->setAnchor(layout2, Qt::AnchorBottom, layout, Qt::AnchorBottom, 1);

    // transfer widget1 to layout2
    layout2->setAnchor(layout2, Qt::AnchorLeft, widget1, Qt::AnchorLeft, 1);
    layout2->setAnchor(layout2, Qt::AnchorTop, widget1, Qt::AnchorTop, 1);
    layout2->setAnchor(widget1, Qt::AnchorRight, layout2, Qt::AnchorRight, 1);
    layout2->setAnchor(widget1, Qt::AnchorBottom, layout2, Qt::AnchorBottom, 1);

    widget->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(widget1->geometry(), QRectF(51,2,47,96));
    delete widget;
    }

    // One widget, set first to one layout then to another. Child reparented.
    // In addition widget as a direct child of another widget. Child reparented.
    {
    QGraphicsWidget *widget1 = new QGraphicsWidget;
    TheAnchorLayout *layout1 = new TheAnchorLayout();
    widget1->setLayout(layout1);

    TestWidget *childWidget = new TestWidget();

    // childWidget to layout1
    layout1->setAnchor(layout1, Qt::AnchorLeft, childWidget, Qt::AnchorLeft, 1);
    layout1->setAnchor(layout1, Qt::AnchorTop, childWidget, Qt::AnchorTop, 1);
    layout1->setAnchor(childWidget, Qt::AnchorRight, layout1, Qt::AnchorRight, 1);
    layout1->setAnchor(childWidget, Qt::AnchorBottom, layout1, Qt::AnchorBottom, 1);

    widget1->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(childWidget->geometry(), QRectF(1,1,98,98));
    QCOMPARE(childWidget->parentLayoutItem(), layout1);
    QGraphicsWidget *widget2 = new QGraphicsWidget;
    TheAnchorLayout *layout2 = new TheAnchorLayout();
    widget2->setLayout(layout2);

    // childWidget to layout2
    layout2->setAnchor(layout2, Qt::AnchorLeft, childWidget, Qt::AnchorLeft, 1);
    layout2->setAnchor(layout2, Qt::AnchorTop, childWidget, Qt::AnchorTop, 1);
    layout2->setAnchor(childWidget, Qt::AnchorRight, layout2, Qt::AnchorRight, 1);
    layout2->setAnchor(childWidget, Qt::AnchorBottom, layout2, Qt::AnchorBottom, 1);

    QGraphicsWidget *widget3 = new QGraphicsWidget;
    QGraphicsWidget *widget4 = new QGraphicsWidget;
    // widget4 is a direct child of widget3 (i.e. not in any layout)
    widget4->setParentItem(widget3);

    // widget4 to layout2
    layout2->setAnchor(layout2, Qt::AnchorLeft, widget4, Qt::AnchorLeft, 1);
    layout2->setAnchor(layout2, Qt::AnchorTop, widget4, Qt::AnchorTop, 1);
    layout2->setAnchor(widget4, Qt::AnchorRight, layout2, Qt::AnchorRight, 1);
    layout2->setAnchor(widget4, Qt::AnchorBottom, layout2, Qt::AnchorBottom, 1);

    widget2->setGeometry(QRectF(0,0,100,100));
    QCOMPARE(childWidget->geometry(), QRectF(1,1,98,98));
    QCOMPARE(childWidget->parentLayoutItem(), layout2);
    QCOMPARE(widget4->geometry(), QRectF(1,1,98,98));
    QCOMPARE(widget4->parentLayoutItem(), layout2);
    QCOMPARE(widget4->parentItem(), widget2);

    delete widget4;
    delete widget3;
    delete widget1;
    delete childWidget;
    delete widget2;
    }
}

void tst_QGraphicsAnchorLayout1::testBasicLayout_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<BasicLayoutTestDataList>("data");
    QTest::addColumn<BasicLayoutTestResultList>("result");

    typedef BasicLayoutTestData BasicData;
    typedef BasicLayoutTestResult BasicResult;

    // One widget, basic
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 20)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 30)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 40)
            ;

        theResult
            << BasicResult(0, QRectF(20, 10, 150, 50) )
            ;

        QTest::newRow("One, simple") << QSizeF(200, 100) << theData << theResult;
    }

    // One widget, duplicates
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 20)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 30)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 40)

            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 0)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorRight, 0)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 0)
            ;

        theResult
            << BasicResult(0, QRectF(0, 0, 200, 100) )
            ;

        QTest::newRow("One, duplicates") << QSizeF(200, 100) << theData << theResult;
    }

    // One widget, mixed
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorBottom, 80)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorRight, 150)
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorRight, 150)
            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorBottom, 80)
            ;

        theResult
            << BasicResult(0, QRectF(50, 20, 100, 60) )
            ;

        QTest::newRow("One, mixed") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets (same layout), different ordering
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)

            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 10)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 180, 80) )
            << BasicResult(1, QRectF(10, 10, 180, 80) )
            ;

        QTest::newRow("Two, orderings") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, duplicate anchors
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 30)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 20)

            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 10)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(1, Qt::AnchorTop, -1, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorRight, 1, Qt::AnchorRight, 0)
            ;

        theResult
            << BasicResult(0, QRectF(30, 10, 160, 70) )
            << BasicResult(1, QRectF(10, 0, 190, 90) )
            ;

        QTest::newRow("Two, duplicates") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, mixed
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorBottom, 90)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorRight, 190)
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorRight, 190)
            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorBottom, 90)

            << BasicData(1, Qt::AnchorTop, -1, Qt::AnchorBottom, 20)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 10)
            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorRight, 20)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 180, 80) )
            << BasicResult(1, QRectF(10, 80, 10, 10) )
            ;

        QTest::newRow("Two, mixed") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 1 horizontal connection, first completely defined
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 180)

            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 10)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 20)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 10, 80) )
            << BasicResult(1, QRectF(30, 10, 160, 70) )
            ;

        QTest::newRow("Two, 1h connected") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 2 horizontal connections, first completely defined
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 180)

            // ### QGAL is not sensible to the argument order in this case
            //     To achieve the desired result we must explicitly set a negative
            //     spacing.
            // << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorRight, 100)
            << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorRight, -100)

            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 30)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 20)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 10, 80) )
            << BasicResult(1, QRectF(50, 10, 60, 70) )
            ;

        QTest::newRow("Two, 2h connected") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 1 vertical connection, first completely defined
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 180)

            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 30)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(0, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 20)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 10, 80) )
            << BasicResult(1, QRectF(30, 20, 160, 60) )
            ;

        QTest::newRow("Two, 1v connected") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 2 vertical connections, first completely defined
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 180)

            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 30)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(0, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorBottom, 0, Qt::AnchorBottom, 20)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 10, 80) )
            << BasicResult(1, QRectF(30, 20, 160, 50) )
            ;

        QTest::newRow("Two, 2v connected") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 1 horizontal and 1 vertical connection, first completely defined
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 180)

            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 80)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorRight, 100)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorBottom, 0, Qt::AnchorBottom, 10)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 10, 80) )
            << BasicResult(1, QRectF(80, 10, 40, 70) )
            ;

        QTest::newRow("Two, 1h+1v connected") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 2 horizontal and 2 vertical connections, first completely defined
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 180)

            << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorLeft, 80)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorRight, 100)
            << BasicData(0, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorBottom, 0, Qt::AnchorBottom, 10)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 10, 80) )
            << BasicResult(1, QRectF(90, 20, 30, 60) )
            ;

        QTest::newRow("Two, 2h+2v connected") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 2 horizontal and 2 vertical connections, dependent on each other.
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorRight, 150)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, 1, Qt::AnchorBottom, 10)

            << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorLeft, 90)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(0, Qt::AnchorTop, 1, Qt::AnchorTop, 10)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 20)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 30, 60) )
            << BasicResult(1, QRectF(100, 20, 90, 60) )
            ;

        QTest::newRow("Two, 2h+2v connected2") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, connected, overlapping
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            // << BasicData(1, Qt::AnchorLeft, 0, Qt::AnchorRight, 30)
            // ### QGAL has different semantics and assumes right edges are always
            //     to the left of left edges. Thus we need the minus sign here.
            << BasicData(1, Qt::AnchorLeft, 0, Qt::AnchorRight, -30)
            << BasicData(0, Qt::AnchorBottom, 1, Qt::AnchorBottom, 40)

            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 40)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 20)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
            ;

        theResult
            << BasicResult(0, QRectF(10, 10, 60, 40) )
            << BasicResult(1, QRectF(40, 20, 150, 70) )
            ;

        QTest::newRow("Two, connected overlapping") << QSizeF(200, 100) << theData << theResult;
    }
}

void tst_QGraphicsAnchorLayout1::testNegativeSpacing_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<BasicLayoutTestDataList>("data");
    QTest::addColumn<BasicLayoutTestResultList>("result");

    typedef BasicLayoutTestData BasicData;
    typedef BasicLayoutTestResult BasicResult;

    // One widget, negative spacing
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        /// ### QGAL assumes items are always inside the layout.
        //      In this case, the negative spacing would make the item
        //      grow beyond the layout edges, which is OK, but gives a
        //      different result.
        //      Changing the direction of anchors (-1 to 0 or vice-versa)
        //      has no effect in this case.

        theData
            // << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, -10)
            // << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, -20)
            // << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorRight, -30)
            // << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorBottom, -40)

            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, -10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, -20)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, -30)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, -40)

            ;

        theResult
            // << BasicResult(0, QRectF(20, 10, 150, 50) )
            << BasicResult(0, QRectF(-20, -10, 250, 150) )
            ;

        QTest::newRow("One, simple (n)") << QSizeF(200, 100) << theData << theResult;
    }

    // One widget, duplicates, negative spacing
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, -20)
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, -20)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorRight, -30)
            << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorBottom, -40)

            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, -10)
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, -10)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorRight, -10)
            << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorBottom, -10)
            ;

        theResult
            // ### Same as above...
            // << BasicResult(0, QRectF(10, 10, 180, 80) )
            << BasicResult(0, QRectF(-10, -10, 220, 120) )
            ;

        QTest::newRow("One, duplicates (n)") << QSizeF(200, 100) << theData << theResult;
    }

    // One widget, mixed, negative spacing
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            // ### All anchors of negative spacing between the layout and an
            //     item are handled as to make sure the item is _outside_ the
            //     layout.
            //     To keep it inside, one _must_ use positive spacings.
            // << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorTop, -80)
            // << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorLeft, -150)
            // << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorLeft, -150)
            // << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorTop, -80)

            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorTop, 80)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorLeft, 150)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorLeft, 150)
            << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorTop, 80)
            ;

        theResult
            << BasicResult(0, QRectF(50, 20, 100, 60) )
            ;

        QTest::newRow("One, mixed (n)") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 1 horizontal connection, first completely defined, negative spacing
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            // << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, -10)
            // << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorBottom, -10)
            // << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, -10)
            // << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorRight, -180)

            // << BasicData(1, Qt::AnchorLeft, 0, Qt::AnchorRight, -10)
            // << BasicData(-1, Qt::AnchorRight, 1, Qt::AnchorRight, -10)
            // << BasicData(1, Qt::AnchorTop, -1, Qt::AnchorTop, -10)
            // << BasicData(-1, Qt::AnchorBottom, 1, Qt::AnchorBottom, -20)

            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, -10)
            << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorBottom, -10)
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, -10)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorRight, 180)

            << BasicData(1, Qt::AnchorLeft, 0, Qt::AnchorRight, -10)
            << BasicData(-1, Qt::AnchorRight, 1, Qt::AnchorRight, -10)
            << BasicData(1, Qt::AnchorTop, -1, Qt::AnchorTop, -10)
            << BasicData(-1, Qt::AnchorBottom, 1, Qt::AnchorBottom, -20)

            ;

        theResult
            // << BasicResult(0, QRectF(10, 10, 10, 80) )
            // << BasicResult(1, QRectF(30, 10, 160, 70) )

            << BasicResult(0, QRectF(-10, -10, 30, 120) )
            << BasicResult(1, QRectF(10, -10, 200, 130) )
            ;

        QTest::newRow("Two, 1h connected (n)") << QSizeF(200, 100) << theData << theResult;
    }

    // Basic case - two widgets, 2 horizontal and 2 vertical connections, dependent on each other, negative spacing
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, -10)
            << BasicData(1, Qt::AnchorRight, 0, Qt::AnchorRight, -150)
            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, -10)
            << BasicData(1, Qt::AnchorBottom, 0, Qt::AnchorBottom, -10)

            << BasicData(1, Qt::AnchorLeft, 0, Qt::AnchorLeft, -90)
            << BasicData(-1, Qt::AnchorRight, 1, Qt::AnchorRight, -10)
            << BasicData(1, Qt::AnchorTop, 0, Qt::AnchorTop, -10)
            << BasicData(-1, Qt::AnchorBottom, 1, Qt::AnchorBottom, -20)
            ;

        theResult
            // << BasicResult(0, QRectF(10, 10, 30, 60) )
            // << BasicResult(1, QRectF(100, 20, 90, 60) )
            << BasicResult(0, QRectF(-10, -10, 70, 120) )
            << BasicResult(1, QRectF(80, 0, 130, 120) )
            ;

        QTest::newRow("Two, 2h+2v connected2 (n)") << QSizeF(200, 100) << theData << theResult;
    }
}

void tst_QGraphicsAnchorLayout1::testMixedSpacing_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<BasicLayoutTestDataList>("data");
    QTest::addColumn<BasicLayoutTestResultList>("result");

    typedef BasicLayoutTestData BasicData;
    typedef BasicLayoutTestResult BasicResult;

    // Two widgets, partial overlapping
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, -50)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 50)
            << BasicData(1, Qt::AnchorRight, 0, Qt::AnchorRight, 15)

            // << BasicData(1, Qt::AnchorTop, 0, Qt::AnchorBottom, 5)
            << BasicData(1, Qt::AnchorTop, 0, Qt::AnchorBottom, -5)
            << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorLeft, -10)
            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 20)
            << BasicData(-1, Qt::AnchorBottom, 1, Qt::AnchorBottom, -5)
            ;

        theResult
            // << BasicResult(0, QRectF(50, 10, 45, 40) )
            // << BasicResult(1, QRectF(40, 45, 40, 50) )
            << BasicResult(0, QRectF(-50, 10, 145, 40) )
            << BasicResult(1, QRectF(-60, 45, 140, 60) )
            ;

        QTest::newRow("Two, partial overlap") << QSizeF(100, 100) << theData << theResult;
    }

    // Two widgets, complete overlapping
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 5)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorRight, 0)
            << BasicData(0, Qt::AnchorTop, 1, Qt::AnchorTop, 0)
            << BasicData(1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 25)

            << BasicData(1, Qt::AnchorBottom, 0, Qt::AnchorBottom, 0)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 5)
            << BasicData(1, Qt::AnchorLeft, -1, Qt::AnchorRight, 50)
            << BasicData(-1, Qt::AnchorRight, 1, Qt::AnchorRight, -10)
            ;

        theResult
            << BasicResult(0, QRectF(65, 5, 35, 35) )
            << BasicResult(1, QRectF(40, 5, 60, 35) )
            ;

        QTest::newRow("Two, complete overlap") << QSizeF(90, 45) << theData << theResult;
    }

    // Five widgets, v shaped, edges shared
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            // edges shared
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 0)
            << BasicData(1, Qt::AnchorRight, 2, Qt::AnchorLeft, 0)
            << BasicData(2, Qt::AnchorRight, 3, Qt::AnchorLeft, 0)
            << BasicData(3, Qt::AnchorRight, 4, Qt::AnchorLeft, 0)
            << BasicData(1, Qt::AnchorBottom, 2, Qt::AnchorTop, 0)
            << BasicData(0, Qt::AnchorBottom, 1, Qt::AnchorTop, 0)
            << BasicData(3, Qt::AnchorBottom, 2, Qt::AnchorTop, 0)
            << BasicData(4, Qt::AnchorBottom, 3, Qt::AnchorTop, 0)
            << BasicData(0, Qt::AnchorBottom, 4, Qt::AnchorBottom, 0)
            << BasicData(1, Qt::AnchorBottom, 3, Qt::AnchorBottom, 0)
            << BasicData(0, Qt::AnchorTop, 4, Qt::AnchorTop, 0)
            << BasicData(1, Qt::AnchorTop, 3, Qt::AnchorTop, 0)

            // margins
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 5)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 5)
            << BasicData(2, Qt::AnchorBottom, -1, Qt::AnchorBottom, 5)
            // << BasicData(-1, Qt::AnchorRight, 4, Qt::AnchorRight, -5)
            << BasicData(-1, Qt::AnchorRight, 4, Qt::AnchorRight, 5)

            // additional details for exact size determination easily
            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 25)
            << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorRight, 50)
            // << BasicData(-1, Qt::AnchorRight, 3, Qt::AnchorRight, -25)
            // << BasicData(-1, Qt::AnchorRight, 2, Qt::AnchorRight, -50)
            << BasicData(-1, Qt::AnchorRight, 3, Qt::AnchorRight, 25)
            << BasicData(-1, Qt::AnchorRight, 2, Qt::AnchorRight, 50)
            << BasicData(-1, Qt::AnchorTop, 3, Qt::AnchorBottom, 50)
            // << BasicData(-1, Qt::AnchorBottom, 3, Qt::AnchorTop, -50)
            << BasicData(-1, Qt::AnchorBottom, 3, Qt::AnchorTop, 50)

            ;

        theResult
            << BasicResult(0, QRectF(5,5,20,20))
            << BasicResult(1, QRectF(25,25,25,25))
            << BasicResult(2, QRectF(50,50,25,20))
            << BasicResult(3, QRectF(75,25,25,25))
            << BasicResult(4, QRectF(100,5,20,20))
            ;

        QTest::newRow("Five, V shape") << QSizeF(125, 75) << theData << theResult;
    }

    // ### The behavior is different in QGraphicsAnchorLayout. What happens here is
    //     that when the above anchors are set, the layout size hints are changed.
    //     In the example, the minimum item width is 5, thus the minimum layout width
    //     becomes 105 (50 + 5 + 50). Once that size hint is set, trying to set
    //     the widget size to (10, 10) is not possible because
    //     QGraphicsWidget::setGeometry() will enforce the minimum is respected.
    if (0)
    // One widget, unsolvable
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
                << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 50)
                << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 50)
                ;
        theResult
                << BasicResult(0, QRectF(0,0,0,0))
                ;

        QTest::newRow("One widget, unsolvable") << QSizeF(10, 10) << theData << theResult;
    }

    // Two widgets, one has fixed size
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
                << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 50)
                << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 50)
            // not supported, use sizePolicy instead
            // << BasicData(0, Qt::AnchorLeft, 0, Qt::AnchorRight, 50)

                << BasicData(-1, Qt::AnchorLeft, 1, Qt::AnchorLeft, 50)
                << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 50)
                ;
        theResult
                << BasicResult(0, QRectF(50,0,50,50))
                << BasicResult(1, QRectF(50,0,50,50))
                ;

        QTest::newRow("Two widgets, one has fixed size") << QSizeF(150, 150) << theData << theResult;
    }
}

void tst_QGraphicsAnchorLayout1::testMulti_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<BasicLayoutTestDataList>("data");
    QTest::addColumn<BasicLayoutTestResultList>("result");

    typedef BasicLayoutTestData BasicData;
    typedef BasicLayoutTestResult BasicResult;

    // Multiple widgets, all overllapping
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        const int n = 30;
        for ( int i = 0 ; i < n; i++ ) {
            theData
                << BasicData(-1, Qt::AnchorTop, i, Qt::AnchorTop, 20)
                << BasicData(-1, Qt::AnchorLeft, i, Qt::AnchorLeft, 10)
                // << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, -40)
                // << BasicData(-1, Qt::AnchorRight, i, Qt::AnchorRight, -30);
                << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, 40)
                << BasicData(-1, Qt::AnchorRight, i, Qt::AnchorRight, 30);

            theResult
                << BasicResult(i, QRectF(10, 20, 160, 40) );
        }


        QTest::newRow("Overlapping multi") << QSizeF(200, 100) << theData << theResult;
    }

    // Multiple widgets, linear order
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        const qreal height = 1000.f;
        const qreal width = 2000.f;

        const int n = 30;

        const qreal verticalStep = height/qreal(n+2);
        const qreal horizontalStep = width/qreal(n+2);

        for ( int i = 0 ; i < n; i++ ) {

            if ( i == 0 ) {
                // First item
                theData
                    << BasicData(-1, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i+1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(i+1, Qt::AnchorRight, i, Qt::AnchorRight, -horizontalStep);

            } else if ( i == n-1 ) {
                // Last item
                theData
                    << BasicData(i-1, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, verticalStep)
                    << BasicData(-1, Qt::AnchorRight, i, Qt::AnchorRight, horizontalStep);
                    // << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    // << BasicData(-1, Qt::AnchorRight, i, Qt::AnchorRight, -horizontalStep);

            } else {
                // items in the middle
                theData
                    << BasicData(i-1, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i+1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(i+1, Qt::AnchorRight, i, Qt::AnchorRight, -horizontalStep);
                    // << BasicData(i+1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    // << BasicData(i+1, Qt::AnchorRight, i, Qt::AnchorRight, -horizontalStep);

            }

            theResult
                << BasicResult(i, QRectF((i+1)*horizontalStep, (i+1)*verticalStep, horizontalStep, verticalStep) );
        }


        if (sizeof(qreal) == 4) {
            qDebug("Linear multi: Skipping! (qreal has too little precision, result will be wrong)");
        } else {
            QTest::newRow("Linear multi") << QSizeF(width, height) << theData << theResult;
        }
    }

    // Multiple widgets, V shape
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        const qreal height = 100.f;
        const qreal width = 200.f;

        const int n = 31; // odd number please (3,5,7... )

        const qreal verticalStep = height/(2.f+(n+1)/2.f);
        const qreal horizontalStep = width/(n+2.f);

        for ( int i = 0 ; i < n; i++ ) {

            if ( i == 0 ) {
                // First item
                theData
                    << BasicData(-1, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i+1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(i, Qt::AnchorRight, i+1, Qt::AnchorRight, horizontalStep);

            } else if ( i == n-1 ) {
                // Last item
                theData
                    << BasicData(i-1, Qt::AnchorTop, i, Qt::AnchorTop, -verticalStep)
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i-1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(i, Qt::AnchorRight, -1, Qt::AnchorRight, horizontalStep);
            } else if ( i == ((n-1)/2) ) {
                // midway
                theData
                    << BasicData(i-1, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    // << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, verticalStep)
                    << BasicData(i, Qt::AnchorRight, i+1, Qt::AnchorRight, horizontalStep);
            } else if ( i < ((n-1)/2) ) {
                // before midway - going down
                theData
                    << BasicData(i-1, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i+1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(i, Qt::AnchorRight, i+1, Qt::AnchorRight, horizontalStep);

            } else {
                // after midway - going up
                theData
                    << BasicData(i-1, Qt::AnchorTop, i, Qt::AnchorTop, -verticalStep)
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i-1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(i, Qt::AnchorRight, i+1, Qt::AnchorRight, horizontalStep);

            }

            if ( i <= ((n-1)/2) ) {
                // until midway
                theResult
                    << BasicResult(i, QRectF((i+1)*horizontalStep, (i+1)*verticalStep, horizontalStep, verticalStep) );
            } else {
                // after midway
                theResult
                    << BasicResult(i, QRectF((i+1)*horizontalStep, (n-i)*verticalStep, horizontalStep, verticalStep) );
            }

        }
        if (sizeof(qreal) == 4) {
            qDebug("V multi: Skipping! (qreal has too little precision, result will be wrong)");
        } else {
            QTest::newRow("V multi") << QSizeF(width, height) << theData << theResult;
        }
    }

    // Multiple widgets, grid
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        const qreal height = 100.f;
        const qreal width = 200.f;

        const int d = 10; // items per dimension
        const int n = d*d;

        const qreal verticalStep = height/(d+2.f);
        const qreal horizontalStep = width/(d+2.f);

        for ( int i = 0 ; i < n; i++ ) {
            if ( i%d == 0 ) {
                // left side item
                theData
                    << BasicData(-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i+1, Qt::AnchorRight, i, Qt::AnchorRight, -horizontalStep);
            } else if ( (i+1)%d == 0 ) {
                // rigth side item
                theData
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    // << BasicData(-1, Qt::AnchorRight, i, Qt::AnchorRight, -horizontalStep);
                    << BasicData(-1, Qt::AnchorRight, i, Qt::AnchorRight, horizontalStep);
            } else {
                // horizontal middle
                theData
                    << BasicData(i-1, Qt::AnchorLeft, i, Qt::AnchorLeft, horizontalStep)
                    << BasicData(i+1, Qt::AnchorRight, i, Qt::AnchorRight, -horizontalStep);
            }

            if ( i < d ) {
                // top line
                theData
                    << BasicData(-1, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(i+d, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep);
            } else if ( i >= (d-1)*d ){
                // bottom line
                theData
                    << BasicData(i-d, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    // << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep)
                    << BasicData(-1, Qt::AnchorBottom, i, Qt::AnchorBottom, verticalStep);
            } else {
                // vertical middle
                theData
                    << BasicData(i-d, Qt::AnchorTop, i, Qt::AnchorTop, verticalStep)
                    << BasicData(i+d, Qt::AnchorBottom, i, Qt::AnchorBottom, -verticalStep);
            }

            theResult
                << BasicResult(i, QRectF(((i%d)+1)*horizontalStep, ((i/d)+1)*verticalStep, horizontalStep, verticalStep) );
        }

        if (sizeof(qreal) == 4) {
            qDebug("Grid multi: Skipping! (qreal has too little precision, result will be wrong)");
        } else {
            QTest::newRow("Grid multi") << QSizeF(200, 100) << theData << theResult;
        }
    }
}

inline QGraphicsLayoutItem *getItem(
        int index,
        const QList<QGraphicsWidget *>& widgets,
        QGraphicsLayoutItem *defaultItem)
{
    if (index < 0) {
        return defaultItem;
    }

    return widgets[index];
}

static bool fuzzierCompare(qreal a, qreal b)
{
    return qAbs(a - b) <= qreal(0.0001);
}

static bool fuzzierCompare(const QRectF &r1, const QRectF &r2)
{

    return fuzzierCompare(r1.x(), r2.x()) && fuzzierCompare(r1.y(), r2.y())
        && fuzzierCompare(r1.width(), r2.width()) && fuzzierCompare(r1.height(), r2.height());
}

void tst_QGraphicsAnchorLayout1::testBasicLayout()
{
    QFETCH(QSizeF, size);
    QFETCH(BasicLayoutTestDataList, data);
    QFETCH(BasicLayoutTestResultList, result);

    QGraphicsWidget *widget = new QGraphicsWidget;

    // Determine amount of widgets to add.
    int widgetCount = -1;
    for (int i = 0; i < data.count(); ++i) {
        const BasicLayoutTestData item = data[i];
        widgetCount = qMax(widgetCount, item.firstIndex);
        widgetCount = qMax(widgetCount, item.secondIndex);
    }
    ++widgetCount; // widgetCount is max of indices.

    // Create dummy widgets
    QList<QGraphicsWidget *> widgets;
    for (int i = 0; i < widgetCount; ++i)
        widgets << new TestWidget(0, QLatin1Char('W') + QString::number(i));

    // Setup anchor layout
    TheAnchorLayout *layout = new TheAnchorLayout;

    for (int i = 0; i < data.count(); ++i) {
        const BasicLayoutTestData item = data[i];
        layout->setAnchor(
            getItem(item.firstIndex, widgets, layout),
            item.firstEdge,
            getItem(item.secondIndex, widgets, layout),
            item.secondEdge,
            item.spacing );
    }

    widget->setLayout(layout);
    widget->setContentsMargins(0,0,0,0);

    widget->resize(size);
    QCOMPARE(widget->size(), size);

    // Validate
    for (int i = 0; i < result.count(); ++i) {
        const BasicLayoutTestResult item = result[i];
        QRectF expected = item.rect;
        QRectF actual = widgets[item.index]->geometry();

        QVERIFY(fuzzierCompare(actual, expected));
    }

    // Test mirrored mode
    widget->setLayoutDirection(Qt::RightToLeft);
    layout->activate();
    // Validate
    for (int j = 0; j < result.count(); ++j) {
        const BasicLayoutTestResult item = result[j];
        QRectF mirroredRect(item.rect);
        // only valid cases are mirrored
        if (mirroredRect.isValid()){
            mirroredRect.moveLeft(size.width()-item.rect.width()-item.rect.left());
        }
        QRectF expected = mirroredRect;
        QRectF actual = widgets[item.index]->geometry();

        QVERIFY(fuzzierCompare(actual, expected));
    }

    qDeleteAll(widgets);
    delete widget;
}

void tst_QGraphicsAnchorLayout1::testNegativeSpacing()
{
    // use the same frame
    testBasicLayout();
}

void tst_QGraphicsAnchorLayout1::testMixedSpacing()
{
    // use the same frame
    testBasicLayout();
}

void tst_QGraphicsAnchorLayout1::testMulti()
{
    // use the same frame
    testBasicLayout();
}

void tst_QGraphicsAnchorLayout1::testCenterAnchors_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<BasicLayoutTestDataList>("data");
    QTest::addColumn<BasicLayoutTestResultList>("result");

    typedef BasicLayoutTestData BasicData;
    typedef BasicLayoutTestResult BasicResult;

    // Basic center case
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            // << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, -10)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorRight, 15)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorVerticalCenter, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 5);

        theResult
            << BasicResult(0, QRectF(5, 5, 10, 10) );

        QTest::newRow("center, basic") << QSizeF(20, 20) << theData << theResult;
    }

    // Basic center case, with invalid (shouldn't affect on result)
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            // << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, -10)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorRight, 15)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorVerticalCenter, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 5)

            // bogus definitions
            << BasicData(0, Qt::AnchorHorizontalCenter, -1, Qt::AnchorBottom, 5)
            << BasicData(0, Qt::AnchorHorizontalCenter, 1, Qt::AnchorVerticalCenter, 5)
            << BasicData(0, Qt::AnchorVerticalCenter, -1, Qt::AnchorRight, 5)
            << BasicData(0, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 666)
            << BasicData(0, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 999)
            << BasicData(0, Qt::AnchorLeft, 0, Qt::AnchorLeft, 333)
            << BasicData(-1, Qt::AnchorRight, -1, Qt::AnchorRight, 222)
            << BasicData(0, Qt::AnchorTop, 0, Qt::AnchorTop, 111)
            << BasicData(0, Qt::AnchorBottom, 0, Qt::AnchorBottom, 444);

        theResult
            << BasicResult(0, QRectF(5, 5, 10, 10) );

        QTest::newRow("center, basic with invalid") << QSizeF(20, 20) << theData << theResult;
    }

    // Basic center case 2
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 0)
            // Not supported << BasicData(0, Qt::AnchorHorizontalCenter, 0, Qt::AnchorRight, 5)
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorRight, 5)
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 0)
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorTop, -5);

        theResult
            << BasicResult(0, QRectF(5, 5, 10, 10) );

        QTest::newRow("center, basic 2") << QSizeF(20, 20) << theData << theResult;
    }

    // Basic center case, overrides
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 10)
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 20)
            << BasicData(0, Qt::AnchorHorizontalCenter, -1, Qt::AnchorHorizontalCenter, 30)
            << BasicData(0, Qt::AnchorVerticalCenter, -1, Qt::AnchorVerticalCenter, 40)
            // actual data:
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 0)
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 0)
            // << BasicData(0, Qt::AnchorHorizontalCenter, 0, Qt::AnchorRight, 5)
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorRight, 5)
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorTop, -5);

        theResult
            << BasicResult(0, QRectF(5, 5, 10, 10) );

        QTest::newRow("center, overrides") << QSizeF(20, 20) << theData << theResult;
    }

    // Two nested
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorLeft, 0)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorBottom, 0)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorRight, 0)
            << BasicData(0, Qt::AnchorVerticalCenter, 1, Qt::AnchorTop, 0)
            << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorLeft, 0)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorRight, 0)
            << BasicData(0, Qt::AnchorBottom, 1, Qt::AnchorBottom, 0);

        theResult
            << BasicResult(0, QRectF(20, 0, 20, 40))
            << BasicResult(1, QRectF(20, 20, 20, 20));

        QTest::newRow("center, two nested") << QSizeF(40, 40) << theData << theResult;
    }

    // Two overlap
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        // theData
        //     // horizontal
        //     << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 20)
        //     << BasicData(0, Qt::AnchorHorizontalCenter, 1, Qt::AnchorLeft, 0)
        //     << BasicData(1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorRight, -5)
        //     << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
        //     << BasicData(0, Qt::AnchorHorizontalCenter, 0, Qt::AnchorRight, 10)
        //     // vertical is pretty much same as horizontal, just roles swapped
        //     << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 20)
        //     << BasicData(1, Qt::AnchorVerticalCenter, 0, Qt::AnchorTop, 0)
        //     << BasicData(0, Qt::AnchorVerticalCenter, 1, Qt::AnchorBottom, -5)
        //     << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 10)
        //     << BasicData(1, Qt::AnchorVerticalCenter, 1, Qt::AnchorBottom, 10);

        theData
            // horizontal
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 20)
            << BasicData(0, Qt::AnchorHorizontalCenter, 1, Qt::AnchorLeft, 0)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorHorizontalCenter, 5)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorRight, 20)
            // vertical
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 20)
            << BasicData(1, Qt::AnchorVerticalCenter, 0, Qt::AnchorTop, 0)
            << BasicData(1, Qt::AnchorBottom, 0, Qt::AnchorVerticalCenter, 5)
            << BasicData(1, Qt::AnchorBottom, 0, Qt::AnchorBottom, 20);

        theResult
            << BasicResult(0, QRectF(20, 30, 20, 30))
            << BasicResult(1, QRectF(30, 20, 30, 20));

        QTest::newRow("center, two overlap") << QSizeF(70, 70) << theData << theResult;
    }

    // Three
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 0)
            << BasicData(0, Qt::AnchorHorizontalCenter, 2, Qt::AnchorHorizontalCenter, 75)
            << BasicData(1, Qt::AnchorRight, 2, Qt::AnchorLeft, 10)
            << BasicData(1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, -30)
            << BasicData(2, Qt::AnchorRight, -1, Qt::AnchorRight, 0)
            << BasicData(1, Qt::AnchorLeft, 1, Qt::AnchorRight, 30)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 10)

            << BasicData(1, Qt::AnchorTop, -1, Qt::AnchorTop, 0)
            << BasicData(1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 35)
            << BasicData(1, Qt::AnchorVerticalCenter, 2, Qt::AnchorVerticalCenter, 15)
            << BasicData(1, Qt::AnchorBottom, 2, Qt::AnchorTop, 5)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 0)
            << BasicData(2, Qt::AnchorBottom, 0, Qt::AnchorTop, 5)
            << BasicData(0, Qt::AnchorTop, 0, Qt::AnchorBottom, 20);

        theResult
            << BasicResult(0, QRectF(0, 30, 10, 20))
            << BasicResult(1, QRectF(20, 0, 30, 10))
            << BasicResult(2, QRectF(60, 15, 40, 10));

        QTest::newRow("center, three") << QSizeF(100, 50) << theData << theResult;
    }

    // Two, parent center
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            // vertical is pretty much same as horizontal, just roles swapped
            << BasicData(-1, Qt::AnchorVerticalCenter, 1, Qt::AnchorVerticalCenter, -15)
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 10)
            << BasicData(-1, Qt::AnchorBottom, 0, Qt::AnchorBottom, 0)
            << BasicData(1, Qt::AnchorTop, 0, Qt::AnchorTop, 0)
            // horizontal
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, -15)
            << BasicData(-1, Qt::AnchorHorizontalCenter, 1, Qt::AnchorHorizontalCenter, 10)
            << BasicData(-1, Qt::AnchorRight, 1, Qt::AnchorRight, 0)
            << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorLeft, 0);

        theResult
            << BasicResult(0, QRectF(20, 20, 30, 80))
            << BasicResult(1, QRectF(20, 20, 80, 30));

        QTest::newRow("center, parent") << QSizeF(100, 100) << theData << theResult;
    }

    // Two, parent center 2
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            // << BasicData(1, Qt::AnchorLeft, -1, Qt::AnchorHorizontalCenter, 15)
            << BasicData(1, Qt::AnchorLeft, -1, Qt::AnchorHorizontalCenter, -15)
            << BasicData(1, Qt::AnchorRight, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, -1, Qt::AnchorRight, 5)
            << BasicData(-1, Qt::AnchorHorizontalCenter, 1, Qt::AnchorRight, -5)
            // vertical
            << BasicData(0, Qt::AnchorVerticalCenter, 1, Qt::AnchorVerticalCenter, 20)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 10)
            << BasicData(0, Qt::AnchorBottom, 1, Qt::AnchorBottom, 20)
            << BasicData(0, Qt::AnchorTop, 1, Qt::AnchorTop, 20)
            << BasicData(0, Qt::AnchorBottom, 1, Qt::AnchorTop, 10);

        theResult
            << BasicResult(0, QRectF(30, 10, 15, 10))
            << BasicResult(1, QRectF(10, 30, 10, 10));

        QTest::newRow("center, parent 2") << QSizeF(50, 50) << theData << theResult;
    }

    // Two, parent center 3
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorRight, -5)
            << BasicData(-1, Qt::AnchorHorizontalCenter, 1, Qt::AnchorLeft, 5)
            // << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorRight, 100)
            << BasicData(0, Qt::AnchorLeft, 1, Qt::AnchorRight, -100)
            << BasicData(0, Qt::AnchorLeft, -1, Qt::AnchorLeft, 0)

            // vertical
            << BasicData(0, Qt::AnchorVerticalCenter, 1, Qt::AnchorVerticalCenter, 55)
            << BasicData(0, Qt::AnchorTop, -1, Qt::AnchorTop, 0)
            << BasicData(1, Qt::AnchorBottom, -1, Qt::AnchorBottom, 0)
            << BasicData(0, Qt::AnchorBottom, 1, Qt::AnchorTop, 10)
            // << BasicData(0, Qt::AnchorTop, 0, Qt::AnchorBottom, 45)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorBottom, 45)
            ;

        theResult
            << BasicResult(0, QRectF(0, 0, 45, 45))
            << BasicResult(1, QRectF(55, 55, 45, 45));

        QTest::newRow("center, parent 3") << QSizeF(100, 100) << theData << theResult;
    }

}

void tst_QGraphicsAnchorLayout1::testCenterAnchors()
{
    if (strcmp(QTest::currentDataTag(), "center, basic with invalid") == 0) {
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor edges of different orientations");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor edges of different orientations");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor edges of different orientations");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
    } else if (strcmp(QTest::currentDataTag(), "center, three") == 0) {
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
    }

    // use the same frame
    testBasicLayout();
}

void tst_QGraphicsAnchorLayout1::testRemoveCenterAnchor_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<BasicLayoutTestDataList>("data");
    QTest::addColumn<BasicLayoutTestDataList>("removeData");
    QTest::addColumn<BasicLayoutTestResultList>("result");

    typedef BasicLayoutTestData BasicData;
    typedef BasicLayoutTestResult BasicResult;

    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestDataList theRemoveData;
        BasicLayoutTestResultList theResult;

        theData
            // << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, -10)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorRight, 15)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorVerticalCenter, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 5)

            << BasicData(-1, Qt::AnchorHorizontalCenter, 1, Qt::AnchorHorizontalCenter, 66)
            << BasicData(1, Qt::AnchorVerticalCenter, -1, Qt::AnchorVerticalCenter, 99)
            << BasicData(0, Qt::AnchorHorizontalCenter, 1, Qt::AnchorHorizontalCenter, 33)
            ;

        theRemoveData
            << BasicData(-1, Qt::AnchorHorizontalCenter, 1, Qt::AnchorHorizontalCenter, 0)
            << BasicData(1, Qt::AnchorVerticalCenter, -1, Qt::AnchorVerticalCenter, 0)
            << BasicData(0, Qt::AnchorHorizontalCenter, 1, Qt::AnchorHorizontalCenter, 0);

        theResult
            << BasicResult(0, QRectF(5, 5, 10, 10) );

        QTest::newRow("remove, center, basic") << QSizeF(20, 20) << theData
            << theRemoveData << theResult;
    }

    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestDataList theRemoveData;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 0)
            << BasicData(0, Qt::AnchorHorizontalCenter, 2, Qt::AnchorHorizontalCenter, 75)
            << BasicData(1, Qt::AnchorRight, 2, Qt::AnchorLeft, 10)
            << BasicData(1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, -30)
            << BasicData(2, Qt::AnchorRight, -1, Qt::AnchorRight, 0)
            << BasicData(1, Qt::AnchorLeft, 1, Qt::AnchorRight, 30)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 10)

            // extra:
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 66)
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 33)
            << BasicData(0, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 55)
            << BasicData(1, Qt::AnchorVerticalCenter, 1, Qt::AnchorVerticalCenter, 55)

            << BasicData(1, Qt::AnchorTop, -1, Qt::AnchorTop, 0)
            << BasicData(1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 35)
            << BasicData(1, Qt::AnchorVerticalCenter, 2, Qt::AnchorVerticalCenter, 15)
            << BasicData(1, Qt::AnchorBottom, 2, Qt::AnchorTop, 5)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 0)
            << BasicData(2, Qt::AnchorBottom, 0, Qt::AnchorTop, 5)
            << BasicData(0, Qt::AnchorTop, 0, Qt::AnchorBottom, 20);

        theRemoveData
            << BasicData(-1, Qt::AnchorVerticalCenter, 0, Qt::AnchorVerticalCenter, 66)
            << BasicData(-1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 33)
            << BasicData(0, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 55)
            << BasicData(1, Qt::AnchorVerticalCenter, 1, Qt::AnchorVerticalCenter, 55);

        theResult
            << BasicResult(0, QRectF(0, 30, 10, 20))
            << BasicResult(1, QRectF(20, 0, 30, 10))
            << BasicResult(2, QRectF(60, 15, 40, 10));

        QTest::newRow("remove, center, three") << QSizeF(100, 50) << theData << theRemoveData << theResult;
    }

    // add edge (item0,edge0,item1,edge1), remove (item1,edge1,item0,edge0)
    {
        BasicLayoutTestDataList theData;
        BasicLayoutTestDataList theRemoveData;
        BasicLayoutTestResultList theResult;

        theData
            // << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, -10)
            << BasicData(-1, Qt::AnchorRight, 0, Qt::AnchorHorizontalCenter, 10)
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorRight, 15)
            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorVerticalCenter, 10)
            << BasicData(0, Qt::AnchorBottom, -1, Qt::AnchorBottom, 5)

            << BasicData(-1, Qt::AnchorHorizontalCenter, 1, Qt::AnchorHorizontalCenter, 66)
            << BasicData(1, Qt::AnchorVerticalCenter, -1, Qt::AnchorVerticalCenter, 99)
            << BasicData(0, Qt::AnchorHorizontalCenter, 1, Qt::AnchorHorizontalCenter, 33)
            << BasicData(0, Qt::AnchorLeft, 0, Qt::AnchorRight, 22)
            << BasicData(0, Qt::AnchorTop, 0, Qt::AnchorBottom, 11)
            ;

        theRemoveData
            << BasicData(1, Qt::AnchorHorizontalCenter, -1, Qt::AnchorHorizontalCenter, 0)
            << BasicData(-1, Qt::AnchorVerticalCenter, 1, Qt::AnchorVerticalCenter, 0)
            << BasicData(1, Qt::AnchorHorizontalCenter, 0, Qt::AnchorHorizontalCenter, 0)
            << BasicData(0, Qt::AnchorRight, 0, Qt::AnchorLeft, 0)
            << BasicData(0, Qt::AnchorBottom, 0, Qt::AnchorTop, 0)
            ;

        theResult
            << BasicResult(0, QRectF(5, 5, 10, 10) );

        QTest::newRow("remove, center, basic 2") << QSizeF(20, 20) << theData
            << theRemoveData << theResult;
    }

}

void tst_QGraphicsAnchorLayout1::testRemoveCenterAnchor()
{
    if (strcmp(QTest::currentDataTag(), "remove, center, three") == 0) {
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
    } else if (strcmp(QTest::currentDataTag(), "remove, center, basic 2") == 0) {
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsAnchorLayout::addAnchor(): Cannot anchor the item to itself");
    }

    QFETCH(QSizeF, size);
    QFETCH(BasicLayoutTestDataList, data);
    QFETCH(BasicLayoutTestDataList, removeData);
    QFETCH(BasicLayoutTestResultList, result);

    QGraphicsWidget *widget = new QGraphicsWidget;

    // Determine amount of widgets to add.
    int widgetCount = -1;
    for (int i = 0; i < data.count(); ++i) {
        const BasicLayoutTestData item = data[i];
        widgetCount = qMax(widgetCount, item.firstIndex);
        widgetCount = qMax(widgetCount, item.secondIndex);
    }
    ++widgetCount; // widgetCount is max of indices.

    // Create dummy widgets
    QList<QGraphicsWidget *> widgets;
    for (int i = 0; i < widgetCount; ++i) {
        TestWidget *w = new TestWidget;
        widgets << w;
    }

    // Setup anchor layout
    TheAnchorLayout *layout = new TheAnchorLayout;

    for (int i = 0; i < data.count(); ++i) {
        const BasicLayoutTestData item = data[i];
        layout->setAnchor(
            getItem(item.firstIndex, widgets, layout),
            item.firstEdge,
            getItem(item.secondIndex, widgets, layout),
            item.secondEdge,
            item.spacing );
    }

    for (int i = 0; i < removeData.count(); ++i) {
        const BasicLayoutTestData item = removeData[i];
        layout->removeAnchor(
            getItem(item.firstIndex, widgets, layout),
            item.firstEdge,
            getItem(item.secondIndex, widgets, layout),
            item.secondEdge);
    }

    widget->setLayout(layout);
    widget->setContentsMargins(0,0,0,0);

    widget->resize(size);
    QCOMPARE(widget->size(), size);

    // Validate
    for (int i = 0; i < result.count(); ++i) {
        const BasicLayoutTestResult item = result[i];

        QCOMPARE(widgets[item.index]->geometry(), item.rect);
    }

    qDeleteAll(widgets);
    delete widget;
}

void tst_QGraphicsAnchorLayout1::testSingleSizePolicy_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<QSizePolicy>("policy");
    QTest::addColumn<bool>("valid");

// FIXED
    {
        QSizePolicy sizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        QTest::newRow("single size policy: fixed ok") << QSizeF(70, 70) << sizePolicy << true;
    }
/*
    {
        QSizePolicy sizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        QTest::newRow("single size policy: fixed too big") << QSizeF(100, 100) << sizePolicy << false;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        QTest::newRow("single size policy: fixed too small") << QSizeF(50, 50) << sizePolicy << false;
    }
*/
// MINIMUM
    {
        QSizePolicy sizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        QTest::newRow("single size policy: minimum bigger ok") << QSizeF(100, 100) << sizePolicy << true;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        QTest::newRow("single size policy: minimum limit ok") << QSizeF(70, 70) << sizePolicy << true;
    }
/*
    {
        QSizePolicy sizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        QTest::newRow("single size policy: minimum too small") << QSizeF(50, 50) << sizePolicy << false;
    }
*/
// MAXIMUM
    {
        QSizePolicy sizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
        QTest::newRow("single size policy: maximum small ok") << QSizeF(50, 50) << sizePolicy << true;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
        QTest::newRow("single size policy: maximum limit ok") << QSizeF(70, 70) << sizePolicy << true;
    }
/*
    {
        QSizePolicy sizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
        QTest::newRow("single size policy: maximum bigger fail") << QSizeF(100, 100) << sizePolicy << false;
    }
*/
// PREFERRED
    {
        QSizePolicy sizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        QTest::newRow("single size policy: preferred bigger ok") << QSizeF(100, 100) << sizePolicy << true;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        QTest::newRow("single size policy: preferred smaller ok") << QSizeF(50, 50)  << sizePolicy << true;
    }
/*
    {
        QSizePolicy sizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        QTest::newRow("single size policy: preferred too big") << QSizeF(700, 700) << sizePolicy << false;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        QTest::newRow("single size policy: preferred too small") << QSizeF(21, 21) << sizePolicy << false;
    }
*/
// MINIMUMEXPANDING

    {
        QSizePolicy sizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
        QTest::newRow("single size policy: min.expanding bigger ok") << QSizeF(100, 100) << sizePolicy << true;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
        QTest::newRow("single size policy: min.expanding limit ok") << QSizeF(70, 70) << sizePolicy << true;
    }

    /*{
        QSizePolicy sizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
        QTest::newRow("single size policy: min.expanding too small") << QSizeF(50, 50) << sizePolicy << false;
    }*/

// EXPANDING

    {
        QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
        QTest::newRow("single size policy: expanding bigger ok") << QSizeF(100, 100) << sizePolicy << true;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
        QTest::newRow("single size policy: expanding smaller ok") << QSizeF(50, 50)  << sizePolicy << true;
    }

 // IGNORED

    {
        QSizePolicy sizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
        QTest::newRow("single size policy: ignored bigger ok") << QSizeF(100, 100) << sizePolicy << true;
    }

    {
        QSizePolicy sizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
        QTest::newRow("single size policy: ignored smaller ok") << QSizeF(50, 50)  << sizePolicy << true;
    }
}

void tst_QGraphicsAnchorLayout1::testSingleSizePolicy()
{
    QFETCH(QSizeF, size);
    QFETCH(QSizePolicy, policy);
    QFETCH(bool, valid);

    // create objects
    QGraphicsWidget widget;
    TheAnchorLayout *layout = new TheAnchorLayout;
    TestWidget *childWidget = new TestWidget;

    // set anchors
    layout->setAnchor( layout, Qt::AnchorLeft, childWidget, Qt::AnchorLeft, 10 );
    layout->setAnchor( childWidget, Qt::AnchorRight, layout, Qt::AnchorRight, 10 );
    layout->setAnchor( layout, Qt::AnchorTop, childWidget, Qt::AnchorTop, 10 );
    layout->setAnchor( childWidget, Qt::AnchorBottom, layout, Qt::AnchorBottom, 10 );

    widget.setLayout( layout );

    // set test case specific: policy and size
    childWidget->setSizePolicy( policy );
    widget.setGeometry( QRectF( QPoint(0,0), size ) );

    QCOMPARE( layout->isValid() , valid );

    const QRectF childRect = childWidget->geometry();
    Q_UNUSED( childRect );
}

void tst_QGraphicsAnchorLayout1::testDoubleSizePolicy_data()
{
    // tests only horizontal direction
    QTest::addColumn<QSizePolicy>("policy1");
    QTest::addColumn<QSizePolicy>("policy2");
    QTest::addColumn<qreal>("width1");
    QTest::addColumn<qreal>("width2");

    // layout size always 100x100 and size hints for items are 5<50<500
    // gabs: 10-item1-10-item2-10

    {
        QSizePolicy sizePolicy1( QSizePolicy::Fixed, QSizePolicy::Fixed );
        QSizePolicy sizePolicy2( QSizePolicy::Preferred, QSizePolicy::Preferred );
        const qreal width1 = 50;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: fixed-preferred") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    {
        QSizePolicy sizePolicy1( QSizePolicy::Minimum, QSizePolicy::Minimum );
        QSizePolicy sizePolicy2( QSizePolicy::Preferred, QSizePolicy::Preferred );
        const qreal width1 = 50;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: minimum-preferred") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    {
        QSizePolicy sizePolicy1( QSizePolicy::Maximum, QSizePolicy::Maximum );
        QSizePolicy sizePolicy2( QSizePolicy::Preferred, QSizePolicy::Preferred );
        const qreal width1 = 35;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: maximum-preferred") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    {
        QSizePolicy sizePolicy1( QSizePolicy::Preferred, QSizePolicy::Preferred );
        QSizePolicy sizePolicy2( QSizePolicy::Preferred, QSizePolicy::Preferred );
        const qreal width1 = 35;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: preferred-preferred") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    {
        QSizePolicy sizePolicy1( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
        QSizePolicy sizePolicy2( QSizePolicy::Preferred, QSizePolicy::Preferred );
        const qreal width1 = 50;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: min.expanding-preferred") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    {
        QSizePolicy sizePolicy1( QSizePolicy::Expanding, QSizePolicy::Expanding );
        QSizePolicy sizePolicy2( QSizePolicy::Preferred, QSizePolicy::Preferred );
        const qreal width1 = 35;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: expanding-preferred") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    // QGAL handling of ignored flag is different
    if (0)
    {
        QSizePolicy sizePolicy1( QSizePolicy::Ignored, QSizePolicy::Ignored );
        QSizePolicy sizePolicy2( QSizePolicy::Preferred, QSizePolicy::Preferred );
        const qreal width1 = 35;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: ignored-preferred") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    /*{
        QSizePolicy sizePolicy1( QSizePolicy::Fixed, QSizePolicy::Fixed );
        QSizePolicy sizePolicy2( QSizePolicy::Fixed, QSizePolicy::Fixed );
        const qreal width1 = -1;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: fixed-fixed invalid") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }*/

    /*{
        QSizePolicy sizePolicy1( QSizePolicy::Minimum, QSizePolicy::Minimum );
        QSizePolicy sizePolicy2( QSizePolicy::Fixed, QSizePolicy::Fixed );
        const qreal width1 = -1;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: minimum-fixed invalid") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }*/

    {
        QSizePolicy sizePolicy1( QSizePolicy::Maximum, QSizePolicy::Maximum );
        QSizePolicy sizePolicy2( QSizePolicy::Fixed, QSizePolicy::Fixed );
        const qreal width1 = 20;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: maximum-fixed") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    {
        QSizePolicy sizePolicy1( QSizePolicy::Preferred, QSizePolicy::Preferred );
        QSizePolicy sizePolicy2( QSizePolicy::Fixed, QSizePolicy::Fixed );
        const qreal width1 = 20;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: preferred-fixed") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    /*{
        QSizePolicy sizePolicy1( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
        QSizePolicy sizePolicy2( QSizePolicy::Fixed, QSizePolicy::Fixed );
        const qreal width1 = -1;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: min.expanding-fixed invalid") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }*/

    {
        QSizePolicy sizePolicy1( QSizePolicy::Expanding, QSizePolicy::Expanding );
        QSizePolicy sizePolicy2( QSizePolicy::Fixed, QSizePolicy::Fixed );
        const qreal width1 = 20;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: expanding-fixed") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }

    {
        QSizePolicy sizePolicy1( QSizePolicy::Ignored, QSizePolicy::Ignored );
        QSizePolicy sizePolicy2( QSizePolicy::Fixed, QSizePolicy::Fixed );
        const qreal width1 = 20;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("double size policy: ignored-fixed") << sizePolicy1 << sizePolicy2 << width1 << width2;
    }
}

void tst_QGraphicsAnchorLayout1::testDoubleSizePolicy()
{
    QFETCH(QSizePolicy, policy1);
    QFETCH(QSizePolicy, policy2);
    QFETCH(qreal, width1);
    QFETCH(qreal, width2);

    // create objects
    QGraphicsWidget widget;
    TheAnchorLayout *layout = new TheAnchorLayout;
    TestWidget *childWidget1 = new TestWidget;
    TestWidget *childWidget2 = new TestWidget;

    // set anchors
    layout->setAnchor( layout, Qt::AnchorLeft, childWidget1, Qt::AnchorLeft, 10 );
    layout->setAnchor( childWidget1, Qt::AnchorRight, childWidget2, Qt::AnchorLeft, 10 );
    layout->setAnchor( childWidget2, Qt::AnchorRight, layout, Qt::AnchorRight, 10 );

    widget.setLayout( layout );

    // set test case specific: policy
    childWidget1->setSizePolicy( policy1 );
    childWidget2->setSizePolicy( policy2 );

    widget.setGeometry( QRectF( QPoint(0,0), QSize( 100,100 ) ) );

    // check results:
    if ( width1 == -1.0f ) {
        // invalid
        QCOMPARE( layout->isValid() , false );
    } else {
        // valid
        QCOMPARE( childWidget1->geometry().width(), width1 );
        QCOMPARE( childWidget2->geometry().width(), width2 );
    }
}

typedef QMap<int,qreal> SizeHintArray;
Q_DECLARE_METATYPE(SizeHintArray)

void tst_QGraphicsAnchorLayout1::testSizeDistribution_data()
{
    // tests only horizontal direction
    QTest::addColumn<SizeHintArray>("sizeHints1");
    QTest::addColumn<SizeHintArray>("sizeHints2");
    QTest::addColumn<qreal>("width1");
    QTest::addColumn<qreal>("width2");

    // layout size always 100x100 and size policy for items is preferred-preferred
    // gabs: 10-item1-10-item2-10

    {
        SizeHintArray sizeHints1;
        sizeHints1.insert( Qt::MinimumSize, 30 );
        sizeHints1.insert( Qt::PreferredSize, 35 );
        sizeHints1.insert( Qt::MaximumSize, 40 );

        SizeHintArray sizeHints2;
        sizeHints2.insert( Qt::MinimumSize, 5 );
        sizeHints2.insert( Qt::PreferredSize, 35 );
        sizeHints2.insert( Qt::MaximumSize, 300 );

        const qreal width1 = 35;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("size distribution: preferred equal") << sizeHints1 << sizeHints2 << width1 << width2;
    }

    {
        SizeHintArray sizeHints1;
        sizeHints1.insert( Qt::MinimumSize, 0 );
        sizeHints1.insert( Qt::PreferredSize, 20 );
        sizeHints1.insert( Qt::MaximumSize, 100 );

        SizeHintArray sizeHints2;
        sizeHints2.insert( Qt::MinimumSize, 0 );
        sizeHints2.insert( Qt::PreferredSize, 50 );
        sizeHints2.insert( Qt::MaximumSize, 100 );

        const qreal width1 = 20;
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("size distribution: preferred non-equal") << sizeHints1 << sizeHints2 << width1 << width2;
    }

    {
        SizeHintArray sizeHints1;
        sizeHints1.insert( Qt::MinimumSize, 0 );
        sizeHints1.insert( Qt::PreferredSize, 40 );
        sizeHints1.insert( Qt::MaximumSize, 100 );

        SizeHintArray sizeHints2;
        sizeHints2.insert( Qt::MinimumSize, 0 );
        sizeHints2.insert( Qt::PreferredSize, 60 );
        sizeHints2.insert( Qt::MaximumSize, 100 );

        const qreal width1 = 28; // got from manual calculation
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("size distribution: below preferred") << sizeHints1 << sizeHints2 << width1 << width2;
    }

    {
        SizeHintArray sizeHints1;
        sizeHints1.insert( Qt::MinimumSize, 0 );
        sizeHints1.insert( Qt::PreferredSize, 10 );
        sizeHints1.insert( Qt::MaximumSize, 100 );

        SizeHintArray sizeHints2;
        sizeHints2.insert( Qt::MinimumSize, 0 );
        sizeHints2.insert( Qt::PreferredSize, 40 );
        sizeHints2.insert( Qt::MaximumSize, 100 );

        const qreal width1 = 22; // got from manual calculation
        const qreal width2 = 100-10-10-10-width1;
        QTest::newRow("size distribution: above preferred") << sizeHints1 << sizeHints2 << width1 << width2;
    }

}

void tst_QGraphicsAnchorLayout1::testSizeDistribution()
{
    QFETCH(SizeHintArray, sizeHints1);
    QFETCH(SizeHintArray, sizeHints2);
    QFETCH(qreal, width1);
    QFETCH(qreal, width2);

    // sanity-check the test data - MinimumSize <= PreferredSize <= MaximumSize
    QVERIFY( sizeHints1.value( Qt::MinimumSize ) <= sizeHints1.value( Qt::PreferredSize ) );
    QVERIFY( sizeHints1.value( Qt::PreferredSize ) <= sizeHints1.value( Qt::MaximumSize ) );
    QVERIFY( sizeHints2.value( Qt::MinimumSize ) <= sizeHints2.value( Qt::PreferredSize ) );
    QVERIFY( sizeHints2.value( Qt::PreferredSize ) <= sizeHints2.value( Qt::MaximumSize ) );

    // create objects
    QGraphicsWidget widget;
    TheAnchorLayout *layout = new TheAnchorLayout;
    TestWidget *childWidget1 = new TestWidget;
    TestWidget *childWidget2 = new TestWidget;

    // set anchors
    layout->setAnchor( layout, Qt::AnchorLeft, childWidget1, Qt::AnchorLeft, 10 );
    layout->setAnchor( childWidget1, Qt::AnchorRight, childWidget2, Qt::AnchorLeft, 10 );
    layout->setAnchor( childWidget2, Qt::AnchorRight, layout, Qt::AnchorRight, 10 );

    widget.setLayout( layout );

    // set test case specific: size hints
    childWidget1->setMinimumWidth( sizeHints1.value( Qt::MinimumSize ) );
    childWidget1->setPreferredWidth( sizeHints1.value( Qt::PreferredSize ) );
    childWidget1->setMaximumWidth( sizeHints1.value( Qt::MaximumSize ) );

    childWidget2->setMinimumWidth( sizeHints2.value( Qt::MinimumSize ) );
    childWidget2->setPreferredWidth( sizeHints2.value( Qt::PreferredSize ) );
    childWidget2->setMaximumWidth( sizeHints2.value( Qt::MaximumSize ) );

    widget.setGeometry( QRectF( QPoint(0,0), QSize( 100,100 ) ) );

    // check results:
    if ( width1 == -1.0f ) {
        // invalid
        QCOMPARE( layout->isValid() , false );
    } else {
        // valid
        QCOMPARE( float(childWidget1->geometry().width()), float(width1) );
        QCOMPARE( float(childWidget2->geometry().width()), float(width2) );
    }
}

void tst_QGraphicsAnchorLayout1::testSizeHint()
{
    QGraphicsWidget *widget[5];

    for( int i = 0; i < 5; i++ ) {
        widget[i] = new QGraphicsWidget;
        widget[i]->setMinimumSize( 10, 10 );
        widget[i]->setPreferredSize( 20, 20 );
        widget[i]->setMaximumSize( 40, 40 );
    }

    // one, basic
    {
        TheAnchorLayout *layout = new TheAnchorLayout();


        layout->setAnchor(layout, Qt::AnchorLeft, widget[0], Qt::AnchorLeft, 0 );
        layout->setAnchor(layout, Qt::AnchorRight, widget[0], Qt::AnchorRight, 0 );

        layout->setAnchor(layout, Qt::AnchorTop, widget[0], Qt::AnchorTop, 0 );
        layout->setAnchor(layout, Qt::AnchorBottom, widget[0], Qt::AnchorBottom, 0 );

        QCOMPARE( layout->minimumSize(), widget[0]->minimumSize() );
        QCOMPARE( layout->preferredSize(), widget[0]->preferredSize() );
        QCOMPARE( layout->maximumSize(), widget[0]->maximumSize() );


        delete layout;
    }

    // one, basic again
    {
        TheAnchorLayout *layout = new TheAnchorLayout();


        layout->setAnchor(layout, Qt::AnchorLeft, widget[0], Qt::AnchorLeft, 10 );
        // layout->setAnchor(layout, Qt::AnchorRight, widget[0], Qt::AnchorRight, -10 );
        layout->setAnchor(layout, Qt::AnchorRight, widget[0], Qt::AnchorRight, 10 );

        layout->setAnchor(layout, Qt::AnchorTop, widget[0], Qt::AnchorTop, 10 );
        // layout->setAnchor(layout, Qt::AnchorBottom, widget[0], Qt::AnchorBottom, -10 );
        layout->setAnchor(layout, Qt::AnchorBottom, widget[0], Qt::AnchorBottom, 10 );

        QCOMPARE( layout->minimumSize(), widget[0]->minimumSize() + QSizeF( 20, 20 ) );
        QCOMPARE( layout->preferredSize(), widget[0]->preferredSize() + QSizeF( 20, 20 ) );
        QCOMPARE( layout->maximumSize(), widget[0]->maximumSize() + QSizeF( 20, 20 ) );

        delete layout;
    }

    // two, serial
    {
        TheAnchorLayout *layout = new TheAnchorLayout();


        layout->setAnchor(layout, Qt::AnchorLeft, widget[0], Qt::AnchorLeft, 0 );
        layout->setAnchor(layout, Qt::AnchorTop, widget[0], Qt::AnchorTop, 0 );
        layout->setAnchor(layout, Qt::AnchorBottom, widget[0], Qt::AnchorBottom, 0 );

        layout->setAnchor(widget[0], Qt::AnchorRight, widget[1], Qt::AnchorLeft, 0 );
        layout->setAnchor(widget[1], Qt::AnchorRight, layout, Qt::AnchorRight, 0 );


        QCOMPARE( layout->minimumSize(), widget[0]->minimumSize() + QSizeF( widget[1]->minimumWidth(), 0 ) );
        QCOMPARE( layout->preferredSize(), widget[0]->preferredSize() + QSizeF( widget[1]->preferredWidth(), 0 ) );
        QCOMPARE( layout->maximumSize(), widget[0]->maximumSize() + QSizeF( widget[1]->maximumWidth(), 0 ) );

        delete layout;
    }

    // two, parallel
    {
        TheAnchorLayout *layout = new TheAnchorLayout();


        layout->setAnchor(layout, Qt::AnchorLeft, widget[0], Qt::AnchorLeft, 0 );
        layout->setAnchor(layout, Qt::AnchorTop, widget[0], Qt::AnchorTop, 0 );
        layout->setAnchor(layout, Qt::AnchorBottom, widget[0], Qt::AnchorBottom, 0 );
        layout->setAnchor(layout, Qt::AnchorRight, widget[0], Qt::AnchorRight, 0 );

        layout->setAnchor(layout, Qt::AnchorLeft, widget[1], Qt::AnchorLeft, 0 );
        layout->setAnchor(layout, Qt::AnchorTop, widget[1], Qt::AnchorTop, 0 );
        layout->setAnchor(layout, Qt::AnchorBottom, widget[1], Qt::AnchorBottom, 0 );
        layout->setAnchor(layout, Qt::AnchorRight, widget[1], Qt::AnchorRight, 0 );

        QCOMPARE( layout->minimumSize(), widget[0]->minimumSize() );
        QCOMPARE( layout->preferredSize(), widget[0]->preferredSize() );
        QCOMPARE( layout->maximumSize(), widget[0]->maximumSize() );

        delete layout;
    }

    // five, serial
    {
        TheAnchorLayout *layout = new TheAnchorLayout();


        layout->setAnchor(layout, Qt::AnchorLeft, widget[0], Qt::AnchorLeft, 0 );
        layout->setAnchor(layout, Qt::AnchorTop, widget[0], Qt::AnchorTop, 0 );
        layout->setAnchor(layout, Qt::AnchorBottom, widget[0], Qt::AnchorBottom, 0 );

        layout->setAnchor(widget[0], Qt::AnchorRight, widget[1], Qt::AnchorLeft, 0 );
        layout->setAnchor(widget[1], Qt::AnchorRight, widget[2], Qt::AnchorLeft, 0 );
        layout->setAnchor(widget[2], Qt::AnchorRight, widget[3], Qt::AnchorLeft, 0 );
        layout->setAnchor(widget[3], Qt::AnchorRight, widget[4], Qt::AnchorLeft, 0 );
        layout->setAnchor(widget[4], Qt::AnchorRight, layout, Qt::AnchorRight, 0 );


        QCOMPARE( layout->minimumSize(), widget[0]->minimumSize() +
        QSizeF( widget[1]->minimumWidth() +
        widget[2]->minimumWidth() +
        widget[3]->minimumWidth() +
        widget[4]->minimumWidth(), 0 ) );

        QCOMPARE( layout->preferredSize(), widget[0]->preferredSize() +
        QSizeF( widget[1]->preferredWidth() +
        widget[2]->preferredWidth() +
        widget[3]->preferredWidth() +
        widget[4]->preferredWidth(), 0 ) );

        QCOMPARE( layout->maximumSize(), widget[0]->maximumSize() +
        QSizeF( widget[1]->maximumWidth() +
        widget[2]->maximumWidth() +
        widget[3]->maximumWidth() +
        widget[4]->maximumWidth(), 0 ) );

        delete layout;
    }


    for( int i = 0; i < 5; i++ ) {
        delete widget[i];
    }
}

#ifdef TEST_COMPLEX_CASES

void tst_QGraphicsAnchorLayout1::testComplexCases_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<BasicLayoutTestDataList>("data");
    QTest::addColumn<AnchorItemSizeHintList>("sizehint");
    QTest::addColumn<BasicLayoutTestResultList>("result");

    typedef BasicLayoutTestData BasicData;
    typedef BasicLayoutTestResult BasicResult;

    // Three widgets, the same sizehint
    {
        BasicLayoutTestDataList theData;
        AnchorItemSizeHintList theSizeHint;
        BasicLayoutTestResultList theResult1;
        BasicLayoutTestResultList theResult2;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 2, Qt::AnchorLeft, 10)

            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(2, Qt::AnchorRight, -1, Qt::AnchorRight, 10)

            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 2, Qt::AnchorTop, 0)
         ;

        theSizeHint
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
         ;
        theResult1
            << BasicResult(0, QRectF(10, 0, 30, 50) )
            << BasicResult(1, QRectF(50, 0, 30, 50) )
            << BasicResult(2, QRectF(50, 0, 30, 50) )
            ;

        theResult2
            << BasicResult(0, QRectF(10, 0, 60, 50) )
            << BasicResult(1, QRectF(80, 0, 60, 50) )
            << BasicResult(2, QRectF(80, 0, 60, 50) )
            ;

        QTest::newRow("Three, the same sizehint(1)") << QSizeF(90, 50) << theData << theSizeHint << theResult1;
        QTest::newRow("Three, the same sizehint(2)") << QSizeF(150, 50) << theData << theSizeHint << theResult2;
    }

    // Three widgets, serial is bigger
    {
        BasicLayoutTestDataList theData;
        AnchorItemSizeHintList theSizeHint;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 2, Qt::AnchorLeft, 10)

            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(2, Qt::AnchorRight, -1, Qt::AnchorRight, 10)

            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 2, Qt::AnchorTop, 0)
         ;

        theSizeHint
            << AnchorItemSizeHint( 0, 100, 200, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
         ;

        theResult
            << BasicResult(0, QRectF(10, 0, 70, 50) )
            << BasicResult(1, QRectF(90, 0, 35, 50) )
            << BasicResult(2, QRectF(90, 0, 35, 50) );

        QTest::newRow("Three, serial is bigger") << QSizeF(135, 50) << theData << theSizeHint << theResult;

        // theResult
        //     << BasicResult(0, QRectF(10, 0, 80, 50) )
        //     << BasicResult(1, QRectF(100, 0, 60, 50) )
        //     << BasicResult(2, QRectF(100, 0, 60, 50) )
        // ;

        // QTest::newRow("Three, serial is bigger") << QSizeF(170, 50) << theData << theSizeHint << theResult;
    }


    // Three widgets, parallel is bigger
    {
        BasicLayoutTestDataList theData;
        AnchorItemSizeHintList theSizeHint;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 2, Qt::AnchorLeft, 10)

            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(2, Qt::AnchorRight, -1, Qt::AnchorRight, 10)

            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 2, Qt::AnchorTop, 0)
         ;

        theSizeHint
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 100, 200, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
         ;

        // ### QGAL uses a different preferred size calculation algorithm.
        //     This algorithm was discussed with Jan-Arve and tries to grow
        //     items instead of shrinking them.
        //     In this case, the preferred size of each item becomes:
        //     Item 0: 50
        //     Item 1: 100 (grows to avoid shrinking item 2)
        //     Item 2: 100
        //     Therefore, the preferred size of the parent widget becomes
        //     180 == (10 + 50 + 10 + 100 + 10)
        //     As we set its size to 150, each widget is shrinked in the same
        //     ratio, in order to achieve the width of 150 == (10 + 40 + 10 + 80 + 10)

        theResult
            << BasicResult(0, QRectF(10, 0, 40, 50) )
            << BasicResult(1, QRectF(60, 0, 80, 50) )
            << BasicResult(2, QRectF(60, 0, 80, 50) )
            ;

        QTest::newRow("Three, parallel is bigger") << QSizeF(150, 50) << theData << theSizeHint << theResult;

        // #ifdef PREFERRED_IS_AVERAGE
        //         theResult
        //             << BasicResult(0, QRectF(10, 0, 50, 50) )
        //             << BasicResult(1, QRectF(70, 0, 75, 50) )
        //             << BasicResult(2, QRectF(70, 0, 75, 50) )
        //             ;

        //         QTest::newRow("Three, parallel is bigger") << QSizeF(155, 50) << theData << theSizeHint << theResult;
        // #else
        //         theResult
        //             << BasicResult(0, QRectF(10, 0, 50, 50) )
        //             << BasicResult(1, QRectF(70, 0, 66.66666666666666, 50) )
        //             << BasicResult(2, QRectF(70, 0, 60.66666666666666, 50) )
        //             ;

        //         QTest::newRow("Three, parallel is bigger") << QSizeF(146.66666666666666, 50) << theData << theSizeHint << theResult;
        // #endif
    }

    // Three widgets, the same sizehint, one center anchor
    {
        BasicLayoutTestDataList theData;
        AnchorItemSizeHintList theSizeHint;
        BasicLayoutTestResultList theResult;

        theData
            << BasicData(-1, Qt::AnchorLeft, 0, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorRight, 1, Qt::AnchorLeft, 10)
            << BasicData(0, Qt::AnchorHorizontalCenter, 2, Qt::AnchorLeft, 10)

            << BasicData(1, Qt::AnchorRight, -1, Qt::AnchorRight, 10)
            << BasicData(2, Qt::AnchorRight, -1, Qt::AnchorRight, 10)

            << BasicData(-1, Qt::AnchorTop, 0, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 1, Qt::AnchorTop, 0)
            << BasicData(-1, Qt::AnchorTop, 2, Qt::AnchorTop, 0)
         ;

        theSizeHint
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
            << AnchorItemSizeHint( 0, 50, 100, 0, 50, 100 )
         ;
        theResult
            << BasicResult(0, QRectF(10, 0, 40, 50) )
            << BasicResult(1, QRectF(60, 0, 40, 50) )
            << BasicResult(2, QRectF(40, 0, 60, 50) )
            ;

            ;

        QTest::newRow("Three, the same sizehint, one center anchor") << QSizeF(110, 50) << theData << theSizeHint << theResult;
    }
}

void tst_QGraphicsAnchorLayout1::testComplexCases()
{
    QFETCH(QSizeF, size);
    QFETCH(BasicLayoutTestDataList, data);
    QFETCH(AnchorItemSizeHintList, sizehint);
    QFETCH(BasicLayoutTestResultList, result);

    QGraphicsWidget *widget = new QGraphicsWidget;

    // Determine amount of widgets to add.
    int widgetCount = -1;
    for (int i = 0; i < data.count(); ++i) {
        const BasicLayoutTestData item = data[i];
        widgetCount = qMax(widgetCount, item.firstIndex);
        widgetCount = qMax(widgetCount, item.secondIndex);
    }
    ++widgetCount; // widgetCount is max of indices.

    // Create dummy widgets
    QList<QGraphicsWidget *> widgets;
    for (int i = 0; i < widgetCount; ++i) {
        TestWidget *w = new TestWidget;

        w->setMinimumWidth( sizehint[i].hmin );
        w->setPreferredWidth( sizehint[i].hpref );
        w->setMaximumWidth( sizehint[i].hmax );

        w->setMinimumHeight( sizehint[i].vmin );
        w->setPreferredHeight( sizehint[i].vpref );
        w->setMaximumHeight( sizehint[i].vmax );

        widgets << w;
    }

    // Setup anchor layout
    TheAnchorLayout *layout = new TheAnchorLayout;

    for (int i = 0; i < data.count(); ++i) {
        const BasicLayoutTestData item = data[i];
        layout->setAnchor(
            getItem(item.firstIndex, widgets, layout),
            item.firstEdge,
            getItem(item.secondIndex, widgets, layout),
            item.secondEdge,
            item.spacing );
    }

    widget->setLayout(layout);
    widget->setContentsMargins(0,0,0,0);

    widget->resize(size);
    QCOMPARE(widget->size(), size);

    // Validate
    for (int i = 0; i < result.count(); ++i) {
        const BasicLayoutTestResult item = result[i];
        QCOMPARE(widgets[item.index]->geometry(), item.rect);
    }

    // Test mirrored mode
    widget->setLayoutDirection(Qt::RightToLeft);
    layout->activate();
    // Validate
    for (int j = 0; j < result.count(); ++j) {
        const BasicLayoutTestResult item = result[j];
        QRectF mirroredRect(item.rect);
        // only valid cases are mirrored
        if (mirroredRect.isValid()){
            mirroredRect.moveLeft(size.width()-item.rect.width()-item.rect.left());
        }
        QCOMPARE(widgets[item.index]->geometry(), mirroredRect);
        delete widgets[item.index];
    }

    delete widget;
}
#endif //TEST_COMPLEX_CASES


QTEST_MAIN(tst_QGraphicsAnchorLayout1)
#include "tst_qgraphicsanchorlayout1.moc"
//-----------------------------------------------------------------------------

