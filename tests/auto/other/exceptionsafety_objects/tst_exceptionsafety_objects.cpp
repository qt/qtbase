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

#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtTest/QtTest>

#include <stddef.h>
#include <exception>

QT_USE_NAMESPACE

// this test only works with GLIBC

#include "oomsimulator.h"
#include "3rdparty/memcheck.h"

class tst_ExceptionSafety_Objects: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
#ifndef QT_NO_EXCEPTIONS
    void cleanupTestCase();

private slots:
    void objects_data();
    void objects();

    void widgets_data();
    void widgets();

    void vector_data();
    void vector();

    void list_data();
    void list();

    void linkedList_data();
    void linkedList();

private:
    static QtMessageHandler testMessageHandler;
    static void safeMessageHandler(QtMsgType, const QMessageLogContext&, const QString&);
#endif
};

#ifdef QT_NO_EXCEPTIONS
void tst_ExceptionSafety_Objects::initTestCase()
{
    QSKIP("This test requires exception support");
}

#else
// helper structs to create an arbitrary widget
struct AbstractTester
{
    virtual ~AbstractTester() {}
    virtual void operator()(QObject *parent) = 0;
};
Q_DECLARE_METATYPE(AbstractTester *)

typedef void (*TestFunction)(QObject*);
Q_DECLARE_METATYPE(TestFunction)

template <typename T>
struct ObjectCreator : public AbstractTester
{
    void operator()(QObject *)
    {
        QScopedPointer<T> ptr(new T);
    }
};

struct BitArrayCreator : public AbstractTester
{
    void operator()(QObject *)
    { QScopedPointer<QBitArray> bitArray(new QBitArray(100, true)); }
};

struct ByteArrayMatcherCreator : public AbstractTester
{
    void operator()(QObject *)
    { QScopedPointer<QByteArrayMatcher> ptr(new QByteArrayMatcher("ralf test",8)); }
};

struct CryptographicHashCreator : public AbstractTester
{
    void operator()(QObject *)
    {
        QScopedPointer<QCryptographicHash> ptr(new QCryptographicHash(QCryptographicHash::Sha1));
        ptr->addData("ralf test",8);
    }
};

struct DataStreamCreator : public AbstractTester
{
    void operator()(QObject *)
    {
        QScopedPointer<QByteArray> arr(new QByteArray("hallo, test"));
        QScopedPointer<QDataStream> ptr(new QDataStream(arr.data(), QIODevice::ReadWrite));
        ptr->writeBytes("ralf test",8);
    }
};

struct DirCreator : public AbstractTester
{
    void operator()(QObject *)
    {
        QDir::cleanPath("../////././");
        QScopedPointer<QDir> ptr(new QDir("."));
        while( ptr->cdUp() )
            ; // just going up
        ptr->count();
        ptr->exists(ptr->path());

        QStringList filters;
        filters << "*.cpp" << "*.cxx" << "*.cc";
        ptr->setNameFilters(filters);
    }
};

void tst_ExceptionSafety_Objects::objects_data()
{
    QTest::addColumn<AbstractTester *>("objectCreator");

#define NEWROW(T) QTest::newRow(#T) << static_cast<AbstractTester *>(new ObjectCreator<T >)
    NEWROW(QObject);
    NEWROW(QBuffer);
    NEWROW(QFile);
    NEWROW(QProcess);
    NEWROW(QSettings);
    NEWROW(QThread);
    NEWROW(QThreadPool);
    NEWROW(QTranslator);

#define NEWROW2(T, CREATOR) QTest::newRow(#T) << static_cast<AbstractTester *>(new CREATOR)
    NEWROW2(QBitArray, BitArrayCreator);
    NEWROW2(QByteArrayMatcher, ByteArrayMatcherCreator);
    NEWROW2(QCryptographicHash, CryptographicHashCreator);
    NEWROW2(QDataStream, DataStreamCreator);
    NEWROW2(QDir, DirCreator);
}

