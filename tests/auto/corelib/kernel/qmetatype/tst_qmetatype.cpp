/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore>
#include <QtTest/QtTest>

#ifdef Q_OS_LINUX
# include <pthread.h>
#endif

Q_DECLARE_METATYPE(QMetaType::Type)

class tst_QMetaType: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QVariant> prop READ prop WRITE setProp)

public:
    tst_QMetaType() { propList << 42 << "Hello"; }

    QList<QVariant> prop() const { return propList; }
    void setProp(const QList<QVariant> &list) { propList = list; }

private:
    QList<QVariant> propList;

private slots:
    void defined();
    void threadSafety();
    void namespaces();
    void qMetaTypeId();
    void properties();
    void normalizedTypes();
    void typeName_data();
    void typeName();
    void create_data();
    void create();
    void createCopy_data();
    void createCopy();
    void sizeOf_data();
    void sizeOf();
    void construct_data();
    void construct();
    void constructCopy_data();
    void constructCopy();
    void typedefs();
    void isRegistered_data();
    void isRegistered();
    void unregisterType();
    void QTBUG11316_registerStreamBuiltin();

};

struct Foo { int i; };

void tst_QMetaType::defined()
{
    QCOMPARE(int(QMetaTypeId2<QString>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<Foo>::Defined), 0);
    QCOMPARE(int(QMetaTypeId2<void*>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<int*>::Defined), 0);
}

struct Bar
{
    Bar()
    {
        // check re-entrancy
        if (!QMetaType::isRegistered(qRegisterMetaType<Foo>("Foo"))) {
            qWarning("%s: re-entrancy test failed", Q_FUNC_INFO);
            ++failureCount;
        }
    }

public:
    static int failureCount;
};

int Bar::failureCount = 0;

class MetaTypeTorturer: public QThread
{
    Q_OBJECT
protected:
    void run()
    {
        for (int i = 0; i < 1000; ++i) {
            const QByteArray name = QString("Bar%1_%2").arg(i).arg((size_t)QThread::currentThreadId()).toLatin1();
            const char *nm = name.constData();
            int tp = qRegisterMetaType<Bar>(nm);
#ifdef Q_OS_LINUX
            pthread_yield();
#endif
            if (!QMetaType::isRegistered(tp)) {
                ++failureCount;
                qWarning() << name << "is not a registered metatype";
            }
            if (QMetaType::type(nm) != tp) {
                ++failureCount;
                qWarning() << "Wrong metatype returned for" << name;
            }
            if (QMetaType::typeName(tp) != name) {
                ++failureCount;
                qWarning() << "Wrong typeName returned for" << tp;
            }
            void *buf = QMetaType::create(tp, 0);
            void *buf2 = QMetaType::create(tp, buf);
            if (!buf) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, 0)";
            }
            if (!buf2) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, buf)";
            }
            QMetaType::destroy(tp, buf);
            QMetaType::destroy(tp, buf2);
        }
    }
public:
    MetaTypeTorturer() : failureCount(0) { }
    int failureCount;
};

void tst_QMetaType::threadSafety()
{
    MetaTypeTorturer t1;
    MetaTypeTorturer t2;
    MetaTypeTorturer t3;

    t1.start();
    t2.start();
    t3.start();

    QVERIFY(t1.wait());
    QVERIFY(t2.wait());
    QVERIFY(t3.wait());

    QCOMPARE(t1.failureCount, 0);
    QCOMPARE(t2.failureCount, 0);
    QCOMPARE(t3.failureCount, 0);
    QCOMPARE(Bar::failureCount, 0);
}

namespace TestSpace
{
    struct Foo { double d; };

}
Q_DECLARE_METATYPE(TestSpace::Foo)

void tst_QMetaType::namespaces()
{
    TestSpace::Foo nf = { 11.12 };
    QVariant v = qVariantFromValue(nf);
    QCOMPARE(qvariant_cast<TestSpace::Foo>(v).d, 11.12);
}

