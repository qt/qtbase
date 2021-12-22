/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "tst_qmetatype.h"

#include <QtCore/private/qmetaobjectbuilder_p.h>

#ifdef Q_OS_LINUX
# include <pthread.h>
#endif

#include <algorithm>
#include <memory>
#include <vector>

// mingw gcc 4.8 also takes way too long, letting the CI system abort the test
#if defined(__MINGW32__)
# define TST_QMETATYPE_BROKEN_COMPILER
#endif

Q_DECLARE_METATYPE(QMetaType::Type)

namespace CheckTypeTraits
{
struct NoOperators
{
    int x;
};
using Nested = QVector<std::pair<int, QMap<QStringList, QVariant>>>;
using Nested2 = QVector<std::pair<int, QVector<QPair<QStringList, QVariant>>>>;

// basic types
static_assert(QTypeTraits::has_operator_equal_v<bool>);
static_assert(QTypeTraits::has_operator_less_than_v<bool>);
static_assert(QTypeTraits::has_operator_equal_v<int>);
static_assert(QTypeTraits::has_operator_less_than_v<int>);
static_assert(QTypeTraits::has_operator_equal_v<double>);
static_assert(QTypeTraits::has_operator_less_than_v<double>);

// no comparison operators
static_assert(!QTypeTraits::has_operator_equal_v<NoOperators>);
static_assert(!QTypeTraits::has_operator_less_than_v<NoOperators>);

// standard Qt types
static_assert(QTypeTraits::has_operator_equal_v<QString>);
static_assert(QTypeTraits::has_operator_less_than_v<QString>);
static_assert(QTypeTraits::has_operator_equal_v<QVariant>);
static_assert(!QTypeTraits::has_operator_less_than_v<QVariant>);

// QList
static_assert(QTypeTraits::has_operator_equal_v<QStringList>);
static_assert(QTypeTraits::has_operator_less_than_v<QStringList>);
static_assert(!QTypeTraits::has_operator_equal_v<QList<NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<QList<NoOperators>>);
static_assert(QTypeTraits::has_operator_equal_v<QList<QVariant>>);
static_assert(!QTypeTraits::has_operator_less_than_v<QList<QVariant>>);

// QPair
static_assert(QTypeTraits::has_operator_equal_v<QPair<int, QString>>);
static_assert(QTypeTraits::has_operator_less_than_v<QPair<int, QString>>);
static_assert(!QTypeTraits::has_operator_equal_v<QPair<int, NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<QPair<int, NoOperators>>);

// QMap
static_assert(QTypeTraits::has_operator_equal_v<QMap<int, QString>>);
static_assert(!QTypeTraits::has_operator_less_than_v<QMap<int, QString>>);
static_assert(!QTypeTraits::has_operator_equal_v<QMap<int, NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<QMap<int, NoOperators>>);

// QHash
static_assert(QTypeTraits::has_operator_equal_v<QHash<int, QString>>);
static_assert(!QTypeTraits::has_operator_less_than_v<QHash<int, QString>>);
static_assert(!QTypeTraits::has_operator_equal_v<QHash<int, NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<QHash<int, NoOperators>>);

// QSharedPointer
static_assert(QTypeTraits::has_operator_equal_v<QSharedPointer<QString>>);
// smart pointer equality doesn't depend on T
static_assert(QTypeTraits::has_operator_equal_v<QSharedPointer<NoOperators>>);

// std::vector
static_assert(QTypeTraits::has_operator_equal_v<std::vector<QString>>);
static_assert(QTypeTraits::has_operator_less_than_v<std::vector<QString>>);
static_assert(!QTypeTraits::has_operator_equal_v<std::vector<NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::vector<NoOperators>>);
static_assert(QTypeTraits::has_operator_equal_v<std::vector<QVariant>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::vector<QVariant>>);

// std::pair
static_assert(QTypeTraits::has_operator_equal_v<std::pair<int, QString>>);
static_assert(QTypeTraits::has_operator_less_than_v<std::pair<int, QString>>);
static_assert(!QTypeTraits::has_operator_equal_v<std::pair<int, NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::pair<int, NoOperators>>);

// std::tuple
static_assert(QTypeTraits::has_operator_equal_v<std::tuple<int, QString, double>>);
static_assert(QTypeTraits::has_operator_less_than_v<std::tuple<int, QString, double>>);
static_assert(!QTypeTraits::has_operator_equal_v<std::tuple<int, QString, NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::tuple<int, QString, NoOperators>>);

// std::map
static_assert(QTypeTraits::has_operator_equal_v<std::map<int, QString>>);
static_assert(QTypeTraits::has_operator_less_than_v<std::map<int, QString>>);
static_assert(!QTypeTraits::has_operator_equal_v<std::map<int, NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::map<int, NoOperators>>);

// std::optional
static_assert(QTypeTraits::has_operator_equal_v<std::optional<QString>>);
static_assert(QTypeTraits::has_operator_less_than_v<std::optional<QString>>);
static_assert(!QTypeTraits::has_operator_equal_v<std::optional<NoOperators>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<NoOperators>>);

// nested types
static_assert(QTypeTraits::has_operator_equal_v<Nested>);
static_assert(!QTypeTraits::has_operator_less_than_v<Nested>);
static_assert(QTypeTraits::has_operator_equal_v<std::tuple<int, Nested>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::tuple<int, Nested>>);
static_assert(QTypeTraits::has_operator_equal_v<std::tuple<int, Nested>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::tuple<int, Nested>>);

static_assert(QTypeTraits::has_operator_equal_v<Nested2>);
static_assert(!QTypeTraits::has_operator_less_than_v<Nested2>);
static_assert(QTypeTraits::has_operator_equal_v<std::tuple<int, Nested2>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::tuple<int, Nested2>>);
static_assert(QTypeTraits::has_operator_equal_v<std::tuple<int, Nested2>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::tuple<int, Nested2>>);

}

