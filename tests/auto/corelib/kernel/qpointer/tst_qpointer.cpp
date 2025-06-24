// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>

#include <QtCore/private/qobject_p.h>
#include <QRunnable>
#include <QSemaphore>
#include <QThreadPool>

#include <QPointer>
#ifndef QT_NO_WIDGETS
#include <QWidget>
#endif

using namespace std::chrono_literals;

class Interface
{
public:
    virtual ~Interface() = default;
    virtual void meep() const = 0;
};

class ObjectImplementingInterface : public QObject, public Interface
{
    Q_OBJECT
public:
    using QObject::QObject;

public Q_SLOTS:
    void meep() const override {}
};

class tst_QPointer : public QObject
{
    Q_OBJECT
public:
    inline tst_QPointer *me() const
    { return const_cast<tst_QPointer *>(this); }

private slots:
    void constructors();
    void ctad();
    void conversion();
    void destructor();
    void assignment_operators();
    void compareCompiles();
    void equality_operators();
    void equality_operators_interface();
    void swap();
    void isNull();
    void dereference_operators();
    void disconnect();
    void castDuringDestruction();
    void threadSafety();
    void raceCondition();

    void qvariantCast();
    void constPointer();
    void constQPointer();
};

// check that nullptr QPointer construction is Q_CONSTINIT:
[[maybe_unused]] Q_CONSTINIT static QPointer<QFile> s_file1;
[[maybe_unused]] Q_CONSTINIT static QPointer<QFile> s_file2 = {};
[[maybe_unused]] Q_CONSTINIT static QPointer<QFile> s_file3 = nullptr;
[[maybe_unused]] Q_CONSTINIT static QPointer<QFile> s_file4 = 0; // legacy nullptr

void tst_QPointer::constructors()
{
    struct Derived : QObject {};
    Derived derived;

    QPointer<QObject> p1;
    QPointer<QObject> p2(this);
    QPointer<QObject> p3(p2);
    QPointer<QObject> p4 = &derived;
    QCOMPARE(p1, QPointer<QObject>(0));
    QCOMPARE(p2, QPointer<QObject>(this));
    QCOMPARE(p3, QPointer<QObject>(this));
    QCOMPARE(p4, &derived);
}

void tst_QPointer::ctad()
{

    {
        QObject o;
        QPointer po = &o;
        static_assert(std::is_same_v<decltype(po), QPointer<QObject>>);
        QPointer poc = po;
        static_assert(std::is_same_v<decltype(poc), QPointer<QObject>>);
        QPointer pom = std::move(po);
        static_assert(std::is_same_v<decltype(pom), QPointer<QObject>>);
    }
    {
        const QObject co;
        QPointer pco = &co;
        static_assert(std::is_same_v<decltype(pco), QPointer<const QObject>>);
        QPointer pcoc = pco;
        static_assert(std::is_same_v<decltype(pcoc), QPointer<const QObject>>);
        QPointer pcom = std::move(pco);
        static_assert(std::is_same_v<decltype(pcom), QPointer<const QObject>>);
    }
    {
        QFile f;
        QPointer pf = &f;
        static_assert(std::is_same_v<decltype(pf), QPointer<QFile>>);
        QPointer pfc = pf;
        static_assert(std::is_same_v<decltype(pfc), QPointer<QFile>>);
        QPointer pfm = std::move(pf);
        static_assert(std::is_same_v<decltype(pfm), QPointer<QFile>>);
    }
    {
        const QFile cf;
        QPointer pcf = &cf;
        static_assert(std::is_same_v<decltype(pcf), QPointer<const QFile>>);
        QPointer pcfc = pcf;
        static_assert(std::is_same_v<decltype(pcfc), QPointer<const QFile>>);
        QPointer pcfm = std::move(pcf);
        static_assert(std::is_same_v<decltype(pcfm), QPointer<const QFile>>);
    }
}

