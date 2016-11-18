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


#include <QtCore>
#include <QtTest/QtTest>

#include "tst_qmetatype.h"
#include "tst_qvariant_common.h"

#ifdef Q_OS_LINUX
# include <pthread.h>
#endif

#include <algorithm>

// At least these specific versions of MSVC2010 has a severe performance problem with this file,
// taking about 1 hour to compile if the portion making use of variadic macros is enabled.
#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 160030319) && (_MSC_FULL_VER <= 160040219)
# define TST_QMETATYPE_BROKEN_COMPILER
#endif

// mingw gcc 4.8 also takes way too long, letting the CI system abort the test
#if defined(__MINGW32__)
# define TST_QMETATYPE_BROKEN_COMPILER
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
    void type_data();
    void type();
    void type_fromSubString_data();
    void type_fromSubString();
    void create_data();
    void create();
    void createCopy_data();
    void createCopy();
    void sizeOf_data();
    void sizeOf();
    void sizeOfStaticLess_data();
    void sizeOfStaticLess();
    void flags_data();
    void flags();
    void flagsStaticLess_data();
    void flagsStaticLess();
    void flagsBinaryCompatibility5_0_data();
    void flagsBinaryCompatibility5_0();
    void construct_data();
    void construct();
    void constructCopy_data();
    void constructCopy();
    void typedefs();
    void registerType();
    void isRegistered_data();
    void isRegistered();
    void isRegisteredStaticLess_data();
    void isRegisteredStaticLess();
    void isEnum();
    void registerStreamBuiltin();
    void automaticTemplateRegistration();
    void saveAndLoadBuiltin_data();
    void saveAndLoadBuiltin();
    void saveAndLoadCustom();
    void metaObject_data();
    void metaObject();
    void constexprMetaTypeIds();
    void constRefs();
    void convertCustomType_data();
    void convertCustomType();
    void compareCustomType_data();
    void compareCustomType();
    void compareCustomEqualOnlyType();
    void customDebugStream();
};

struct Foo { int i; };


class CustomQObject : public QObject
{
    Q_OBJECT
public:
    CustomQObject(QObject *parent = 0)
      : QObject(parent)
    {
    }
    enum CustomQEnum { Val1, Val2 };
    Q_ENUM(CustomQEnum)
};
class CustomGadget {
    Q_GADGET
};
class CustomGadget_NonDefaultConstructible {
    Q_GADGET
public:
    CustomGadget_NonDefaultConstructible(int) {};
};

class CustomNonQObject {};
class GadgetDerived : public CustomGadget {};

void tst_QMetaType::defined()
{
    QCOMPARE(int(QMetaTypeId2<QString>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<Foo>::Defined), 0);
    QCOMPARE(int(QMetaTypeId2<void*>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<int*>::Defined), 0);
    QCOMPARE(int(QMetaTypeId2<CustomQObject::CustomQEnum>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<CustomGadget>::Defined), 1);
    QVERIFY(!QMetaTypeId2<GadgetDerived>::Defined);
    QVERIFY(int(QMetaTypeId2<CustomQObject*>::Defined));
    QVERIFY(!QMetaTypeId2<CustomQObject>::Defined);
    QVERIFY(!QMetaTypeId2<CustomNonQObject>::Defined);
    QVERIFY(!QMetaTypeId2<CustomNonQObject*>::Defined);
    QVERIFY(!QMetaTypeId2<CustomGadget_NonDefaultConstructible>::Defined);
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
        Bar space[1];
        space[0].~Bar();

        const QByteArray postFix =  '_'
            + QByteArray::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));

        for (int i = 0; i < 1000; ++i) {
            const QByteArray name = "Bar" + QByteArray::number(i) + postFix;
            const char *nm = name.constData();
            int tp = qRegisterMetaType<Bar>(nm);
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
            pthread_yield();
#endif
            QMetaType info(tp);
            if (!info.isValid()) {
                ++failureCount;
                qWarning() << "Wrong typeInfo returned for" << tp;
            }
            if (!info.isRegistered()) {
                ++failureCount;
                qWarning() << name << "is not a registered metatype";
            }
            if (QMetaType::typeFlags(tp) != (QMetaType::NeedsConstruction | QMetaType::NeedsDestruction)) {
                ++failureCount;
                qWarning() << "Wrong typeInfo returned for" << tp;
            }
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
            void *buf1 = QMetaType::create(tp, 0);
            void *buf2 = QMetaType::create(tp, buf1);
            void *buf3 = info.create(tp, 0);
            void *buf4 = info.create(tp, buf1);

            QMetaType::construct(tp, space, 0);
            QMetaType::destruct(tp, space);
            QMetaType::construct(tp, space, buf1);
            QMetaType::destruct(tp, space);

            info.construct(space, 0);
            info.destruct(space);
            info.construct(space, buf1);
            info.destruct(space);

            if (!buf1) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, 0)";
            }
            if (!buf2) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, buf)";
            }
            if (!buf3) {
                ++failureCount;
                qWarning() << "Null buffer returned by info.create(tp, 0)";
            }
            if (!buf4) {
                ++failureCount;
                qWarning() << "Null buffer returned by infocreate(tp, buf)";
            }
            QMetaType::destroy(tp, buf1);
            QMetaType::destroy(tp, buf2);
            info.destroy(buf3);
            info.destroy(buf4);
        }
        new (space) Bar;
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
    struct QungTfu {};
}
Q_DECLARE_METATYPE(TestSpace::Foo)

#define ADD_TESTSPACE(F) TestSpace::F
Q_DECLARE_METATYPE(ADD_TESTSPACE(QungTfu))

void tst_QMetaType::namespaces()
{
    TestSpace::Foo nf = { 11.12 };
    QVariant v = QVariant::fromValue(nf);
    QCOMPARE(qvariant_cast<TestSpace::Foo>(v).d, 11.12);

    int qungTfuId = qRegisterMetaType<ADD_TESTSPACE(QungTfu)>();
    QCOMPARE(QMetaType::typeName(qungTfuId), "TestSpace::QungTfu");
}