struct BaseGenericType
{
    int m_typeId = -1;
    QMetaType m_metatype;
    virtual void *constructor(int typeId, void *where, const void *copy) = 0;
    virtual void staticMetacallFunction(QMetaObject::Call _c, int _id, void **_a) = 0;
    virtual void saveOperator(QDataStream & out) const = 0;
    virtual void loadOperator(QDataStream &in) = 0;
    virtual ~BaseGenericType() {}
};

struct GenericGadgetType : BaseGenericType
{
    void *constructor(int typeId, void *where, const void *copy) override
    {
        GenericGadgetType *ret = where ? new(where) GenericGadgetType : new GenericGadgetType;
        ret->m_typeId = typeId;
        if (copy) {
            Q_ASSERT(ret->m_typeId == reinterpret_cast<const GenericGadgetType*>(copy)->m_typeId);
            *ret = *reinterpret_cast<const GenericGadgetType*>(copy);
        } else {
            ret->properties = properties;
        }
        return ret;
    }

    void staticMetacallFunction(QMetaObject::Call _c, int _id, void **_a) override
    {
        if (_c == QMetaObject::ReadProperty) {
            if (_id < properties.size()) {
                const auto &prop = properties.at(_id);
                prop.metaType().destruct(_a[0]);
                prop.metaType().construct(_a[0], prop.constData());
            }
        } else if (_c == QMetaObject::WriteProperty) {
            if (_id < properties.size()) {
                auto & prop = properties[_id];
                prop = QVariant(prop.metaType(), _a[0]);
            }
        }
    }

    void saveOperator(QDataStream & out) const override
    {
        for (const auto &prop : properties)
            out << prop;
    }

    void loadOperator(QDataStream &in) override
    {
        for (auto &prop : properties)
            in >> prop;
    }
    QList<QVariant> properties;
};

struct GenericPODType : BaseGenericType
{
    // BaseGenericType interface
    void *constructor(int typeId, void *where, const void *copy) override
    {
        GenericPODType *ret = where ? new(where) GenericPODType : new GenericPODType;
        ret->m_typeId = typeId;
        if (copy) {
            Q_ASSERT(ret->m_typeId == reinterpret_cast<const GenericPODType*>(copy)->m_typeId);
            *ret = *reinterpret_cast<const GenericPODType*>(copy);
        } else {
            ret->podData = podData;
        }
        return ret;
    }

    void staticMetacallFunction(QMetaObject::Call _c, int _id, void **_a) override
    {
        Q_UNUSED(_c);
        Q_UNUSED(_id);
        Q_UNUSED(_a);
        Q_ASSERT(false);
    }

    void saveOperator(QDataStream &out) const override
    {
        out << podData;
    }
    void loadOperator(QDataStream &in) override
    {
        in >> podData;
    }
    QByteArray podData;
};

// The order of the next two statics matters!
//
// need to use shared_ptr, for its template ctor, since QMetaTypeInterface isn't polymorphic,
// but the test derives from it
static std::vector<std::shared_ptr<QtPrivate::QMetaTypeInterface>> s_metaTypeInterfaces;

using RegisteredType = QPair<std::shared_ptr<BaseGenericType>, std::shared_ptr<QMetaObject>>;
static QHash<int, RegisteredType> s_managedTypes;

static void GadgetsStaticMetacallFunction(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    reinterpret_cast<BaseGenericType*>(_o)->staticMetacallFunction(_c, _id, _a);
}

static void GadgetTypedDestructor(int typeId, void *ptr)
{
    QCOMPARE(typeId, reinterpret_cast<BaseGenericType*>(ptr)->m_typeId);
    reinterpret_cast<BaseGenericType*>(ptr)->~BaseGenericType();
}

static void *GadgetTypedConstructor(int type, void *where, const void *copy)
{
    auto it = s_managedTypes.find(type);
    if (it == s_managedTypes.end())
        return nullptr; // crash the test
    return it->first->constructor(type, where, copy);
}

static void GadgetSaveOperator(const QtPrivate::QMetaTypeInterface *, QDataStream & out, const void *data)
{
    reinterpret_cast<const BaseGenericType *>(data)->saveOperator(out);
}

static void GadgetLoadOperator(const QtPrivate::QMetaTypeInterface *, QDataStream &in, void *data)
{
    reinterpret_cast<BaseGenericType *>(data)->loadOperator(in);
}

struct Foo { int i; };