void tst_QMetaType::qMetaTypeId()
{
    QCOMPARE(::qMetaTypeId<QString>(), int(QMetaType::QString));
    QCOMPARE(::qMetaTypeId<int>(), int(QMetaType::Int));
    QCOMPARE(::qMetaTypeId<TestSpace::Foo>(), QMetaType::type("TestSpace::Foo"));

    QCOMPARE(::qMetaTypeId<char>(), QMetaType::type("char"));
    QCOMPARE(::qMetaTypeId<uchar>(), QMetaType::type("unsigned char"));
    QCOMPARE(::qMetaTypeId<signed char>(), QMetaType::type("signed char"));
    QCOMPARE(::qMetaTypeId<qint8>(), QMetaType::type("qint8"));
}

void tst_QMetaType::properties()
{
    qRegisterMetaType<QList<QVariant> >("QList<QVariant>");

    QVariant v = property("prop");

    QCOMPARE(v.typeName(), "QVariantList");

    QList<QVariant> values = v.toList();
    QCOMPARE(values.count(), 2);
    QCOMPARE(values.at(0).toInt(), 42);

    values << 43 << "world";

    QVERIFY(setProperty("prop", values));
    v = property("prop");
    QCOMPARE(v.toList().count(), 4);
}

template <typename T>
struct Whity { T t; };

Q_DECLARE_METATYPE( Whity < int > )
Q_DECLARE_METATYPE(Whity<double>)

void tst_QMetaType::normalizedTypes()
{
    int WhityIntId = ::qMetaTypeId<Whity<int> >();
    int WhityDoubleId = ::qMetaTypeId<Whity<double> >();

    QCOMPARE(QMetaType::type("Whity<int>"), WhityIntId);
    QCOMPARE(QMetaType::type(" Whity < int > "), WhityIntId);
    QCOMPARE(QMetaType::type("Whity<int >"), WhityIntId);

    QCOMPARE(QMetaType::type("Whity<double>"), WhityDoubleId);
    QCOMPARE(QMetaType::type(" Whity< double > "), WhityDoubleId);
    QCOMPARE(QMetaType::type("Whity<double >"), WhityDoubleId);

    QCOMPARE(qRegisterMetaType<Whity<int> >(" Whity < int > "), WhityIntId);
    QCOMPARE(qRegisterMetaType<Whity<int> >("Whity<int>"), WhityIntId);
    QCOMPARE(qRegisterMetaType<Whity<int> >("Whity<int > "), WhityIntId);

    QCOMPARE(qRegisterMetaType<Whity<double> >(" Whity < double > "), WhityDoubleId);
    QCOMPARE(qRegisterMetaType<Whity<double> >("Whity<double>"), WhityDoubleId);
    QCOMPARE(qRegisterMetaType<Whity<double> >("Whity<double > "), WhityDoubleId);
}