// create and destructs an object, and lets each and every allocation
// during construction and destruction fail.
template <typename T>
static void doOOMTest(T &testFunc, QObject *parent, int start=0)
{
    int currentOOMIndex = start;
    bool caught = false;
    bool done = false;

    AllocFailer allocFailer(0);
    int allocCountBefore = allocFailer.currentAllocIndex();

    do {
        allocFailer.reactivateAt(++currentOOMIndex);

        caught = false;

        try {
            testFunc(parent);
        } catch (const std::bad_alloc &) {
            caught = true;
        } catch (const std::exception &ex) {
            if (strcmp(ex.what(), "autotest swallow") != 0)
                throw;
            caught = true;
        }

        if (!caught) {
            void *buf = malloc(42);
            if (buf) {
                // we got memory here - oom test is over.
                free(buf);
                done = true;
            }
        }

        // if we get a FAIL, stop executing now
        if (QTest::currentTestFailed())
            done = true;

//#define REALLY_VERBOSE
#ifdef REALLY_VERBOSE
    fprintf(stderr, " OOM Index: %d\n", currentOOMIndex);
#endif


    } while (caught || !done);

    allocFailer.deactivate();

//#define VERBOSE
#ifdef VERBOSE
    fprintf(stderr, "OOM Test done, checked allocs: %d (range %d - %d)\n", currentOOMIndex,
                allocCountBefore, allocFailer.currentAllocIndex());
#else
    Q_UNUSED(allocCountBefore);
#endif
}

static int alloc1Failed = 0;
static int alloc2Failed = 0;
static int alloc3Failed = 0;
static int alloc4Failed = 0;
static int malloc1Failed = 0;
static int malloc2Failed = 0;

// Tests that new, new[] and malloc() fail at least once during OOM testing.
class SelfTestObject : public QObject
{
public:
    SelfTestObject(QObject *parent = 0)
        : QObject(parent)
    {
        try { delete new int; } catch (const std::bad_alloc &) { ++alloc1Failed; throw; }
        try { delete [] new double[5]; } catch (const std::bad_alloc &) { ++alloc2Failed; throw ;}
        void *buf = malloc(42);
        if (buf)
            free(buf);
        else
            ++malloc1Failed;
    }

    ~SelfTestObject()
    {
        try { delete new int; } catch (const std::bad_alloc &) { ++alloc3Failed; }
        try { delete [] new double[5]; } catch (const std::bad_alloc &) { ++alloc4Failed; }
        void *buf = malloc(42);
        if (buf)
            free(buf);
        else
            ++malloc2Failed = true;
    }
};

QtMessageHandler tst_ExceptionSafety_Objects::testMessageHandler;

void tst_ExceptionSafety_Objects::safeMessageHandler(QtMsgType type, const QMessageLogContext &ctxt,
                                                     const QString &msg)
{
    // this temporarily suspends OOM testing while handling a message
    int currentIndex = mallocFailIndex;
    AllocFailer allocFailer(0);
    allocFailer.deactivate();
    (*testMessageHandler)(type, ctxt, msg);
    allocFailer.reactivateAt(currentIndex);
}

typedef void (*PVF)();
PVF defaultTerminate;
void debugTerminate()
{
    // you can detect uncaught exceptions with a breakpoint in here
    (*defaultTerminate)();
}

PVF defaultUnexpected;
void debugUnexpected()
{
    // you can detect unexpected exceptions with a breakpoint in here
    (*defaultUnexpected)();
}

void tst_ExceptionSafety_Objects::initTestCase()
{
    // set handlers for bad exception cases, you might want to step in and breakpoint the default handlers too
    defaultTerminate = std::set_terminate(&debugTerminate);
    defaultUnexpected = std::set_unexpected(&debugUnexpected);
    testMessageHandler = qInstallMessageHandler(safeMessageHandler);

    QVERIFY(AllocFailer::initialize());

    // sanity check whether OOM simulation works
    AllocFailer allocFailer(0);

    // malloc fail index is 0 -> this malloc should fail.
    void *buf = malloc(42);
    allocFailer.deactivate();
    QVERIFY(!buf);

    // malloc fail index is 1 - second malloc should fail.
    allocFailer.reactivateAt(1);
    buf = malloc(42);
    void *buf2 = malloc(42);
    allocFailer.deactivate();

    QVERIFY(buf);
    free(buf);
    QVERIFY(!buf2);

    ObjectCreator<SelfTestObject> *selfTest = new ObjectCreator<SelfTestObject>;
    doOOMTest(*selfTest, 0);
    delete selfTest;
    QCOMPARE(alloc1Failed, 1);
    QCOMPARE(alloc2Failed, 1);
    QCOMPARE(alloc3Failed, 2);
    QCOMPARE(alloc4Failed, 3);
    QCOMPARE(malloc1Failed, 1);
    QCOMPARE(malloc2Failed, 1);
}

void tst_ExceptionSafety_Objects::cleanupTestCase()
{
    qInstallMessageHandler(testMessageHandler);
}