class CustomQObject : public QObject
{
    Q_OBJECT
public:
    CustomQObject(QObject *parent = nullptr)
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

// cannot use Q_GADGET due to moc limitations but wants to behave like
// a Q_GADGET in Qml land
template<typename T>
class GadgetDerivedAndTyped : public CustomGadget {};

Q_DECLARE_METATYPE(GadgetDerivedAndTyped<int>)
Q_DECLARE_METATYPE(GadgetDerivedAndTyped<int>*)

void tst_QMetaType::registerGadget(const char *name, const QList<GadgetPropertyType> &gadgetProperties)
{
    QMetaObjectBuilder gadgetBuilder;
    gadgetBuilder.setClassName(name);
    MetaObjectFlags metaObjectflags = DynamicMetaObject | PropertyAccessInStaticMetaCall;
    gadgetBuilder.setFlags(metaObjectflags);
    auto dynamicGadgetProperties = std::make_shared<GenericGadgetType>();
    for (const auto &prop : gadgetProperties) {
        int propertyType = QMetaType::fromName(prop.type).id();
        dynamicGadgetProperties->properties.push_back(QVariant(QMetaType(propertyType)));
        auto dynamicPropery = gadgetBuilder.addProperty(prop.name, prop.type);
        dynamicPropery.setWritable(true);
        dynamicPropery.setReadable(true);
    }
    auto meta = gadgetBuilder.toMetaObject();
    meta->d.static_metacall = &GadgetsStaticMetacallFunction;
    meta->d.superdata = nullptr;
    const auto flags = QMetaType::IsGadget | QMetaType::NeedsConstruction | QMetaType::NeedsDestruction;
    struct TypeInfo : public QtPrivate::QMetaTypeInterface
    {
        QMetaObject *mo;
    };

    auto typeInfo = s_metaTypeInterfaces.emplace_back(new TypeInfo {
        {
            0, alignof(GenericGadgetType), sizeof(GenericGadgetType), uint(flags), 0,
            [](const QtPrivate::QMetaTypeInterface *self) -> const QMetaObject * {
                return reinterpret_cast<const TypeInfo *>(self)->mo;
            },
            name,
            [](const QtPrivate::QMetaTypeInterface *self, void *where) { GadgetTypedConstructor(self->typeId, where, nullptr); },
            [](const QtPrivate::QMetaTypeInterface *self, void *where, const void *copy) { GadgetTypedConstructor(self->typeId, where, copy); },
            [](const QtPrivate::QMetaTypeInterface *self, void *where, void *copy) { GadgetTypedConstructor(self->typeId, where, copy); },
            [](const QtPrivate::QMetaTypeInterface *self, void *ptr) { GadgetTypedDestructor(self->typeId, ptr); },
            nullptr,
            nullptr,
            nullptr,
            GadgetSaveOperator,
            GadgetLoadOperator,
            nullptr
        },
        meta
    }).get();
    QMetaType gadgetMetaType(typeInfo);
    dynamicGadgetProperties->m_metatype = gadgetMetaType;
    int gadgetTypeId = QMetaType(typeInfo).id();
    QVERIFY(gadgetTypeId > 0);
    s_managedTypes[gadgetTypeId] = qMakePair(dynamicGadgetProperties, std::shared_ptr<QMetaObject>{meta, [](QMetaObject *ptr){ ::free(ptr); }});
}

void tst_QMetaType::defined()
{
    QCOMPARE(int(QMetaTypeId2<QString>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<Foo>::Defined), 0);
    QCOMPARE(int(QMetaTypeId2<void*>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<int*>::Defined), 0);
    QCOMPARE(int(QMetaTypeId2<CustomQObject::CustomQEnum>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<CustomGadget>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<CustomGadget*>::Defined), 1);
    QVERIFY(!QMetaTypeId2<GadgetDerived>::Defined);
    QVERIFY(!QMetaTypeId2<GadgetDerived*>::Defined);
    QVERIFY(int(QMetaTypeId2<CustomQObject*>::Defined));
    QVERIFY(!QMetaTypeId2<CustomQObject>::Defined);
    QVERIFY(!QMetaTypeId2<CustomNonQObject>::Defined);
    QVERIFY(!QMetaTypeId2<CustomNonQObject*>::Defined);
    QVERIFY(!QMetaTypeId2<CustomGadget_NonDefaultConstructible>::Defined);

    // registered with Q_DECLARE_METATYPE
    QVERIFY(QMetaTypeId2<GadgetDerivedAndTyped<int>>::Defined);
    QVERIFY(QMetaTypeId2<GadgetDerivedAndTyped<int>*>::Defined);
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
    ~Bar() {}

public:
    static int failureCount;
};

int Bar::failureCount = 0;

class MetaTypeTorturer: public QThread
{
    Q_OBJECT
protected:
    void run() override
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
    struct Foo { double d; public: ~Foo() {} };
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

void tst_QMetaType::id()
{
    QCOMPARE(QMetaType(QMetaType::QString).id(), QMetaType::QString);
    QCOMPARE(QMetaType(::qMetaTypeId<TestSpace::Foo>()).id(), ::qMetaTypeId<TestSpace::Foo>());
    QCOMPARE(QMetaType::fromType<TestSpace::Foo>().id(), ::qMetaTypeId<TestSpace::Foo>());
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
    QTest::newRow("QMetaType::User-1") << (int(QMetaType::User) - 1) << static_cast<const char *>(nullptr);
    QTest::newRow("QMetaType::FirstWidgetsType-1") << (int(QMetaType::FirstWidgetsType) - 1) << static_cast<const char *>(nullptr);

    QTest::newRow("Whity<double>") << ::qMetaTypeId<Whity<double> >() << QString::fromLatin1("Whity<double>");
    QTest::newRow("Whity<int>") << ::qMetaTypeId<Whity<int> >() << QString::fromLatin1("Whity<int>");
    QTest::newRow("Testspace::Foo") << ::qMetaTypeId<TestSpace::Foo>() << QString::fromLatin1("TestSpace::Foo");

    QTest::newRow("-1") << -1 << QString();
    QTest::newRow("-124125534") << -124125534 << QString();
    QTest::newRow("124125534") << 124125534 << QString();

    // automatic registration
    QTest::newRow("QHash<int,int>") << ::qMetaTypeId<QHash<int, int>>() << QString::fromLatin1("QHash<int,int>");
    QTest::newRow("QMap<int,int>") << ::qMetaTypeId<QMap<int, int>>() << QString::fromLatin1("QMap<int,int>");
    QTest::newRow("QList<QMap<int,int>>") << ::qMetaTypeId<QList<QMap<int, int>>>() << QString::fromLatin1("QList<QMap<int,int>>");

    // automatic registration with automatic QList to QList aliasing
    QTest::newRow("QList<int>") << ::qMetaTypeId<QList<int>>() << QString::fromLatin1("QList<int>");
    QTest::newRow("QList<QList<int>>") << ::qMetaTypeId<QList<QList<int>>>() << QString::fromLatin1("QList<QList<int>>");

    QTest::newRow("CustomQObject*") << ::qMetaTypeId<CustomQObject*>() << QString::fromLatin1("CustomQObject*");
    QTest::newRow("CustomGadget") << ::qMetaTypeId<CustomGadget>() << QString::fromLatin1("CustomGadget");
    QTest::newRow("CustomGadget*") << ::qMetaTypeId<CustomGadget*>() << QString::fromLatin1("CustomGadget*");
    QTest::newRow("CustomQObject::CustomQEnum") << ::qMetaTypeId<CustomQObject::CustomQEnum>() << QString::fromLatin1("CustomQObject::CustomQEnum");
    QTest::newRow("Qt::ArrowType") << ::qMetaTypeId<Qt::ArrowType>() << QString::fromLatin1("Qt::ArrowType");

    // template instance class derived from Q_GADGET enabled class
    QTest::newRow("GadgetDerivedAndTyped<int>") << ::qMetaTypeId<GadgetDerivedAndTyped<int>>() << QString::fromLatin1("GadgetDerivedAndTyped<int>");
    QTest::newRow("GadgetDerivedAndTyped<int>*") << ::qMetaTypeId<GadgetDerivedAndTyped<int>*>() << QString::fromLatin1("GadgetDerivedAndTyped<int>*");
}

void tst_QMetaType::typeName()
{
    QFETCH(int, aType);
    QFETCH(QString, aTypeName);

    if (aType >= QMetaType::FirstWidgetsType)
        QSKIP("The test doesn't link against QtWidgets.");

    const char *rawname = QMetaType::typeName(aType);
    QString name = QString::fromLatin1(rawname);

    QCOMPARE(name, aTypeName);
    QCOMPARE(name.toLatin1(), QMetaObject::normalizedType(name.toLatin1().constData()));
    QCOMPARE(rawname == nullptr, aTypeName.isNull());

    QMetaType mt(aType);
    if (mt.isValid()) { // Gui type are not valid
        QCOMPARE(QString::fromLatin1(QMetaType(aType).name()), aTypeName);
    }

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

    if (aType >= QMetaType::FirstWidgetsType)
        QSKIP("The test doesn't link against QtWidgets.");
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
        static_assert(( QMetaTypeId2<T>::IsBuiltIn ));
        enum { value = true };
    };
}

#define CHECK_BUILTIN(MetaTypeName, MetaTypeId, RealType) static_assert_trigger< RealType >::value &&
static_assert(( FOR_EACH_CORE_METATYPE(CHECK_BUILTIN) true ));
#undef CHECK_BUILTIN
static_assert(( QMetaTypeId2<QList<QVariant> >::IsBuiltIn));
static_assert(( QMetaTypeId2<QMap<QString,QVariant> >::IsBuiltIn));
static_assert(( QMetaTypeId2<QObject*>::IsBuiltIn));
static_assert((!QMetaTypeId2<tst_QMetaType*>::IsBuiltIn)); // QObject subclass
static_assert((!QMetaTypeId2<QList<int> >::IsBuiltIn));
static_assert((!QMetaTypeId2<QMap<int,int> >::IsBuiltIn));
static_assert((!QMetaTypeId2<QMetaType::Type>::IsBuiltIn));

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

template<typename T>
constexpr size_t getSize = sizeof(T);
template<>
constexpr size_t getSize<void> = 0;

void tst_QMetaType::sizeOf_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<size_t>("size");

