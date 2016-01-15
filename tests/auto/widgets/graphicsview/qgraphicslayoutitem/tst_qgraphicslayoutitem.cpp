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


#include <QtTest/QtTest>
#include <qgraphicslayoutitem.h>
#include <float.h>
#include <limits.h>

class tst_QGraphicsLayoutItem : public QObject {
Q_OBJECT

private slots:
    void qgraphicslayoutitem();

    void contentsRect();
    void effectiveSizeHint_data();
    void effectiveSizeHint();
    void getContentsMargins();
    void isLayout_data();
    void isLayout();
    void maximumSize();
    void minimumSize();
    void parentLayoutItem_data();
    void parentLayoutItem();
    void preferredSize();
    void setMaximumSize_data();
    void setMaximumSize();
    void setMinimumSize_data();
    void setMinimumSize();
    void setPreferredSize_data();
    void setPreferredSize();
    void setSizePolicy_data();
    void setPreferredSize2();
    void setSizePolicy();
};

// Subclass that exposes the protected functions.
class SubQGraphicsLayoutItem : public QGraphicsLayoutItem {
public:
    SubQGraphicsLayoutItem(QGraphicsLayoutItem *par = 0, bool layout = false)
            : QGraphicsLayoutItem(par, layout), updateGeometryCalled(0)
        {}

    // QGraphicsLayoutItem::geometry is a pure virtual function
    QRectF geometry() const
        { return QRectF(); }

    // QGraphicsLayoutItem::setGeometry is a pure virtual function
    void setGeometry(QRectF const& rect)
        { Q_UNUSED(rect); }

    // QGraphicsLayoutItem::sizeHint is a pure virtual function
    QSizeF sizeHint(Qt::SizeHint which, QSizeF const& constraint = QSizeF()) const
        { Q_UNUSED(which); Q_UNUSED(constraint); return QSizeF(); }

    void updateGeometry()
        { updateGeometryCalled++; QGraphicsLayoutItem::updateGeometry(); }
    int updateGeometryCalled;

};

void tst_QGraphicsLayoutItem::qgraphicslayoutitem()
{
    SubQGraphicsLayoutItem layoutItem;
    layoutItem.contentsRect();
    layoutItem.effectiveSizeHint(Qt::MinimumSize);
    layoutItem.geometry();
    QCOMPARE(layoutItem.isLayout(), false);
    layoutItem.maximumSize();
    layoutItem.minimumSize();
    QCOMPARE(layoutItem.parentLayoutItem(), (QGraphicsLayoutItem*)0);
    layoutItem.preferredSize();
    layoutItem.sizePolicy();
    layoutItem.sizeHint(Qt::MinimumSize);
}

// QRectF contentsRect() const public
void tst_QGraphicsLayoutItem::contentsRect()
{
    SubQGraphicsLayoutItem layoutItem;
    QRectF f = layoutItem.contentsRect();
    QCOMPARE(f, QRectF(QPoint(), QSizeF(0, 0)));
}
Q_DECLARE_METATYPE(Qt::SizeHint)
void tst_QGraphicsLayoutItem::effectiveSizeHint_data()
{
    QTest::addColumn<Qt::SizeHint>("sizeHint");
    QTest::addColumn<QSizeF>("constraint");
    for (int i = 0; i < 15; ++i) {
        QTestData &data = QTest::newRow(QByteArray::number(i).constData());
        switch(i % 5) {
        case 0: data << Qt::MinimumSize; break;
        case 1: data << Qt::PreferredSize; break;
        case 2: data << Qt::MaximumSize; break;
        case 3: data << Qt::MinimumDescent; break;
        case 4: data << Qt::NSizeHints; break;
        }
        switch(i % 3) {
        case 0: data << QSizeF(-1, -1); break;
        case 1: data << QSizeF(0, 0); break;
        case 2: data << QSizeF(10, 10); break;
        }
    }
}