void tst_QMetaType::qMetaTypeId()
{
    QCOMPARE(::qMetaTypeId<QString>(), int(QMetaType::QString));
    QCOMPARE(::qMetaTypeId<int>(), int(QMetaType::Int));
    QCOMPARE(::qMetaTypeId<TestSpace::Foo>(), QMetaType::type("TestSpace::Foo"));

    QCOMPARE(::qMetaTypeId<char>(), QMetaType::type("char"));
    QCOMPARE(::qMetaTypeId<uchar>(), QMetaType::type("unsigned char"));
    QCOMPARE(::qMetaTypeId<signed char>(), QMetaType::type("signed char"));
    QVERIFY(::qMetaTypeId<signed char>() != ::qMetaTypeId<char>());
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
    QTest::newRow(#RealType) << int(QMetaType::MetaTypeName) << #RealType;

void tst_QMetaType::typeName_data()
{
    QTest::addColumn<int>("aType");
    QTest::addColumn<QString>("aTypeName");

    QT_FOR_EACH_STATIC_TYPE(TYPENAME_DATA)
    QTest::newRow("QMetaType::UnknownType") << int(QMetaType::UnknownType) << static_cast<const char*>(0);

    QTest::newRow("Whity<double>") << ::qMetaTypeId<Whity<double> >() << QString::fromLatin1("Whity<double>");
    QTest::newRow("Whity<int>") << ::qMetaTypeId<Whity<int> >() << QString::fromLatin1("Whity<int>");
    QTest::newRow("Testspace::Foo") << ::qMetaTypeId<TestSpace::Foo>() << QString::fromLatin1("TestSpace::Foo");

    QTest::newRow("-1") << -1 << QString();
    QTest::newRow("-124125534") << -124125534 << QString();
    QTest::newRow("124125534") << 124125534 << QString();

    // automatic registration
    QTest::newRow("QList<int>") << ::qMetaTypeId<QList<int> >() << QString::fromLatin1("QList<int>");
    QTest::newRow("QHash<int,int>") << ::qMetaTypeId<QHash<int, int> >() << QString::fromLatin1("QHash<int,int>");
    QTest::newRow("QMap<int,int>") << ::qMetaTypeId<QMap<int, int> >() << QString::fromLatin1("QMap<int,int>");
    QTest::newRow("QVector<QList<int>>") << ::qMetaTypeId<QVector<QList<int> > >() << QString::fromLatin1("QVector<QList<int> >");
    QTest::newRow("QVector<QMap<int,int>>") << ::qMetaTypeId<QVector<QMap<int, int> > >() << QString::fromLatin1("QVector<QMap<int,int> >");

    QTest::newRow("CustomQObject*") << ::qMetaTypeId<CustomQObject*>() << QString::fromLatin1("CustomQObject*");
    QTest::newRow("CustomGadget") << ::qMetaTypeId<CustomGadget>() << QString::fromLatin1("CustomGadget");
    QTest::newRow("CustomQObject::CustomQEnum") << ::qMetaTypeId<CustomQObject::CustomQEnum>() << QString::fromLatin1("CustomQObject::CustomQEnum");
    QTest::newRow("Qt::ArrowType") << ::qMetaTypeId<Qt::ArrowType>() << QString::fromLatin1("Qt::ArrowType");
}

void tst_QMetaType::typeName()
{
    QFETCH(int, aType);
    QFETCH(QString, aTypeName);

    QString name = QString::fromLatin1(QMetaType::typeName(aType));

    QCOMPARE(name, aTypeName);
    QCOMPARE(name.toLatin1(), QMetaObject::normalizedType(name.toLatin1().constData()));
}

void tst_QMetaType::type_data()
{
    QTest::addColumn<int>("aType");
    QTest::addColumn<QByteArray>("aTypeName");

#define TST_QMETATYPE_TYPE_DATA(MetaTypeName, MetaTypeId, RealType)\
    QTest::newRow(#RealType) << int(QMetaType::MetaTypeName) << QByteArray( #RealType );
#define TST_QMETATYPE_TYPE_DATA_ALIAS(MetaTypeName, MetaTypeId, AliasType, RealTypeString)\
    QTest::newRow(RealTypeString) << int(QMetaType::MetaTypeName) << QByteArray( #AliasType );

    QTest::newRow("empty") << int(QMetaType::UnknownType) << QByteArray();

    QT_FOR_EACH_STATIC_TYPE(TST_QMETATYPE_TYPE_DATA)
    QT_FOR_EACH_STATIC_ALIAS_TYPE(TST_QMETATYPE_TYPE_DATA_ALIAS)

#undef TST_QMETATYPE_TYPE_DATA
#undef TST_METATYPE_TYPE_DATA_ALIAS
}

void tst_QMetaType::type()
{
    QFETCH(int, aType);
    QFETCH(QByteArray, aTypeName);

    // QMetaType::type(QByteArray)
    QCOMPARE(QMetaType::type(aTypeName), aType);
    // QMetaType::type(const char *)
    QCOMPARE(QMetaType::type(aTypeName.constData()), aType);
}

void tst_QMetaType::type_fromSubString_data()
{
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("expectedType");

    // The test string is defined in the test function below
    QTest::newRow("int") << 0 << 3 << int(QMetaType::Int);
    QTest::newRow("boo") << 3 << 3 << 0;
    QTest::newRow("bool") << 3 << 4 << int(QMetaType::Bool);
    QTest::newRow("intbool") << 0 << 7 << 0;
    QTest::newRow("QMetaType::Type") << 7 << 15 << ::qMetaTypeId<QMetaType::Type>();
    QTest::newRow("double") << 22 << 6 << int(QMetaType::Double);
}

void tst_QMetaType::type_fromSubString()
{
    static const char *types = "intboolQMetaType::Typedoublexxx";
    QFETCH(int, offset);
    QFETCH(int, size);
    QFETCH(int, expectedType);
    QByteArray ba = QByteArray::fromRawData(types + offset, size);
    QCOMPARE(QMetaType::type(ba), expectedType);
}

namespace {
    template <typename T>
    struct static_assert_trigger {
        Q_STATIC_ASSERT(( QMetaTypeId2<T>::IsBuiltIn ));
        enum { value = true };
    };
}

#define CHECK_BUILTIN(MetaTypeName, MetaTypeId, RealType) static_assert_trigger< RealType >::value &&
Q_STATIC_ASSERT(( FOR_EACH_CORE_METATYPE(CHECK_BUILTIN) true ));
#undef CHECK_BUILTIN
Q_STATIC_ASSERT(( QMetaTypeId2<QList<QVariant> >::IsBuiltIn));
Q_STATIC_ASSERT(( QMetaTypeId2<QMap<QString,QVariant> >::IsBuiltIn));
Q_STATIC_ASSERT(( QMetaTypeId2<QObject*>::IsBuiltIn));
Q_STATIC_ASSERT((!QMetaTypeId2<tst_QMetaType*>::IsBuiltIn)); // QObject subclass
Q_STATIC_ASSERT((!QMetaTypeId2<QList<int> >::IsBuiltIn));
Q_STATIC_ASSERT((!QMetaTypeId2<QMap<int,int> >::IsBuiltIn));
Q_STATIC_ASSERT((!QMetaTypeId2<QMetaType::Type>::IsBuiltIn));

void tst_QMetaType::create_data()
{
    QTest::addColumn<int>("type");
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(QMetaType::typeName(QMetaType::MetaTypeName)) << int(QMetaType::MetaTypeName);
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

template<int ID>
static void testCreateHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    QMetaType info(ID);
    void *actual1 = QMetaType::create(ID);
    void *actual2 = info.create();
    if (DefaultValueTraits<ID>::IsInitialized) {
        Type *expected = DefaultValueFactory<ID>::create();
        QCOMPARE(*static_cast<Type *>(actual1), *expected);
        QCOMPARE(*static_cast<Type *>(actual2), *expected);
        delete expected;
    }
    QMetaType::destroy(ID, actual1);
    info.destroy(actual2);
}

template<>
void testCreateHelper<QMetaType::Void>()
{
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

    QFETCH(int, type);
    TypeTestFunctionGetter::get(type)();
}

template<int ID>
static void testCreateCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    QMetaType info(ID);
    void *actual1 = QMetaType::create(ID, expected);
    void *actual2 = info.create(expected);
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
    QMetaType::destroy(ID, actual1);
    info.destroy(actual2);
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

    QFETCH(int, type);
    TypeTestFunctionGetter::get(type)();
}

void tst_QMetaType::sizeOf_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<size_t>("size");

    QTest::newRow("QMetaType::UnknownType") << int(QMetaType::UnknownType) << size_t(0);
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << int(QMetaType::MetaTypeName) << size_t(QTypeInfo<RealType>::sizeOf);
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW

    QTest::newRow("Whity<double>") << ::qMetaTypeId<Whity<double> >() << sizeof(Whity<double>);
    QTest::newRow("Whity<int>") << ::qMetaTypeId<Whity<int> >() << sizeof(Whity<int>);
    QTest::newRow("Testspace::Foo") << ::qMetaTypeId<TestSpace::Foo>() << sizeof(TestSpace::Foo);

    QTest::newRow("-1") << -1 << size_t(0);
    QTest::newRow("-124125534") << -124125534 << size_t(0);
    QTest::newRow("124125534") << 124125534 << size_t(0);
}

void tst_QMetaType::sizeOf()
{
    QFETCH(int, type);
    QFETCH(size_t, size);
    QCOMPARE(size_t(QMetaType::sizeOf(type)), size);
}

void tst_QMetaType::sizeOfStaticLess_data()
{
    sizeOf_data();
}

void tst_QMetaType::sizeOfStaticLess()
{
    QFETCH(int, type);
    QFETCH(size_t, size);
    QCOMPARE(size_t(QMetaType(type).sizeOf()), size);
}

struct CustomMovable {};
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(CustomMovable, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(CustomMovable);

class CustomObject : public QObject
{
    Q_OBJECT
public:
    CustomObject(QObject *parent = 0)
      : QObject(parent)
    {

    }
};
Q_DECLARE_METATYPE(CustomObject*);

struct SecondBase {};

class CustomMultiInheritanceObject : public QObject, SecondBase
{
    Q_OBJECT
public:
    CustomMultiInheritanceObject(QObject *parent = 0)
      : QObject(parent)
    {

    }
};
Q_DECLARE_METATYPE(CustomMultiInheritanceObject*);

class C { char _[4]; };
class M { char _[4]; };
class P { char _[4]; };

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(M, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(P, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

// avoid the comma:
typedef QPair<C,C> QPairCC;
typedef QPair<C,M> QPairCM;
typedef QPair<C,P> QPairCP;
typedef QPair<M,C> QPairMC;
typedef QPair<M,M> QPairMM;
typedef QPair<M,P> QPairMP;
typedef QPair<P,C> QPairPC;
typedef QPair<P,M> QPairPM;
typedef QPair<P,P> QPairPP;

Q_DECLARE_METATYPE(QPairCC)
Q_DECLARE_METATYPE(QPairCM)
Q_DECLARE_METATYPE(QPairCP)
Q_DECLARE_METATYPE(QPairMC)
Q_DECLARE_METATYPE(QPairMM)
Q_DECLARE_METATYPE(QPairMP)
Q_DECLARE_METATYPE(QPairPC)
Q_DECLARE_METATYPE(QPairPM)
Q_DECLARE_METATYPE(QPairPP)

enum FlagsDataEnum {};
Q_DECLARE_METATYPE(FlagsDataEnum);

void tst_QMetaType::flags_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isMovable");
    QTest::addColumn<bool>("isComplex");
    QTest::addColumn<bool>("isPointerToQObject");
    QTest::addColumn<bool>("isEnum");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId \
        << bool(!QTypeInfo<RealType>::isStatic) \
        << bool(QTypeInfo<RealType>::isComplex) \
        << bool(QtPrivate::IsPointerToTypeDerivedFromQObject<RealType>::Value) \
        << bool(std::is_enum<RealType>::value);
QT_FOR_EACH_STATIC_CORE_CLASS(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_CORE_POINTER(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
    QTest::newRow("TestSpace::Foo") << ::qMetaTypeId<TestSpace::Foo>() << false << true << false << false;
    QTest::newRow("Whity<double>") << ::qMetaTypeId<Whity<double> >() << false << true << false << false;
    QTest::newRow("CustomMovable") << ::qMetaTypeId<CustomMovable>() << true << true << false << false;
    QTest::newRow("CustomObject*") << ::qMetaTypeId<CustomObject*>() << true << false << true << false;
    QTest::newRow("CustomMultiInheritanceObject*") << ::qMetaTypeId<CustomMultiInheritanceObject*>() << true << false << true << false;
    QTest::newRow("QPair<C,C>") << ::qMetaTypeId<QPair<C,C> >() << false << true  << false << false;
    QTest::newRow("QPair<C,M>") << ::qMetaTypeId<QPair<C,M> >() << false << true  << false << false;
    QTest::newRow("QPair<C,P>") << ::qMetaTypeId<QPair<C,P> >() << false << true  << false << false;
    QTest::newRow("QPair<M,C>") << ::qMetaTypeId<QPair<M,C> >() << false << true  << false << false;
    QTest::newRow("QPair<M,M>") << ::qMetaTypeId<QPair<M,M> >() << true  << true  << false << false;
    QTest::newRow("QPair<M,P>") << ::qMetaTypeId<QPair<M,P> >() << true  << true  << false << false;
    QTest::newRow("QPair<P,C>") << ::qMetaTypeId<QPair<P,C> >() << false << true  << false << false;
    QTest::newRow("QPair<P,M>") << ::qMetaTypeId<QPair<P,M> >() << true  << true  << false << false;
    QTest::newRow("QPair<P,P>") << ::qMetaTypeId<QPair<P,P> >() << true  << false << false << false;
    QTest::newRow("FlagsDataEnum") << ::qMetaTypeId<FlagsDataEnum>() << true << true << false << true;

    // invalid ids.
    QTest::newRow("-1") << -1 << false << false << false << false;
    QTest::newRow("-124125534") << -124125534 << false << false << false << false;
    QTest::newRow("124125534") << 124125534 << false << false << false << false;
}

void tst_QMetaType::flags()
{
    QFETCH(int, type);
    QFETCH(bool, isMovable);
    QFETCH(bool, isComplex);
    QFETCH(bool, isPointerToQObject);
    QFETCH(bool, isEnum);

    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsConstruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsDestruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::MovableType), isMovable);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::PointerToQObject), isPointerToQObject);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::IsEnumeration), isEnum);
}