void tst_ExceptionSafety_Objects::objects()
{
    QLatin1String tag = QLatin1String(QTest::currentDataTag());
    if (tag == QLatin1String("QFile")
        || tag == QLatin1String("QProcess")
        || tag == QLatin1String("QSettings")
        || tag == QLatin1String("QThread")
        || tag == QLatin1String("QThreadPool"))
        QSKIP("This type of object is not currently strongly exception safe");

    QFETCH(AbstractTester *, objectCreator);

    doOOMTest(*objectCreator, 0);

    delete objectCreator;
}

template <typename T>
struct WidgetCreator : public AbstractTester
{
    void operator()(QObject *parent)
    {
        if (parent && !parent->isWidgetType())
            qFatal("%s: parent must be either null or a widget type", Q_FUNC_INFO);
        QScopedPointer<T> ptr(parent ? new T(static_cast<QWidget *>(parent)) : new T);
    }
};

// QSizeGrip doesn't have a default constructor - always pass parent (even though it might be 0)
template <> struct WidgetCreator<QSizeGrip> : public AbstractTester
{
    void operator()(QObject *parent)
    {
        if (parent && !parent->isWidgetType())
            qFatal("%s: parent must be either null or a widget type", Q_FUNC_INFO);
        QScopedPointer<QSizeGrip> ptr(new QSizeGrip(static_cast<QWidget *>(parent)));
    }
};

// QDesktopWidget doesn't need a parent.
template <> struct WidgetCreator<QDesktopWidget> : public AbstractTester
{
    void operator()(QObject *parent)
    {
        if (parent && !parent->isWidgetType())
            qFatal("%s: parent must be either null or a widget type", Q_FUNC_INFO);
        QScopedPointer<QDesktopWidget> ptr(new QDesktopWidget());
    }
};
void tst_ExceptionSafety_Objects::widgets_data()
{
    QTest::addColumn<AbstractTester *>("widgetCreator");

#undef NEWROW
#define NEWROW(T) QTest::newRow(#T) << static_cast<AbstractTester *>(new WidgetCreator<T >)

    NEWROW(QWidget);

    NEWROW(QButtonGroup);
    NEWROW(QCheckBox);
    NEWROW(QColumnView);
    NEWROW(QComboBox);
    NEWROW(QCommandLinkButton);
    NEWROW(QDateEdit);
    NEWROW(QDateTimeEdit);
    NEWROW(QDesktopWidget);
    NEWROW(QDial);
    NEWROW(QDoubleSpinBox);
    NEWROW(QFocusFrame);
    NEWROW(QFontComboBox);
    NEWROW(QFrame);
    NEWROW(QGroupBox);
    NEWROW(QLabel);
    NEWROW(QLCDNumber);
    NEWROW(QLineEdit);
    NEWROW(QListView);
    NEWROW(QListWidget);
    NEWROW(QMainWindow);
    NEWROW(QMenu);
    NEWROW(QMenuBar);
    NEWROW(QPlainTextEdit);
    NEWROW(QProgressBar);
    NEWROW(QPushButton);
    NEWROW(QRadioButton);
    NEWROW(QScrollArea);
    NEWROW(QScrollBar);
    NEWROW(QSizeGrip);
    NEWROW(QSlider);
    NEWROW(QSpinBox);
    NEWROW(QSplitter);
    NEWROW(QStackedWidget);
    NEWROW(QStatusBar);
    NEWROW(QTabBar);
    NEWROW(QTableView);
    NEWROW(QTableWidget);
    NEWROW(QTabWidget);
    NEWROW(QTextBrowser);
    NEWROW(QTextEdit);
    NEWROW(QTimeEdit);
    NEWROW(QToolBar);
    NEWROW(QToolBox);
    NEWROW(QToolButton);
    NEWROW(QTreeView);
    NEWROW(QTreeWidget);
}

