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
#include <qgraphicsitem.h>
#include <qpen.h>


class tst_QGraphicsPolygonItem : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void qgraphicspolygonitem_data();
    void qgraphicspolygonitem();
    void boundingRect_data();
    void boundingRect();
    void contains_data();
    void contains();
    void fillRule_data();
    void fillRule();
    void isObscuredBy_data();
    void isObscuredBy();
    void opaqueArea_data();
    void opaqueArea();
    void polygon_data();
    void polygon();
    void shape_data();
    void shape();
    void extension_data();
    void extension();
    void setExtension_data();
    void setExtension();
    void supportsExtension_data();
    void supportsExtension();
};

// Subclass that exposes the protected functions.
class SubQGraphicsPolygonItem : public QGraphicsPolygonItem
{
public:
    enum Extension {
        UserExtension = QGraphicsItem::UserExtension
    };

    SubQGraphicsPolygonItem(QGraphicsItem *parent = 0) : QGraphicsPolygonItem(parent)
    {
    }

    SubQGraphicsPolygonItem(const QPolygonF &polygon, QGraphicsItem *parent = 0) : QGraphicsPolygonItem(polygon, parent)
    {
    }

    QVariant call_extension(QVariant const& variant) const
        { return SubQGraphicsPolygonItem::extension(variant); }

    void call_setExtension(SubQGraphicsPolygonItem::Extension extension, QVariant const& variant)
        { return SubQGraphicsPolygonItem::setExtension((QGraphicsItem::Extension)extension, variant); }

    bool call_supportsExtension(SubQGraphicsPolygonItem::Extension extension) const
        { return SubQGraphicsPolygonItem::supportsExtension((QGraphicsItem::Extension)extension); }
};

// This will be called before the first test function is executed.
// It is only called once.
void tst_QGraphicsPolygonItem::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_QGraphicsPolygonItem::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_QGraphicsPolygonItem::init()
{
}

// This will be called after every test function.
void tst_QGraphicsPolygonItem::cleanup()
{
}

void tst_QGraphicsPolygonItem::qgraphicspolygonitem_data()
{
}

void tst_QGraphicsPolygonItem::qgraphicspolygonitem()
{
    SubQGraphicsPolygonItem item;

    item.boundingRect();
    item.contains(QPoint());
    item.isObscuredBy(0);
    item.opaqueArea();
    //item.paint();
    item.shape();
    item.type();
    item.call_extension(QVariant());
    item.call_setExtension(SubQGraphicsPolygonItem::UserExtension, QVariant());
    item.call_supportsExtension(SubQGraphicsPolygonItem::UserExtension);
    item.fillRule();
    item.polygon();
    item.setFillRule(Qt::OddEvenFill);
    item.setPolygon(QPolygonF());
}

void tst_QGraphicsPolygonItem::boundingRect_data()
{
    QTest::addColumn<QPolygonF>("polygon");
    QTest::addColumn<QRectF>("boundingRect");
    QTest::newRow("null") << QPolygonF() << QRectF();
    QPolygonF example;
    example << QPointF(10.4, 20.5) << QPointF(20.2, 30.2);
    QTest::newRow("example") << example << example.boundingRect();
    // ### set pen width?
}

// public QRectF boundingRect() const
void tst_QGraphicsPolygonItem::boundingRect()
{
    QFETCH(QPolygonF, polygon);
    QFETCH(QRectF, boundingRect);

    SubQGraphicsPolygonItem item(polygon);
    item.setPen(QPen(Qt::black, 0));
    QCOMPARE(item.boundingRect(), boundingRect);
}

void tst_QGraphicsPolygonItem::contains_data()
{
    QTest::addColumn<QPolygonF>("polygon");
    QTest::addColumn<QPointF>("point");
    QTest::addColumn<bool>("contains");
    QTest::newRow("null") << QPolygonF() << QPointF() << false;
}

// public bool contains(QPointF const& point) const
void tst_QGraphicsPolygonItem::contains()
{
    QFETCH(QPolygonF, polygon);
    QFETCH(QPointF, point);
    QFETCH(bool, contains);

    SubQGraphicsPolygonItem item(polygon);

    QCOMPARE(item.contains(point), contains);
}

Q_DECLARE_METATYPE(Qt::FillRule)
void tst_QGraphicsPolygonItem::fillRule_data()
{
    QTest::addColumn<QPolygonF>("polygon");
    QTest::addColumn<Qt::FillRule>("fillRule");
    QTest::newRow("OddEvenFill") << QPolygonF() << Qt::OddEvenFill;
    QTest::newRow("WindingFill") << QPolygonF() << Qt::WindingFill;
}

// public Qt::FillRule fillRule() const
void tst_QGraphicsPolygonItem::fillRule()
{
    QFETCH(QPolygonF, polygon);
    QFETCH(Qt::FillRule, fillRule);

    SubQGraphicsPolygonItem item(polygon);

    item.setFillRule(fillRule);
    QCOMPARE(item.fillRule(), fillRule);
    // ### Check that the painting is different?
}