void tst_QMetaType::flagsStaticLess_data()
{
    flags_data();
}

void tst_QMetaType::flagsStaticLess()
{
    QFETCH(int, type);
    QFETCH(bool, isMovable);
    QFETCH(bool, isComplex);

    int flags = QMetaType(type).flags();
    QCOMPARE(bool(flags & QMetaType::NeedsConstruction), isComplex);
    QCOMPARE(bool(flags & QMetaType::NeedsDestruction), isComplex);
    QCOMPARE(bool(flags & QMetaType::MovableType), isMovable);
}

void tst_QMetaType::flagsBinaryCompatibility5_0_data()
{
    // Changing traits of a built-in type is illegal from BC point of view.
    // Traits are saved in code of an application and in the Qt library which means
    // that there may be a mismatch.
    // The test is loading data generated by this code:
    //
    //        QByteArray buffer;
    //        buffer.reserve(2 * QMetaType::User);
    //        for (quint32 i = 0; i < QMetaType::User; ++i) {
    //            if (QMetaType::isRegistered(i)) {
    //                buffer.append(i);
    //                buffer.append(quint32(QMetaType::typeFlags(i)));
    //            }
    //        }
    //        QFile file("/tmp/typeFlags.bin");
    //        file.open(QIODevice::WriteOnly);
    //        file.write(buffer);
    //        file.close();

    QTest::addColumn<quint32>("id");
    QTest::addColumn<quint32>("flags");

    QFile file(QFINDTESTDATA("typeFlags.bin"));
    file.open(QIODevice::ReadOnly);
    QByteArray buffer = file.readAll();

    for (int i = 0; i < buffer.size(); i+=2) {
        const quint32 id = buffer.at(i);
        const quint32 flags = buffer.at(i + 1);
        QVERIFY2(QMetaType::isRegistered(id), "A type could not be removed in BC way");
        QTest::newRow(QMetaType::typeName(id)) << id << flags;
    }
}

void tst_QMetaType::flagsBinaryCompatibility5_0()
{
    QFETCH(quint32, id);
    QFETCH(quint32, flags);

    quint32 mask_5_0 = 0x1fb; // Only compare the values that were already defined in 5.0

    QCOMPARE(quint32(QMetaType::typeFlags(id)) & mask_5_0, flags & mask_5_0);
}

void tst_QMetaType::construct_data()
{
    create_data();
}

template<int ID>
static void testConstructHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    QMetaType info(ID);
    int size = info.sizeOf();
    void *storage1 = qMallocAligned(size, Q_ALIGNOF(Type));
    void *actual1 = QMetaType::construct(ID, storage1, /*copy=*/0);
    void *storage2 = qMallocAligned(size, Q_ALIGNOF(Type));
    void *actual2 = info.construct(storage2, /*copy=*/0);
    QCOMPARE(actual1, storage1);
    QCOMPARE(actual2, storage2);
    if (DefaultValueTraits<ID>::IsInitialized) {
        Type *expected = DefaultValueFactory<ID>::create();
        QCOMPARE(*static_cast<Type *>(actual1), *expected);
        QCOMPARE(*static_cast<Type *>(actual2), *expected);
        delete expected;
    }
    QMetaType::destruct(ID, actual1);
    qFreeAligned(storage1);
    info.destruct(actual2);
    qFreeAligned(storage2);

    QVERIFY(QMetaType::construct(ID, 0, /*copy=*/0) == 0);
    QMetaType::destruct(ID, 0);

    QVERIFY(info.construct(0, /*copy=*/0) == 0);
    info.destruct(0);
}

template<>
void testConstructHelper<QMetaType::Void>()
{
    /*int size = */ QMetaType::sizeOf(QMetaType::Void);
    void *storage = 0;
    void *actual = QMetaType::construct(QMetaType::Void, storage, /*copy=*/0);
    QCOMPARE(actual, storage);
    if (DefaultValueTraits<QMetaType::Void>::IsInitialized) {
        QVERIFY(DefaultValueFactory<QMetaType::Void>::create());
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

    QFETCH(int, type);
    TypeTestFunctionGetter::get(type)();
}

template<int ID>
static void testConstructCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    QMetaType info(ID);
    int size = QMetaType::sizeOf(ID);
    QCOMPARE(info.sizeOf(), size);
    void *storage1 = qMallocAligned(size, Q_ALIGNOF(Type));
    void *actual1 = QMetaType::construct(ID, storage1, expected);
    void *storage2 = qMallocAligned(size, Q_ALIGNOF(Type));
    void *actual2 = info.construct(storage2, expected);
    QCOMPARE(actual1, storage1);
    QCOMPARE(actual2, storage2);
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
    QMetaType::destruct(ID, actual1);
    qFreeAligned(storage1);
    info.destruct(actual2);
    qFreeAligned(storage2);

    QVERIFY(QMetaType::construct(ID, 0, expected) == 0);
    QVERIFY(info.construct(0, expected) == 0);

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

    QFETCH(int, type);
    TypeTestFunctionGetter::get(type)();
}

typedef QString CustomString;
Q_DECLARE_METATYPE(CustomString) //this line is useless

void tst_QMetaType::typedefs()
{
    QCOMPARE(QMetaType::type("long long"), int(QMetaType::LongLong));
    QCOMPARE(QMetaType::type("unsigned long long"), int(QMetaType::ULongLong));
    QCOMPARE(QMetaType::type("qint8"), int(QMetaType::SChar));
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

void tst_QMetaType::registerType()
{
    // Built-in
    QCOMPARE(qRegisterMetaType<QString>("QString"), int(QMetaType::QString));
    QCOMPARE(qRegisterMetaType<QString>("QString"), int(QMetaType::QString));

    // Custom
    int fooId = qRegisterMetaType<TestSpace::Foo>("TestSpace::Foo");
    QVERIFY(fooId >= int(QMetaType::User));
    QCOMPARE(qRegisterMetaType<TestSpace::Foo>("TestSpace::Foo"), fooId);

    int movableId = qRegisterMetaType<CustomMovable>("CustomMovable");
    QVERIFY(movableId >= int(QMetaType::User));
    QCOMPARE(qRegisterMetaType<CustomMovable>("CustomMovable"), movableId);

    // Alias to built-in
    typedef QString MyString;

    QCOMPARE(qRegisterMetaType<MyString>("MyString"), int(QMetaType::QString));
    QCOMPARE(qRegisterMetaType<MyString>("MyString"), int(QMetaType::QString));

    QCOMPARE(QMetaType::type("MyString"), int(QMetaType::QString));

    // Alias to custom type
    typedef CustomMovable MyMovable;
    typedef TestSpace::Foo MyFoo;

    QCOMPARE(qRegisterMetaType<MyMovable>("MyMovable"), movableId);
    QCOMPARE(qRegisterMetaType<MyMovable>("MyMovable"), movableId);

    QCOMPARE(QMetaType::type("MyMovable"), movableId);

    QCOMPARE(qRegisterMetaType<MyFoo>("MyFoo"), fooId);
    QCOMPARE(qRegisterMetaType<MyFoo>("MyFoo"), fooId);

    QCOMPARE(QMetaType::type("MyFoo"), fooId);

    // cannot unregister built-in types
    QVERIFY(!QMetaType::unregisterType(QMetaType::QString));
    QCOMPARE(QMetaType::type("QString"), int(QMetaType::QString));
    QCOMPARE(QMetaType::type("MyString"), int(QMetaType::QString));

    // cannot unregister declared types
    QVERIFY(!QMetaType::unregisterType(fooId));
    QCOMPARE(QMetaType::type("TestSpace::Foo"), fooId);
    QCOMPARE(QMetaType::type("MyFoo"), fooId);

    // test unregistration of dynamic types (used by Qml)
    int unregId = QMetaType::registerType("UnregisterMe",
                                          0,
                                          0,
                                          QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Destruct,
                                          QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Construct,
                                          0, QMetaType::TypeFlags(), 0);
    QCOMPARE(QMetaType::registerTypedef("UnregisterMeTypedef", unregId), unregId);
    int unregId2 = QMetaType::registerType("UnregisterMe2",
                                           0,
                                           0,
                                           QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Destruct,
                                           QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Construct,
                                           0, QMetaType::TypeFlags(), 0);
    QVERIFY(unregId >= int(QMetaType::User));
    QCOMPARE(unregId2, unregId + 2);

    QVERIFY(QMetaType::unregisterType(unregId));
    QCOMPARE(QMetaType::type("UnregisterMe"), 0);
    QCOMPARE(QMetaType::type("UnregisterMeTypedef"), 0);
    QCOMPARE(QMetaType::type("UnregisterMe2"), unregId2);
    QVERIFY(QMetaType::unregisterType(unregId2));
    QCOMPARE(QMetaType::type("UnregisterMe2"), 0);

    // re-registering should always return the lowest free index
    QCOMPARE(QMetaType::registerType("UnregisterMe2",
                                     0,
                                     0,
                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Destruct,
                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Construct,
                                     0, QMetaType::TypeFlags(), 0), unregId);
    QCOMPARE(QMetaType::registerType("UnregisterMe",
                                     0,
                                     0,
                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Destruct,
                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<void>::Construct,
                                     0, QMetaType::TypeFlags(), 0), unregId + 1);
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
    QTest::newRow("QMetaType::UnknownType") << int(QMetaType::UnknownType) << false;
}

void tst_QMetaType::isRegistered()
{
    QFETCH(int, typeId);
    QFETCH(bool, registered);
    QCOMPARE(QMetaType::isRegistered(typeId), registered);
}

enum isEnumTest_Enum0 {};
struct isEnumTest_Struct0 { enum A{}; };

enum isEnumTest_Enum1 {};
struct isEnumTest_Struct1 {};

Q_DECLARE_METATYPE(isEnumTest_Struct1)
Q_DECLARE_METATYPE(isEnumTest_Enum1)

void tst_QMetaType::isEnum()
{
    int type0 = qRegisterMetaType<int>("int");
    QVERIFY((QMetaType::typeFlags(type0) & QMetaType::IsEnumeration) == 0);

    int type1 = qRegisterMetaType<isEnumTest_Enum0>("isEnumTest_Enum0");
    QVERIFY((QMetaType::typeFlags(type1) & QMetaType::IsEnumeration) == QMetaType::IsEnumeration);

    int type2 = qRegisterMetaType<isEnumTest_Struct0>("isEnumTest_Struct0");
    QVERIFY((QMetaType::typeFlags(type2) & QMetaType::IsEnumeration) == 0);

    int type3 = qRegisterMetaType<isEnumTest_Enum0 *>("isEnumTest_Enum0 *");
    QVERIFY((QMetaType::typeFlags(type3) & QMetaType::IsEnumeration) == 0);

    int type4 = qRegisterMetaType<isEnumTest_Struct0::A>("isEnumTest_Struct0::A");
    QVERIFY((QMetaType::typeFlags(type4) & QMetaType::IsEnumeration) == QMetaType::IsEnumeration);

    int type5 = ::qMetaTypeId<isEnumTest_Struct1>();
    QVERIFY((QMetaType::typeFlags(type5) & QMetaType::IsEnumeration) == 0);

    int type6 = ::qMetaTypeId<isEnumTest_Enum1>();
    QVERIFY((QMetaType::typeFlags(type6) & QMetaType::IsEnumeration) == QMetaType::IsEnumeration);
}

void tst_QMetaType::isRegisteredStaticLess_data()
{
    isRegistered_data();
}

void tst_QMetaType::isRegisteredStaticLess()
{
    QFETCH(int, typeId);
    QFETCH(bool, registered);
    QCOMPARE(QMetaType(typeId).isRegistered(), registered);
}

void tst_QMetaType::registerStreamBuiltin()
{
    //should not crash;
    qRegisterMetaTypeStreamOperators<QString>("QString");
    qRegisterMetaTypeStreamOperators<QVariant>("QVariant");
}

typedef QHash<int, uint> IntUIntHash;
Q_DECLARE_METATYPE(IntUIntHash)
typedef QMap<int, uint> IntUIntMap;
Q_DECLARE_METATYPE(IntUIntMap)
typedef QPair<int, uint> IntUIntPair;
Q_DECLARE_METATYPE(IntUIntPair)

struct CustomComparable
{
  CustomComparable(int i_ = 0) :i(i_) { }
  bool operator==(const CustomComparable &other) const
  {
      return i == other.i;
  }
  int i;
};

struct UnregisteredType {};

typedef QHash<int, CustomComparable> IntComparableHash;
Q_DECLARE_METATYPE(IntComparableHash)
typedef QMap<int, CustomComparable> IntComparableMap;
Q_DECLARE_METATYPE(IntComparableMap)
typedef QPair<int, CustomComparable> IntComparablePair;
Q_DECLARE_METATYPE(IntComparablePair)

typedef QHash<int, int> IntIntHash;
typedef int NaturalNumber;
class AutoMetaTypeObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(IntIntHash someHash READ someHash CONSTANT)
    Q_PROPERTY(NaturalNumber someInt READ someInt CONSTANT)
public:
    AutoMetaTypeObject(QObject *parent = 0)
      : QObject(parent), m_int(42)
    {
        m_hash.insert(4, 2);
    }

    QHash<int,int> someHash() const
    {
        return m_hash;
    }

    int someInt() const
    {
        return m_int;
    }

private:
    QHash<int,int> m_hash;
    int m_int;
};