void tst_ExceptionSafety_Objects::widgets()
{
    QLatin1String tag = QLatin1String(QTest::currentDataTag());
    if (tag == QLatin1String("QColumnView")
        || tag == QLatin1String("QComboBox")
        || tag == QLatin1String("QCommandLinkButton")
        || tag == QLatin1String("QDateEdit")
        || tag == QLatin1String("QDateTimeEdit")
        || tag == QLatin1String("QDesktopWidget")
        || tag == QLatin1String("QDoubleSpinBox")
        || tag == QLatin1String("QFontComboBox")
        || tag == QLatin1String("QGroupBox")
        || tag == QLatin1String("QLineEdit")
        || tag == QLatin1String("QListView")
        || tag == QLatin1String("QListWidget")
        || tag == QLatin1String("QMainWindow")
        || tag == QLatin1String("QMenu")
        || tag == QLatin1String("QMenuBar")
        || tag == QLatin1String("QPlainTextEdit")
        || tag == QLatin1String("QProgressBar")
        || tag == QLatin1String("QPushButton")
        || tag == QLatin1String("QScrollArea")
        || tag == QLatin1String("QSpinBox")
        || tag == QLatin1String("QStackedWidget")
        || tag == QLatin1String("QStatusBar")
        || tag == QLatin1String("QTableView")
        || tag == QLatin1String("QTableWidget")
        || tag == QLatin1String("QTabWidget")
        || tag == QLatin1String("QTextBrowser")
        || tag == QLatin1String("QTextEdit")
        || tag == QLatin1String("QTimeEdit")
        || tag == QLatin1String("QToolBar")
        || tag == QLatin1String("QToolBox")
        || tag == QLatin1String("QTreeView")
        || tag == QLatin1String("QTreeWidget"))
        QSKIP("This type of widget is not currently strongly exception safe");

    if (tag == QLatin1String("QWidget"))
        QSKIP("QTBUG-18927");

    QFETCH(AbstractTester *, widgetCreator);

    doOOMTest(*widgetCreator, 0, 00000);

    QWidget parent;
    doOOMTest(*widgetCreator, &parent, 00000);

    delete widgetCreator;

    // if the test reaches here without crashing, we passed :)
    QVERIFY(true);
}

struct Integer
{
    Integer(int value = 42)
        : ptr(new int(value))
    {
        ++instanceCount;
    }

    Integer(const Integer &other)
        : ptr(new int(*other.ptr))
    {
        ++instanceCount;
    }

    Integer &operator=(const Integer &other)
    {
        int *newPtr = new int(*other.ptr);
        delete ptr;
        ptr = newPtr;
        return *this;
    }

    ~Integer()
    {
        --instanceCount;
        delete ptr;
    }

    int value() const
    {
        return *ptr;
    }

    int *ptr;
    static int instanceCount;
};

int Integer::instanceCount = 0;

struct IntegerMoveable
    {
    IntegerMoveable(int value = 42)
        : val(value)
    {
        delete new int;
        ++instanceCount;
    }

    IntegerMoveable(const IntegerMoveable &other)
        : val(other.val)
    {
        delete new int;
        ++instanceCount;
    }

    IntegerMoveable &operator=(const IntegerMoveable &other)
    {
        delete new int;
        val = other.val;
        return *this;
    }

    ~IntegerMoveable()
    {
        --instanceCount;
    }

    int value() const
    {
        return val;
    }

    int val;
    static int instanceCount;
    };

