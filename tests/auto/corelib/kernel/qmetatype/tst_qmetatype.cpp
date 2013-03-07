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


#include <QtCore>
#include <QtTest/QtTest>

#ifdef Q_OS_LINUX
# include <pthread.h>
#endif

// At least these specific versions of MSVC2010 has a severe performance problem with this file,
// taking about 1 hour to compile if the portion making use of variadic macros is enabled.
#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 160030319) && (_MSC_FULL_VER <= 160040219)
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
    void metaObject();
    void constexprMetaTypeIds();
    void constRefs();
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
};

class CustomNonQObject {};

void tst_QMetaType::defined()
{
    QCOMPARE(int(QMetaTypeId2<QString>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<Foo>::Defined), 0);
    QCOMPARE(int(QMetaTypeId2<void*>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<int*>::Defined), 0);
    QVERIFY(QMetaTypeId2<CustomQObject*>::Defined);
    QVERIFY(!QMetaTypeId2<CustomQObject>::Defined);
    QVERIFY(!QMetaTypeId2<CustomNonQObject>::Defined);
    QVERIFY(!QMetaTypeId2<CustomNonQObject*>::Defined);
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

        for (int i = 0; i < 1000; ++i) {
            const QByteArray name = QString("Bar%1_%2").arg(i).arg((size_t)QThread::currentThreadId()).toLatin1();
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

}
Q_DECLARE_METATYPE(TestSpace::Foo)

void tst_QMetaType::namespaces()
{
    TestSpace::Foo nf = { 11.12 };
    QVariant v = QVariant::fromValue(nf);
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
    QTest::newRow(#RealType) << QMetaType::MetaTypeName << #RealType;

void tst_QMetaType::typeName_data()
{
    QTest::addColumn<QMetaType::Type>("aType");
    QTest::addColumn<QString>("aTypeName");

    QT_FOR_EACH_STATIC_TYPE(TYPENAME_DATA)
    QTest::newRow("QMetaType::UnknownType") << QMetaType::UnknownType << static_cast<const char*>(0);

    QTest::newRow("Whity<double>") << static_cast<QMetaType::Type>(::qMetaTypeId<Whity<double> >()) << QString::fromLatin1("Whity<double>");
    QTest::newRow("Whity<int>") << static_cast<QMetaType::Type>(::qMetaTypeId<Whity<int> >()) << QString::fromLatin1("Whity<int>");
    QTest::newRow("Testspace::Foo") << static_cast<QMetaType::Type>(::qMetaTypeId<TestSpace::Foo>()) << QString::fromLatin1("TestSpace::Foo");

    QTest::newRow("-1") << QMetaType::Type(-1) << QString();
    QTest::newRow("-124125534") << QMetaType::Type(-124125534) << QString();
    QTest::newRow("124125534") << QMetaType::Type(124125534) << QString();

    // automatic registration
    QTest::newRow("QList<int>") << static_cast<QMetaType::Type>(::qMetaTypeId<QList<int> >()) << QString::fromLatin1("QList<int>");
    QTest::newRow("QHash<int,int>") << static_cast<QMetaType::Type>(::qMetaTypeId<QHash<int, int> >()) << QString::fromLatin1("QHash<int,int>");
    QTest::newRow("QMap<int,int>") << static_cast<QMetaType::Type>(::qMetaTypeId<QMap<int, int> >()) << QString::fromLatin1("QMap<int,int>");
    QTest::newRow("QVector<QList<int>>") << static_cast<QMetaType::Type>(::qMetaTypeId<QVector<QList<int> > >()) << QString::fromLatin1("QVector<QList<int> >");
    QTest::newRow("QVector<QMap<int,int>>") << static_cast<QMetaType::Type>(::qMetaTypeId<QVector<QMap<int, int> > >()) << QString::fromLatin1("QVector<QMap<int,int> >");
}

void tst_QMetaType::typeName()
{
    QFETCH(QMetaType::Type, aType);
    QFETCH(QString, aTypeName);

    QString name = QString::fromLatin1(QMetaType::typeName(aType));

    QCOMPARE(name, aTypeName);
    QCOMPARE(name.toLatin1(), QMetaObject::normalizedType(name.toLatin1().constData()));
}

#define FOR_EACH_PRIMITIVE_METATYPE(F) \
    QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(F) \
    QT_FOR_EACH_STATIC_CORE_POINTER(F) \

#define FOR_EACH_COMPLEX_CORE_METATYPE(F) \
    QT_FOR_EACH_STATIC_CORE_CLASS(F)

#define FOR_EACH_CORE_METATYPE(F) \
    FOR_EACH_PRIMITIVE_METATYPE(F) \
    FOR_EACH_COMPLEX_CORE_METATYPE(F) \

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
template<> struct TestValueFactory<QMetaType::SChar> {
    static signed char *create() { return new signed char(-12); }
};
template<> struct TestValueFactory<QMetaType::UChar> {
    static uchar *create() { return new uchar('u'); }
};
template<> struct TestValueFactory<QMetaType::Float> {
    static float *create() { return new float(3.14f); }
};
template<> struct TestValueFactory<QMetaType::QObjectStar> {
    static QObject * *create() { return new QObject *(0); }
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
template<> struct TestValueFactory<QMetaType::QUuid> {
    static QUuid *create() { return new QUuid(); }
};
template<> struct TestValueFactory<QMetaType::QModelIndex> {
    static QModelIndex *create() { return new QModelIndex(); }
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
template<> struct TestValueFactory<QMetaType::QRegularExpression> {
    static QRegularExpression *create()
    {
#ifndef QT_NO_REGEXP
        return new QRegularExpression("abc.*def");
#else
        return 0;
#endif
    }
};
template<> struct TestValueFactory<QMetaType::QJsonValue> {
    static QJsonValue *create() { return new QJsonValue(123.); }
};
template<> struct TestValueFactory<QMetaType::QJsonObject> {
    static QJsonObject *create() {
        QJsonObject *o = new QJsonObject();
        o->insert("a", 123.);
        o->insert("b", true);
        o->insert("c", QJsonValue::Null);
        o->insert("d", QLatin1String("ciao"));
        return o;
    }
};
template<> struct TestValueFactory<QMetaType::QJsonArray> {
    static QJsonArray *create() {
        QJsonArray *a = new QJsonArray();
        a->append(123.);
        a->append(true);
        a->append(QJsonValue::Null);
        a->append(QLatin1String("ciao"));
        return a;
    }
};
template<> struct TestValueFactory<QMetaType::QJsonDocument> {
    static QJsonDocument *create() {
        return new QJsonDocument(
            QJsonDocument::fromJson("{ 'foo': 123, 'bar': [true, null, 'ciao'] }")
        );
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

    QFETCH(QMetaType::Type, type);
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

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

void tst_QMetaType::sizeOf_data()
{
    QTest::addColumn<QMetaType::Type>("type");
    QTest::addColumn<size_t>("size");

    QTest::newRow("QMetaType::UnknownType") << QMetaType::UnknownType << size_t(0);
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << QMetaType::MetaTypeName << size_t(QTypeInfo<RealType>::sizeOf);
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW

    QTest::newRow("Whity<double>") << static_cast<QMetaType::Type>(::qMetaTypeId<Whity<double> >()) << sizeof(Whity<double>);
QTest::newRow("Whity<int>") << static_cast<QMetaType::Type>(::qMetaTypeId<Whity<int> >()) << sizeof(Whity<int>);
    QTest::newRow("Testspace::Foo") << static_cast<QMetaType::Type>(::qMetaTypeId<TestSpace::Foo>()) << sizeof(TestSpace::Foo);

    QTest::newRow("-1") << QMetaType::Type(-1) << size_t(0);
    QTest::newRow("-124125534") << QMetaType::Type(-124125534) << size_t(0);
    QTest::newRow("124125534") << QMetaType::Type(124125534) << size_t(0);
}

void tst_QMetaType::sizeOf()
{
    QFETCH(QMetaType::Type, type);
    QFETCH(size_t, size);
    QCOMPARE(size_t(QMetaType::sizeOf(type)), size);
}

void tst_QMetaType::sizeOfStaticLess_data()
{
    sizeOf_data();
}

void tst_QMetaType::sizeOfStaticLess()
{
    QFETCH(QMetaType::Type, type);
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
        << bool(Q_IS_ENUM(RealType));
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
    QTest::newRow("FlagsDataEnum") << ::qMetaTypeId<FlagsDataEnum>() << false << true << false << true;

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

    QCOMPARE(quint32(QMetaType::typeFlags(id)), flags);
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

    QFETCH(QMetaType::Type, type);
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

    QFETCH(QMetaType::Type, type);
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

void tst_QMetaType::automaticTemplateRegistration()
{
  {
    QList<int> intList;
    intList << 42;
    QVERIFY(QVariant::fromValue(intList).value<QList<int> >().first() == 42);
    QVector<QList<int> > vectorList;
    vectorList << intList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<int> > >().first().first() == 42);
  }

  {
    QList<QByteArray> bytearrayList;
    bytearrayList << QByteArray("foo");
    QVERIFY(QVariant::fromValue(bytearrayList).value<QList<QByteArray> >().first() == QByteArray("foo"));
    QVector<QList<QByteArray> > vectorList;
    vectorList << bytearrayList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<QByteArray> > >().first().first() == QByteArray("foo"));
  }

  QCOMPARE(::qMetaTypeId<QVariantList>(), (int)QMetaType::QVariantList);
  QCOMPARE(::qMetaTypeId<QList<QVariant> >(), (int)QMetaType::QVariantList);

  {
    QList<QVariant> variantList;
    variantList << 42;
    QVERIFY(QVariant::fromValue(variantList).value<QList<QVariant> >().first() == 42);
    QVector<QList<QVariant> > vectorList;
    vectorList << variantList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<QVariant> > >().first().first() == 42);
  }

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
    typedef QHash<int, UnregisteredType> IntUnregisteredTypeHash;
    QVERIFY(qRegisterMetaType<IntUnregisteredTypeHash>("IntUnregisteredTypeHash") > 0);
  }
  {
    typedef QList<UnregisteredType> UnregisteredTypeList;
    QVERIFY(qRegisterMetaType<UnregisteredTypeList>("UnregisteredTypeList") > 0);
  }

#if defined(Q_COMPILER_VARIADIC_MACROS) && !defined(TST_QMETATYPE_BROKEN_COMPILER)

    #define FOR_EACH_STATIC_PRIMITIVE_TYPE(F, ...) \
        F(bool, __VA_ARGS__) \
        F(int, __VA_ARGS__) \
        F(uint, __VA_ARGS__) \
        F(qlonglong, __VA_ARGS__) \
        F(qulonglong, __VA_ARGS__) \
        F(double, __VA_ARGS__) \
        F(long, __VA_ARGS__) \
        F(short, __VA_ARGS__) \
        F(char, __VA_ARGS__) \
        F(ulong, __VA_ARGS__) \
        F(ushort, __VA_ARGS__) \
        F(uchar, __VA_ARGS__) \
        F(float, __VA_ARGS__) \
        F(QObject*, __VA_ARGS__) \
        F(QString, __VA_ARGS__) \
        F(CustomMovable, __VA_ARGS__)

    #define FOR_EACH_STATIC_PRIMITIVE_TYPE2(F, ...) \
        F(bool, __VA_ARGS__) \
        F(int, __VA_ARGS__) \
        F(uint, __VA_ARGS__) \
        F(qlonglong, __VA_ARGS__) \
        F(qulonglong, __VA_ARGS__) \
        F(double, __VA_ARGS__) \
        F(long, __VA_ARGS__) \
        F(short, __VA_ARGS__) \
        F(char, __VA_ARGS__) \
        F(ulong, __VA_ARGS__) \
        F(ushort, __VA_ARGS__) \
        F(uchar, __VA_ARGS__) \
        F(float, __VA_ARGS__) \
        F(QObject*, __VA_ARGS__) \
        F(QString, __VA_ARGS__) \
        F(CustomMovable, __VA_ARGS__)


    #define CREATE_AND_VERIFY_CONTAINER(CONTAINER, ...) \
        { \
            CONTAINER< __VA_ARGS__ > t; \
            const QVariant v = QVariant::fromValue(t); \
            QByteArray tn = #CONTAINER + QByteArray("<"); \
            const QList<QByteArray> args = QByteArray(#__VA_ARGS__).split(','); \
            tn += args.first().trimmed(); \
            if (args.size() > 1) { \
              QList<QByteArray>::const_iterator it = args.constBegin() + 1; \
              const QList<QByteArray>::const_iterator end = args.constEnd(); \
              for (; it != end; ++it) { \
                tn += ","; \
                tn += it->trimmed(); \
              } \
            } \
            if (tn.endsWith('>')) \
                tn += ' '; \
            tn += ">"; \
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

    #define PRINT_1ARG_TEMPLATE(RealName, ...) \
        FOR_EACH_1ARG_TEMPLATE_TYPE(CREATE_AND_VERIFY_CONTAINER, RealName)

    #define FOR_EACH_2ARG_TEMPLATE_TYPE(F, RealName, ...) \
        F(QHash, __VA_ARGS__) \
        F(QMap, __VA_ARGS__) \
        F(QPair, __VA_ARGS__)

    #define PRINT_2ARG_TEMPLATE_INTERNAL(RealName, ...) \
        FOR_EACH_2ARG_TEMPLATE_TYPE(CREATE_AND_VERIFY_CONTAINER, 0, RealName, __VA_ARGS__)

    #define PRINT_2ARG_TEMPLATE(RealName, ...) \
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

#endif // Q_COMPILER_VARIADIC_MACROS

#define TEST_SMARTPOINTER(SMARTPOINTER, ELEMENT_TYPE, FLAG_TEST, FROMVARIANTFUNCTION) \
    { \
        SMARTPOINTER < ELEMENT_TYPE > sp(new ELEMENT_TYPE); \
        sp.data()->setObjectName("Test name"); \
        QVariant v = QVariant::fromValue(sp); \
        QCOMPARE(v.typeName(), #SMARTPOINTER "<" #ELEMENT_TYPE ">"); \
        QVERIFY(QMetaType::typeFlags(::qMetaTypeId<SMARTPOINTER < ELEMENT_TYPE > >()) & QMetaType::FLAG_TEST); \
        SMARTPOINTER < QObject > extractedPtr = FROMVARIANTFUNCTION<QObject>(v); \
        QCOMPARE(extractedPtr.data()->objectName(), sp.data()->objectName()); \
    }

    TEST_SMARTPOINTER(QSharedPointer, QObject, SharedPointerToQObject, qSharedPointerFromVariant)
    TEST_SMARTPOINTER(QSharedPointer, QFile, SharedPointerToQObject, qSharedPointerFromVariant)
    TEST_SMARTPOINTER(QSharedPointer, QTemporaryFile, SharedPointerToQObject, qSharedPointerFromVariant)
    TEST_SMARTPOINTER(QSharedPointer, MyObject, SharedPointerToQObject, qSharedPointerFromVariant)

    TEST_SMARTPOINTER(QWeakPointer, QObject, WeakPointerToQObject, qWeakPointerFromVariant)
    TEST_SMARTPOINTER(QWeakPointer, QFile, WeakPointerToQObject, qWeakPointerFromVariant)
    TEST_SMARTPOINTER(QWeakPointer, QTemporaryFile, WeakPointerToQObject, qWeakPointerFromVariant)
    TEST_SMARTPOINTER(QWeakPointer, MyObject, WeakPointerToQObject, qWeakPointerFromVariant)

    TEST_SMARTPOINTER(QPointer, QObject, TrackingPointerToQObject, qPointerFromVariant)
    TEST_SMARTPOINTER(QPointer, QFile, TrackingPointerToQObject, qPointerFromVariant)
    TEST_SMARTPOINTER(QPointer, QTemporaryFile, TrackingPointerToQObject, qPointerFromVariant)
    TEST_SMARTPOINTER(QPointer, MyObject, TrackingPointerToQObject, qPointerFromVariant)

#undef TEST_SMARTPOINTER
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
DECLARE_NONSTREAMABLE(QJsonValue)
DECLARE_NONSTREAMABLE(QJsonObject)
DECLARE_NONSTREAMABLE(QJsonArray)
DECLARE_NONSTREAMABLE(QJsonDocument)
DECLARE_NONSTREAMABLE(QObject*)
DECLARE_NONSTREAMABLE(QWidget*)

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

void tst_QMetaType::metaObject()
{
    QCOMPARE(QMetaType::metaObjectForType(QMetaType::QObjectStar), &QObject::staticMetaObject);
    QCOMPARE(QMetaType::metaObjectForType(::qMetaTypeId<QFile*>()), &QFile::staticMetaObject);
    QCOMPARE(QMetaType::metaObjectForType(::qMetaTypeId<MyObject*>()), &MyObject::staticMetaObject);
    QCOMPARE(QMetaType::metaObjectForType(QMetaType::Int), static_cast<const QMetaObject *>(0));

    QCOMPARE(QMetaType(QMetaType::QObjectStar).metaObject(), &QObject::staticMetaObject);
    QCOMPARE(QMetaType(::qMetaTypeId<QFile*>()).metaObject(), &QFile::staticMetaObject);
    QCOMPARE(QMetaType(::qMetaTypeId<MyObject*>()).metaObject(), &MyObject::staticMetaObject);
    QCOMPARE(QMetaType(QMetaType::Int).metaObject(), static_cast<const QMetaObject *>(0));
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

// Compile-time test, it should be possible to register function pointer types
class Undefined;

typedef Undefined (*UndefinedFunction0)();
typedef Undefined (*UndefinedFunction1)(Undefined);
typedef Undefined (*UndefinedFunction2)(Undefined, Undefined);
typedef Undefined (*UndefinedFunction3)(Undefined, Undefined, Undefined);

Q_DECLARE_METATYPE(UndefinedFunction0);
Q_DECLARE_METATYPE(UndefinedFunction1);
Q_DECLARE_METATYPE(UndefinedFunction2);
Q_DECLARE_METATYPE(UndefinedFunction3);

QTEST_MAIN(tst_QMetaType)
#include "tst_qmetatype.moc"