class MyObject : public QObject
{
  Q_OBJECT
public:
  MyObject(QObject *parent = 0)
    : QObject(parent)
  {
  }
};
typedef MyObject* MyObjectPtr;
Q_DECLARE_METATYPE(MyObjectPtr)

#if defined(Q_COMPILER_VARIADIC_MACROS) && !defined(TST_QMETATYPE_BROKEN_COMPILER)
static QByteArray createTypeName(const char *begin, const char *va)
{
    QByteArray tn(begin);
    const QList<QByteArray> args = QByteArray(va).split(',');
    tn += args.first().trimmed();
    if (args.size() > 1) {
        QList<QByteArray>::const_iterator it = args.constBegin() + 1;
        const QList<QByteArray>::const_iterator end = args.constEnd();
        for (; it != end; ++it) {
            tn += ",";
            tn += it->trimmed();
        }
    }
    if (tn.endsWith('>'))
        tn += ' ';
    tn += '>';
    return tn;
}
#endif

Q_DECLARE_METATYPE(const void*)

void tst_QMetaType::automaticTemplateRegistration()
{
#define TEST_SEQUENTIAL_CONTAINER(CONTAINER, VALUE_TYPE) \
  { \
    CONTAINER<VALUE_TYPE> innerContainer; \
    innerContainer.push_back(42); \
    QVERIFY(*QVariant::fromValue(innerContainer).value<CONTAINER<VALUE_TYPE> >().begin() == 42); \
    QVector<CONTAINER<VALUE_TYPE> > outerContainer; \
    outerContainer << innerContainer; \
    QVERIFY(*QVariant::fromValue(outerContainer).value<QVector<CONTAINER<VALUE_TYPE> > >().first().begin() == 42); \
  }

  TEST_SEQUENTIAL_CONTAINER(QList, int)
  TEST_SEQUENTIAL_CONTAINER(std::vector, int)
  TEST_SEQUENTIAL_CONTAINER(std::list, int)

  {
    std::vector<bool> vecbool;
    vecbool.push_back(true);
    vecbool.push_back(false);
    vecbool.push_back(true);
    QVERIFY(QVariant::fromValue(vecbool).value<std::vector<bool> >().front() == true);
    QVector<std::vector<bool> > vectorList;
    vectorList << vecbool;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<std::vector<bool> > >().first().front() == true);
  }

  {
    QList<unsigned> unsignedList;
    unsignedList << 123;
    QVERIFY(QVariant::fromValue(unsignedList).value<QList<unsigned> >().first() == 123);
    QVector<QList<unsigned> > vectorList;
    vectorList << unsignedList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<unsigned> > >().first().first() == 123);
  }

  QCOMPARE(::qMetaTypeId<QVariantList>(), (int)QMetaType::QVariantList);
  QCOMPARE(::qMetaTypeId<QList<QVariant> >(), (int)QMetaType::QVariantList);

  TEST_SEQUENTIAL_CONTAINER(QList, QVariant)
  TEST_SEQUENTIAL_CONTAINER(std::vector, QVariant)
  TEST_SEQUENTIAL_CONTAINER(std::list, QVariant)

  {
    QList<QSharedPointer<QObject> > sharedPointerList;
    QObject *testObject = new QObject;
    sharedPointerList << QSharedPointer<QObject>(testObject);
    QVERIFY(QVariant::fromValue(sharedPointerList).value<QList<QSharedPointer<QObject> > >().first() == testObject);
    QVector<QList<QSharedPointer<QObject> > > vectorList;
    vectorList << sharedPointerList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<QSharedPointer<QObject> > > >().first().first() == testObject);
  }
  {
    IntIntHash intIntHash;
    intIntHash.insert(4, 2);
    QCOMPARE(QVariant::fromValue(intIntHash).value<IntIntHash>().value(4), 2);

    AutoMetaTypeObject amto;

    qRegisterMetaType<QHash<int, int> >("IntIntHash");
    QVariant hashVariant = amto.property("someHash");
    QCOMPARE(hashVariant.value<IntIntHash>().value(4), 2);

    qRegisterMetaType<int>("NaturalNumber");
    QVariant intVariant = amto.property("someInt");
    QCOMPARE(intVariant.value<NaturalNumber>(), 42);
  }
  {
    IntUIntHash intUIntHash;
    intUIntHash.insert(4, 2);
    QCOMPARE(QVariant::fromValue(intUIntHash).value<IntUIntHash>().value(4), (uint)2);
  }
  {
    IntComparableHash intComparableHash;
    CustomComparable m;
    intComparableHash.insert(4, m);
    QCOMPARE(QVariant::fromValue(intComparableHash).value<IntComparableHash>().value(4), m);
  }
  {
    QVariantHash variantHash;
    variantHash.insert(QStringLiteral("4"), 2);
    QCOMPARE(QVariant::fromValue(variantHash).value<QVariantHash>().value(QStringLiteral("4")), QVariant(2));
  }
  {
    typedef QMap<int, int> IntIntMap;
    IntIntMap intIntMap;
    intIntMap.insert(4, 2);
    QCOMPARE(QVariant::fromValue(intIntMap).value<IntIntMap>().value(4), 2);
  }
  {
    IntUIntMap intUIntMap;
    intUIntMap.insert(4, 2);
    QCOMPARE(QVariant::fromValue(intUIntMap).value<IntUIntMap>().value(4), (uint)2);
  }
  {
    IntComparableMap intComparableMap;
    CustomComparable m;
    intComparableMap.insert(4, m);
    QCOMPARE(QVariant::fromValue(intComparableMap).value<IntComparableMap>().value(4), m);
  }
  {
    QVariantMap variantMap;
    variantMap.insert(QStringLiteral("4"), 2);
    QCOMPARE(QVariant::fromValue(variantMap).value<QVariantMap>().value(QStringLiteral("4")), QVariant(2));
  }
  {
    typedef std::map<int, int> IntIntMap;
    IntIntMap intIntMap;
    intIntMap[4] = 2;
    QCOMPARE(QVariant::fromValue(intIntMap).value<IntIntMap>()[4], 2);
  }
  {
    typedef std::map<int, uint> StdIntUIntMap;
    StdIntUIntMap intUIntMap;
    intUIntMap[4] = 2;
    QCOMPARE(QVariant::fromValue(intUIntMap).value<StdIntUIntMap>()[4], (uint)2);
  }
  {
    typedef std::map<int, CustomObject*> StdMapIntCustomObject ;
    StdMapIntCustomObject intComparableMap;
    CustomObject *o = 0;
    intComparableMap[4] = o;
    QCOMPARE(QVariant::fromValue(intComparableMap).value<StdMapIntCustomObject >()[4], o);
  }
  {
    typedef std::map<QString, QVariant> StdMapStringVariant;
    StdMapStringVariant variantMap;
    variantMap[QStringLiteral("4")] = 2;
    QCOMPARE(QVariant::fromValue(variantMap).value<StdMapStringVariant>()[QStringLiteral("4")], QVariant(2));
  }
  {
    typedef QPair<int, int> IntIntPair;
    IntIntPair intIntPair = qMakePair(4, 2);
    QCOMPARE(QVariant::fromValue(intIntPair).value<IntIntPair>().first, 4);
    QCOMPARE(QVariant::fromValue(intIntPair).value<IntIntPair>().second, 2);
  }
  {
    IntUIntPair intUIntPair = qMakePair<int, uint>(4, 2);
    QCOMPARE(QVariant::fromValue(intUIntPair).value<IntUIntPair>().first, 4);
    QCOMPARE(QVariant::fromValue(intUIntPair).value<IntUIntPair>().second, (uint)2);
  }
  {
    CustomComparable m;
    IntComparablePair intComparablePair = qMakePair(4, m);
    QCOMPARE(QVariant::fromValue(intComparablePair).value<IntComparablePair>().first, 4);
    QCOMPARE(QVariant::fromValue(intComparablePair).value<IntComparablePair>().second, m);
  }
  {
    typedef std::pair<int, int> IntIntPair;
    IntIntPair intIntPair = std::make_pair(4, 2);
    QCOMPARE(QVariant::fromValue(intIntPair).value<IntIntPair>().first, 4);
    QCOMPARE(QVariant::fromValue(intIntPair).value<IntIntPair>().second, 2);
  }
  {
    typedef std::pair<int, uint> StdIntUIntPair;
    StdIntUIntPair intUIntPair = std::make_pair<int, uint>(4, 2);
    QCOMPARE(QVariant::fromValue(intUIntPair).value<StdIntUIntPair>().first, 4);
    QCOMPARE(QVariant::fromValue(intUIntPair).value<StdIntUIntPair>().second, (uint)2);
  }
  {
    typedef std::pair<int, CustomQObject*> StdIntComparablePair;
    CustomQObject* o = 0;
    StdIntComparablePair intComparablePair = std::make_pair(4, o);
    QCOMPARE(QVariant::fromValue(intComparablePair).value<StdIntComparablePair>().first, 4);
    QCOMPARE(QVariant::fromValue(intComparablePair).value<StdIntComparablePair>().second, o);
  }
  {
    typedef QHash<int, UnregisteredType> IntUnregisteredTypeHash;
    QVERIFY(qRegisterMetaType<IntUnregisteredTypeHash>("IntUnregisteredTypeHash") > 0);
  }
  {
    typedef QList<UnregisteredType> UnregisteredTypeList;
    QVERIFY(qRegisterMetaType<UnregisteredTypeList>("UnregisteredTypeList") > 0);
  }

