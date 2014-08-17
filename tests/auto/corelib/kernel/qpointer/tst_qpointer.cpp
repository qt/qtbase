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

#include <QtTest/QtTest>

#include <QPointer>
#ifndef QT_NO_WIDGETS
#include <QWidget>
#endif

class tst_QPointer : public QObject
{
    Q_OBJECT
public:
    inline tst_QPointer *me() const
    { return const_cast<tst_QPointer *>(this); }

private slots:
    void constructors();
    void destructor();
    void assignment_operators();
    void equality_operators();
    void swap();
    void isNull();
    void dereference_operators();
    void disconnect();
    void castDuringDestruction();
    void threadSafety();

    void qvariantCast();
    void constPointer();
};

void tst_QPointer::constructors()
{
    QPointer<QObject> p1;
    QPointer<QObject> p2(this);
    QPointer<QObject> p3(p2);
    QCOMPARE(p1, QPointer<QObject>(0));
    QCOMPARE(p2, QPointer<QObject>(this));
    QCOMPARE(p3, QPointer<QObject>(this));
}

void tst_QPointer::destructor()
{
    // Make two QPointer's to the same object
    QObject *object = new QObject;
    QPointer<QObject> p1 = object;
    QPointer<QObject> p2 = object;
    // Check that they point to the correct object
    QCOMPARE(p1, QPointer<QObject>(object));
    QCOMPARE(p2, QPointer<QObject>(object));
    QCOMPARE(p1, p2);
    // Destroy the guarded object
    delete object;
    // Check that both pointers were zeroed
    QCOMPARE(p1, QPointer<QObject>(0));
    QCOMPARE(p2, QPointer<QObject>(0));
    QCOMPARE(p1, p2);
}

void tst_QPointer::assignment_operators()
{
    QPointer<QObject> p1;
    QPointer<QObject> p2;

    // Test assignment with a QObject-derived object pointer
    p1 = this;
    p2 = p1;
    QCOMPARE(p1, QPointer<QObject>(this));
    QCOMPARE(p2, QPointer<QObject>(this));
    QCOMPARE(p1, QPointer<QObject>(p2));

    // Test assignment with a null pointer
    p1 = 0;
    p2 = p1;
    QCOMPARE(p1, QPointer<QObject>(0));
    QCOMPARE(p2, QPointer<QObject>(0));
    QCOMPARE(p1, QPointer<QObject>(p2));

    // Test assignment with a real QObject pointer
    QObject *object = new QObject;

    p1 = object;
    p2 = p1;
    QCOMPARE(p1, QPointer<QObject>(object));
    QCOMPARE(p2, QPointer<QObject>(object));
    QCOMPARE(p1, QPointer<QObject>(p2));

    // Test assignment with the same pointer that's already guarded
    p1 = object;
    p2 = p1;
    QCOMPARE(p1, QPointer<QObject>(object));
    QCOMPARE(p2, QPointer<QObject>(object));
    QCOMPARE(p1, QPointer<QObject>(p2));

    // Cleanup
    delete object;
}

void tst_QPointer::equality_operators()
{
    QPointer<QObject> p1;
    QPointer<QObject> p2;

    QVERIFY(p1 == p2);

    QObject *object = 0;
#ifndef QT_NO_WIDGETS
    QWidget *widget = 0;
#endif

    p1 = object;
    QVERIFY(p1 == p2);
    QVERIFY(p1 == object);
    p2 = object;
    QVERIFY(p2 == p1);
    QVERIFY(p2 == object);

    p1 = this;
    QVERIFY(p1 != p2);
    p2 = p1;
    QVERIFY(p1 == p2);

    // compare to zero
    p1 = 0;
    QVERIFY(p1 == 0);
    QVERIFY(0 == p1);
    QVERIFY(p2 != 0);
    QVERIFY(0 != p2);
    QVERIFY(p1 == object);
    QVERIFY(object == p1);
    QVERIFY(p2 != object);
    QVERIFY(object != p2);
#ifndef QT_NO_WIDGETS
    QVERIFY(p1 == widget);
    QVERIFY(widget == p1);
    QVERIFY(p2 != widget);
    QVERIFY(widget != p2);
#endif
}

void tst_QPointer::swap()
{
    QPointer<QObject> c1, c2;
    {
        QObject o;
        c1 = &o;
        QVERIFY(c2.isNull());
        QCOMPARE(c1.data(), &o);
        c1.swap(c2);
        QVERIFY(c1.isNull());
        QCOMPARE(c2.data(), &o);
    }
    QVERIFY(c1.isNull());
    QVERIFY(c2.isNull());
}

void tst_QPointer::isNull()
{
    QPointer<QObject> p1;
    QVERIFY(p1.isNull());
    p1 = this;
    QVERIFY(!p1.isNull());
    p1 = 0;
    QVERIFY(p1.isNull());
}

void tst_QPointer::dereference_operators()
{
    QPointer<tst_QPointer> p1 = this;
    QPointer<tst_QPointer> p2;

    // operator->() -- only makes sense if not null
    QObject *object = p1->me();
    QCOMPARE(object, this);

    // operator*() -- only makes sense if not null
    QObject &ref = *p1;
    QCOMPARE(&ref, this);

    // operator T*()
    QCOMPARE(static_cast<QObject *>(p1), this);
    QCOMPARE(static_cast<QObject *>(p2), static_cast<QObject *>(0));

    // data()
    QCOMPARE(p1.data(), this);
    QCOMPARE(p2.data(), static_cast<QObject *>(0));
}