#define TYPENAME_DATA(MetaTypeName, MetaTypeId, RealType)\
    QTest::newRow(#RealType) << QMetaType::MetaTypeName << #RealType;

#define TYPENAME_DATA_ALIAS(MetaTypeName, MetaTypeId, AliasType, RealTypeString)\
    QTest::newRow(RealTypeString) << QMetaType::MetaTypeName << #AliasType;

void tst_QMetaType::typeName_data()
{
    QTest::addColumn<QMetaType::Type>("aType");
    QTest::addColumn<QString>("aTypeName");

    QT_FOR_EACH_STATIC_TYPE(TYPENAME_DATA)
    QT_FOR_EACH_STATIC_ALIAS_TYPE(TYPENAME_DATA_ALIAS)
}

void tst_QMetaType::typeName()
{
    QFETCH(QMetaType::Type, aType);
    QFETCH(QString, aTypeName);

    QCOMPARE(QString::fromLatin1(QMetaType::typeName(aType)), aTypeName);
}

#define FOR_EACH_PRIMITIVE_METATYPE(F) \
    QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(F) \
    QT_FOR_EACH_STATIC_CORE_POINTER(F) \

#define FOR_EACH_COMPLEX_CORE_METATYPE(F) \
    QT_FOR_EACH_STATIC_CORE_CLASS(F)

#define FOR_EACH_CORE_METATYPE(F) \
    FOR_EACH_PRIMITIVE_METATYPE(F) \
    FOR_EACH_COMPLEX_CORE_METATYPE(F) \

template <int ID>
struct MetaEnumToType {};

#define DEFINE_META_ENUM_TO_TYPE(MetaTypeName, MetaTypeId, RealType) \
template<> \
struct MetaEnumToType<QMetaType::MetaTypeName> { \
    typedef RealType Type; \
};
FOR_EACH_CORE_METATYPE(DEFINE_META_ENUM_TO_TYPE)
#undef DEFINE_META_ENUM_TO_TYPE

template <int ID>
struct DefaultValueFactory
{
    typedef typename MetaEnumToType<ID>::Type Type;
    static Type *create() { return new Type; }
};

template <>
struct DefaultValueFactory<QMetaType::Void>
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    static Type *create() { return 0; }
};

template <int ID>
struct DefaultValueTraits
{
    // By default we assume that a default-constructed value (new T) is
    // initialized; e.g. QCOMPARE(*(new T), *(new T)) should succeed
    enum { IsInitialized = true };
};

#define DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS(MetaTypeName, MetaTypeId, RealType) \
template<> struct DefaultValueTraits<QMetaType::MetaTypeName> { \
    enum { IsInitialized = false }; \
};
// Primitive types (int et al) aren't initialized
FOR_EACH_PRIMITIVE_METATYPE(DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS)
#undef DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS

template <int ID>
struct TestValueFactory {};

template<> struct TestValueFactory<QMetaType::Void> {
    static void *create() { return 0; }
};

