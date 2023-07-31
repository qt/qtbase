// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmetatype.h"

#include <QtCore/private/qmetaobjectbuilder_p.h>

#ifdef Q_OS_LINUX
# include <pthread.h>
#endif

#include <algorithm>
#include <memory>
#include <vector>

#include <QtCore/qflags.h>

Q_DECLARE_METATYPE(QMetaType::Type)
Q_DECLARE_METATYPE(QPartialOrdering)

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

// optionals of nesteds
static_assert(QTypeTraits::has_operator_equal_v<std::optional<std::variant<QString>>>);
static_assert(QTypeTraits::has_operator_less_than_v<std::optional<std::variant<QString>>>);
static_assert(!QTypeTraits::has_operator_equal_v<std::optional<std::variant<NoOperators>>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<std::variant<NoOperators>>>);

static_assert(QTypeTraits::has_operator_equal_v<std::optional<Nested>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<Nested>>);
static_assert(QTypeTraits::has_operator_equal_v<std::optional<std::tuple<int, Nested>>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<std::tuple<int, Nested>>>);
static_assert(QTypeTraits::has_operator_equal_v<std::optional<std::tuple<int, Nested>>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<std::tuple<int, Nested>>>);

static_assert(QTypeTraits::has_operator_equal_v<std::optional<Nested2>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<Nested2>>);
static_assert(QTypeTraits::has_operator_equal_v<std::optional<std::tuple<int, Nested2>>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<std::tuple<int, Nested2>>>);
static_assert(QTypeTraits::has_operator_equal_v<std::optional<std::tuple<int, Nested2>>>);
static_assert(!QTypeTraits::has_operator_less_than_v<std::optional<std::tuple<int, Nested2>>>);

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

#if QT_CONFIG(thread)
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
            QMetaType info(tp);
            if (!info.isValid()) {
                ++failureCount;
                qWarning() << "Wrong typeInfo returned for" << tp;
            }
            if (info.flags() != (QMetaType::NeedsConstruction | QMetaType::NeedsDestruction |
                                 QMetaType::NeedsCopyConstruction | QMetaType::NeedsMoveConstruction)) {
                ++failureCount;
                qWarning() << "Wrong typeInfo returned for" << tp << "got"
                           << Qt::showbase << Qt::hex << info.flags();
            }
            if (!info.isRegistered()) {
                ++failureCount;
                qWarning() << name << "is not a registered metatype";
            }
            if (!QMetaType::isRegistered(tp)) {
                ++failureCount;
                qWarning() << name << "is not a registered metatype";
            }
            if (QMetaType::fromName(nm).id() != tp) {
                ++failureCount;
                qWarning() << "Wrong metatype returned for" << name;
            }

#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
            void *buf1 = QMetaType::create(tp, nullptr);
            void *buf2 = QMetaType::create(tp, buf1);
            QMetaType::construct(tp, space, nullptr);
            QMetaType::destruct(tp, space);
            QMetaType::construct(tp, space, buf1);
            QMetaType::destruct(tp, space);

            if (!buf1) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, nullptr)";
            }
            if (!buf2) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, buf)";
            }

            QMetaType::destroy(tp, buf1);
            QMetaType::destroy(tp, buf2);
QT_WARNING_POP
#endif

            void *buf3 = info.create(nullptr);
            void *buf4 = info.create(buf3);

            info.construct(space, nullptr);
            info.destruct(space);
            info.construct(space, buf3);
            info.destruct(space);

            if (!buf3) {
                ++failureCount;
                qWarning() << "Null buffer returned by info.create(nullptr)";
            }
            if (!buf4) {
                ++failureCount;
                qWarning() << "Null buffer returned by info.create(buf)";
            }

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
#endif

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
    QCOMPARE(QMetaType(qungTfuId).name(), "TestSpace::QungTfu");
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
    QCOMPARE(::qMetaTypeId<TestSpace::Foo>(), QMetaType::fromType<TestSpace::Foo>().id());

    QCOMPARE(::qMetaTypeId<char>(), QMetaType::fromType<char>().id());
    QCOMPARE(::qMetaTypeId<uchar>(), QMetaType::fromType<unsigned char>().id());
    QCOMPARE(::qMetaTypeId<signed char>(), QMetaType::fromType<signed char>().id());
    QVERIFY(::qMetaTypeId<signed char>() != ::qMetaTypeId<char>());
    QCOMPARE(::qMetaTypeId<qint8>(), QMetaType::fromType<qint8>().id());
}

void tst_QMetaType::properties()
{
    qRegisterMetaType<QList<QVariant> >("QList<QVariant>");

    QVariant v = property("prop");

    QCOMPARE(v.typeName(), "QVariantList");

    QList<QVariant> values = v.toList();
    QCOMPARE(values.size(), 2);
    QCOMPARE(values.at(0).toInt(), 42);

    values << 43 << "world";

    QVERIFY(setProperty("prop", values));
    v = property("prop");
    QCOMPARE(v.toList().size(), 4);
}

void tst_QMetaType::normalizedTypes()
{
    int WhityIntId = ::qMetaTypeId<Whity<int> >();
    int WhityDoubleId = ::qMetaTypeId<Whity<double> >();

    QCOMPARE(QMetaType::fromName("Whity<int>").id(), WhityIntId);
    QCOMPARE(QMetaType::fromName(" Whity < int > ").id(), WhityIntId);
    QCOMPARE(QMetaType::fromName("Whity<int >").id(), WhityIntId);

    QCOMPARE(QMetaType::fromName("Whity<double>").id(), WhityDoubleId);
    QCOMPARE(QMetaType::fromName(" Whity< double > ").id(), WhityDoubleId);
    QCOMPARE(QMetaType::fromName("Whity<double >").id(), WhityDoubleId);

    QCOMPARE(qRegisterMetaType<Whity<int> >(" Whity < int > "), WhityIntId);
    QCOMPARE(qRegisterMetaType<Whity<int> >("Whity<int>"), WhityIntId);
    QCOMPARE(qRegisterMetaType<Whity<int> >("Whity<int > "), WhityIntId);

    QCOMPARE(qRegisterMetaType<Whity<double> >(" Whity < double > "), WhityDoubleId);
    QCOMPARE(qRegisterMetaType<Whity<double> >("Whity<double>"), WhityDoubleId);
    QCOMPARE(qRegisterMetaType<Whity<double> >("Whity<double > "), WhityDoubleId);
}

