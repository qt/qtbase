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
#include <qgraphicsitem.h>
#include <qgraphicsscene.h>
#include <qgraphicssceneevent.h>
#include <qgraphicsview.h>
#include <qstyleoption.h>
#include <private/qobject_p.h>

class tst_QGraphicsObject : public QObject {
    Q_OBJECT

private slots:
    void pos();
    void x();
    void y();
    void z();
    void opacity();
    void enabled();
    void visible();
    void deleted();
};

class MyGraphicsObject : public QGraphicsObject
{
public:
    MyGraphicsObject() : QGraphicsObject() {}
    virtual QRectF boundingRect() const { return QRectF(); }
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
};

void tst_QGraphicsObject::pos()
{
    MyGraphicsObject object;
    QSignalSpy xSpy(&object, SIGNAL(xChanged()));
    QSignalSpy ySpy(&object, SIGNAL(yChanged()));
    QVERIFY(object.pos() == QPointF(0, 0));
    object.setPos(10, 10);
    QCOMPARE(xSpy.count(), 1);
    QCOMPARE(ySpy.count(), 1);

    QCOMPARE(object.pos(), QPointF(10,10));

    object.setPos(10, 10);
    QCOMPARE(xSpy.count(), 1);
    QCOMPARE(ySpy.count(), 1);

    object.setProperty("pos", QPointF(0, 0));
    QCOMPARE(xSpy.count(), 2);
    QCOMPARE(ySpy.count(), 2);
    QCOMPARE(object.property("pos").toPointF(), QPointF(0,0));

    object.setProperty("pos", QPointF(10, 0));
    QCOMPARE(xSpy.count(), 3);
    QCOMPARE(ySpy.count(), 2);
    QCOMPARE(object.property("pos").toPointF(), QPointF(10,0));

    object.setProperty("pos", QPointF(10, 10));
    QCOMPARE(xSpy.count(), 3);
    QCOMPARE(ySpy.count(), 3);
    QVERIFY(object.property("pos") == QPointF(10, 10));
}

void tst_QGraphicsObject::x()
{
    MyGraphicsObject object;
    QSignalSpy xSpy(&object, SIGNAL(xChanged()));
    QSignalSpy ySpy(&object, SIGNAL(yChanged()));
    QVERIFY(object.pos() == QPointF(0, 0));
    object.setX(10);
    QCOMPARE(xSpy.count(), 1);
    QCOMPARE(ySpy.count(), 0);

    QVERIFY(object.pos() == QPointF(10, 0));
    QCOMPARE(object.x(), qreal(10));

    object.setX(10);
    QCOMPARE(xSpy.count(), 1);
    QCOMPARE(ySpy.count(), 0);

    object.setProperty("x", 0);
    QCOMPARE(xSpy.count(), 2);
    QCOMPARE(ySpy.count(), 0);
    QCOMPARE(object.property("x").toDouble(), double(0));
}

void tst_QGraphicsObject::y()
{
    MyGraphicsObject object;
    QSignalSpy xSpy(&object, SIGNAL(xChanged()));
    QSignalSpy ySpy(&object, SIGNAL(yChanged()));
    QVERIFY(object.pos() == QPointF(0, 0));
    object.setY(10);
    QCOMPARE(xSpy.count(), 0);
    QCOMPARE(ySpy.count(), 1);

    QVERIFY(object.pos() == QPointF(0, 10));
    QCOMPARE(object.y(), qreal(10));

    object.setY(10);
    QCOMPARE(xSpy.count(), 0);
    QCOMPARE(ySpy.count(), 1);

    object.setProperty("y", 0);
    QCOMPARE(xSpy.count(), 0);
    QCOMPARE(ySpy.count(), 2);
    QCOMPARE(object.property("y").toDouble(), qreal(0));
}

void tst_QGraphicsObject::z()
{
    MyGraphicsObject object;
    QSignalSpy zSpy(&object, SIGNAL(zChanged()));
    QCOMPARE(object.zValue(), qreal(0));
    object.setZValue(10);
    QCOMPARE(zSpy.count(), 1);

    QCOMPARE(object.zValue(), qreal(10));

    object.setZValue(10);
    QCOMPARE(zSpy.count(), 1);

    object.setProperty("z", 0);
    QCOMPARE(zSpy.count(), 2);
    QCOMPARE(object.property("z").toDouble(), double(0));
}

void tst_QGraphicsObject::opacity()
{
    MyGraphicsObject object;
    QSignalSpy spy(&object, SIGNAL(opacityChanged()));
    QCOMPARE(object.opacity(), 1.);
    object.setOpacity(0);
    QCOMPARE(spy.count(), 1);

    QCOMPARE(object.opacity(), 0.);

    object.setOpacity(0);
    QCOMPARE(spy.count(), 1);

    object.setProperty("opacity", .5);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(object.property("opacity").toDouble(), .5);
}

void tst_QGraphicsObject::enabled()
{
    MyGraphicsObject object;
    QSignalSpy spy(&object, SIGNAL(enabledChanged()));
    QVERIFY(object.isEnabled());
    object.setEnabled(false);
    QCOMPARE(spy.count(), 1);

    QVERIFY(!object.isEnabled());

    object.setEnabled(false);
    QCOMPARE(spy.count(), 1);

    object.setProperty("enabled", true);
    QCOMPARE(spy.count(), 2);
    QVERIFY(object.property("enabled").toBool());
}

void tst_QGraphicsObject::visible()
{
    MyGraphicsObject object;
    QSignalSpy spy(&object, SIGNAL(visibleChanged()));
    QVERIFY(object.isVisible());
    object.setVisible(false);
    QCOMPARE(spy.count(), 1);

    QVERIFY(!object.isVisible());

    object.setVisible(false);
    QCOMPARE(spy.count(), 1);

    object.setProperty("visible", true);
    QCOMPARE(spy.count(), 2);
    QVERIFY(object.property("visible").toBool());
}

class DeleteTester : public QGraphicsObject
{
public:
    DeleteTester(bool *w, bool *pw, QGraphicsItem *parent = 0)
        : QGraphicsObject(parent), wasDeleted(w), parentWasDeleted(pw)
    { }

    ~DeleteTester()
    {
        *wasDeleted = QObjectPrivate::get(this)->wasDeleted;
        if (QGraphicsItem *p = parentItem()) {
            if (QGraphicsObject *o = p->toGraphicsObject())
                *parentWasDeleted = QObjectPrivate::get(o)->wasDeleted;
        }
    }

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0)
    { }
    QRectF boundingRect() const
    { return QRectF(); }

    bool *wasDeleted;
    bool *parentWasDeleted;
};

void tst_QGraphicsObject::deleted()
{
    bool item1_parentWasDeleted = false;
    bool item1_wasDeleted = false;
    bool item2_parentWasDeleted = false;
    bool item2_wasDeleted = false;
    DeleteTester *item1 = new DeleteTester(&item1_wasDeleted, &item1_parentWasDeleted);
    DeleteTester *item2 = new DeleteTester(&item2_wasDeleted, &item2_parentWasDeleted, item1);
    Q_UNUSED(item2);
    delete item1;

    QVERIFY(!item1_wasDeleted); // destructor not called yet
    QVERIFY(!item1_parentWasDeleted); // no parent
    QVERIFY(!item2_wasDeleted); // destructor not called yet
    QVERIFY(item2_parentWasDeleted);
}

QTEST_MAIN(tst_QGraphicsObject)
#include "tst_qgraphicsobject.moc"