    QTest::newRow("QMetaType::UnknownType") << int(QMetaType::UnknownType) << size_t(0);
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << int(QMetaType::MetaTypeName) << getSize<RealType>;
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

template <typename T>
auto getAlignOf()
{
    if constexpr (std::is_same_v<T, void>)
        return 0;
    else
        return alignof(T);
}

void tst_QMetaType::alignOf_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<size_t>("size");

    QTest::newRow("QMetaType::UnknownType") << int(QMetaType::UnknownType) << size_t(0);
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << int(QMetaType::MetaTypeName) << size_t(getAlignOf<RealType>());
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW

    QTest::newRow("Whity<double>") << ::qMetaTypeId<Whity<double> >() << alignof(Whity<double>);
    QTest::newRow("Whity<int>") << ::qMetaTypeId<Whity<int> >() << alignof(Whity<int>);
    QTest::newRow("Testspace::Foo") << ::qMetaTypeId<TestSpace::Foo>() << alignof(TestSpace::Foo);

    QTest::newRow("-1") << -1 << size_t(0);
    QTest::newRow("-124125534") << -124125534 << size_t(0);
    QTest::newRow("124125534") << 124125534 << size_t(0);
}

void tst_QMetaType::alignOf()
{
    QFETCH(int, type);
    QFETCH(size_t, size);
    QCOMPARE(size_t(QMetaType(type).alignOf()), size);
}

class CustomObject : public QObject
{
    Q_OBJECT
public:
    CustomObject(QObject *parent = nullptr)
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
    CustomMultiInheritanceObject(QObject *parent = nullptr)
      : QObject(parent)
    {

    }
};
Q_DECLARE_METATYPE(CustomMultiInheritanceObject*);

class C { Q_DECL_UNUSED_MEMBER char _[4]; public: C() = default; C(const C&) {} };
class M { Q_DECL_UNUSED_MEMBER char _[4]; public: M() {} };
class P { Q_DECL_UNUSED_MEMBER char _[4]; };