#define TYPENAME_DATA(MetaTypeName, MetaTypeId, RealType)\
    QTest::newRow(#RealType) << int(QMetaType::MetaTypeName) << #RealType;

namespace enumerations {
    enum Test { a = 0 };
}

static void ignoreInvalidMetaTypeWarning(int typeId)
{
    if (typeId < 0 || typeId > QMetaType::User + 500 ||
            (typeId > QMetaType::LastCoreType && typeId < QMetaType::FirstGuiType) ||
            (typeId > QMetaType::LastGuiType && typeId < QMetaType::FirstWidgetsType) ||
            (typeId > QMetaType::LastWidgetsType && typeId < QMetaType::User)) {
        QTest::ignoreMessage(QtWarningMsg, "Trying to construct an instance of an invalid type, type id: "
                             + QByteArray::number(typeId));
    }
}

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

    QTest::newRow("msvcKeywordPartOfName") << ::qMetaTypeId<enumerations::Test>() << QString::fromLatin1("enumerations::Test");
}

void tst_QMetaType::typeName()
{
    QFETCH(int, aType);
    QFETCH(QString, aTypeName);

    if (aType >= QMetaType::FirstWidgetsType)
        QSKIP("The test doesn't link against QtWidgets.");

    ignoreInvalidMetaTypeWarning(aType);
    const char *rawname = QMetaType(aType).name();
    QString name = QString::fromLatin1(rawname);

    QCOMPARE(name, aTypeName);
    QCOMPARE(name.toLatin1(), QMetaObject::normalizedType(name.toLatin1().constData()));
    QCOMPARE(rawname == nullptr, aTypeName.isNull());

    ignoreInvalidMetaTypeWarning(aType);
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
    QCOMPARE(QMetaType::fromName(aTypeName).id(), aType);
    QCOMPARE(QMetaType::fromName(aTypeName.constData()).id(), aType);
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
    QCOMPARE(QMetaType::fromName(ba).id(), expectedType);
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
    QTest::newRow("unknown-type") << int(QMetaType::UnknownType);
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(QMetaType(QMetaType::MetaTypeName).name()) << int(QMetaType::MetaTypeName);
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

template<int ID>
static void testCreateHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    auto expected = std::unique_ptr<Type>(DefaultValueFactory<ID>::create());
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    void *actual1 = QMetaType::create(ID);
    auto cleanup1 = qScopeGuard([actual1]() {
        QMetaType::destroy(ID, actual1);
    });
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    QMetaType info(ID);
    void *actual2 = info.create();
    auto cleanup2 = qScopeGuard([&info, actual2]() {
        info.destroy(actual2);
    });
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
}

template<>
void testCreateHelper<QMetaType::Void>()
{
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    void *actual1 = QMetaType::create(QMetaType::Void);
    QMetaType::destroy(QMetaType::Void, actual1);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    QMetaType info(QMetaType::Void);
    void *actual2 = info.create();
    info.destroy(actual2);
}


typedef void (*TypeTestFunction)();

void tst_QMetaType::create()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
            case QMetaType::UnknownType:
                return []() {
                    QCOMPARE(QMetaType().create(), nullptr);
                };
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
    auto expected = std::unique_ptr<Type>(TestValueFactory<ID>::create());
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    void *actual1 = QMetaType::create(ID, expected.get());
    auto cleanup1 = qScopeGuard([actual1]() {
        QMetaType::destroy(ID, actual1);
    });
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    QMetaType info(ID);
    void *actual2 = info.create(expected.get());
    auto cleanup2 = qScopeGuard([&info, actual2]() {
        info.destroy(actual2);
    });
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
}

template<>
void testCreateCopyHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    Type *expected = TestValueFactory<QMetaType::Void>::create();
    QCOMPARE(expected, nullptr); // we do not need to delete it
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    void *actual1 = QMetaType::create(QMetaType::Void, expected);
    auto cleanup1 = qScopeGuard([actual1]() {
        QMetaType::destroy(QMetaType::Void, actual1);
    });
    QCOMPARE(static_cast<Type *>(actual1), expected);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    QMetaType info(QMetaType::Void);
    void *actual2 = info.create(expected);
    auto cleanup2 = qScopeGuard([&info, actual2]() {
        info.destroy(actual2);
    });
    QCOMPARE(static_cast<Type *>(actual2), expected);
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
            case QMetaType::UnknownType:
                return []() {
                    char buf[1] = {};
                    QCOMPARE(QMetaType().create(&buf), nullptr);
                };
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
    ignoreInvalidMetaTypeWarning(type);
    QMetaType metaType(type);
    QCOMPARE(size_t(metaType.sizeOf()), size);
}

void tst_QMetaType::sizeOfStaticLess_data()
{
    sizeOf_data();
}