#if defined(Q_COMPILER_VARIADIC_MACROS) && !defined(TST_QMETATYPE_BROKEN_COMPILER)

    #define FOR_EACH_STATIC_PRIMITIVE_TYPE(F) \
        F(bool) \
        F(int) \
        F(qulonglong) \
        F(double) \
        F(short) \
        F(char) \
        F(ulong) \
        F(uchar) \
        F(float) \
        F(QObject*) \
        F(QString) \
        F(CustomMovable)

    #define FOR_EACH_STATIC_PRIMITIVE_TYPE2(F, SecondaryRealName) \
        F(uint, SecondaryRealName) \
        F(qlonglong, SecondaryRealName) \
        F(char, SecondaryRealName) \
        F(uchar, SecondaryRealName) \
        F(QObject*, SecondaryRealName)

    #define CREATE_AND_VERIFY_CONTAINER(CONTAINER, ...) \
        { \
            CONTAINER< __VA_ARGS__ > t; \
            const QVariant v = QVariant::fromValue(t); \
            QByteArray tn = createTypeName(#CONTAINER "<", #__VA_ARGS__); \
            const int type = QMetaType::type(tn); \
            const int expectedType = ::qMetaTypeId<CONTAINER< __VA_ARGS__ > >(); \
            QCOMPARE(type, expectedType); \
        }

    #define FOR_EACH_1ARG_TEMPLATE_TYPE(F, TYPE) \
        F(QList, TYPE) \
        F(QVector, TYPE) \
        F(QLinkedList, TYPE) \
        F(QVector, TYPE) \
        F(QVector, TYPE) \
        F(QQueue, TYPE) \
        F(QStack, TYPE) \
        F(QSet, TYPE)

    #define PRINT_1ARG_TEMPLATE(RealName) \
        FOR_EACH_1ARG_TEMPLATE_TYPE(CREATE_AND_VERIFY_CONTAINER, RealName)

    #define FOR_EACH_2ARG_TEMPLATE_TYPE(F, RealName1, RealName2) \
        F(QHash, RealName1, RealName2) \
        F(QMap, RealName1, RealName2) \
        F(QPair, RealName1, RealName2)

    #define PRINT_2ARG_TEMPLATE_INTERNAL(RealName1, RealName2) \
        FOR_EACH_2ARG_TEMPLATE_TYPE(CREATE_AND_VERIFY_CONTAINER, RealName1, RealName2)

    #define PRINT_2ARG_TEMPLATE(RealName) \
        FOR_EACH_STATIC_PRIMITIVE_TYPE2(PRINT_2ARG_TEMPLATE_INTERNAL, RealName)

    #define REGISTER_TYPEDEF(TYPE, ARG1, ARG2) \
      qRegisterMetaType<TYPE <ARG1, ARG2> >(#TYPE "<" #ARG1 "," #ARG2 ">");

    REGISTER_TYPEDEF(QHash, int, uint)
    REGISTER_TYPEDEF(QMap, int, uint)
    REGISTER_TYPEDEF(QPair, int, uint)

    FOR_EACH_STATIC_PRIMITIVE_TYPE(
      PRINT_1ARG_TEMPLATE
    )
    FOR_EACH_STATIC_PRIMITIVE_TYPE(
      PRINT_2ARG_TEMPLATE
    )

    CREATE_AND_VERIFY_CONTAINER(QList, QList<QMap<int, QHash<char, QVariantList> > >)
    CREATE_AND_VERIFY_CONTAINER(QVector, void*)
    CREATE_AND_VERIFY_CONTAINER(QVector, const void*)
    CREATE_AND_VERIFY_CONTAINER(QList, void*)
    CREATE_AND_VERIFY_CONTAINER(QPair, void*, void*)
    CREATE_AND_VERIFY_CONTAINER(QHash, void*, void*)
    CREATE_AND_VERIFY_CONTAINER(QHash, const void*, const void*)

#endif // Q_COMPILER_VARIADIC_MACROS

#define TEST_OWNING_SMARTPOINTER(SMARTPOINTER, ELEMENT_TYPE, FLAG_TEST, FROMVARIANTFUNCTION) \
    { \
        SMARTPOINTER < ELEMENT_TYPE > sp(new ELEMENT_TYPE); \
        sp.data()->setObjectName("Test name"); \
        QVariant v = QVariant::fromValue(sp); \
        QCOMPARE(v.typeName(), #SMARTPOINTER "<" #ELEMENT_TYPE ">"); \
        QVERIFY(QMetaType::typeFlags(::qMetaTypeId<SMARTPOINTER < ELEMENT_TYPE > >()) & QMetaType::FLAG_TEST); \
        SMARTPOINTER < QObject > extractedPtr = FROMVARIANTFUNCTION<QObject>(v); \
        QCOMPARE(extractedPtr.data()->objectName(), sp.data()->objectName()); \
    }

    TEST_OWNING_SMARTPOINTER(QSharedPointer, QObject, SharedPointerToQObject, qSharedPointerFromVariant)
    TEST_OWNING_SMARTPOINTER(QSharedPointer, QFile, SharedPointerToQObject, qSharedPointerFromVariant)
    TEST_OWNING_SMARTPOINTER(QSharedPointer, QTemporaryFile, SharedPointerToQObject, qSharedPointerFromVariant)
    TEST_OWNING_SMARTPOINTER(QSharedPointer, MyObject, SharedPointerToQObject, qSharedPointerFromVariant)
#undef TEST_OWNING_SMARTPOINTER

#define TEST_NONOWNING_SMARTPOINTER(SMARTPOINTER, ELEMENT_TYPE, FLAG_TEST, FROMVARIANTFUNCTION) \
    { \
        ELEMENT_TYPE elem; \
        SMARTPOINTER < ELEMENT_TYPE > sp(&elem); \
        sp.data()->setObjectName("Test name"); \
        QVariant v = QVariant::fromValue(sp); \
        QCOMPARE(v.typeName(), #SMARTPOINTER "<" #ELEMENT_TYPE ">"); \
        QVERIFY(QMetaType::typeFlags(::qMetaTypeId<SMARTPOINTER < ELEMENT_TYPE > >()) & QMetaType::FLAG_TEST); \
        SMARTPOINTER < QObject > extractedPtr = FROMVARIANTFUNCTION<QObject>(v); \
        QCOMPARE(extractedPtr.data()->objectName(), sp.data()->objectName()); \
    }

    TEST_NONOWNING_SMARTPOINTER(QWeakPointer, QObject, WeakPointerToQObject, qWeakPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QWeakPointer, QFile, WeakPointerToQObject, qWeakPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QWeakPointer, QTemporaryFile, WeakPointerToQObject, qWeakPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QWeakPointer, MyObject, WeakPointerToQObject, qWeakPointerFromVariant)

    TEST_NONOWNING_SMARTPOINTER(QPointer, QObject, TrackingPointerToQObject, qPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QPointer, QFile, TrackingPointerToQObject, qPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QPointer, QTemporaryFile, TrackingPointerToQObject, qPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QPointer, MyObject, TrackingPointerToQObject, qPointerFromVariant)
#undef TEST_NONOWNING_SMARTPOINTER
}

template <typename T>
struct StreamingTraits
{
    enum { isStreamable = 1 }; // Streamable by default
};

// Non-streamable types

#define DECLARE_NONSTREAMABLE(Type) \
    template<> struct StreamingTraits<Type> { enum { isStreamable = 0 }; };

DECLARE_NONSTREAMABLE(void)
DECLARE_NONSTREAMABLE(void*)
DECLARE_NONSTREAMABLE(QModelIndex)
DECLARE_NONSTREAMABLE(QPersistentModelIndex)
DECLARE_NONSTREAMABLE(QJsonValue)
DECLARE_NONSTREAMABLE(QJsonObject)
DECLARE_NONSTREAMABLE(QJsonArray)
DECLARE_NONSTREAMABLE(QJsonDocument)
DECLARE_NONSTREAMABLE(QObject*)
DECLARE_NONSTREAMABLE(QWidget*)
DECLARE_NONSTREAMABLE(std::nullptr_t)

#define DECLARE_GUI_CLASS_NONSTREAMABLE(MetaTypeName, MetaTypeId, RealType) \
    DECLARE_NONSTREAMABLE(RealType)
QT_FOR_EACH_STATIC_GUI_CLASS(DECLARE_GUI_CLASS_NONSTREAMABLE)
#undef DECLARE_GUI_CLASS_NONSTREAMABLE

#define DECLARE_WIDGETS_CLASS_NONSTREAMABLE(MetaTypeName, MetaTypeId, RealType) \
    DECLARE_NONSTREAMABLE(RealType)
QT_FOR_EACH_STATIC_WIDGETS_CLASS(DECLARE_WIDGETS_CLASS_NONSTREAMABLE)
#undef DECLARE_WIDGETS_CLASS_NONSTREAMABLE

#undef DECLARE_NONSTREAMABLE

void tst_QMetaType::saveAndLoadBuiltin_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isStreamable");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId << bool(StreamingTraits<RealType>::isStreamable);
    QT_FOR_EACH_STATIC_TYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QMetaType::saveAndLoadBuiltin()
{
    QFETCH(int, type);
    QFETCH(bool, isStreamable);

    void *value = QMetaType::create(type);

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);
    QCOMPARE(QMetaType::save(stream, type, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable) {
        QVERIFY(QMetaType::load(stream, type, value)); // Hmmm, shouldn't it return false?
        QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    }

    stream.device()->seek(0);
    stream.resetStatus();
    QCOMPARE(QMetaType::load(stream, type, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable) {
        QVERIFY(QMetaType::load(stream, type, value)); // Hmmm, shouldn't it return false?
        QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    }

    QMetaType::destroy(type, value);
}

struct CustomStreamableType
{
    int a;
};
Q_DECLARE_METATYPE(CustomStreamableType)

QDataStream &operator<<(QDataStream &out, const CustomStreamableType &t)
{
    out << t.a; return out;
}

QDataStream &operator>>(QDataStream &in, CustomStreamableType &t)
{
    int a;
    in >> a;
    if (in.status() == QDataStream::Ok)
        t.a = a;
    return in;
}

void tst_QMetaType::saveAndLoadCustom()
{
    CustomStreamableType t;
    t.a = 123;

    int id = ::qMetaTypeId<CustomStreamableType>();
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);
    QVERIFY(!QMetaType::save(stream, id, &t));
    QCOMPARE(stream.status(), QDataStream::Ok);
    QVERIFY(!QMetaType::load(stream, id, &t));
    QCOMPARE(stream.status(), QDataStream::Ok);

    qRegisterMetaTypeStreamOperators<CustomStreamableType>("CustomStreamableType");
    QVERIFY(QMetaType::save(stream, id, &t));
    QCOMPARE(stream.status(), QDataStream::Ok);

    CustomStreamableType t2;
    t2.a = -1;
    QVERIFY(QMetaType::load(stream, id, &t2)); // Hmmm, shouldn't it return false?
    QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    QCOMPARE(t2.a, -1);

    stream.device()->seek(0);
    stream.resetStatus();
    QVERIFY(QMetaType::load(stream, id, &t2));
    QCOMPARE(stream.status(), QDataStream::Ok);
    QCOMPARE(t2.a, t.a);

    QVERIFY(QMetaType::load(stream, id, &t2)); // Hmmm, shouldn't it return false?
    QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
}

class MyGadget {
    Q_GADGET;
public:
    enum MyEnum { Val1, Val2, Val3 };
    Q_ENUM(MyEnum)
};

Q_DECLARE_METATYPE(MyGadget);
Q_DECLARE_METATYPE(const QMetaObject *);
Q_DECLARE_METATYPE(Qt::ScrollBarPolicy);
Q_DECLARE_METATYPE(MyGadget::MyEnum);

void tst_QMetaType::metaObject_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<const QMetaObject*>("result");
    QTest::addColumn<bool>("isGadget");
    QTest::addColumn<bool>("isQObjectPtr");

    QTest::newRow("QObject") << int(QMetaType::QObjectStar) << &QObject::staticMetaObject << false << true;
    QTest::newRow("QFile*") << ::qMetaTypeId<QFile*>() << &QFile::staticMetaObject << false << true;
    QTest::newRow("MyObject*") << ::qMetaTypeId<MyObject*>() << &MyObject::staticMetaObject << false << true;
    QTest::newRow("int") << int(QMetaType::Int) << static_cast<const QMetaObject *>(0) << false << false;
    QTest::newRow("QEasingCurve") << ::qMetaTypeId<QEasingCurve>() <<  &QEasingCurve::staticMetaObject << true << false;
    QTest::newRow("MyGadget") << ::qMetaTypeId<MyGadget>() <<  &MyGadget::staticMetaObject << true << false;
    QTest::newRow("MyEnum") << ::qMetaTypeId<MyGadget::MyEnum>() <<  &MyGadget::staticMetaObject << false << false;
    QTest::newRow("Qt::ScrollBarPolicy") << ::qMetaTypeId<Qt::ScrollBarPolicy>() <<  &QObject::staticQtMetaObject << false << false;
}


void tst_QMetaType::metaObject()
{
    QFETCH(int, type);
    QFETCH(const QMetaObject *, result);
    QFETCH(bool, isGadget);
    QFETCH(bool, isQObjectPtr);

    QCOMPARE(QMetaType::metaObjectForType(type), result);
    QMetaType mt(type);
    QCOMPARE(mt.metaObject(), result);
    QCOMPARE(!!(mt.flags() & QMetaType::IsGadget), isGadget);
    QCOMPARE(!!(mt.flags() & QMetaType::PointerToQObject), isQObjectPtr);
}

#define METATYPE_ID_FUNCTION(Type, MetaTypeId, Name) \
  case ::qMetaTypeId< Name >(): metaType = MetaTypeIdStruct<MetaTypeId>::Value;

#define REGISTER_METATYPE_FUNCTION(Type, MetaTypeId, Name) \
  case qRegisterMetaType< Name >(): metaType = RegisterMetaTypeStruct<MetaTypeId>::Value;

template<int>
struct MetaTypeIdStruct
{
};

template<int>
struct RegisterMetaTypeStruct
{
};

#define METATYPE_ID_STRUCT(Type, MetaTypeId, Name) \
template<> \
struct MetaTypeIdStruct< ::qMetaTypeId< Name >()> \
{ \
    enum { Value = ::qMetaTypeId< Name >() }; \
};

#define REGISTER_METATYPE_STRUCT(Type, MetaTypeId, Name) \
template<> \
struct RegisterMetaTypeStruct<qRegisterMetaType< Name >()> \
{ \
    enum { Value = qRegisterMetaType< Name >() }; \
};

#if defined(Q_COMPILER_CONSTEXPR)
QT_FOR_EACH_STATIC_TYPE(METATYPE_ID_STRUCT)
QT_FOR_EACH_STATIC_TYPE(REGISTER_METATYPE_STRUCT)

template<int i = ::qMetaTypeId<int>()>
struct MetaTypeIdStructDefaultTemplateValue
{
  enum { Value };
};

template<int i = qRegisterMetaType<int>()>
struct RegisterMetaTypeStructDefaultTemplateValue
{
  enum { Value };
};
#endif

void tst_QMetaType::constexprMetaTypeIds()
{
#if defined(Q_COMPILER_CONSTEXPR)
    int id = 0;
    int metaType;

    switch(id) {
      QT_FOR_EACH_STATIC_TYPE(METATYPE_ID_FUNCTION)
      metaType = MetaTypeIdStructDefaultTemplateValue<>::Value;
    default:;
    }

    switch (id) {
      QT_FOR_EACH_STATIC_TYPE(REGISTER_METATYPE_FUNCTION)
      metaType = RegisterMetaTypeStructDefaultTemplateValue<>::Value;
    default:;
    }
    Q_UNUSED(metaType);
#else
    QSKIP("The test needs a compiler supporting constexpr");
#endif
}

void tst_QMetaType::constRefs()
{
    QCOMPARE(::qMetaTypeId<const int &>(), ::qMetaTypeId<int>());
    QCOMPARE(::qMetaTypeId<const QString &>(), ::qMetaTypeId<QString>());
    QCOMPARE(::qMetaTypeId<const CustomMovable &>(), ::qMetaTypeId<CustomMovable>());
    QCOMPARE(::qMetaTypeId<const QList<CustomMovable> &>(), ::qMetaTypeId<QList<CustomMovable> >());
#if defined(Q_COMPILER_CONSTEXPR)
    Q_STATIC_ASSERT(::qMetaTypeId<const int &>() == ::qMetaTypeId<int>());
#endif
}

struct CustomConvertibleType
{
    explicit CustomConvertibleType(const QVariant &foo = QVariant()) : m_foo(foo) {}
    virtual ~CustomConvertibleType() {}
    QString toString() const { return m_foo.toString(); }
    operator QPoint() const { return QPoint(12, 34); }
    template<typename To>
    To convert() const { return s_value.value<To>();}
    template<typename To>
    To convertOk(bool *ok) const { *ok = s_ok; return s_value.value<To>();}

    QVariant m_foo;
    static QVariant s_value;
    static bool s_ok;
};

bool operator<(const CustomConvertibleType &lhs, const CustomConvertibleType &rhs)
{ return lhs.m_foo < rhs.m_foo; }
bool operator==(const CustomConvertibleType &lhs, const CustomConvertibleType &rhs)
{ return lhs.m_foo == rhs.m_foo; }
bool operator!=(const CustomConvertibleType &lhs, const CustomConvertibleType &rhs)
{ return !operator==(lhs, rhs); }

QVariant CustomConvertibleType::s_value;
bool CustomConvertibleType::s_ok = true;

struct CustomConvertibleType2
{
    // implicit
    CustomConvertibleType2(const CustomConvertibleType &t = CustomConvertibleType())
        : m_foo(t.m_foo) {}
    virtual ~CustomConvertibleType2() {}

    QVariant m_foo;
};

struct CustomDebugStreamableType
{
    QString toString() const { return "test"; }
};

QDebug operator<<(QDebug dbg, const CustomDebugStreamableType&)
{
    return dbg << "string-content";
}

bool operator==(const CustomConvertibleType2 &lhs, const CustomConvertibleType2 &rhs)
{ return lhs.m_foo == rhs.m_foo; }
bool operator!=(const CustomConvertibleType2 &lhs, const CustomConvertibleType2 &rhs)
{ return !operator==(lhs, rhs); }


struct CustomEqualsOnlyType
{
    explicit CustomEqualsOnlyType(int value = 0) : val(value) {}
    virtual ~CustomEqualsOnlyType() {}

    int val;
};
bool operator==(const CustomEqualsOnlyType &lhs, const CustomEqualsOnlyType &rhs)
{ return lhs.val == rhs.val;}
bool operator!=(const CustomEqualsOnlyType &lhs, const CustomEqualsOnlyType &rhs)
{ return !operator==(lhs, rhs); }

Q_DECLARE_METATYPE(CustomConvertibleType);
Q_DECLARE_METATYPE(CustomConvertibleType2);
Q_DECLARE_METATYPE(CustomDebugStreamableType);
Q_DECLARE_METATYPE(CustomEqualsOnlyType);

template<typename T, typename U>
U convert(const T &t)
{
    return t;
}

template<typename From>
struct ConvertFunctor
{
    CustomConvertibleType operator()(const From& f) const
    {
        return CustomConvertibleType(QVariant::fromValue(f));
    }
};

template<typename From, typename To>
bool hasRegisteredConverterFunction()
{
    return QMetaType::hasRegisteredConverterFunction<From, To>();
}

template<typename From, typename To>
void testCustomTypeNotYetConvertible()
{
    QVERIFY((!hasRegisteredConverterFunction<From, To>()));
    QVERIFY((!QVariant::fromValue<From>(From()).canConvert(qMetaTypeId<To>())));
}

template<typename From, typename To>
void testCustomTypeConvertible()
{
    QVERIFY((hasRegisteredConverterFunction<From, To>()));
    QVERIFY((QVariant::fromValue<From>(From()).canConvert(qMetaTypeId<To>())));
}

void customTypeNotYetConvertible()
{
    testCustomTypeNotYetConvertible<CustomConvertibleType, QString>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, bool>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, int>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, double>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, float>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QRect>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QRectF>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QPoint>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QPointF>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QSize>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QSizeF>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QLine>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QLineF>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, QChar>();
    testCustomTypeNotYetConvertible<QString, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<bool, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<int, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<double, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<float, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QRect, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QRectF, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QPoint, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QPointF, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QSize, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QSizeF, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QLine, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QLineF, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<QChar, CustomConvertibleType>();
    testCustomTypeNotYetConvertible<CustomConvertibleType, CustomConvertibleType2>();
}

void registerCustomTypeConversions()
{
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QString>(&CustomConvertibleType::convertOk<QString>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, bool>(&CustomConvertibleType::convert<bool>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, int>(&CustomConvertibleType::convertOk<int>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, double>(&CustomConvertibleType::convert<double>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, float>(&CustomConvertibleType::convertOk<float>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QRect>(&CustomConvertibleType::convert<QRect>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QRectF>(&CustomConvertibleType::convertOk<QRectF>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QPoint>(convert<CustomConvertibleType,QPoint>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QPointF>(&CustomConvertibleType::convertOk<QPointF>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QSize>(&CustomConvertibleType::convert<QSize>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QSizeF>(&CustomConvertibleType::convertOk<QSizeF>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QLine>(&CustomConvertibleType::convert<QLine>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QLineF>(&CustomConvertibleType::convertOk<QLineF>)));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, QChar>(&CustomConvertibleType::convert<QChar>)));
    QVERIFY((QMetaType::registerConverter<QString, CustomConvertibleType>(ConvertFunctor<QString>())));
    QVERIFY((QMetaType::registerConverter<bool, CustomConvertibleType>(ConvertFunctor<bool>())));
    QVERIFY((QMetaType::registerConverter<int, CustomConvertibleType>(ConvertFunctor<int>())));
    QVERIFY((QMetaType::registerConverter<double, CustomConvertibleType>(ConvertFunctor<double>())));
    QVERIFY((QMetaType::registerConverter<float, CustomConvertibleType>(ConvertFunctor<float>())));
    QVERIFY((QMetaType::registerConverter<QRect, CustomConvertibleType>(ConvertFunctor<QRect>())));
    QVERIFY((QMetaType::registerConverter<QRectF, CustomConvertibleType>(ConvertFunctor<QRectF>())));
    QVERIFY((QMetaType::registerConverter<QPoint, CustomConvertibleType>(ConvertFunctor<QPoint>())));
    QVERIFY((QMetaType::registerConverter<QPointF, CustomConvertibleType>(ConvertFunctor<QPointF>())));
    QVERIFY((QMetaType::registerConverter<QSize, CustomConvertibleType>(ConvertFunctor<QSize>())));
    QVERIFY((QMetaType::registerConverter<QSizeF, CustomConvertibleType>(ConvertFunctor<QSizeF>())));
    QVERIFY((QMetaType::registerConverter<QLine, CustomConvertibleType>(ConvertFunctor<QLine>())));
    QVERIFY((QMetaType::registerConverter<QLineF, CustomConvertibleType>(ConvertFunctor<QLineF>())));
    QVERIFY((QMetaType::registerConverter<QChar, CustomConvertibleType>(ConvertFunctor<QChar>())));
    QVERIFY((QMetaType::registerConverter<CustomConvertibleType, CustomConvertibleType2>()));
    QTest::ignoreMessage(QtWarningMsg, "Type conversion already registered from type CustomConvertibleType to type CustomConvertibleType2");
    QVERIFY((!QMetaType::registerConverter<CustomConvertibleType, CustomConvertibleType2>()));
}

void tst_QMetaType::convertCustomType_data()
{
    customTypeNotYetConvertible();
    registerCustomTypeConversions();

    QTest::addColumn<bool>("ok");
    QTest::addColumn<QString>("testQString");
    QTest::addColumn<bool>("testBool");
    QTest::addColumn<int>("testInt");
    QTest::addColumn<double>("testDouble");
    QTest::addColumn<float>("testFloat");
    QTest::addColumn<QRect>("testQRect");
    QTest::addColumn<QRectF>("testQRectF");
    QTest::addColumn<QPoint>("testQPoint");
    QTest::addColumn<QPointF>("testQPointF");
    QTest::addColumn<QSize>("testQSize");
    QTest::addColumn<QSizeF>("testQSizeF");
    QTest::addColumn<QLine>("testQLine");
    QTest::addColumn<QLineF>("testQLineF");
    QTest::addColumn<QChar>("testQChar");
    QTest::addColumn<CustomConvertibleType>("testCustom");

    QTest::newRow("default") << true
                             << QString::fromLatin1("string") << true << 15
                             << double(3.14) << float(3.6) << QRect(1, 2, 3, 4)
                             << QRectF(1.4, 1.9, 10.9, 40.2) << QPoint(12, 34)
                             << QPointF(9.2, 2.7) << QSize(4, 9) << QSizeF(3.3, 9.8)
                             << QLine(3, 9, 29, 4) << QLineF(38.9, 28.9, 102.3, 0.0)
                             << QChar('Q') << CustomConvertibleType(QString::fromLatin1("test"));
    QTest::newRow("not ok") << false
                            << QString::fromLatin1("string") << true << 15
                            << double(3.14) << float(3.6) << QRect(1, 2, 3, 4)
                            << QRectF(1.4, 1.9, 10.9, 40.2) << QPoint(12, 34)
                            << QPointF(9.2, 2.7) << QSize(4, 9) << QSizeF(3.3, 9.8)
                            << QLine(3, 9, 29, 4) << QLineF()
                            << QChar('Q') << CustomConvertibleType(42);
}

void tst_QMetaType::convertCustomType()
{
    QFETCH(bool, ok);
    CustomConvertibleType::s_ok = ok;

    CustomConvertibleType t;
    QVariant v = QVariant::fromValue(t);
    QFETCH(QString, testQString);
    CustomConvertibleType::s_value = testQString;
    QCOMPARE(v.toString(), ok ? testQString : QString());
    QCOMPARE(v.value<QString>(), ok ? testQString : QString());
    QVERIFY(CustomConvertibleType::s_value.canConvert<CustomConvertibleType>());
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toString()), testQString);

    QFETCH(bool, testBool);
    CustomConvertibleType::s_value = testBool;
    QCOMPARE(v.toBool(), testBool);
    QCOMPARE(v.value<bool>(), testBool);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toBool()), testBool);

    QFETCH(int, testInt);
    CustomConvertibleType::s_value = testInt;
    QCOMPARE(v.toInt(), ok ? testInt : 0);
    QCOMPARE(v.value<int>(), ok ? testInt : 0);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toInt()), testInt);

    QFETCH(double, testDouble);
    CustomConvertibleType::s_value = testDouble;
    QCOMPARE(v.toDouble(), testDouble);
    QCOMPARE(v.value<double>(), testDouble);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toDouble()), testDouble);

    QFETCH(float, testFloat);
    CustomConvertibleType::s_value = testFloat;
    QCOMPARE(v.toFloat(), ok ? testFloat : 0.0);
    QCOMPARE(v.value<float>(), ok ? testFloat : 0.0);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toFloat()), testFloat);

    QFETCH(QRect, testQRect);
    CustomConvertibleType::s_value = testQRect;
    QCOMPARE(v.toRect(), testQRect);
    QCOMPARE(v.value<QRect>(), testQRect);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toRect()), testQRect);

    QFETCH(QRectF, testQRectF);
    CustomConvertibleType::s_value = testQRectF;
    QCOMPARE(v.toRectF(), ok ? testQRectF : QRectF());
    QCOMPARE(v.value<QRectF>(), ok ? testQRectF : QRectF());
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toRectF()), testQRectF);

    QFETCH(QPoint, testQPoint);
    CustomConvertibleType::s_value = testQPoint;
    QCOMPARE(v.toPoint(), testQPoint);
    QCOMPARE(v.value<QPoint>(), testQPoint);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toPoint()), testQPoint);

    QFETCH(QPointF, testQPointF);
    CustomConvertibleType::s_value = testQPointF;
    QCOMPARE(v.toPointF(), ok ? testQPointF : QPointF());
    QCOMPARE(v.value<QPointF>(), ok ? testQPointF : QPointF());
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toPointF()), testQPointF);

    QFETCH(QSize, testQSize);
    CustomConvertibleType::s_value = testQSize;
    QCOMPARE(v.toSize(), testQSize);
    QCOMPARE(v.value<QSize>(), testQSize);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toSize()), testQSize);

    QFETCH(QSizeF, testQSizeF);
    CustomConvertibleType::s_value = testQSizeF;
    QCOMPARE(v.toSizeF(), ok ? testQSizeF : QSizeF());
    QCOMPARE(v.value<QSizeF>(), ok ? testQSizeF : QSizeF());
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toSizeF()), testQSizeF);

    QFETCH(QLine, testQLine);
    CustomConvertibleType::s_value = testQLine;
    QCOMPARE(v.toLine(), testQLine);
    QCOMPARE(v.value<QLine>(), testQLine);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toLine()), testQLine);

    QFETCH(QLineF, testQLineF);
    CustomConvertibleType::s_value = testQLineF;
    QCOMPARE(v.toLineF(), ok ? testQLineF : QLineF());
    QCOMPARE(v.value<QLineF>(), ok ? testQLineF : QLineF());
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toLineF()), testQLineF);

    QFETCH(QChar, testQChar);
    CustomConvertibleType::s_value = testQChar;
    QCOMPARE(v.toChar(), testQChar);
    QCOMPARE((CustomConvertibleType::s_value.value<CustomConvertibleType>().m_foo.toChar()), testQChar);

    QFETCH(CustomConvertibleType, testCustom);
    v = QVariant::fromValue(testCustom);
    QVERIFY(v.canConvert(::qMetaTypeId<CustomConvertibleType2>()));
    QCOMPARE(v.value<CustomConvertibleType2>().m_foo, testCustom.m_foo);
}