QT_BEGIN_NAMESPACE
#if defined(Q_CC_GNU) && Q_CC_GNU < 501
Q_DECLARE_TYPEINFO(M, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(P, Q_PRIMITIVE_TYPE);
#endif
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
    QTest::addColumn<bool>("isQmlList");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId \
        << bool(QTypeInfo<RealType>::isRelocatable) \
        << bool(QTypeInfo<RealType>::isComplex) \
        << bool(QtPrivate::IsPointerToTypeDerivedFromQObject<RealType>::Value) \
        << bool(std::is_enum<RealType>::value) \
        << false;
QT_FOR_EACH_STATIC_CORE_CLASS(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_CORE_POINTER(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
    QTest::newRow("TestSpace::Foo") << ::qMetaTypeId<TestSpace::Foo>() << false << true << false << false << false;
    QTest::newRow("Whity<double>") << ::qMetaTypeId<Whity<double> >() << true << true << false << false << false;
    QTest::newRow("CustomMovable") << ::qMetaTypeId<CustomMovable>() << true << true << false << false << false;
    QTest::newRow("CustomObject*") << ::qMetaTypeId<CustomObject*>() << true << false << true << false << false;
    QTest::newRow("CustomMultiInheritanceObject*") << ::qMetaTypeId<CustomMultiInheritanceObject*>() << true << false << true << false << false;
    QTest::newRow("QPair<C,C>") << ::qMetaTypeId<QPair<C,C> >() << false << true  << false << false << false;
    QTest::newRow("QPair<C,M>") << ::qMetaTypeId<QPair<C,M> >() << false << true  << false << false << false;
    QTest::newRow("QPair<C,P>") << ::qMetaTypeId<QPair<C,P> >() << false << true  << false << false << false;
    QTest::newRow("QPair<M,C>") << ::qMetaTypeId<QPair<M,C> >() << false << true  << false << false << false;
    QTest::newRow("QPair<M,M>") << ::qMetaTypeId<QPair<M,M> >() << true  << true  << false << false << false;
    QTest::newRow("QPair<M,P>") << ::qMetaTypeId<QPair<M,P> >() << true  << true  << false << false << false;
    QTest::newRow("QPair<P,C>") << ::qMetaTypeId<QPair<P,C> >() << false << true  << false << false << false;
    QTest::newRow("QPair<P,M>") << ::qMetaTypeId<QPair<P,M> >() << true  << true  << false << false << false;
    QTest::newRow("QPair<P,P>") << ::qMetaTypeId<QPair<P,P> >() << true  << false << false << false << false;
    QTest::newRow("FlagsDataEnum") << ::qMetaTypeId<FlagsDataEnum>() << true << false << false << true << false;

    // invalid ids.
    QTest::newRow("-1") << -1 << false << false << false << false << false;
    QTest::newRow("-124125534") << -124125534 << false << false << false << false << false;
    QTest::newRow("124125534") << 124125534 << false << false << false << false << false;
}

void tst_QMetaType::flags()
{
    QFETCH(int, type);
    QFETCH(bool, isMovable);
    QFETCH(bool, isComplex);
    QFETCH(bool, isPointerToQObject);
    QFETCH(bool, isEnum);
    QFETCH(bool, isQmlList);

    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsConstruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsDestruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::RelocatableType), isMovable);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::PointerToQObject), isPointerToQObject);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::IsEnumeration), isEnum);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::IsQmlList), isQmlList);
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
    QCOMPARE(bool(flags & QMetaType::RelocatableType), isMovable);
}

void tst_QMetaType::flagsBinaryCompatibility6_0_data()
{
//     Changing traits of a built-in type is illegal from BC point of view.
//     Traits are saved in code of an application and in the Qt library which means
//     that there may be a mismatch.
//     The test is loading data generated by this code:
//
//            QList<quint32> buffer;
//            buffer.reserve(2 * QMetaType::User);
//            for (quint32 i = 0; i < QMetaType::LastCoreType; ++i) {
//                if (QMetaType::isRegistered(i)) {
//                    buffer.append(i);
//                    buffer.append(quint32(QMetaType::typeFlags(i)));
//                }
//            }
//            QFile file("/tmp/typeFlags.bin");
//            file.open(QIODevice::WriteOnly);
//            QDataStream ds(&file);
//            ds << buffer;
//            file.close();

    QTest::addColumn<quint32>("id");
    QTest::addColumn<quint32>("flags");

    QFile file(QFINDTESTDATA("typeFlags.bin"));
    file.open(QIODevice::ReadOnly);
    QList<quint32> buffer;
    QDataStream ds(&file);
    ds >> buffer;

    for (int i = 0; i < buffer.size(); i+=2) {
        const quint32 id = buffer.at(i);
        const quint32 flags = buffer.at(i + 1);
        if (id > QMetaType::LastCoreType)
            continue; // We do not link against QtGui, so we do longer consider such type as registered
        QVERIFY2(QMetaType::isRegistered(id), "A type could not be removed in BC way");
        QTest::newRow(QMetaType::typeName(id)) << id << flags;
    }
}