void tst_QMetaType::sizeOfStaticLess()
{
    QFETCH(int, type);
    QFETCH(size_t, size);
    ignoreInvalidMetaTypeWarning(type);
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
    ignoreInvalidMetaTypeWarning(type);
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

template <typename T> void addFlagsRow(const char *name, int id = qMetaTypeId<T>())
{
    QTest::newRow(name)
            << id
            << bool(QTypeInfo<T>::isRelocatable)
            << bool(!std::is_default_constructible_v<T> || !QTypeInfo<T>::isValueInitializationBitwiseZero)
            << bool(!std::is_trivially_copy_constructible_v<T>)
            << bool(!std::is_trivially_destructible_v<T>)
            << bool(QtPrivate::IsPointerToTypeDerivedFromQObject<T>::Value)
            << bool(std::is_enum<T>::value)
            << false;
}

void tst_QMetaType::flags_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isRelocatable");
    QTest::addColumn<bool>("needsConstruction");
    QTest::addColumn<bool>("needsCopyConstruction");
    QTest::addColumn<bool>("needsDestruction");
    QTest::addColumn<bool>("isPointerToQObject");
    QTest::addColumn<bool>("isEnum");
    QTest::addColumn<bool>("isQmlList");

    // invalid ids.
    QTest::newRow("-1") << -1 << false << false << false << false << false << false << false;
    QTest::newRow("-124125534") << -124125534 << false << false << false << false << false << false << false;
    QTest::newRow("124125534") << 124125534 << false << false << false << false << false << false << false;

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    addFlagsRow<RealType>(#RealType, MetaTypeId);
QT_FOR_EACH_STATIC_PRIMITIVE_NON_VOID_TYPE(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_CORE_CLASS(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_CORE_POINTER(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
    addFlagsRow<TestSpace::Foo>("TestSpace::Foo");
    addFlagsRow<Whity<double> >("Whity<double> ");
    addFlagsRow<CustomMovable>("CustomMovable");
    addFlagsRow<CustomObject*>("CustomObject*");
    addFlagsRow<CustomMultiInheritanceObject*>("CustomMultiInheritanceObject*");
    addFlagsRow<QPair<C,C> >("QPair<C,C>");
    addFlagsRow<QPair<C,M> >("QPair<C,M>");
    addFlagsRow<QPair<C,P> >("QPair<C,P>");
    addFlagsRow<QPair<M,C> >("QPair<M,C>");
    addFlagsRow<QPair<M,M> >("QPair<M,M>");
    addFlagsRow<QPair<M,P> >("QPair<M,P>");
    addFlagsRow<QPair<P,C> >("QPair<P,C>");
    addFlagsRow<QPair<P,M> >("QPair<P,M>");
    addFlagsRow<QPair<P,P> >("QPair<P,P>");
    addFlagsRow<FlagsDataEnum>("FlagsDataEnum");
}

void tst_QMetaType::flags()
{
    QFETCH(int, type);
    QFETCH(bool, isRelocatable);
    QFETCH(bool, needsConstruction);
    QFETCH(bool, needsCopyConstruction);
    QFETCH(bool, needsDestruction);
    QFETCH(bool, isPointerToQObject);
    QFETCH(bool, isEnum);
    QFETCH(bool, isQmlList);

    ignoreInvalidMetaTypeWarning(type);
    QMetaType meta(type);

    QCOMPARE(bool(meta.flags() & QMetaType::NeedsConstruction), needsConstruction);
    QCOMPARE(bool(meta.flags() & QMetaType::NeedsCopyConstruction), needsCopyConstruction);
    QCOMPARE(bool(meta.flags() & QMetaType::NeedsDestruction), needsDestruction);
    QCOMPARE(bool(meta.flags() & QMetaType::RelocatableType), isRelocatable);
    QCOMPARE(bool(meta.flags() & QMetaType::PointerToQObject), isPointerToQObject);
    QCOMPARE(bool(meta.flags() & QMetaType::IsEnumeration), isEnum);
    QCOMPARE(bool(meta.flags() & QMetaType::IsQmlList), isQmlList);
}

class NonDefaultConstructible
{
   NonDefaultConstructible(int) {}
};

struct MoveOnly
{
    MoveOnly() = default;
    MoveOnly(const MoveOnly &) = delete;
    MoveOnly(MoveOnly &&) = default;
    MoveOnly &operator=(const MoveOnly &) = delete;
    MoveOnly &operator=(MoveOnly &&) = default;
};

class Indestructible
{
protected:
    ~Indestructible() {}
};

template <typename T> static void addFlags2Row(QMetaType metaType = QMetaType::fromType<T>())
{
    QTest::newRow(metaType.name() ? metaType.name() : "UnknownType")
            << metaType
            << std::is_default_constructible_v<T>
            << std::is_copy_constructible_v<T>
            << std::is_move_constructible_v<T>
            << std::is_destructible_v<T>
            << (QTypeTraits::has_operator_equal<T>::value || QTypeTraits::has_operator_less_than<T>::value)
            << QTypeTraits::has_operator_less_than<T>::value;
};

void tst_QMetaType::flags2_data()
{
    QTest::addColumn<QMetaType>("type");
    QTest::addColumn<bool>("isDefaultConstructible");
    QTest::addColumn<bool>("isCopyConstructible");
    QTest::addColumn<bool>("isMoveConstructible");
    QTest::addColumn<bool>("isDestructible");
    QTest::addColumn<bool>("isEqualityComparable");
    QTest::addColumn<bool>("isOrdered");

    addFlags2Row<void>(QMetaType());
    addFlags2Row<void>();

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    addFlags2Row<RealType>();
    QT_FOR_EACH_STATIC_PRIMITIVE_NON_VOID_TYPE(ADD_METATYPE_TEST_ROW)
    QT_FOR_EACH_STATIC_CORE_CLASS(ADD_METATYPE_TEST_ROW)
    QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(ADD_METATYPE_TEST_ROW)
    QT_FOR_EACH_STATIC_CORE_POINTER(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW

    addFlags2Row<NonDefaultConstructible>();
    addFlags2Row<MoveOnly>();
    addFlags2Row<QObject>();
    addFlags2Row<Indestructible>();
}

void tst_QMetaType::flags2()
{
    QFETCH(QMetaType, type);
    QFETCH(bool, isDefaultConstructible);
    QFETCH(bool, isCopyConstructible);
    QFETCH(bool, isMoveConstructible);
    QFETCH(bool, isDestructible);
    QFETCH(bool, isEqualityComparable);
    QFETCH(bool, isOrdered);

    QCOMPARE(type.isDefaultConstructible(), isDefaultConstructible);
    QCOMPARE(type.isCopyConstructible(), isCopyConstructible);
    QCOMPARE(type.isMoveConstructible(), isMoveConstructible);
    QCOMPARE(type.isDestructible(), isDestructible);
    QCOMPARE(type.isEqualityComparable(), isEqualityComparable);
    QCOMPARE(type.isOrdered(), isOrdered);
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
        QTest::newRow(QMetaType(id).name()) << id << flags;
    }
}

void tst_QMetaType::flagsBinaryCompatibility6_0()
{
    QFETCH(quint32, id);
    QFETCH(quint32, flags);

    const auto currentFlags = QMetaType(id).flags();
    auto expectedFlags = QMetaType::TypeFlags(flags);

    // Only compare the values that were already defined in 5.0.
    // In 6.5, some types lost NeedsConstruction and NeedsDestruction, but
    // that's acceptable if that's because they were trivial
    quint32 mask_5_0 = 0x1ff & ~quint32(QMetaType::NeedsConstruction | QMetaType::NeedsDestruction
                                        | QMetaType::RelocatableType);

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
    auto expected = std::unique_ptr<Type>(DefaultValueFactory<ID>::create());
    QMetaType info(ID);
    int size = info.sizeOf();
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    void *storage1 = qMallocAligned(size, alignof(Type));
    void *actual1 = QMetaType::construct(ID, storage1, /*copy=*/nullptr);
    auto cleanup1 = qScopeGuard([storage1, actual1]() {
        QMetaType::destruct(ID, actual1);
        qFreeAligned(storage1);
    });
    QCOMPARE(actual1, storage1);
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
    QCOMPARE(QMetaType::construct(ID, nullptr, /*copy=*/nullptr), nullptr);
    QMetaType::destruct(ID, nullptr);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    void *storage2 = qMallocAligned(size, alignof(Type));
    void *actual2 = info.construct(storage2, /*copy=*/nullptr);
    auto cleanup2 = qScopeGuard([&info, storage2, actual2]() {
        info.destruct(actual2);
        qFreeAligned(storage2);
    });
    QCOMPARE(actual2, storage2);
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
    QCOMPARE(info.construct(nullptr, /*copy=*/nullptr), nullptr);
    info.destruct(nullptr);
}

template<>
void testConstructHelper<QMetaType::Void>()
{
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    void *storage1 = nullptr;
    void *actual1 = QMetaType::construct(QMetaType::Void, storage1, /*copy=*/nullptr);
    auto cleanup1 = qScopeGuard([storage1, actual1]() {
        QMetaType::destruct(QMetaType::Void, actual1);
        qFreeAligned(storage1);
    });
    QCOMPARE(actual1, storage1);
    QCOMPARE(QMetaType::construct(QMetaType::Void, nullptr, /*copy=*/nullptr), nullptr);
    QMetaType::destruct(QMetaType::Void, nullptr);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    QMetaType info(QMetaType::Void);
    void *storage2 = nullptr;
    void *actual2 = info.construct(storage2, /*copy=*/nullptr);
    auto cleanup2 = qScopeGuard([&info, storage2, actual2]() {
        info.destruct(actual2);
        qFreeAligned(storage2);
    });
    QCOMPARE(actual2, storage2);
    QVERIFY(info.construct(nullptr, /*copy=*/nullptr) == nullptr);
    info.destruct(nullptr);
}

void tst_QMetaType::construct()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
            case QMetaType::UnknownType:
                return []() {
                    char buf[1];
                    QCOMPARE(QMetaType().construct(&buf), nullptr);
                };
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


namespace TriviallyConstructibleTests {

enum Enum0 {};
enum class Enum1 {};

static_assert(QTypeInfo<int>::isValueInitializationBitwiseZero);
static_assert(QTypeInfo<double>::isValueInitializationBitwiseZero);
static_assert(QTypeInfo<Enum0>::isValueInitializationBitwiseZero);
static_assert(QTypeInfo<Enum1>::isValueInitializationBitwiseZero);
static_assert(QTypeInfo<int *>::isValueInitializationBitwiseZero);
static_assert(QTypeInfo<void *>::isValueInitializationBitwiseZero);
static_assert(QTypeInfo<std::nullptr_t>::isValueInitializationBitwiseZero);

struct A {};
struct B { B() {} };
struct C { ~C() {} };
struct D { D(int) {} };
struct E { E() {} ~E() {} };
struct F { int i; };
struct G { G() : i(0) {} int i; };
struct H { constexpr H() : i(0) {} int i; };
struct I { I() : i(42) {} int i; };
struct J { constexpr J() : i(42) {} int i; };
struct K { K() : i(0) {} ~K() {} int i; };

static_assert(!QTypeInfo<A>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<B>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<C>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<D>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<E>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<F>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<G>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<H>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<I>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<J>::isValueInitializationBitwiseZero);
static_assert(!QTypeInfo<K>::isValueInitializationBitwiseZero);

} // namespace TriviallyConstructibleTests

// Value-initializing these trivially constructible types cannot be achieved by
// memset(0) into their storage. For instance, on Itanium, a pointer to a data
// member needs to be value-initialized by setting it to -1.

// Fits into QVariant
struct TrivialTypeNotZeroInitableSmall {
    int TrivialTypeNotZeroInitableSmall::*pdm;
};

static_assert(std::is_trivially_default_constructible_v<TrivialTypeNotZeroInitableSmall>);
static_assert(!QTypeInfo<TrivialTypeNotZeroInitableSmall>::isValueInitializationBitwiseZero);
static_assert(sizeof(TrivialTypeNotZeroInitableSmall) < sizeof(QVariant)); // also checked more thoroughly below

// Does not fit into QVariant internal storage
struct TrivialTypeNotZeroInitableBig {
    int a;
    double b;
    char c;
    int array[42];
    void (TrivialTypeNotZeroInitableBig::*pmf)();
    int TrivialTypeNotZeroInitableBig::*pdm;
};

static_assert(std::is_trivially_default_constructible_v<TrivialTypeNotZeroInitableBig>);
static_assert(!QTypeInfo<TrivialTypeNotZeroInitableSmall>::isValueInitializationBitwiseZero);
static_assert(sizeof(TrivialTypeNotZeroInitableBig) > sizeof(QVariant)); // also checked more thoroughly below

void tst_QMetaType::defaultConstructTrivial_QTBUG_109594()
{
    // MSVC miscompiles value-initialization of pointers to data members,
    // https://developercommunity.visualstudio.com/t/Pointer-to-data-member-is-not-initialize/10238905
    {
        QMetaType mt = QMetaType::fromType<TrivialTypeNotZeroInitableSmall>();
        QVERIFY(mt.isDefaultConstructible());
        auto ptr = static_cast<TrivialTypeNotZeroInitableSmall *>(mt.create());
        const auto cleanup = qScopeGuard([=] {
            mt.destroy(ptr);
        });
#ifdef Q_CC_MSVC_ONLY
        QEXPECT_FAIL("", "MSVC compiler bug", Continue);
#endif
        QCOMPARE(ptr->pdm, nullptr);

        QVariant v(mt);
        QVERIFY(QVariant::Private::canUseInternalSpace(mt.iface()));
        auto obj = v.value<TrivialTypeNotZeroInitableSmall>();
#ifdef Q_CC_MSVC_ONLY
        QEXPECT_FAIL("", "MSVC compiler bug", Continue);
#endif
        QCOMPARE(obj.pdm, nullptr);
    }

    {
        QMetaType mt = QMetaType::fromType<TrivialTypeNotZeroInitableBig>();
        QVERIFY(mt.isDefaultConstructible());
        auto ptr = static_cast<TrivialTypeNotZeroInitableBig *>(mt.create());
        const auto cleanup = qScopeGuard([=] {
            mt.destroy(ptr);
        });
        QCOMPARE(ptr->a, 0);
        QCOMPARE(ptr->b, 0.0);
        QCOMPARE(ptr->c, '\0');
        QCOMPARE(ptr->pmf, nullptr);
        for (int i : ptr->array)
            QCOMPARE(i, 0);
#ifdef Q_CC_MSVC_ONLY
        QEXPECT_FAIL("", "MSVC compiler bug", Continue);
#endif
        QCOMPARE(ptr->pdm, nullptr);

        QVariant v(mt);
        QVERIFY(!QVariant::Private::canUseInternalSpace(mt.iface()));
        auto obj = v.value<TrivialTypeNotZeroInitableBig>();
        QCOMPARE(obj.a, 0);
        QCOMPARE(obj.b, 0.0);
        QCOMPARE(obj.c, '\0');
        QCOMPARE(obj.pmf, nullptr);
        for (int i : obj.array)
            QCOMPARE(i, 0);
#ifdef Q_CC_MSVC_ONLY
        QEXPECT_FAIL("", "MSVC compiler bug", Continue);
#endif
        QCOMPARE(obj.pdm, nullptr);
    }
}

void tst_QMetaType::typedConstruct()
{
    auto testMetaObjectWriteOnGadget = [](QVariant &gadget, const QList<GadgetPropertyType> &properties)
    {
        auto metaObject = QMetaType(gadget.userType()).metaObject();
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
        auto metaObject = QMetaType(gadget.userType()).metaObject();
        QVERIFY(metaObject != nullptr);
        QCOMPARE(metaObject->methodCount(), 0);
        QCOMPARE(metaObject->propertyCount(), properties.size());
        for (int i = 0; i < metaObject->propertyCount(); ++i) {
            auto prop = metaObject->property(i);
            QCOMPARE(properties[i].name, prop.name());
            QCOMPARE(properties[i].type, prop.typeName());
            if (!QMetaType(prop.userType()).flags().testFlag(QMetaType::IsGadget))
                QCOMPARE(properties[i].testData, prop.readOnGadget(gadget.constData()));
        }
    };

    QList<GadgetPropertyType> dynamicGadget1 = {
        {"int", "int_prop", 34526},
        {"float", "float_prop", 1.23f},
        {"QString", "string_prop", QString{"Test QString"}}
    };
    registerGadget("DynamicGadget1", dynamicGadget1);

    QVariant testGadget1(QMetaType::fromName("DynamicGadget1"));
    testMetaObjectWriteOnGadget(testGadget1, dynamicGadget1);
    testMetaObjectReadOnGadget(testGadget1, dynamicGadget1);


    QList<GadgetPropertyType> dynamicGadget2 = {
        {"int", "int_prop", 512},
        {"double", "double_prop", 4.56},
        {"QString", "string_prop", QString{"Another String"}},
        {"DynamicGadget1", "dynamicGadget1_prop", testGadget1}
    };
    registerGadget("DynamicGadget2", dynamicGadget2);
    QVariant testGadget2(QMetaType::fromName("DynamicGadget2"));
    testMetaObjectWriteOnGadget(testGadget2, dynamicGadget2);
    testMetaObjectReadOnGadget(testGadget2, dynamicGadget2);
    auto g2mo = QMetaType(testGadget2.userType()).metaObject();
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
    QCOMPARE(podTypeId, QMetaType::fromName(podTypeName).id());
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
    auto expected = std::unique_ptr<Type>(TestValueFactory<ID>::create());
    QMetaType info(ID);
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    const int size = QMetaType::sizeOf(ID);
    QCOMPARE(info.sizeOf(), size);
    void *storage1 = qMallocAligned(size, alignof(Type));
    void *actual1 = QMetaType::construct(ID, storage1, expected.get());
    auto cleanup1 = qScopeGuard([storage1, actual1]() {
        QMetaType::destruct(ID, actual1);
        qFreeAligned(storage1);
    });
    QCOMPARE(actual1, storage1);
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
    QCOMPARE(QMetaType::construct(ID, nullptr, nullptr), nullptr);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    void *storage2 = qMallocAligned(info.sizeOf(), alignof(Type));
    void *actual2 = info.construct(storage2, expected.get());
    auto cleanup2 = qScopeGuard([&info, storage2, actual2]() {
        info.destruct(actual2);
        qFreeAligned(storage2);
    });
    QCOMPARE(actual2, storage2);
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
    QCOMPARE(info.construct(nullptr, expected.get()), nullptr);
}

template<>
void testConstructCopyHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    Type *expected = TestValueFactory<QMetaType::Void>::create();
    QCOMPARE(expected, nullptr); // we do not need to delete it
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    void *storage1 = nullptr;
    void *actual1 = QMetaType::construct(QMetaType::Void, storage1, expected);
    auto cleanup1 = qScopeGuard([storage1, actual1]() {
        QMetaType::destruct(QMetaType::Void, actual1);
        qFreeAligned(storage1);
    });
    QCOMPARE(actual1, storage1);
    QCOMPARE(QMetaType::construct(QMetaType::Void, nullptr, nullptr), nullptr);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
    QMetaType info(QMetaType::Void);
    void *storage2 = nullptr;
    void *actual2 = info.construct(storage2, expected);
    auto cleanup2 = qScopeGuard([&info, storage2, actual2]() {
        info.destruct(actual2);
        qFreeAligned(storage2);
    });
    QCOMPARE(actual2, storage2);
    QCOMPARE(info.construct(nullptr, expected), nullptr);
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
            case QMetaType::UnknownType:
                return []() {
                    char buf[1], buf2[1] = {};
                    QCOMPARE(QMetaType().construct(&buf, &buf2), nullptr);
                };
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

void tst_QMetaType::selfCompare_data()
{
    qRegisterMetaType<QPartialOrdering>();
    QTest::addColumn<int>("type");
    QTest::addColumn<QPartialOrdering>("order");

    auto orderingFor = [](QMetaType::Type t) {
        if (t == QMetaType::UnknownType || t == QMetaType::Void)
            return QPartialOrdering::Unordered;
        return QPartialOrdering::Equivalent;
    };

    QTest::newRow("unknown-type") << int(QMetaType::UnknownType) << orderingFor(QMetaType::UnknownType);

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(QMetaType(QMetaType::MetaTypeName).name()) << int(QMetaType::MetaTypeName) << orderingFor(QMetaType::MetaTypeName);
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QMetaType::selfCompare()
{
    QFETCH(int, type);
    QFETCH(QPartialOrdering, order);

    QMetaType t(type);
    void *v1 = t.create(nullptr);
    void *v2 = t.create(nullptr);
    auto scope = qScopeGuard([=] {
        t.destroy(v1);
        t.destroy(v2);
    });

    // all these types have an equality comparator
    QCOMPARE(t.equals(v1, v2), order == QPartialOrdering::Equivalent);

    if (t.iface() && t.iface()->lessThan)
        QCOMPARE(t.compare(v1, v2), order);

    // for the primitive types, do a memcmp() too
    switch (type) {
    default:
        break;

#define ADD_METATYPE_CASE(MetaTypeName, MetaTypeId, RealType) \
    case QMetaType::MetaTypeName:
FOR_EACH_PRIMITIVE_METATYPE(ADD_METATYPE_CASE)
#undef ADD_METATYPE_CASE
        QCOMPARE(memcmp(v1, v2, t.sizeOf()), 0);
    }
}

typedef QString CustomString;
Q_DECLARE_METATYPE(CustomString) //this line is useless

void tst_QMetaType::typedefs()
{
    QCOMPARE(QMetaType::fromName("long long").id(), int(QMetaType::LongLong));
    QCOMPARE(QMetaType::fromName("unsigned long long").id(), int(QMetaType::ULongLong));
    QCOMPARE(QMetaType::fromName("qint8").id(), int(QMetaType::SChar));
    QCOMPARE(QMetaType::fromName("quint8").id(), int(QMetaType::UChar));
    QCOMPARE(QMetaType::fromName("qint16").id(), int(QMetaType::Short));
    QCOMPARE(QMetaType::fromName("quint16").id(), int(QMetaType::UShort));
    QCOMPARE(QMetaType::fromName("qint32").id(), int(QMetaType::Int));
    QCOMPARE(QMetaType::fromName("quint32").id(), int(QMetaType::UInt));
    QCOMPARE(QMetaType::fromName("qint64").id(), int(QMetaType::LongLong));
    QCOMPARE(QMetaType::fromName("quint64").id(), int(QMetaType::ULongLong));

    // make sure the qreal typeId is the type id of the type it's defined to
    QCOMPARE(QMetaType::fromName("qreal").id(), ::qMetaTypeId<qreal>());

    qRegisterMetaType<CustomString>("CustomString");
    QCOMPARE(QMetaType::fromName("CustomString").id(), ::qMetaTypeId<CustomString>());

    typedef Whity<double> WhityDouble;
    qRegisterMetaType<WhityDouble>("WhityDouble");
    QCOMPARE(QMetaType::fromName("WhityDouble").id(), ::qMetaTypeId<WhityDouble>());
}

struct RegisterTypeType {};

void tst_QMetaType::registerType()
{
    // Built-in
    QCOMPARE(qRegisterMetaType<QString>("QString"), int(QMetaType::QString));
    QCOMPARE(qRegisterMetaType<QString>("QString"), int(QMetaType::QString));
    qRegisterMetaType(QMetaType::fromType<QString>());

    // Custom
    int fooId = qRegisterMetaType<TestSpace::Foo>("TestSpace::Foo");
    QVERIFY(fooId >= int(QMetaType::User));
    QCOMPARE(qRegisterMetaType<TestSpace::Foo>("TestSpace::Foo"), fooId);
    qRegisterMetaType(QMetaType::fromType<TestSpace::Foo>());

    int movableId = qRegisterMetaType<CustomMovable>("CustomMovable");
    QVERIFY(movableId >= int(QMetaType::User));
    QCOMPARE(qRegisterMetaType<CustomMovable>("CustomMovable"), movableId);
    qRegisterMetaType(QMetaType::fromType<CustomMovable>());

    // Aliases are deprecated

    // Alias to built-in
    typedef QString MyString;

    QCOMPARE(qRegisterMetaType<MyString>("MyString"), int(QMetaType::QString));
    QCOMPARE(qRegisterMetaType<MyString>("MyString"), int(QMetaType::QString));

    QCOMPARE(QMetaType::fromType<MyString>().id(), int(QMetaType::QString));

    // Alias to custom type
    typedef CustomMovable MyMovable;
    typedef TestSpace::Foo MyFoo;

    QCOMPARE(qRegisterMetaType<MyMovable>("MyMovable"), movableId);
    QCOMPARE(qRegisterMetaType<MyMovable>("MyMovable"), movableId);

    QCOMPARE(QMetaType::fromType<MyMovable>().id(), movableId);

    QCOMPARE(qRegisterMetaType<MyFoo>("MyFoo"), fooId);
    QCOMPARE(qRegisterMetaType<MyFoo>("MyFoo"), fooId);

    QCOMPARE(QMetaType::fromType<MyFoo>().id(), fooId);

    // this portion of the test can only be run once
    static bool typeWasRegistered = false;
    if (!typeWasRegistered) {
        QMetaType mt = QMetaType::fromType<RegisterTypeType>();
        QVERIFY(mt.isValid());
        QCOMPARE_NE(mt.name(), nullptr);

        QVERIFY(!mt.isRegistered());
        QVERIFY(!QMetaType::fromName(mt.name()).isValid());

        QCOMPARE_GT(qRegisterMetaType(mt), 0);
        typeWasRegistered = true;

        QVERIFY(mt.isRegistered());
        QVERIFY(QMetaType::fromName(mt.name()).isValid());
    }
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
    QTest::newRow("IsRegisteredDummyType + 1000") << (dummyTypeId + 1000) << false;
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
    QVERIFY((QMetaType(type0).flags() & QMetaType::IsEnumeration) == 0);

    int type1 = qRegisterMetaType<isEnumTest_Enum0>("isEnumTest_Enum0");
    QVERIFY((QMetaType(type1).flags() & QMetaType::IsEnumeration) == QMetaType::IsEnumeration);

    int type2 = qRegisterMetaType<isEnumTest_Struct0>("isEnumTest_Struct0");
    QVERIFY((QMetaType(type2).flags() & QMetaType::IsEnumeration) == 0);

    int type3 = qRegisterMetaType<isEnumTest_Enum0 *>("isEnumTest_Enum0 *");
    QVERIFY((QMetaType(type3).flags() & QMetaType::IsEnumeration) == 0);

    int type4 = qRegisterMetaType<isEnumTest_Struct0::A>("isEnumTest_Struct0::A");
    QVERIFY((QMetaType(type4).flags() & QMetaType::IsEnumeration) == QMetaType::IsEnumeration);

    int type5 = ::qMetaTypeId<isEnumTest_Struct1>();
    QVERIFY((QMetaType(type5).flags() & QMetaType::IsEnumeration) == 0);

    int type6 = ::qMetaTypeId<isEnumTest_Enum1>();
    QVERIFY((QMetaType(type6).flags() & QMetaType::IsEnumeration) == QMetaType::IsEnumeration);
}

enum E1 : unsigned char {};
enum E2 : qlonglong {};
enum class E3 : unsigned short {};

namespace myflags {

    Q_NAMESPACE

    enum  Flag1 : int { A, B };
    enum  Flag2 : short { X, Y };

    Q_DECLARE_FLAGS(Flags1, myflags::Flag1);
    Q_FLAG_NS(Flags1)
    Q_DECLARE_FLAGS(Flags2, myflags::Flag2);
    Q_FLAG_NS(Flags2)

}

template <typename T>
using getUnderlyingTypeNormalized = std::conditional_t<
    std::is_signed_v<std::underlying_type_t<T>>,
    typename QIntegerForSize<sizeof(T)>::Signed,
    typename QIntegerForSize<sizeof(T)>::Unsigned
>;

void tst_QMetaType::underlyingType_data()
{
    QTest::addColumn<QMetaType>("source");
    QTest::addColumn<QMetaType>("underlying");

    QTest::newRow("invalid") << QMetaType() << QMetaType();
    QTest::newRow("plain") << QMetaType::fromType<isEnumTest_Enum1>()
                           << QMetaType::fromType<getUnderlyingTypeNormalized<isEnumTest_Enum1>>();
    QTest::newRow("uchar") << QMetaType::fromType<E1>()
                           << QMetaType::fromType<getUnderlyingTypeNormalized<E1>>();
    QTest::newRow("long") << QMetaType::fromType<E2>()
                          << QMetaType::fromType<getUnderlyingTypeNormalized<E2>>();
    QTest::newRow("class_ushort") << QMetaType::fromType<E3>()
                                  << QMetaType::fromType<getUnderlyingTypeNormalized<E3>>();
    QTest::newRow("flags_int") << QMetaType::fromType<myflags::Flags1>()
                               << QMetaType::fromType<int>();
    QTest::newRow("flags_short")  << QMetaType::fromType<myflags::Flags2>()
                                  << QMetaType::fromType<int>(); // sic, not short!
}

void tst_QMetaType::underlyingType()
{
    QFETCH(QMetaType, source);
    QFETCH(QMetaType, underlying);
    QCOMPARE(source.underlyingType(), underlying);
}

void tst_QMetaType::isRegisteredStaticLess_data()
{
    isRegistered_data();
}

void tst_QMetaType::isRegisteredStaticLess()
{
    QFETCH(int, typeId);
    QFETCH(bool, registered);
    ignoreInvalidMetaTypeWarning(typeId);
    QCOMPARE(QMetaType(typeId).isRegistered(), registered);
}

struct NotARegisteredType {};
void tst_QMetaType::isNotRegistered()
{
    QVERIFY(!QMetaType::fromType<NotARegisteredType>().isRegistered());
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

void tst_QMetaType::automaticTemplateRegistration_1()
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


    REGISTER_TYPEDEF(QHash, int, uint)
    REGISTER_TYPEDEF(QMap, int, uint)
    REGISTER_TYPEDEF(QPair, int, uint)

    FOR_EACH_STATIC_PRIMITIVE_TYPE(
      PRINT_1ARG_TEMPLATE
    )

    CREATE_AND_VERIFY_CONTAINER(QList, QList<QMap<int, QHash<char, QList<QVariant>>>>)
    CREATE_AND_VERIFY_CONTAINER(QList, void*)
    CREATE_AND_VERIFY_CONTAINER(QList, const void*)
    CREATE_AND_VERIFY_CONTAINER(QList, void*)
    CREATE_AND_VERIFY_CONTAINER(std::pair, void*, void*)
    CREATE_AND_VERIFY_CONTAINER(QHash, void*, void*)
    CREATE_AND_VERIFY_CONTAINER(QHash, const void*, const void*)

#define TEST_OWNING_SMARTPOINTER(SMARTPOINTER, ELEMENT_TYPE, FLAG_TEST, FROMVARIANTFUNCTION) \
    { \
        SMARTPOINTER < ELEMENT_TYPE > sp(new ELEMENT_TYPE); \
        sp.data()->setObjectName("Test name"); \
        QVariant v = QVariant::fromValue(sp); \
        QCOMPARE(v.typeName(), #SMARTPOINTER "<" #ELEMENT_TYPE ">"); \
        QVERIFY(QMetaType(::qMetaTypeId<SMARTPOINTER < ELEMENT_TYPE > >()).flags() & QMetaType::FLAG_TEST); \
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
        QVERIFY(QMetaType(::qMetaTypeId<SMARTPOINTER < ELEMENT_TYPE > >()).flags() & QMetaType::FLAG_TEST); \
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
        QVERIFY(QMetaType(::qMetaTypeId<QWeakPointer < ELEMENT_TYPE > >()).flags() & QMetaType::FLAG_TEST); \
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

    QTest::newRow("unknown-type") << int(QMetaType::UnknownType) << false;

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId << bool(StreamingTraits<RealType>::isStreamable);
    QT_FOR_EACH_STATIC_TYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QMetaType::saveAndLoadBuiltin()
{
    QFETCH(int, type);
    QFETCH(bool, isStreamable);

    QMetaType metaType(type);
    void *value = metaType.create();

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);
    QCOMPARE(metaType.save(stream, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable) {
        QVERIFY(metaType.hasRegisteredDataStreamOperators());
        QVERIFY(metaType.load(stream, value)); // Hmmm, shouldn't it return false?

        // std::nullptr_t is nullary: it doesn't actually read anything
        if (type != QMetaType::Nullptr)
            QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    } else {
        QVERIFY(!metaType.hasRegisteredDataStreamOperators());
    }

    stream.device()->seek(0);
    stream.resetStatus();
    QCOMPARE(metaType.load(stream, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable) {
        QVERIFY(metaType.load(stream, value)); // Hmmm, shouldn't it return false?

        // std::nullptr_t is nullary: it doesn't actually read anything
        if (type != QMetaType::Nullptr)
            QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    }

    metaType.destroy(value);
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
    QMetaType metaType(id);

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);

    QVERIFY(metaType.save(stream, &t));
    QCOMPARE(stream.status(), QDataStream::Ok);

    CustomStreamableType t2;
    t2.a = -1;
    QVERIFY(metaType.load(stream, &t2)); // Hmmm, shouldn't it return false?
    QCOMPARE(stream.status(), QDataStream::ReadPastEnd);
    QCOMPARE(t2.a, -1);

    stream.device()->seek(0);
    stream.resetStatus();
    QVERIFY(metaType.load(stream, &t2));
    QCOMPARE(stream.status(), QDataStream::Ok);
    QCOMPARE(t2.a, t.a);

    QVERIFY(metaType.load(stream, &t2)); // Hmmm, shouldn't it return false?
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

    QTest::newRow("unknown-type") << int(QMetaType::UnknownType) << static_cast<const QMetaObject *>(0) << false << false << false;
    QTest::newRow("QObject") << int(QMetaType::QObjectStar) << &QObject::staticMetaObject << false << false << true;
    QTest::newRow("QFile*") << ::qMetaTypeId<QFile*>() << &QFile::staticMetaObject << false << false << true;
    QTest::newRow("MyObject") << ::qMetaTypeId<MyObject>() << &MyObject::staticMetaObject << false << false << false;
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