template<> struct TestValueFactory<QMetaType::QString> {
    static QString *create() { return new QString(QString::fromLatin1("QString")); }
};
template<> struct TestValueFactory<QMetaType::Int> {
    static int *create() { return new int(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::UInt> {
    static uint *create() { return new uint(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::Bool> {
    static bool *create() { return new bool(true); }
};
template<> struct TestValueFactory<QMetaType::Double> {
    static double *create() { return new double(3.14); }
};
template<> struct TestValueFactory<QMetaType::QByteArray> {
    static QByteArray *create() { return new QByteArray(QByteArray("QByteArray")); }
};
template<> struct TestValueFactory<QMetaType::QChar> {
    static QChar *create() { return new QChar(QChar('q')); }
};
template<> struct TestValueFactory<QMetaType::Long> {
    static long *create() { return new long(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::Short> {
    static short *create() { return new short(0x1234); }
};
template<> struct TestValueFactory<QMetaType::Char> {
    static char *create() { return new char('c'); }
};
template<> struct TestValueFactory<QMetaType::ULong> {
    static ulong *create() { return new ulong(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::UShort> {
    static ushort *create() { return new ushort(0x1234); }
};
template<> struct TestValueFactory<QMetaType::UChar> {
    static uchar *create() { return new uchar('u'); }
};
template<> struct TestValueFactory<QMetaType::Float> {
    static float *create() { return new float(3.14); }
};
template<> struct TestValueFactory<QMetaType::QObjectStar> {
    static QObject * *create() { return new QObject *(0); }
};
template<> struct TestValueFactory<QMetaType::QWidgetStar> {
    static QWidget * *create() { return new QWidget *(0); }
};
template<> struct TestValueFactory<QMetaType::VoidStar> {
    static void * *create() { return new void *(0); }
};
template<> struct TestValueFactory<QMetaType::LongLong> {
    static qlonglong *create() { return new qlonglong(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::ULongLong> {
    static qulonglong *create() { return new qulonglong(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::QStringList> {
    static QStringList *create() { return new QStringList(QStringList() << "Q" << "t"); }
};
template<> struct TestValueFactory<QMetaType::QBitArray> {
    static QBitArray *create() { return new QBitArray(QBitArray(256, true)); }
};
template<> struct TestValueFactory<QMetaType::QDate> {
    static QDate *create() { return new QDate(QDate::currentDate()); }
};
template<> struct TestValueFactory<QMetaType::QTime> {
    static QTime *create() { return new QTime(QTime::currentTime()); }
};
template<> struct TestValueFactory<QMetaType::QDateTime> {
    static QDateTime *create() { return new QDateTime(QDateTime::currentDateTime()); }
};
template<> struct TestValueFactory<QMetaType::QUrl> {
    static QUrl *create() { return new QUrl("http://www.example.org"); }
};
template<> struct TestValueFactory<QMetaType::QLocale> {
    static QLocale *create() { return new QLocale(QLocale::c()); }
};
template<> struct TestValueFactory<QMetaType::QRect> {
    static QRect *create() { return new QRect(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QRectF> {
    static QRectF *create() { return new QRectF(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QSize> {
    static QSize *create() { return new QSize(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QSizeF> {
    static QSizeF *create() { return new QSizeF(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QLine> {
    static QLine *create() { return new QLine(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QLineF> {
    static QLineF *create() { return new QLineF(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QPoint> {
    static QPoint *create() { return new QPoint(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QPointF> {
    static QPointF *create() { return new QPointF(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QEasingCurve> {
    static QEasingCurve *create() { return new QEasingCurve(QEasingCurve::InOutElastic); }
};
template<> struct TestValueFactory<QMetaType::QRegExp> {
    static QRegExp *create()
    {
#ifndef QT_NO_REGEXP
        return new QRegExp("A*");
#else
        return 0;
#endif
    }
};
template<> struct TestValueFactory<QMetaType::QVariant> {
    static QVariant *create() { return new QVariant(QStringList(QStringList() << "Q" << "t")); }
};

void tst_QMetaType::create_data()
{
    QTest::addColumn<QMetaType::Type>("type");
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(QMetaType::typeName(QMetaType::MetaTypeName)) << QMetaType::MetaTypeName;
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

template<int ID>
static void testCreateHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    void *actual = QMetaType::create(ID);
    if (DefaultValueTraits<ID>::IsInitialized) {
        Type *expected = DefaultValueFactory<ID>::create();
        QCOMPARE(*static_cast<Type *>(actual), *expected);
        delete expected;
    }
    QMetaType::destroy(ID, actual);
}

template<>
void testCreateHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    void *actual = QMetaType::create(QMetaType::Void);
    if (DefaultValueTraits<QMetaType::Void>::IsInitialized) {
        QVERIFY(DefaultValueFactory<QMetaType::Void>::create());
    }
    QMetaType::destroy(QMetaType::Void, actual);
}


typedef void (*TypeTestFunction)();

void tst_QMetaType::create()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CREATE_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testCreateHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CREATE_FUNCTION)
#undef RETURN_CREATE_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

template<int ID>
static void testCreateCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    void *actual = QMetaType::create(ID, expected);
    QCOMPARE(*static_cast<Type *>(actual), *expected);
    QMetaType::destroy(ID, actual);
    delete expected;
}

template<>
void testCreateCopyHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    Type *expected = TestValueFactory<QMetaType::Void>::create();
    void *actual = QMetaType::create(QMetaType::Void, expected);
    QCOMPARE(static_cast<Type *>(actual), expected);
    QMetaType::destroy(QMetaType::Void, actual);
}

void tst_QMetaType::createCopy_data()
{
    create_data();
}

void tst_QMetaType::createCopy()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CREATE_COPY_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testCreateCopyHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CREATE_COPY_FUNCTION)
#undef RETURN_CREATE_COPY_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

template<typename T> struct SafeSizeOf { enum {Size = sizeof(T)}; };
template<> struct SafeSizeOf<void> { enum {Size = 0}; };

void tst_QMetaType::sizeOf_data()
{
    QTest::addColumn<QMetaType::Type>("type");
    QTest::addColumn<int>("size");
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << QMetaType::MetaTypeName << int(SafeSizeOf<RealType>::Size);
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QMetaType::sizeOf()
{
    QFETCH(QMetaType::Type, type);
    QFETCH(int, size);
    QCOMPARE(QMetaType::sizeOf(type), size);
}

void tst_QMetaType::construct_data()
{
    create_data();
}

#ifndef Q_ALIGNOF
template<uint N>
struct RoundToNextHighestPowerOfTwo
{
private:
    enum { V1 = N-1 };
    enum { V2 = V1 | (V1 >> 1) };
    enum { V3 = V2 | (V2 >> 2) };
    enum { V4 = V3 | (V3 >> 4) };
    enum { V5 = V4 | (V4 >> 8) };
    enum { V6 = V5 | (V5 >> 16) };
public:
    enum { Value = V6 + 1 };
};
#endif

template<class T>
struct TypeAlignment
{
#ifdef Q_ALIGNOF
    enum { Value = Q_ALIGNOF(T) };
#else
    enum { Value = RoundToNextHighestPowerOfTwo<sizeof(T)>::Value };
#endif
};

template<int ID>
static void testConstructHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    int size = QMetaType::sizeOf(ID);
    void *storage = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual = QMetaType::construct(ID, storage, /*copy=*/0);
    QCOMPARE(actual, storage);
    if (DefaultValueTraits<ID>::IsInitialized) {
        Type *expected = DefaultValueFactory<ID>::create();
        QCOMPARE(*static_cast<Type *>(actual), *expected);
        delete expected;
    }
    QMetaType::destruct(ID, actual);
    qFreeAligned(storage);

    QVERIFY(QMetaType::construct(ID, 0, /*copy=*/0) == 0);
    QMetaType::destruct(ID, 0);
}

template<>
void testConstructHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    /*int size = */ QMetaType::sizeOf(QMetaType::Void);
    void *storage = 0;
    void *actual = QMetaType::construct(QMetaType::Void, storage, /*copy=*/0);
    QCOMPARE(actual, storage);
    if (DefaultValueTraits<QMetaType::Void>::IsInitialized) {
        /*Type *expected = */ DefaultValueFactory<QMetaType::Void>::create();
    }
    QMetaType::destruct(QMetaType::Void, actual);
    qFreeAligned(storage);

    QVERIFY(QMetaType::construct(QMetaType::Void, 0, /*copy=*/0) == 0);
    QMetaType::destruct(QMetaType::Void, 0);
}

void tst_QMetaType::construct()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CONSTRUCT_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testConstructHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CONSTRUCT_FUNCTION)
#undef RETURN_CONSTRUCT_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

template<int ID>
static void testConstructCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    int size = QMetaType::sizeOf(ID);
    void *storage = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual = QMetaType::construct(ID, storage, expected);
    QCOMPARE(actual, storage);
    QCOMPARE(*static_cast<Type *>(actual), *expected);
    QMetaType::destruct(ID, actual);
    qFreeAligned(storage);

    QVERIFY(QMetaType::construct(ID, 0, expected) == 0);

    delete expected;
}

template<>
void testConstructCopyHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    Type *expected = TestValueFactory<QMetaType::Void>::create();
    /* int size = */QMetaType::sizeOf(QMetaType::Void);
    void *storage = 0;
    void *actual = QMetaType::construct(QMetaType::Void, storage, expected);
    QCOMPARE(actual, storage);
    QMetaType::destruct(QMetaType::Void, actual);
    qFreeAligned(storage);

    QVERIFY(QMetaType::construct(QMetaType::Void, 0, expected) == 0);
}

void tst_QMetaType::constructCopy_data()
{
    create_data();
}

void tst_QMetaType::constructCopy()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CONSTRUCT_COPY_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testConstructCopyHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CONSTRUCT_COPY_FUNCTION)
#undef RETURN_CONSTRUCT_COPY_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

typedef QString CustomString;
Q_DECLARE_METATYPE(CustomString) //this line is useless

void tst_QMetaType::typedefs()
{
    QCOMPARE(QMetaType::type("long long"), int(QMetaType::LongLong));
    QCOMPARE(QMetaType::type("unsigned long long"), int(QMetaType::ULongLong));
    QCOMPARE(QMetaType::type("qint8"), int(QMetaType::Char));
    QCOMPARE(QMetaType::type("quint8"), int(QMetaType::UChar));
    QCOMPARE(QMetaType::type("qint16"), int(QMetaType::Short));
    QCOMPARE(QMetaType::type("quint16"), int(QMetaType::UShort));
    QCOMPARE(QMetaType::type("qint32"), int(QMetaType::Int));
    QCOMPARE(QMetaType::type("quint32"), int(QMetaType::UInt));
    QCOMPARE(QMetaType::type("qint64"), int(QMetaType::LongLong));
    QCOMPARE(QMetaType::type("quint64"), int(QMetaType::ULongLong));

    // make sure the qreal typeId is the type id of the type it's defined to
    QCOMPARE(QMetaType::type("qreal"), ::qMetaTypeId<qreal>());

    qRegisterMetaType<CustomString>("CustomString");
    QCOMPARE(QMetaType::type("CustomString"), ::qMetaTypeId<CustomString>());

    typedef Whity<double> WhityDouble;
    qRegisterMetaType<WhityDouble>("WhityDouble");
    QCOMPARE(QMetaType::type("WhityDouble"), ::qMetaTypeId<WhityDouble>());
}

class IsRegisteredDummyType { };

void tst_QMetaType::isRegistered_data()
{
    QTest::addColumn<int>("typeId");
    QTest::addColumn<bool>("registered");

    // predefined/custom types
    QTest::newRow("QMetaType::Void") << int(QMetaType::Void) << true;
    QTest::newRow("QMetaType::Int") << int(QMetaType::Int) << true;

    int dummyTypeId = qRegisterMetaType<IsRegisteredDummyType>("IsRegisteredDummyType");

    QTest::newRow("IsRegisteredDummyType") << dummyTypeId << true;

    // unknown types
    QTest::newRow("-1") << -1 << false;
    QTest::newRow("-42") << -42 << false;
    QTest::newRow("IsRegisteredDummyType + 1") << (dummyTypeId + 1) << false;
}

void tst_QMetaType::isRegistered()
{
    QFETCH(int, typeId);
    QFETCH(bool, registered);
    QCOMPARE(QMetaType::isRegistered(typeId), registered);
}

class RegUnreg
{
public:
    RegUnreg() {};
    RegUnreg(const RegUnreg &) {};
    ~RegUnreg() {};
};

void tst_QMetaType::unregisterType()
{
    // cannot unregister standard types
    int typeId = qRegisterMetaType<QList<QVariant> >("QList<QVariant>");
    QCOMPARE(QMetaType::isRegistered(typeId), true);
    QMetaType::unregisterType("QList<QVariant>");
    QCOMPARE(QMetaType::isRegistered(typeId), true);
    // allow unregister user types
    typeId = qRegisterMetaType<RegUnreg>("RegUnreg");
    QCOMPARE(QMetaType::isRegistered(typeId), true);
    QMetaType::unregisterType("RegUnreg");
    QCOMPARE(QMetaType::isRegistered(typeId), false);
}

void tst_QMetaType::QTBUG11316_registerStreamBuiltin()
{
    //should not crash;
    qRegisterMetaTypeStreamOperators<QString>("QString");
    qRegisterMetaTypeStreamOperators<QVariant>("QVariant");
}

QTEST_MAIN(tst_QMetaType)
#include "tst_qmetatype.moc"