int IntegerMoveable::instanceCount = 0;
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(IntegerMoveable, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

template <typename T, template<typename> class Container>
void containerInsertTest(QObject*)
{
    Container<T> container;

    // insert an item in an empty container
    try {
        container.insert(container.begin(), 41);
    } catch (...) {
        QVERIFY(container.isEmpty());
        QCOMPARE(T::instanceCount, 0);
        return;
    }

    QCOMPARE(container.size(), 1);
    QCOMPARE(T::instanceCount, 1);

    // insert an item before another item
    try {
        container.insert(container.begin(), 42);
    } catch (...) {
        QCOMPARE(container.size(), 1);
        QCOMPARE(container.first().value(), 41);
        QCOMPARE(T::instanceCount, 1);
        return;
    }

    QCOMPARE(T::instanceCount, 2);

    // insert an item in between
    try {
        container.insert(container.begin() + 1, 43);
    } catch (...) {
        QCOMPARE(container.size(), 2);
        QCOMPARE(container.first().value(), 41);
        QCOMPARE((container.begin() + 1)->value(), 42);
        QCOMPARE(T::instanceCount, 2);
        return;
    }

    QCOMPARE(T::instanceCount, 3);
}

template <typename T, template<typename> class Container>
void containerAppendTest(QObject*)
{
    Container<T> container;

    // append to an empty container
    try {
        container.append(42);
    } catch (...) {
        QCOMPARE(container.size(), 0);
        QCOMPARE(T::instanceCount, 0);
        return;
    }

    // append to a container with one item
    try {
        container.append(43);
    } catch (...) {
        QCOMPARE(container.size(), 1);
        QCOMPARE(container.first().value(), 42);
        QCOMPARE(T::instanceCount, 1);
        return;
    }

    Container<T> container2;

    try {
        container2.append(44);
    } catch (...) {
        // don't care
        return;
    }
    QCOMPARE(T::instanceCount, 3);

    // append another container with one item
    try {
        container += container2;
    } catch (...) {
        QCOMPARE(container.size(), 2);
        QCOMPARE(container.first().value(), 42);
        QCOMPARE((container.begin() + 1)->value(), 43);
        QCOMPARE(T::instanceCount, 3);
        return;
    }

    QCOMPARE(T::instanceCount, 4);
}

template <typename T, template<typename> class Container>
void containerEraseTest(QObject*)
{
    Container<T> container;

    try {
        container.append(42);
        container.append(43);
        container.append(44);
        container.append(45);
        container.append(46);
    } catch (...) {
        // don't care
        return;
    }

    // sanity checks
    QCOMPARE(container.size(), 5);
    QCOMPARE(T::instanceCount, 5);

    // delete the first one
    try {
        container.erase(container.begin());
    } catch (...) {
        QCOMPARE(container.size(), 5);
        QCOMPARE(container.first().value(), 42);
        QCOMPARE(T::instanceCount, 5);
        return;
    }

    QCOMPARE(container.size(), 4);
    QCOMPARE(container.first().value(), 43);
    QCOMPARE(T::instanceCount, 4);

    // delete the last one
    try {
        container.erase(container.end() - 1);
    } catch (...) {
        QCOMPARE(container.size(), 4);
        QCOMPARE(T::instanceCount, 4);
        return;
    }

    QCOMPARE(container.size(), 3);
    QCOMPARE(container.first().value(), 43);
    QCOMPARE((container.begin() + 1)->value(), 44);
    QCOMPARE((container.begin() + 2)->value(), 45);
    QCOMPARE(T::instanceCount, 3);

    // delete the middle one
    try {
        container.erase(container.begin() + 1);
    } catch (...) {
        QCOMPARE(container.size(), 3);
        QCOMPARE(container.first().value(), 43);
        QCOMPARE((container.begin() + 1)->value(), 44);
        QCOMPARE((container.begin() + 2)->value(), 45);
        QCOMPARE(T::instanceCount, 3);
        return;
    }

    QCOMPARE(container.size(), 2);
    QCOMPARE(container.first().value(), 43);
    QCOMPARE((container.begin() + 1)->value(), 45);
    QCOMPARE(T::instanceCount, 2);
}

template <template<typename T> class Container>
static void containerData()
{
    QTest::addColumn<TestFunction>("testFunction");

    QTest::newRow("insert static") << static_cast<TestFunction>(containerInsertTest<Integer, Container>);
    QTest::newRow("append static") << static_cast<TestFunction>(containerAppendTest<Integer, Container>);
    QTest::newRow("erase static") << static_cast<TestFunction>(containerEraseTest<Integer, Container>);
    QTest::newRow("insert moveable") << static_cast<TestFunction>(containerInsertTest<IntegerMoveable, Container>);
    QTest::newRow("append moveable") << static_cast<TestFunction>(containerAppendTest<IntegerMoveable, Container>);
    QTest::newRow("erase moveable") << static_cast<TestFunction>(containerEraseTest<IntegerMoveable, Container>);
}

void tst_ExceptionSafety_Objects::vector_data()
{
    containerData<QVector>();
}

void tst_ExceptionSafety_Objects::vector()
{
    QFETCH(TestFunction, testFunction);

    if (QLatin1String(QTest::currentDataTag()) == QLatin1String("insert static")
        || QLatin1String(QTest::currentDataTag()) == QLatin1String("insert moveable"))
        QSKIP("QVector::insert is currently not strongly exception safe");

    doOOMTest(testFunction, 0);
}

void tst_ExceptionSafety_Objects::list_data()
{
    containerData<QList>();
}

void tst_ExceptionSafety_Objects::list()
{
    QFETCH(TestFunction, testFunction);

    doOOMTest(testFunction, 0);
}

void tst_ExceptionSafety_Objects::linkedList_data()
{
    containerData<QLinkedList>();
}

void tst_ExceptionSafety_Objects::linkedList()
{
    QFETCH(TestFunction, testFunction);

    doOOMTest(testFunction, 0);
}

#endif

QTEST_MAIN(tst_ExceptionSafety_Objects)
#include "tst_exceptionsafety_objects.moc"
