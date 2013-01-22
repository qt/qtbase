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
#include <qgraphicsscene.h>
#include <qgraphicsitem.h>

class tst_QGraphicsPixmapItem : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void qgraphicspixmapitem_data();
    void qgraphicspixmapitem();
    void boundingRect_data();
    void boundingRect();
    void contains_data();
    void contains();
    void isObscuredBy_data();
    void isObscuredBy();
    void offset_data();
    void offset();
    void opaqueArea_data();
    void opaqueArea();
    void pixmap_data();
    void pixmap();
    void setPixmap_data();
    void setPixmap();
    void setShapeMode_data();
    void setShapeMode();
    void setTransformationMode_data();
    void setTransformationMode();
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
class SubQGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    enum Extension {
        UserExtension = QGraphicsItem::UserExtension
    };
    SubQGraphicsPixmapItem(QGraphicsItem *parent = 0) : QGraphicsPixmapItem(parent)
    {
    }

    SubQGraphicsPixmapItem(const QPixmap &pixmap, QGraphicsItem *parent = 0) : QGraphicsPixmapItem(pixmap, parent)
    {
    }

    QVariant call_extension(QVariant const& variant) const
        { return SubQGraphicsPixmapItem::extension(variant); }

    void call_setExtension(Extension extension, QVariant const& variant)
        { return SubQGraphicsPixmapItem::setExtension((QGraphicsItem::Extension)extension, variant); }

    bool call_supportsExtension(Extension extension) const
        { return SubQGraphicsPixmapItem::supportsExtension((QGraphicsItem::Extension)extension); }
};

// This will be called before the first test function is executed.
// It is only called once.
void tst_QGraphicsPixmapItem::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_QGraphicsPixmapItem::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_QGraphicsPixmapItem::init()
{
}

// This will be called after every test function.
void tst_QGraphicsPixmapItem::cleanup()
{
}

void tst_QGraphicsPixmapItem::qgraphicspixmapitem_data()
{
}

void tst_QGraphicsPixmapItem::qgraphicspixmapitem()
{
    SubQGraphicsPixmapItem item;
    item.boundingRect();
    item.contains(QPoint());
    item.isObscuredBy(0);
    item.opaqueArea();
    //item.paint();
    QCOMPARE(item.offset(), QPointF());
    QCOMPARE(item.pixmap(), QPixmap());
    QCOMPARE(item.shapeMode(), QGraphicsPixmapItem::MaskShape);
    QCOMPARE(item.transformationMode(), Qt::FastTransformation);
    item.setOffset(0, 0);
    item.setOffset(QPointF(0, 0));
    item.setPixmap(QPixmap());
    item.setShapeMode(QGraphicsPixmapItem::MaskShape);
    item.setTransformationMode(Qt::FastTransformation);
    item.shape();
    item.type();
    item.call_extension(QVariant());
    item.call_setExtension(SubQGraphicsPixmapItem::UserExtension, QVariant());
    item.call_supportsExtension(SubQGraphicsPixmapItem::UserExtension);
}

void tst_QGraphicsPixmapItem::boundingRect_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<QRectF>("boundingRect");
    QTest::newRow("null") << QPixmap() << QRectF();
    QTest::newRow("10x10") << QPixmap(10, 10) << QRectF(0, 0, 10, 10);
}

// public QRectF boundingRect() const
void tst_QGraphicsPixmapItem::boundingRect()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(QRectF, boundingRect);

    SubQGraphicsPixmapItem item(pixmap);
    QCOMPARE(item.boundingRect(), boundingRect);
}

void tst_QGraphicsPixmapItem::contains_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<QPointF>("point");
    QTest::addColumn<bool>("contains");
    QTest::newRow("null") << QPixmap() << QPointF() << false;
    QTest::newRow("10x10, 100x100") << QPixmap(10, 10) << QPointF(100, 100) << false;
    QTest::newRow("10x10, 5x5") << QPixmap(10, 10) << QPointF(5, 5) << true;
    QTest::newRow("border-1") << QPixmap(10, 10) << QPointF(10.5, 10.5) << false;
    QTest::newRow("border-2") << QPixmap(10, 10) << QPointF(-0.5, -0.5) << false;
}

