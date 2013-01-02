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
#include <qgraphicsscene.h>
#include <qgraphicssceneevent.h>
#include <qgraphicsview.h>
#include <qstyleoption.h>
#include <private/qobject_p.h>

class tst_QGraphicsObject : public QObject {
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

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


// This will be called before the first test function is executed.
// It is only called once.
void tst_QGraphicsObject::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_QGraphicsObject::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_QGraphicsObject::init()
{
}

// This will be called after every test function.
void tst_QGraphicsObject::cleanup()
{
}


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

    QVERIFY(object.pos() == QPointF(10,10));

    object.setPos(10, 10);
    QCOMPARE(xSpy.count(), 1);
    QCOMPARE(ySpy.count(), 1);

    object.setProperty("pos", QPointF(0, 0));
    QCOMPARE(xSpy.count(), 2);
    QCOMPARE(ySpy.count(), 2);
    QVERIFY(object.property("pos") == QPointF(0,0));

    object.setProperty("pos", QPointF(10, 0));
    QCOMPARE(xSpy.count(), 3);
    QCOMPARE(ySpy.count(), 2);
    QVERIFY(object.property("pos") == QPointF(10,0));

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
    QVERIFY(object.x() == 10);

    object.setX(10);
    QCOMPARE(xSpy.count(), 1);
    QCOMPARE(ySpy.count(), 0);

    object.setProperty("x", 0);
    QCOMPARE(xSpy.count(), 2);
    QCOMPARE(ySpy.count(), 0);
    QVERIFY(object.property("x") == 0);
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
    QVERIFY(object.y() == 10);

    object.setY(10);
    QCOMPARE(xSpy.count(), 0);
    QCOMPARE(ySpy.count(), 1);

    object.setProperty("y", 0);
    QCOMPARE(xSpy.count(), 0);
    QCOMPARE(ySpy.count(), 2);
    QVERIFY(object.property("y") == 0);
}

void tst_QGraphicsObject::z()
{
    MyGraphicsObject object;
    QSignalSpy zSpy(&object, SIGNAL(zChanged()));
    QVERIFY(object.zValue() == 0);
    object.setZValue(10);
    QCOMPARE(zSpy.count(), 1);

    QVERIFY(object.zValue() == 10);

    object.setZValue(10);
    QCOMPARE(zSpy.count(), 1);

    object.setProperty("z", 0);
    QCOMPARE(zSpy.count(), 2);
    QVERIFY(object.property("z") == 0);
}

void tst_QGraphicsObject::opacity()
{
    MyGraphicsObject object;
    QSignalSpy spy(&object, SIGNAL(opacityChanged()));
    QVERIFY(object.opacity() == 1.);
    object.setOpacity(0);
    QCOMPARE(spy.count(), 1);

    QVERIFY(object.opacity() == 0.);

    object.setOpacity(0);
    QCOMPARE(spy.count(), 1);

    object.setProperty("opacity", .5);
    QCOMPARE(spy.count(), 2);
    QVERIFY(object.property("opacity") == .5);
}

void tst_QGraphicsObject::enabled()
{
    MyGraphicsObject object;
    QSignalSpy spy(&object, SIGNAL(enabledChanged()));
    QVERIFY(object.isEnabled() == true);
    object.setEnabled(false);
    QCOMPARE(spy.count(), 1);

    QVERIFY(object.isEnabled() == false);

    object.setEnabled(false);
    QCOMPARE(spy.count(), 1);

    object.setProperty("enabled", true);
    QCOMPARE(spy.count(), 2);
    QVERIFY(object.property("enabled") == true);
}

void tst_QGraphicsObject::visible()
{
    MyGraphicsObject object;
    QSignalSpy spy(&object, SIGNAL(visibleChanged()));
    QVERIFY(object.isVisible() == true);
    object.setVisible(false);
    QCOMPARE(spy.count(), 1);

    QVERIFY(object.isVisible() == false);

    object.setVisible(false);
    QCOMPARE(spy.count(), 1);

    object.setProperty("visible", true);
    QCOMPARE(spy.count(), 2);
    QVERIFY(object.property("visible") == true);
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