void tst_QMetaType::flagsBinaryCompatibility6_0()
{
    QFETCH(quint32, id);
    QFETCH(quint32, flags);

    const auto currentFlags = QMetaType::typeFlags(id);
    auto expectedFlags = QMetaType::TypeFlags(flags);
    if (!(currentFlags.testFlag(QMetaType::NeedsConstruction) && currentFlags.testFlag(QMetaType::NeedsDestruction))) {
        if (expectedFlags.testFlag(QMetaType::NeedsConstruction) && expectedFlags.testFlag(QMetaType::NeedsDestruction)) {
            // If type changed from RELOCATABLE to trivial, that's fine
            expectedFlags.setFlag(QMetaType::NeedsConstruction, false);
            expectedFlags.setFlag(QMetaType::NeedsDestruction, false);
        }
    }
    quint32 mask_5_0 = 0x1fb; // Only compare the values that were already defined in 5.0

    QCOMPARE(quint32(currentFlags) & mask_5_0, quint32(expectedFlags) & mask_5_0);
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
    void *storage1 = qMallocAligned(size, alignof(Type));
    void *actual1 = QMetaType::construct(ID, storage1, /*copy=*/0);
    void *storage2 = qMallocAligned(size, alignof(Type));
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

void tst_QMetaType::typedConstruct()
{
    auto testMetaObjectWriteOnGadget = [](QVariant &gadget, const QList<GadgetPropertyType> &properties)
    {
        auto metaObject = QMetaType::metaObjectForType(gadget.userType());
        QVERIFY(metaObject != nullptr);
        QCOMPARE(metaObject->methodCount(), 0);
        QCOMPARE(metaObject->propertyCount(), properties.size());
        for (int i = 0; i < metaObject->propertyCount(); ++i) {
            auto prop = metaObject->property(i);
            QCOMPARE(properties[i].name, prop.name());
            QCOMPARE(properties[i].type, prop.typeName());
            prop.writeOnGadget(gadget.data(), properties[i].testData);
        }
    };

    auto testMetaObjectReadOnGadget = [](QVariant gadget, const QList<GadgetPropertyType> &properties)
    {
        auto metaObject = QMetaType::metaObjectForType(gadget.userType());
        QVERIFY(metaObject != nullptr);
        QCOMPARE(metaObject->methodCount(), 0);
        QCOMPARE(metaObject->propertyCount(), properties.size());
        for (int i = 0; i < metaObject->propertyCount(); ++i) {
            auto prop = metaObject->property(i);
            QCOMPARE(properties[i].name, prop.name());
            QCOMPARE(properties[i].type, prop.typeName());
            if (!QMetaType::typeFlags(prop.userType()).testFlag(QMetaType::IsGadget))
                QCOMPARE(properties[i].testData, prop.readOnGadget(gadget.constData()));
        }
    };

    QList<GadgetPropertyType> dynamicGadget1 = {
        {"int", "int_prop", 34526},
        {"float", "float_prop", 1.23f},
        {"QString", "string_prop", QString{"Test QString"}}
    };
    registerGadget("DynamicGadget1", dynamicGadget1);

    QVariant testGadget1(QMetaType(QMetaType::type("DynamicGadget1")));
    testMetaObjectWriteOnGadget(testGadget1, dynamicGadget1);
    testMetaObjectReadOnGadget(testGadget1, dynamicGadget1);


    QList<GadgetPropertyType> dynamicGadget2 = {
        {"int", "int_prop", 512},
        {"double", "double_prop", 4.56},
        {"QString", "string_prop", QString{"Another String"}},
        {"DynamicGadget1", "dynamicGadget1_prop", testGadget1}
    };
    registerGadget("DynamicGadget2", dynamicGadget2);
    QVariant testGadget2(QMetaType(QMetaType::type("DynamicGadget2")));
    testMetaObjectWriteOnGadget(testGadget2, dynamicGadget2);
    testMetaObjectReadOnGadget(testGadget2, dynamicGadget2);
    auto g2mo = QMetaType::metaObjectForType(testGadget2.userType());
    auto dynamicGadget1_prop = g2mo->property(g2mo->indexOfProperty("dynamicGadget1_prop"));
    testMetaObjectReadOnGadget(dynamicGadget1_prop.readOnGadget(testGadget2.constData()), dynamicGadget1);


    // Register POD
    const QByteArray myPodTesData = "My POD test data";
    const char podTypeName[] = "DynamicPOD";
    auto dynamicGadgetProperties = std::make_shared<GenericPODType>();
    dynamicGadgetProperties->podData = myPodTesData;
    const auto flags = QMetaType::NeedsConstruction | QMetaType::NeedsDestruction;
    using TypeInfo = QtPrivate::QMetaTypeInterface;
    auto typeInfo = s_metaTypeInterfaces.emplace_back(new TypeInfo {
        0, alignof(GenericGadgetType), sizeof(GenericGadgetType), uint(flags), 0, nullptr, podTypeName,
        [](const TypeInfo *self, void *where) { GadgetTypedConstructor(self->typeId, where, nullptr); },
        [](const TypeInfo *self, void *where, const void *copy) { GadgetTypedConstructor(self->typeId, where, copy); },
        [](const TypeInfo *self, void *where, void *copy) { GadgetTypedConstructor(self->typeId, where, copy); },
        [](const TypeInfo *self, void *ptr) { GadgetTypedDestructor(self->typeId, ptr); },
        nullptr,
        nullptr,
        nullptr,
        GadgetSaveOperator,
        GadgetLoadOperator,
        nullptr
    }).get();
    QMetaType metatype(typeInfo);
    dynamicGadgetProperties->m_metatype = metatype;
    int podTypeId = metatype.id();
    QVERIFY(podTypeId > 0);
    s_managedTypes[podTypeId] = qMakePair(dynamicGadgetProperties, std::shared_ptr<QMetaObject>{});

    // Test POD
    QCOMPARE(podTypeId, QMetaType::type(podTypeName));
    QVariant podVariant{QMetaType(podTypeId)};
    QCOMPARE(myPodTesData, static_cast<const GenericPODType *>(reinterpret_cast<const BaseGenericType *>(podVariant.constData()))->podData);

    QVariant podVariant1{podVariant};
    podVariant1.detach(); // Test stream operators
    static_cast<GenericPODType *>(reinterpret_cast<BaseGenericType *>(podVariant.data()))->podData.clear();
    QCOMPARE(myPodTesData, static_cast<const GenericPODType *>(reinterpret_cast<const BaseGenericType *>(podVariant1.constData()))->podData);
}

template<int ID>
static void testConstructCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    QMetaType info(ID);
    int size = QMetaType::sizeOf(ID);
    QCOMPARE(info.sizeOf(), size);
    void *storage1 = qMallocAligned(size, alignof(Type));
    void *actual1 = QMetaType::construct(ID, storage1, expected);
    void *storage2 = qMallocAligned(size, alignof(Type));
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
    AutoMetaTypeObject(QObject *parent = nullptr)
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
  MyObject(QObject *parent = nullptr)
    : QObject(parent)
  {
  }
};
typedef MyObject* MyObjectPtr;
Q_DECLARE_METATYPE(MyObjectPtr)