void tst_QPointer::conversion()
{
    // copy-conversion:
    {
        QFile file;
        QPointer<QFile> pf = &file;
        QCOMPARE_EQ(pf, &file);
        QPointer<const QIODevice> pio = pf;
        QCOMPARE_EQ(pio, &file);
        QCOMPARE_EQ(pio.get(), &file);
        QCOMPARE_EQ(pio, pf);
        QCOMPARE_EQ(pio.get(), pf.get());

        // reset
        pio = nullptr;
        QCOMPARE_EQ(pio, nullptr);
        QCOMPARE_EQ(pio.get(), nullptr);

        // copy-assignment
        QCOMPARE_EQ(pf, &file);
        pio = pf;
        QCOMPARE_EQ(pio, &file);
        QCOMPARE_EQ(pio.get(), &file);
        QCOMPARE_EQ(pio, pf);
        QCOMPARE_EQ(pio.get(), pf.get());
    }
    // move-conversion:
    {
        QFile file;
        QPointer<QFile> pf = &file;
        QCOMPARE_EQ(pf, &file);
        QPointer<const QIODevice> pio = std::move(pf);
        QCOMPARE_EQ(pf, nullptr);
        QCOMPARE_EQ(pio, &file);
        QCOMPARE_EQ(pio.get(), &file);

        // reset
        pio = nullptr;
        QCOMPARE_EQ(pio, nullptr);
        QCOMPARE_EQ(pio.get(), nullptr);

        // move-assignment
        pio = QPointer<QFile>(&file);
        QCOMPARE_EQ(pio, &file);
        QCOMPARE_EQ(pio.get(), &file);
    }
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
    p1 = nullptr;
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

void tst_QPointer::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QPointer<QObject>>();
    QTestPrivate::testEqualityOperatorsCompile<QPointer<QObject>, QObject*>();
    QTestPrivate::testEqualityOperatorsCompile<QPointer<QObject>, QWidget*>();
    QTestPrivate::testEqualityOperatorsCompile<QPointer<QObject>, QPointer<QWidget>>();
    QTestPrivate::testEqualityOperatorsCompile<QPointer<QObject>, std::nullptr_t>();
    QTestPrivate::testEqualityOperatorsCompile<QPointer<ObjectImplementingInterface>, Interface*>();
}

void tst_QPointer::equality_operators()
{
    QPointer<QObject> p1;
    QPointer<QObject> p2;

    QT_TEST_EQUALITY_OPS(p1, p2, true);

    QObject *object = nullptr;
#ifndef QT_NO_WIDGETS
    QWidget *widget = nullptr;
#endif

    p1 = object;
    QT_TEST_EQUALITY_OPS(p1, p2, true);
    QT_TEST_EQUALITY_OPS(p1, object, true);
    p2 = object;
    QT_TEST_EQUALITY_OPS(p2, p1, true);
    QT_TEST_EQUALITY_OPS(p2, object, true);
    p1 = this;
    QT_TEST_EQUALITY_OPS(p1, p2, false);
    p2 = p1;
    QT_TEST_EQUALITY_OPS(p1, p2, true);

    // compare to zero
    p1 = nullptr;
    QVERIFY(p1 == 0);
    QVERIFY(0 == p1);
    QVERIFY(p2 != 0);
    QVERIFY(0 != p2);
    QT_TEST_EQUALITY_OPS(p1, nullptr, true);
    QT_TEST_EQUALITY_OPS(p2, nullptr, false);
    QT_TEST_EQUALITY_OPS(p1, object, true);
    QT_TEST_EQUALITY_OPS(p2, object, false);
#ifndef QT_NO_WIDGETS
    QT_TEST_EQUALITY_OPS(p1, widget, true);
    QT_TEST_EQUALITY_OPS(p2, widget, false);
#endif
}

void tst_QPointer::equality_operators_interface()
{
    QObject reaper;
    QPointer<ObjectImplementingInterface> p(new ObjectImplementingInterface(&reaper));
    Interface *i = p.get();

    ObjectImplementingInterface otherP;
    Interface *otherI = &otherP;

    // things that are equal
    QT_TEST_EQUALITY_OPS(p, p, true);
    QT_TEST_EQUALITY_OPS(p, i, true);

    // things that are not equal
    QT_TEST_EQUALITY_OPS(p, nullptr, false);
    QT_TEST_EQUALITY_OPS(p, &otherP, false);
    QT_TEST_EQUALITY_OPS(p, otherI, false);
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
    p1 = nullptr;
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

protected:
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
    QObjectPrivate::get(this)->deleteChildren(); // like ~QWidget() does
}

DerivedChild::~DerivedChild()
{
    QCOMPARE(static_cast<DerivedParent *>(guardedParentPointer), parentPointer);
    QCOMPARE(qobject_cast<DerivedParent *>(guardedParentPointer), parentPointer);
}