// QSizeF effectiveSizeHint(Qt::SizeHint which, QSizeF const& constraint = QSize()) const public
void tst_QGraphicsLayoutItem::effectiveSizeHint()
{
    QFETCH(Qt::SizeHint, sizeHint);
    QFETCH(QSizeF, constraint);
    SubQGraphicsLayoutItem layoutItem;
    QSizeF r = layoutItem.effectiveSizeHint(sizeHint, constraint);
    if (constraint.width() != -1)
        QCOMPARE(r.width(), constraint.width());
    if (constraint.height() != -1)
        QCOMPARE(r.height(), constraint.height());
}

// void getContentsMargins(qreal* left, qreal* top, qreal* right, qreal* bottom) const public
void tst_QGraphicsLayoutItem::getContentsMargins()
{
    SubQGraphicsLayoutItem layoutItem;
    qreal left;
    qreal top;
    qreal right;
    qreal bottom;
    layoutItem.getContentsMargins(&left, &top, &right, &bottom);
    QCOMPARE(left, (qreal)0);
    QCOMPARE(top, (qreal)0);
    QCOMPARE(right, (qreal)0);
    QCOMPARE(bottom, (qreal)0);
}

void tst_QGraphicsLayoutItem::isLayout_data()
{
    QTest::addColumn<bool>("isLayout");
    QTest::newRow("no") << false;
    QTest::newRow("yes") << true;
}

// bool isLayout() const public
void tst_QGraphicsLayoutItem::isLayout()
{
    QFETCH(bool, isLayout);
    SubQGraphicsLayoutItem layoutItem(0, isLayout);
    QCOMPARE(layoutItem.isLayout(), isLayout);
}

// QSizeF maximumSize() const public
void tst_QGraphicsLayoutItem::maximumSize()
{
    SubQGraphicsLayoutItem layoutItem;
    QCOMPARE(layoutItem.maximumSize(), QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    // layoutItem.effectiveSizeHint(Qt::MaximumSize);
}

// QSizeF minimumSize() const public
void tst_QGraphicsLayoutItem::minimumSize()
{
    SubQGraphicsLayoutItem layoutItem;
    QCOMPARE(layoutItem.minimumSize(), QSizeF(0, 0));
    // layoutItem.effectiveSizeHint(Qt::MinimumSize);
}

void tst_QGraphicsLayoutItem::parentLayoutItem_data()
{
    QTest::addColumn<bool>("parent");
    QTest::newRow("no") << false;
    QTest::newRow("yes") << true;
}

// QGraphicsLayoutItem* parentLayoutItem() const public
void tst_QGraphicsLayoutItem::parentLayoutItem()
{
    QFETCH(bool, parent);
    SubQGraphicsLayoutItem parentLayout;
    SubQGraphicsLayoutItem layoutItem(parent ? &parentLayout : 0);
    QCOMPARE(layoutItem.parentLayoutItem(), static_cast<QGraphicsLayoutItem*>( parent ? &parentLayout : 0));
}

// QSizeF preferredSize() const public
void tst_QGraphicsLayoutItem::preferredSize()
{
    SubQGraphicsLayoutItem layoutItem;
    QCOMPARE(layoutItem.preferredSize(), QSizeF(0, 0));
    // layoutItem.effectiveSizeHint(Qt::PreferredSize));
}

void tst_QGraphicsLayoutItem::setMaximumSize_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<QSizeF>("outputSize");
    QTest::newRow("-1") << QSizeF(-1, -1) << QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    QTest::newRow("0") << QSizeF(0, 0) << QSizeF(0, 0);
    QTest::newRow("10") << QSizeF(10, 10) << QSizeF(10, 10);
}

// void setMaximumSize(QSizeF const& size) public
void tst_QGraphicsLayoutItem::setMaximumSize()
{
    QFETCH(QSizeF, size);
    QFETCH(QSizeF, outputSize);
    SubQGraphicsLayoutItem layoutItem;
    QSizeF oldSize = layoutItem.maximumSize();
    layoutItem.setMaximumSize(size);
    if (size.isValid())
        QCOMPARE(layoutItem.updateGeometryCalled, (oldSize == size) ? 0 : 1);
    else
        QVERIFY(!layoutItem.updateGeometryCalled);
    layoutItem.setMinimumSize(1, 1);

    QVERIFY(layoutItem.maximumSize().width() <= outputSize.width());
    QVERIFY(layoutItem.maximumSize().height() <= outputSize.height());
    QVERIFY(layoutItem.minimumSize().width() <= outputSize.width());
    QVERIFY(layoutItem.minimumSize().height() <= outputSize.height());
    QVERIFY(layoutItem.preferredSize().width() <= outputSize.width());
    QVERIFY(layoutItem.preferredSize().height() <= outputSize.height());
}