// public bool contains(QPointF const& point) const
void tst_QGraphicsPixmapItem::contains()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(QPointF, point);
    QFETCH(bool, contains);

    SubQGraphicsPixmapItem item(pixmap);
    QCOMPARE(item.contains(point), contains);
}

void tst_QGraphicsPixmapItem::isObscuredBy_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<QPixmap>("otherPixmap");
    QTest::addColumn<bool>("isObscuredBy");
    QTest::newRow("null") << QPixmap() << QPixmap() << false;
    QTest::newRow("(10, 10) vs. (5, 5)") << QPixmap(10, 10) << QPixmap(5, 5) << false;
    QTest::newRow("(5, 5) vs. (10, 10)") << QPixmap(5, 5) << QPixmap(10, 10) << true;
    QTest::newRow("(10, 10) vs. (10, 10)") << QPixmap(10, 10) << QPixmap(10, 10) << false;
    QTest::newRow("(9, 9) vs. (10, 10)") << QPixmap(8, 8) << QPixmap(10, 10) << true;
    QTest::newRow("(10, 10) vs. (9, 9)") << QPixmap(10, 10) << QPixmap(8, 8) << false;
}

// public bool isObscuredBy(QGraphicsItem const* item) const
void tst_QGraphicsPixmapItem::isObscuredBy()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(QPixmap, otherPixmap);
    QFETCH(bool, isObscuredBy);
    pixmap.fill();
    otherPixmap.fill();

    SubQGraphicsPixmapItem *item = new SubQGraphicsPixmapItem(pixmap);
    SubQGraphicsPixmapItem *otherItem = new SubQGraphicsPixmapItem(otherPixmap);

    item->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    otherItem->setOffset(-otherPixmap.width() / 2.0, -otherPixmap.height() / 2.0);

    QGraphicsScene scene;
    scene.addItem(item);
    scene.addItem(otherItem);
    otherItem->setZValue(1);

    QCOMPARE(item->isObscuredBy(otherItem), isObscuredBy);
}

void tst_QGraphicsPixmapItem::offset_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<QPointF>("offset");
    QTest::newRow("null") << QPixmap() << QPointF();
    QTest::newRow("10x10, 1x1") << QPixmap(10, 10) << QPointF(1, 1);
}

// public QPointF offset() const
void tst_QGraphicsPixmapItem::offset()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(QPointF, offset);

    SubQGraphicsPixmapItem item(pixmap);
    item.setOffset(offset);
    QCOMPARE(item.offset(), offset);

    // ### test actual painting and compare pixmap with offseted one?
}

Q_DECLARE_METATYPE(QPainterPath)
void tst_QGraphicsPixmapItem::opaqueArea_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<QPainterPath>("opaqueArea");
    QTest::newRow("null") << QPixmap() << QPainterPath();
    // Currently QGraphicsPixmapItem just calls QGraphicsItem test there
}

// public QPainterPath opaqueArea() const
void tst_QGraphicsPixmapItem::opaqueArea()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(QPainterPath, opaqueArea);

    SubQGraphicsPixmapItem item;
    QCOMPARE(item.opaqueArea(), opaqueArea);
}

void tst_QGraphicsPixmapItem::pixmap_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::newRow("null") << QPixmap();
    QTest::newRow("10x10") << QPixmap(10, 10);
}

// public QPixmap pixmap() const
void tst_QGraphicsPixmapItem::pixmap()
{
    QFETCH(QPixmap, pixmap);

    SubQGraphicsPixmapItem item(pixmap);
    QCOMPARE(item.pixmap(), pixmap);
}

void tst_QGraphicsPixmapItem::setPixmap_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::newRow("null") << QPixmap();
    QTest::newRow("10x10") << QPixmap(10, 10);
}

// public void setPixmap(QPixmap const& pixmap)
void tst_QGraphicsPixmapItem::setPixmap()
{
    QFETCH(QPixmap, pixmap);

    SubQGraphicsPixmapItem item;
    item.setPixmap(pixmap);
    QCOMPARE(item.pixmap(), pixmap);
}