void tst_QMetaType::compareCustomType_data()
{
    QMetaType::registerComparators<CustomConvertibleType>();

    QTest::addColumn<QVariantList>("unsorted");
    QTest::addColumn<QVariantList>("sorted");

    QTest::newRow("int") << (QVariantList() << 37 << 458 << 1 << 243 << -4 << 383)
                         << (QVariantList() << -4 << 1 << 37 << 243 << 383 << 458);

    QTest::newRow("dobule") << (QVariantList() << 4934.93 << 0.0 << 302.39 << -39.0)
                            << (QVariantList() << -39.0 << 0.0 << 302.39 << 4934.93);

    QTest::newRow("QString") << (QVariantList() << "Hello" << "World" << "this" << "is" << "a" << "test")
                             << (QVariantList() << "a" << "Hello" << "is" << "test" << "this" << "World");

    QTest::newRow("QTime") << (QVariantList() << QTime(14, 39) << QTime(0, 0) << QTime(18, 18) << QTime(9, 27))
                           << (QVariantList() << QTime(0, 0) << QTime(9, 27) << QTime(14, 39) << QTime(18, 18));

    QTest::newRow("QDate") << (QVariantList() << QDate(2013, 3, 23) << QDate(1900, 12, 1) << QDate(2001, 2, 2) << QDate(1982, 12, 16))
                           << (QVariantList() << QDate(1900, 12, 1) << QDate(1982, 12, 16) << QDate(2001, 2, 2) << QDate(2013, 3, 23));

    QTest::newRow("mixed")   << (QVariantList() << "Hello" << "World" << QChar('a') << 38 << QChar('z') << -39 << 4.6)
                             << (QVariantList() << -39 << 4.6 << 38 << QChar('a') << "Hello" << "World" << QChar('z'));

    QTest::newRow("custom") << (QVariantList() << QVariant::fromValue(CustomConvertibleType(1)) << QVariant::fromValue(CustomConvertibleType(100)) << QVariant::fromValue(CustomConvertibleType(50)))
                            << (QVariantList() << QVariant::fromValue(CustomConvertibleType(1)) << QVariant::fromValue(CustomConvertibleType(50)) << QVariant::fromValue(CustomConvertibleType(100)));
}