#if !defined(TST_QMETATYPE_BROKEN_COMPILER)
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
    QList<CONTAINER<VALUE_TYPE> > outerContainer; \
    outerContainer << innerContainer; \
    QVERIFY(*QVariant::fromValue(outerContainer).value<QList<CONTAINER<VALUE_TYPE> > >().first().begin() == 42); \
  }

  TEST_SEQUENTIAL_CONTAINER(QList, int)
  TEST_SEQUENTIAL_CONTAINER(std::vector, int)
  TEST_SEQUENTIAL_CONTAINER(std::list, int)

  {
    std::vector<bool> vecbool;
    vecbool.push_back(true);
    vecbool.push_back(false);
    vecbool.push_back(true);
    QVERIFY(QVariant::fromValue(vecbool).value<std::vector<bool>>().front() == true);
    QList<std::vector<bool>> vectorList;
    vectorList << vecbool;
    QVERIFY(QVariant::fromValue(vectorList).value<QList<std::vector<bool>>>().first().front() == true);
  }

  {
    QList<unsigned> unsignedList;
    unsignedList << 123;
    QVERIFY(QVariant::fromValue(unsignedList).value<QList<unsigned>>().first() == 123);
    QList<QList<unsigned>> vectorList;
    vectorList << unsignedList;
    QVERIFY(QVariant::fromValue(vectorList).value<QList<QList<unsigned>>>().first().first() == 123);
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
    QVERIFY(QVariant::fromValue(sharedPointerList).value<QList<QSharedPointer<QObject>>>().first() == testObject);
    QList<QList<QSharedPointer<QObject>>> vectorList;
    vectorList << sharedPointerList;
    QVERIFY(QVariant::fromValue(vectorList).value<QList<QList<QSharedPointer<QObject>>>>().first().first() == testObject);
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
    CustomObject *o = nullptr;
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
    IntUIntPair intUIntPair = qMakePair(4, 2u);
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
    CustomQObject *o = nullptr;
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

#if !defined(TST_QMETATYPE_BROKEN_COMPILER)

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
            const int expectedType = ::qMetaTypeId<CONTAINER< __VA_ARGS__ > >(); \
            const int type = QMetaType::type(tn); \
            QCOMPARE(type, expectedType); \
            QCOMPARE((QMetaType::fromType<CONTAINER< __VA_ARGS__ >>().id()), expectedType); \
        }

    #define FOR_EACH_1ARG_TEMPLATE_TYPE(F, TYPE) \
        F(QList, TYPE) \
        F(QQueue, TYPE) \
        F(QStack, TYPE) \
        F(QSet, TYPE)

    #define PRINT_1ARG_TEMPLATE(RealName) \
        FOR_EACH_1ARG_TEMPLATE_TYPE(CREATE_AND_VERIFY_CONTAINER, RealName)

    #define FOR_EACH_2ARG_TEMPLATE_TYPE(F, RealName1, RealName2) \
        F(QHash, RealName1, RealName2) \
        F(QMap, RealName1, RealName2) \
        F(std::pair, RealName1, RealName2)

    #define PRINT_2ARG_TEMPLATE_INTERNAL(RealName1, RealName2) \
        FOR_EACH_2ARG_TEMPLATE_TYPE(CREATE_AND_VERIFY_CONTAINER, RealName1, RealName2)

    #define PRINT_2ARG_TEMPLATE(RealName) \
        FOR_EACH_STATIC_PRIMITIVE_TYPE2(PRINT_2ARG_TEMPLATE_INTERNAL, RealName)

    #define REGISTER_TYPEDEF(TYPE, ARG1, ARG2) \
      qRegisterMetaType<TYPE <ARG1, ARG2>>(#TYPE "<" #ARG1 "," #ARG2 ">");

    REGISTER_TYPEDEF(QHash, int, uint)
    REGISTER_TYPEDEF(QMap, int, uint)
    REGISTER_TYPEDEF(QPair, int, uint)

    FOR_EACH_STATIC_PRIMITIVE_TYPE(
      PRINT_1ARG_TEMPLATE
    )
    FOR_EACH_STATIC_PRIMITIVE_TYPE(
      PRINT_2ARG_TEMPLATE
    )

    CREATE_AND_VERIFY_CONTAINER(QList, QList<QMap<int, QHash<char, QList<QVariant>>>>)
    CREATE_AND_VERIFY_CONTAINER(QList, void*)
    CREATE_AND_VERIFY_CONTAINER(QList, const void*)
    CREATE_AND_VERIFY_CONTAINER(QList, void*)
    CREATE_AND_VERIFY_CONTAINER(std::pair, void*, void*)
    CREATE_AND_VERIFY_CONTAINER(QHash, void*, void*)
    CREATE_AND_VERIFY_CONTAINER(QHash, const void*, const void*)

#endif // !defined(TST_QMETATYPE_BROKEN_COMPILER)

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

    TEST_NONOWNING_SMARTPOINTER(QPointer, QObject, TrackingPointerToQObject, qPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QPointer, QFile, TrackingPointerToQObject, qPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QPointer, QTemporaryFile, TrackingPointerToQObject, qPointerFromVariant)
    TEST_NONOWNING_SMARTPOINTER(QPointer, MyObject, TrackingPointerToQObject, qPointerFromVariant)
#undef TEST_NONOWNING_SMARTPOINTER


#define TEST_WEAK_SMARTPOINTER(ELEMENT_TYPE, FLAG_TEST) \
    { \
        ELEMENT_TYPE elem; \
        QSharedPointer < ELEMENT_TYPE > shared(new ELEMENT_TYPE); \
        QWeakPointer < ELEMENT_TYPE > sp(shared); \
        sp.toStrongRef()->setObjectName("Test name"); \
        QVariant v = QVariant::fromValue(sp); \
        QCOMPARE(v.typeName(), "QWeakPointer<" #ELEMENT_TYPE ">"); \
        QVERIFY(QMetaType::typeFlags(::qMetaTypeId<QWeakPointer < ELEMENT_TYPE > >()) & QMetaType::FLAG_TEST); \
    }

    TEST_WEAK_SMARTPOINTER(QObject, WeakPointerToQObject)
    TEST_WEAK_SMARTPOINTER(QFile, WeakPointerToQObject)
    TEST_WEAK_SMARTPOINTER(QTemporaryFile, WeakPointerToQObject)
    TEST_WEAK_SMARTPOINTER(MyObject, WeakPointerToQObject)
#undef TEST_WEAK_SMARTPOINTER
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
DECLARE_NONSTREAMABLE(QObject*)
DECLARE_NONSTREAMABLE(QWidget*)

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
        QVERIFY(QMetaType(type).hasRegisteredDataStreamOperators());
        QVERIFY(QMetaType::load(stream, type, value)); // Hmmm, shouldn't it return false?

        // std::nullptr_t is nullary: it doesn't actually read anything
        if (type != QMetaType::Nullptr)
            QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    } else {
        QVERIFY(!QMetaType(type).hasRegisteredDataStreamOperators());
    }

    stream.device()->seek(0);
    stream.resetStatus();
    QCOMPARE(QMetaType::load(stream, type, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable) {
        QVERIFY(QMetaType::load(stream, type, value)); // Hmmm, shouldn't it return false?

        // std::nullptr_t is nullary: it doesn't actually read anything
        if (type != QMetaType::Nullptr)
            QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    }

    QMetaType::destroy(type, value);
}

struct CustomStreamableType
{
    int a;
};

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
Q_DECLARE_METATYPE(CustomStreamableType)

void tst_QMetaType::saveAndLoadCustom()
{
    CustomStreamableType t;
    t.a = 123;

    int id = ::qMetaTypeId<CustomStreamableType>();
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);

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

class MyQObjectFromGadget : public QObject, public MyGadget
{
    Q_OBJECT
public:
    MyQObjectFromGadget(QObject *parent = nullptr)
        : QObject(parent)
    {}
};

Q_DECLARE_METATYPE(MyGadget);
Q_DECLARE_METATYPE(MyGadget*);
Q_DECLARE_METATYPE(const QMetaObject *);
Q_DECLARE_METATYPE(Qt::ScrollBarPolicy);
Q_DECLARE_METATYPE(MyGadget::MyEnum);
Q_DECLARE_METATYPE(MyQObjectFromGadget*);

void tst_QMetaType::metaObject_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<const QMetaObject*>("result");
    QTest::addColumn<bool>("isGadget");
    QTest::addColumn<bool>("isGadgetPtr");
    QTest::addColumn<bool>("isQObjectPtr");

    QTest::newRow("QObject") << int(QMetaType::QObjectStar) << &QObject::staticMetaObject << false << false << true;
    QTest::newRow("QFile*") << ::qMetaTypeId<QFile*>() << &QFile::staticMetaObject << false << false << true;
    QTest::newRow("MyObject*") << ::qMetaTypeId<MyObject*>() << &MyObject::staticMetaObject << false << false << true;
    QTest::newRow("int") << int(QMetaType::Int) << static_cast<const QMetaObject *>(0) << false << false << false;
    QTest::newRow("QEasingCurve") << ::qMetaTypeId<QEasingCurve>() <<  &QEasingCurve::staticMetaObject << true << false << false;
    QTest::newRow("MyGadget") << ::qMetaTypeId<MyGadget>() <<  &MyGadget::staticMetaObject << true << false << false;
    QTest::newRow("MyGadget*") << ::qMetaTypeId<MyGadget*>() << &MyGadget::staticMetaObject << false << true << false;
    QTest::newRow("MyEnum") << ::qMetaTypeId<MyGadget::MyEnum>() <<  &MyGadget::staticMetaObject << false << false << false;
    QTest::newRow("Qt::ScrollBarPolicy") << ::qMetaTypeId<Qt::ScrollBarPolicy>() <<  &Qt::staticMetaObject << false << false << false;
    QTest::newRow("MyQObjectFromGadget*") << ::qMetaTypeId<MyQObjectFromGadget*>() << &MyQObjectFromGadget::staticMetaObject << false << false << true;

    QTest::newRow("GadgetDerivedAndTyped<int>") << ::qMetaTypeId<GadgetDerivedAndTyped<int>>() <<  &GadgetDerivedAndTyped<int>::staticMetaObject << true << false << false;
    QTest::newRow("GadgetDerivedAndTyped<int>*") << ::qMetaTypeId<GadgetDerivedAndTyped<int>*>() <<  &GadgetDerivedAndTyped<int>::staticMetaObject << false << true << false;
}