Q_DECLARE_METATYPE(QGraphicsPixmapItem::ShapeMode)
void tst_QGraphicsPixmapItem::setShapeMode_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<QGraphicsPixmapItem::ShapeMode>("mode");
    QTest::newRow("MaskShape") << QPixmap() << QGraphicsPixmapItem::MaskShape;
    QTest::newRow("BoundingRectShape") << QPixmap() << QGraphicsPixmapItem::BoundingRectShape;
    QTest::newRow("HeuristicMaskShape") << QPixmap() << QGraphicsPixmapItem::HeuristicMaskShape;
}

// public void setShapeMode(QGraphicsPixmapItem::ShapeMode mode)
void tst_QGraphicsPixmapItem::setShapeMode()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(QGraphicsPixmapItem::ShapeMode, mode);

    SubQGraphicsPixmapItem item(pixmap);
    item.setShapeMode(mode);
    QCOMPARE(item.shapeMode(), mode);
}

Q_DECLARE_METATYPE(Qt::TransformationMode)
void tst_QGraphicsPixmapItem::setTransformationMode_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<Qt::TransformationMode>("mode");
    QTest::newRow("FastTransformation") << QPixmap() << Qt::FastTransformation;
    QTest::newRow("SmoothTransformation") << QPixmap() << Qt::SmoothTransformation;
}

// public void setTransformationMode(Qt::TransformationMode mode)
void tst_QGraphicsPixmapItem::setTransformationMode()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(Qt::TransformationMode, mode);

    SubQGraphicsPixmapItem item(pixmap);
    item.setTransformationMode(mode);
    QCOMPARE(item.transformationMode(), mode);
}

void tst_QGraphicsPixmapItem::shape_data()
{
    QTest::addColumn<QPixmap>("pixmap");
    QTest::addColumn<QPainterPath>("shape");
    QTest::newRow("null") << QPixmap() << QPainterPath();
    // ### what does a normal shape look like?
}

// public QPainterPath shape() const
void tst_QGraphicsPixmapItem::shape()
{
    QFETCH(QPixmap, pixmap);
    QFETCH(QPainterPath, shape);

    SubQGraphicsPixmapItem item(pixmap);
    QCOMPARE(item.shape(), shape);
}

Q_DECLARE_METATYPE(SubQGraphicsPixmapItem::Extension)
void tst_QGraphicsPixmapItem::extension_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QVariant>("extension");
    QTest::newRow("null") << QVariant() << QVariant();
}

// protected QVariant extension(QVariant const& variant) const
void tst_QGraphicsPixmapItem::extension()
{
    QFETCH(QVariant, variant);
    QFETCH(QVariant, extension);

    SubQGraphicsPixmapItem item;
    QCOMPARE(item.call_extension(variant), extension);
}

void tst_QGraphicsPixmapItem::setExtension_data()
{
    QTest::addColumn<SubQGraphicsPixmapItem::Extension>("extension");
    QTest::addColumn<QVariant>("variant");
    QTest::newRow("null") << SubQGraphicsPixmapItem::UserExtension << QVariant();
}

// protected void setExtension(QGraphicsItem::Extension extension, QVariant const& variant)
void tst_QGraphicsPixmapItem::setExtension()
{
    QFETCH(SubQGraphicsPixmapItem::Extension, extension);
    QFETCH(QVariant, variant);

    SubQGraphicsPixmapItem item;
    item.call_setExtension(extension, variant);
}

void tst_QGraphicsPixmapItem::supportsExtension_data()
{
    QTest::addColumn<SubQGraphicsPixmapItem::Extension>("extension");
    QTest::addColumn<bool>("supportsExtension");
    QTest::newRow("null") << SubQGraphicsPixmapItem::UserExtension << false;
}

// protected bool supportsExtension(QGraphicsItem::Extension extension) const
void tst_QGraphicsPixmapItem::supportsExtension()
{
    QFETCH(SubQGraphicsPixmapItem::Extension, extension);
    QFETCH(bool, supportsExtension);

    SubQGraphicsPixmapItem item;
    QCOMPARE(item.call_supportsExtension(extension), supportsExtension);
}

QTEST_MAIN(tst_QGraphicsPixmapItem)
#include "tst_qgraphicspixmapitem.moc"