void tst_QMetaType::compareCustomType()
{
    QFETCH(QVariantList, unsorted);
    QFETCH(QVariantList, sorted);
    std::sort(unsorted.begin(), unsorted.end());
    QCOMPARE(unsorted, sorted);
}

void tst_QMetaType::compareCustomEqualOnlyType()
{
    int metaTypeId = qRegisterMetaType<CustomEqualsOnlyType>();
    QMetaType::registerEqualsComparator<CustomEqualsOnlyType>();
    int result;

    CustomEqualsOnlyType val50(50);
    CustomEqualsOnlyType val100(100);
    CustomEqualsOnlyType val100x(100);

    QVariant variant50 = QVariant::fromValue(val50);
    QVariant variant100 = QVariant::fromValue(val100);
    QVariant variant100x = QVariant::fromValue(val100x);

    QVERIFY(variant50 != variant100);
    QVERIFY(variant50 != variant100x);
    QVERIFY(variant100 != variant50);
    QVERIFY(variant100x != variant50);
    QCOMPARE(variant100, variant100x);
    QCOMPARE(variant100, variant100);

    // compare always fails
    QVERIFY(!(variant50 < variant50));
    QVERIFY(!(variant50 < variant100));
    QVERIFY(!(variant100 < variant50));

    // check QMetaType::compare works/doesn't crash for equals only comparators
    bool wasSuccess = QMetaType::compare(variant50.constData(), variant50.constData(),
                                          metaTypeId, &result);
    QCOMPARE(result, 0);
    QVERIFY(wasSuccess);
    wasSuccess = QMetaType::compare(variant100.constData(), variant100x.constData(),
                                          metaTypeId, &result);
    QCOMPARE(result, 0);
    QVERIFY(wasSuccess);

    wasSuccess = QMetaType::compare(variant50.constData(), variant100.constData(),
                                          metaTypeId, &result);
    QVERIFY(!wasSuccess);

    // check QMetaType::equals works for equals only comparator
    wasSuccess = QMetaType::equals(variant50.constData(), variant50.constData(),
                                   metaTypeId, &result);
    QCOMPARE(result, 0);
    QVERIFY(wasSuccess);
    wasSuccess = QMetaType::equals(variant100.constData(), variant100.constData(),
                                   metaTypeId, &result);
    QCOMPARE(result, 0);
    QVERIFY(wasSuccess);
    wasSuccess = QMetaType::equals(variant100x.constData(), variant100x.constData(),
                                   metaTypeId, &result);
    QCOMPARE(result, 0);
    QVERIFY(wasSuccess);
    wasSuccess = QMetaType::equals(variant100.constData(), variant100x.constData(),
                                   metaTypeId, &result);
    QCOMPARE(result, 0);
    QVERIFY(wasSuccess);
    wasSuccess = QMetaType::equals(variant50.constData(), variant100.constData(),
                                   metaTypeId, &result);
    QCOMPARE(result, -1);
    QVERIFY(wasSuccess);
    wasSuccess = QMetaType::equals(variant50.constData(), variant100x.constData(),
                                   metaTypeId, &result);
    QCOMPARE(result, -1);
    QVERIFY(wasSuccess);

    //check QMetaType::equals for type w/o equals comparator being registered
    CustomMovable movable1;
    CustomMovable movable2;
    wasSuccess = QMetaType::equals(&movable1, &movable2,
                                   qRegisterMetaType<CustomMovable>(), &result);
    QVERIFY(!wasSuccess);

}