void tst_QGraphicsLayoutItem::setMinimumSize_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::newRow("-1") << QSizeF(-1, -1);
    QTest::newRow("0") << QSizeF(0, 0);
    QTest::newRow("10") << QSizeF(10, 10);
}

// void setMinimumSize(QSizeF const& size) public
void tst_QGraphicsLayoutItem::setMinimumSize()
{
    QFETCH(QSizeF, size);
    SubQGraphicsLayoutItem layoutItem;
    QSizeF oldSize = layoutItem.minimumSize();

    layoutItem.setMinimumSize(size);
    if (size.isValid()) {
        QEXPECT_FAIL("0", "updateGeometry() is called when it doesn't have to be.", Continue);
        QCOMPARE(layoutItem.updateGeometryCalled, (oldSize == size) ? 0 : 1);
    } else {
        QVERIFY(!layoutItem.updateGeometryCalled);
    }
    layoutItem.setMaximumSize(5, 5);
    QEXPECT_FAIL("10", "layoutItem.maximumSize().width() < size.width()", Abort);
    QVERIFY(layoutItem.maximumSize().width() >= size.width());
    QVERIFY(layoutItem.maximumSize().height() >= size.height());
    QVERIFY(layoutItem.minimumSize().width() >= size.width());
    QVERIFY(layoutItem.minimumSize().height() >= size.height());
    QVERIFY(layoutItem.preferredSize().width() >= size.width());
    QVERIFY(layoutItem.preferredSize().height() >= size.height());
}

void tst_QGraphicsLayoutItem::setPreferredSize_data()
{
    QTest::addColumn<QSizeF>("size");
    QTest::newRow("-1") << QSizeF(-1, -1);
    QTest::newRow("0") << QSizeF(0, 0);
    QTest::newRow("10") << QSizeF(10, 10);
}

// void setPreferredSize(QSizeF const& size) public
void tst_QGraphicsLayoutItem::setPreferredSize()
{
    QFETCH(QSizeF, size);
    SubQGraphicsLayoutItem layoutItem;
    QSizeF oldSize = layoutItem.preferredSize();
    layoutItem.setPreferredSize(size);
    if (size.isValid())
       QCOMPARE(layoutItem.preferredSize(), size);

    if (size.isValid()) {
        QEXPECT_FAIL("0", "updateGeometry() is called when it doesn't have to be.", Continue);
        QCOMPARE(layoutItem.updateGeometryCalled, (oldSize == size) ? 0 : 1);
    } else {
        QVERIFY(!layoutItem.updateGeometryCalled);
    }
}

void tst_QGraphicsLayoutItem::setPreferredSize2()
{
    SubQGraphicsLayoutItem layoutItem;
    layoutItem.setPreferredSize(QSizeF(30, -1));
    QCOMPARE(layoutItem.preferredWidth(), qreal(30));
}

void tst_QGraphicsLayoutItem::setSizePolicy_data()
{
    QTest::addColumn<QSizePolicy>("policy");
    QTest::newRow("default") << QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed, QSizePolicy::DefaultType);
    QTest::newRow("rand") << QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
}

// void setSizePolicy(QSizePolicy const& policy) public
void tst_QGraphicsLayoutItem::setSizePolicy()
{
    QFETCH(QSizePolicy, policy);
    SubQGraphicsLayoutItem layoutItem;
    QSizePolicy defaultPolicy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::DefaultType);
    QCOMPARE(layoutItem.sizePolicy(), defaultPolicy);

    layoutItem.setSizePolicy(policy);
    QCOMPARE(layoutItem.sizePolicy(), policy);
    QCOMPARE(layoutItem.updateGeometryCalled, (defaultPolicy == policy) ? 0 : 1);
}

QTEST_MAIN(tst_QGraphicsLayoutItem)
#include "tst_qgraphicslayoutitem.moc"