void tst_QGraphicsPolygonItem::isObscuredBy_data()
{
    QTest::addColumn<QPolygonF>("polygon");
    QTest::addColumn<QPolygonF>("otherPolygon");
    QTest::addColumn<bool>("isObscuredBy");
    QTest::newRow("null") << QPolygonF() << QPolygonF() << false;
    //QTest::newRow("ontop-inside") << QPixmap(10, 10) << QPixmap(5, 5) << false;
    //QTest::newRow("ontop-larger") << QPixmap(10, 10) << QPixmap(11, 11) << true;
}

// public bool isObscuredBy(QGraphicsItem const* item) const
void tst_QGraphicsPolygonItem::isObscuredBy()
{
    QFETCH(QPolygonF, polygon);
    QFETCH(QPolygonF, otherPolygon);
    QFETCH(bool, isObscuredBy);
    SubQGraphicsPolygonItem item(polygon);
    SubQGraphicsPolygonItem otherItem(otherPolygon);
    QCOMPARE(item.isObscuredBy(&otherItem), isObscuredBy);
}

Q_DECLARE_METATYPE(QPainterPath)
void tst_QGraphicsPolygonItem::opaqueArea_data()
{
    QTest::addColumn<QPolygonF>("polygon");
    QTest::addColumn<QPainterPath>("opaqueArea");
    QTest::newRow("null") << QPolygonF() << QPainterPath();
    // Currently QGraphicsPolygonItem just calls QGraphicsItem test there
}

// public QPainterPath opaqueArea() const
void tst_QGraphicsPolygonItem::opaqueArea()
{
    QFETCH(QPolygonF, polygon);
    QFETCH(QPainterPath, opaqueArea);

    SubQGraphicsPolygonItem item(polygon);
    QCOMPARE(item.opaqueArea(), opaqueArea);
}

void tst_QGraphicsPolygonItem::polygon_data()
{
    QTest::addColumn<QPolygonF>("polygon");
    QTest::newRow("null") << QPolygonF();
    QPolygonF example;
    example << QPointF(10.4, 20.5) << QPointF(20.2, 30.2);
    QTest::newRow("example") << example;
}

// public QPolygonF polygon() const
void tst_QGraphicsPolygonItem::polygon()
{
    QFETCH(QPolygonF, polygon);

    SubQGraphicsPolygonItem item;
    item.setPolygon(polygon);
    QCOMPARE(item.polygon(), polygon);
}

void tst_QGraphicsPolygonItem::shape_data()
{
    QTest::addColumn<QPainterPath>("shape");
    QTest::newRow("null") << QPainterPath();
    // ### what should a normal shape look like?
}

// public QPainterPath shape() const
void tst_QGraphicsPolygonItem::shape()
{
    QFETCH(QPainterPath, shape);

    SubQGraphicsPolygonItem item;
    QCOMPARE(item.shape(), shape);
}

void tst_QGraphicsPolygonItem::extension_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QVariant>("extension");
    QTest::newRow("null") << QVariant() << QVariant();
}

// protected QVariant extension(QVariant const& variant) const
void tst_QGraphicsPolygonItem::extension()
{
    QFETCH(QVariant, variant);
    QFETCH(QVariant, extension);

    SubQGraphicsPolygonItem item;

    QCOMPARE(item.call_extension(variant), extension);
}

Q_DECLARE_METATYPE(SubQGraphicsPolygonItem::Extension)
void tst_QGraphicsPolygonItem::setExtension_data()
{
    QTest::addColumn<SubQGraphicsPolygonItem::Extension>("extension");
    QTest::addColumn<QVariant>("variant");
    QTest::newRow("null") << SubQGraphicsPolygonItem::Extension() << QVariant();
}

// protected void setExtension(SubQGraphicsPolygonItem::Extension extension, QVariant const& variant)
void tst_QGraphicsPolygonItem::setExtension()
{
    QFETCH(SubQGraphicsPolygonItem::Extension, extension);
    QFETCH(QVariant, variant);

    SubQGraphicsPolygonItem item;
    item.call_setExtension(extension, variant);
}

void tst_QGraphicsPolygonItem::supportsExtension_data()
{
    QTest::addColumn<SubQGraphicsPolygonItem::Extension>("extension");
    QTest::addColumn<bool>("supportsExtension");
    QTest::newRow("null") << SubQGraphicsPolygonItem::Extension() << false;
}

// protected bool supportsExtension(SubQGraphicsPolygonItem::Extension extension) const
void tst_QGraphicsPolygonItem::supportsExtension()
{
    QFETCH(SubQGraphicsPolygonItem::Extension, extension);
    QFETCH(bool, supportsExtension);

    SubQGraphicsPolygonItem item;
    QCOMPARE(item.call_supportsExtension(extension), supportsExtension);
}

QTEST_MAIN(tst_QGraphicsPolygonItem)
#include "tst_qgraphicspolygonitem.moc"