void tst_QMetaType::metaObject()
{
    QFETCH(int, type);
    QFETCH(const QMetaObject *, result);
    QFETCH(bool, isGadget);
    QFETCH(bool, isGadgetPtr);
    QFETCH(bool, isQObjectPtr);

    QCOMPARE(QMetaType::metaObjectForType(type), result);
    QMetaType mt(type);
    QCOMPARE(mt.metaObject(), result);
    QCOMPARE(!!(mt.flags() & QMetaType::IsGadget), isGadget);
    QCOMPARE(!!(mt.flags() & QMetaType::PointerToGadget), isGadgetPtr);
    QCOMPARE(!!(mt.flags() & QMetaType::PointerToQObject), isQObjectPtr);
}

#define METATYPE_ID_FUNCTION(Type, MetaTypeId, Name) \
  case ::qMetaTypeId< Name >(): metaType = MetaTypeIdStruct<MetaTypeId>::Value; break;

#define REGISTER_METATYPE_FUNCTION(Type, MetaTypeId, Name) \
  case qRegisterMetaType< Name >(): metaType = RegisterMetaTypeStruct<MetaTypeId>::Value; break;

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

void tst_QMetaType::constexprMetaTypeIds()
{
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
}

struct S {
  using value_type = S; // used to cause compilation error with Qt6
  int begin();
  int end();
};

// should not cause a compilation failure
// used to cause issues due to S being equal to S::value_type
Q_DECLARE_METATYPE(S)

QTEST_MAIN(tst_QMetaType)
#include "tst_qmetatype.moc"