void tst_QPointer::disconnect()
{
    // Verify that pointer remains guarded when all signals are disconencted.
    QPointer<QObject> p1 = new QObject;
    QVERIFY(!p1.isNull());
    p1->disconnect();
    QVERIFY(!p1.isNull());
    delete static_cast<QObject *>(p1);
    QVERIFY(p1.isNull());
}

class ChildObject : public QObject
{
    QPointer<QObject> guardedPointer;

public:
    ChildObject(QObject *parent)
        : QObject(parent), guardedPointer(parent)
    { }
    ~ChildObject();
};

ChildObject::~ChildObject()
{
    QCOMPARE(static_cast<QObject *>(guardedPointer), static_cast<QObject *>(0));
    QCOMPARE(qobject_cast<QObject *>(guardedPointer), static_cast<QObject *>(0));
}

#ifndef QT_NO_WIDGETS
class ChildWidget : public QWidget
{
    QPointer<QWidget> guardedPointer;

public:
    ChildWidget(QWidget *parent)
        : QWidget(parent), guardedPointer(parent)
    { }
    ~ChildWidget();
};

ChildWidget::~ChildWidget()
{
    QCOMPARE(static_cast<QWidget *>(guardedPointer), parentWidget());
    QCOMPARE(qobject_cast<QWidget *>(guardedPointer), parentWidget());
}
#endif

class DerivedChild;

class DerivedParent : public QObject
{
    Q_OBJECT

    DerivedChild *derivedChild;

public:
    DerivedParent();
    ~DerivedParent();
};

class DerivedChild : public QObject
{
    Q_OBJECT

    DerivedParent *parentPointer;
    QPointer<DerivedParent> guardedParentPointer;

public:
    DerivedChild(DerivedParent *parent)
        : QObject(parent), parentPointer(parent), guardedParentPointer(parent)
    { }
    ~DerivedChild();
};

DerivedParent::DerivedParent()
    : QObject()
{
    derivedChild = new DerivedChild(this);
}

DerivedParent::~DerivedParent()
{
    delete derivedChild;
}

DerivedChild::~DerivedChild()
{
    QCOMPARE(static_cast<DerivedParent *>(guardedParentPointer), parentPointer);
    QCOMPARE(qobject_cast<DerivedParent *>(guardedParentPointer), parentPointer);
}

void tst_QPointer::castDuringDestruction()
{
    {
        QObject *parentObject = new QObject();
        (void) new ChildObject(parentObject);
        delete parentObject;
    }

#ifndef QT_NO_WIDGETS
    {
        QWidget *parentWidget = new QWidget();
        (void) new ChildWidget(parentWidget);
        delete parentWidget;
    }
#endif

    {
        delete new DerivedParent();
    }
}

class TestRunnable : public QObject, public QRunnable {
    void run() {
        QPointer<QObject> obj1 = new QObject;
        QPointer<QObject> obj2 = new QObject;
        obj1->moveToThread(thread()); // this is the owner thread
        obj1->deleteLater(); // the delete will happen in the owner thread
        obj2->moveToThread(thread()); // this is the owner thread
        obj2->deleteLater(); // the delete will happen in the owner thread
    }
};

void tst_QPointer::threadSafety()
{

    QThread owner;
    owner.start();

    QThreadPool pool;
    for (int i = 0; i < 300; i++) {
        QPointer<TestRunnable> task = new TestRunnable;
        task->setAutoDelete(true);
        task->moveToThread(&owner);
        pool.start(task);
    }
    pool.waitForDone();

    owner.quit();
    owner.wait();
}

void tst_QPointer::qvariantCast()
{
    QFile file;
    QPointer<QFile> tracking = &file;
    tracking->setObjectName("A test name");
    QVariant v = QVariant::fromValue(tracking);

    {
        QPointer<QObject> other = qPointerFromVariant<QObject>(v);
        QCOMPARE(other->objectName(), QString::fromLatin1("A test name"));
    }
    {
        QPointer<QIODevice> other = qPointerFromVariant<QIODevice>(v);
        QCOMPARE(other->objectName(), QString::fromLatin1("A test name"));
    }
    {
        QPointer<QFile> other = qPointerFromVariant<QFile>(v);
        QCOMPARE(other->objectName(), QString::fromLatin1("A test name"));
    }
    {
        QPointer<QThread> other = qPointerFromVariant<QThread>(v);
        QVERIFY(!other);
    }
    {
        QPointer<QFile> toBeDeleted = new QFile;
        QVariant deletedVariant = QVariant::fromValue(toBeDeleted);
        delete toBeDeleted;
        QPointer<QObject> deleted = qPointerFromVariant<QObject>(deletedVariant);
        QVERIFY(!deleted);
    }

    // Intentionally does not compile.
//     QPointer<int> sop = qPointerFromVariant<int>(v);
}

void tst_QPointer::constPointer()
{
    // Compile-time test that QPointer<const T> works.
    QPointer<const QFile> fp = new QFile;
    delete fp.data();
}


QTEST_MAIN(tst_QPointer)
#include "tst_qpointer.moc"