struct MessageHandlerCustom : public MessageHandler
{
    MessageHandlerCustom(const int typeId)
        : MessageHandler(typeId, handler)
    {}
    static void handler(QtMsgType, const QMessageLogContext &, const QString &msg)
    {
        QCOMPARE(msg.trimmed(), expectedMessage.trimmed());
    }
    static QString expectedMessage;
};

QString MessageHandlerCustom::expectedMessage;

void tst_QMetaType::customDebugStream()
{
    MessageHandlerCustom handler(::qMetaTypeId<CustomDebugStreamableType>());
    QVariant v1 = QVariant::fromValue(CustomDebugStreamableType());
    handler.expectedMessage = "QVariant(CustomDebugStreamableType, )";
    qDebug() << v1;

    QMetaType::registerConverter<CustomDebugStreamableType, QString>(&CustomDebugStreamableType::toString);
    handler.expectedMessage = "QVariant(CustomDebugStreamableType, \"test\")";
    qDebug() << v1;

    QMetaType::registerDebugStreamOperator<CustomDebugStreamableType>();
    handler.expectedMessage = "QVariant(CustomDebugStreamableType, string-content)";
    qDebug() << v1;
}

// Compile-time test, it should be possible to register function pointer types
class Undefined;

typedef Undefined (*UndefinedFunction0)();
typedef Undefined (*UndefinedFunction1)(Undefined);
typedef Undefined (*UndefinedFunction2)(Undefined, Undefined);
typedef Undefined (*UndefinedFunction3)(Undefined, Undefined, Undefined);
typedef Undefined (*UndefinedFunction4)(Undefined, Undefined, Undefined, Undefined, Undefined, Undefined, Undefined, Undefined);

Q_DECLARE_METATYPE(UndefinedFunction0);
Q_DECLARE_METATYPE(UndefinedFunction1);
Q_DECLARE_METATYPE(UndefinedFunction2);
Q_DECLARE_METATYPE(UndefinedFunction3);
#ifdef Q_COMPILER_VARIADIC_TEMPLATES
Q_DECLARE_METATYPE(UndefinedFunction4);
#endif

QTEST_MAIN(tst_QMetaType)
#include "tst_qmetatype.moc"