//
// The FurtherDerived... hierarchiy mimicks QWidget subclasses:
// Like ~QWidget(), ~DerivedParent() deletes its children before ~QObject() would do it,
// which would have first set QPointers to nullptr. In this situation, therefore,
// guardedParentPointers are _not_ nullptr, yet, and the object they point to is already
// demoted from FurtherDerivedParent to DerivedParent, potentially making some operations
// UB (invalid downcasts).
//

class FurtherDerivedParent;
class FurtherDerivedChild : public DerivedChild
{
    Q_OBJECT

    FurtherDerivedParent *parentPointer;
    QPointer<FurtherDerivedParent> guardedParentPointer;
public:
    FurtherDerivedChild(FurtherDerivedParent *);
    ~FurtherDerivedChild() override;
};

class FurtherDerivedParent : public DerivedParent
{
    Q_OBJECT
public:
    FurtherDerivedParent() : DerivedParent()
    {
        new FurtherDerivedChild(this);
    }
};

FurtherDerivedChild::FurtherDerivedChild(FurtherDerivedParent *parent)
    : DerivedChild(parent),
      parentPointer(parent),
      guardedParentPointer(parent)
{
}

FurtherDerivedChild::~FurtherDerivedChild()
{
    // isNull()
    QVERIFY(!guardedParentPointer.isNull());

    // operator bool()
    QVERIFY(guardedParentPointer);
    QVERIFY(!!guardedParentPointer);

    // Don't use QCOMPARE in these, it may call .data(), which could be UB at this point:
    #define CHECK_NE(lhs, rhs) do { \
        QVERIFY(!(lhs == rhs)); \
        QVERIFY(!(rhs == lhs)); \
        QVERIFY(lhs != rhs); \
        QVERIFY(rhs != lhs); \
    } while (false)

    #define CHECK_EQ(lhs, rhs) do { \
        QVERIFY(!(lhs != rhs)); \
        QVERIFY(!(rhs != lhs)); \
        QVERIFY(lhs == rhs); \
        QVERIFY(rhs == lhs); \
    } while (false)

    // operator==/!= against nullptr
    CHECK_NE(guardedParentPointer, nullptr);

    // operator==/!= against QObject*/DerivedParent*/FutherDerivedParent*
    CHECK_EQ(parentPointer, parentPointer); // verify it's not UB for raw pointers
    // UB after this point!
    return;
    const QObject *parentPointerAsQObject = parentPointer;
    CHECK_EQ(guardedParentPointer, parentPointerAsQObject);
    const DerivedParent *parentPointerAsDerived = parentPointer;
    CHECK_EQ(guardedParentPointer, parentPointerAsDerived);
    CHECK_EQ(guardedParentPointer, DerivedChild::guardedParentPointer);
    CHECK_EQ(guardedParentPointer, parentPointer);
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

    {
        [[maybe_unused]]
        FurtherDerivedParent obj;
    }
}

class TestRunnable : public QObject, public QRunnable {
    void run() override
    {
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

void tst_QPointer::raceCondition()
{
    constexpr int NUM_THREADS = 20;
    constexpr int ITERATIONS_PER_THREAD = 10;

    QSemaphore startSemaphore;

    QObject targetObject;

    std::array<std::unique_ptr<QThread>, NUM_THREADS> threads;

    for (auto &thread : threads) {
        thread.reset(
                QThread::create([&] {
                    startSemaphore.acquire();

                    for (int j = 0; j < ITERATIONS_PER_THREAD; ++j) {
                        QPointer<QObject> pointer(&targetObject);
                        Q_UNUSED(pointer);
                    }
                }));

        thread->start();
    }

    QTest::qWait(100ms);
    startSemaphore.release(threads.size());

    bool allJoined = true;
    for (const auto &thread : threads) {
        allJoined = thread->wait(30s) && allJoined;
    }
    QVERIFY(allJoined);
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

void tst_QPointer::constQPointer()
{
    // Check that const QPointers work. It's a bit weird to mark a pointer
    // const if its value can change, but the shallow-const principle in C/C++
    // allows this, and people use it, so document it with a test.
    //
    // It's unlikely that this test will fail in and out of itself, but it
    // presents the use-case to static and dynamic checkers that can raise
    // a warning (hopefully) should this become an issue.
    QObject *o = new QObject(this);
    const QPointer<QObject> p = o;
    delete o;
    QVERIFY(!p);
}


QTEST_MAIN(tst_QPointer)
#include "tst_qpointer.moc"
