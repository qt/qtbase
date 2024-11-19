// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qobject.h>
#include <qmetaobject.h>
#include <QMap>
#include <QString>

struct CustomType
{
    int padding;
    QString str;
    CustomType(const QString &str = QString()) : str(str) {}
    operator QString() const { return str; }
    friend bool operator==(const CustomType &a, const CustomType &b)
    { return a.str == b.str; }
};

Q_DECLARE_METATYPE(CustomType)

class Base : public QObject
{
    Q_OBJECT
signals:
    void baseSignal(int);
};

class tst_QMetaProperty : public Base
{
    Q_OBJECT
    Q_PROPERTY(EnumType value WRITE setValue READ getValue)
    Q_PROPERTY(EnumType value2 WRITE set_value READ get_value)
    Q_PROPERTY(QString value7 MEMBER value7 RESET resetValue7)
    Q_PROPERTY(int value8 READ value8)
    Q_PROPERTY(int value9 READ value9 CONSTANT)
    Q_PROPERTY(int value10 READ value10 FINAL)
    Q_PROPERTY(QMap<int, int> map MEMBER map)
    Q_PROPERTY(CustomType custom MEMBER custom)
    Q_PROPERTY(int propWithInheritedSig READ propWithInheritedSig NOTIFY baseSignal)
    Q_PROPERTY(QFlags<Qt::AlignmentFlag> qflags_value MEMBER qflags_value)

private slots:
    void hasStdCppSet();
    void isConstant();
    void isFinal();
    void gadget();
    void readAndWriteWithLazyRegistration();
    void mapProperty();
    void conversion();
    void enumsFlags();
    void notifySignalIndex();
    void qflags();

public:
    enum EnumType { EnumType1 };

    void setValue(EnumType) {}
    EnumType getValue() const { return EnumType1; }
    void set_value(EnumType) {}
    EnumType get_value() const { return EnumType1; }

    void resetValue7() { value7 = QStringLiteral("reset"); }
    int value8() const { return 1; }
    int value9() const { return 1; }
    int value10() const { return 1; }

    int propWithInheritedSig() const { return 0; }

    QString value7;
    QMap<int, int> map;
    CustomType custom;
    QFlags<Qt::AlignmentFlag> qflags_value;
};

void tst_QMetaProperty::hasStdCppSet()
{
    const QMetaObject *mo = metaObject();

    QMetaProperty prop = mo->property(mo->indexOfProperty("value"));
    QVERIFY(prop.isValid());
    QVERIFY(prop.hasStdCppSet());

    prop = mo->property(mo->indexOfProperty("value2"));
    QVERIFY(prop.isValid());
    QVERIFY(!prop.hasStdCppSet());
}

void tst_QMetaProperty::isConstant()
{
    const QMetaObject *mo = metaObject();

    QMetaProperty prop = mo->property(mo->indexOfProperty("value8"));
    QVERIFY(prop.isValid());
    QVERIFY(!prop.isConstant());

    prop = mo->property(mo->indexOfProperty("value9"));
    QVERIFY(prop.isValid());
    QVERIFY(prop.isConstant());
}

void tst_QMetaProperty::isFinal()
{
    const QMetaObject *mo = metaObject();

    QMetaProperty prop = mo->property(mo->indexOfProperty("value10"));
    QVERIFY(prop.isValid());
    QVERIFY(prop.isFinal());

    prop = mo->property(mo->indexOfProperty("value9"));
    QVERIFY(prop.isValid());
    QVERIFY(!prop.isFinal());
}

void tst_QMetaProperty::qflags()
{
    const QMetaObject *mo = metaObject();

    QMetaProperty prop = mo->property(mo->indexOfProperty("qflags_value"));
    QVERIFY(prop.isEnumType());
    QVERIFY(prop.isFlagType());
}


class MyGadget {
    Q_GADGET
    Q_PROPERTY(QString value READ getValue WRITE setValue RESET resetValue)
public:
    QString m_value;
    void setValue(const QString &value) { m_value = value; }
    QString getValue() { return m_value; }
    void resetValue() { m_value = QLatin1String("reset"); }
};

void tst_QMetaProperty::gadget()
{
    const QMetaObject *mo = &MyGadget::staticMetaObject;
    QMetaProperty valueProp = mo->property(mo->indexOfProperty("value"));
    QVERIFY(valueProp.isValid());
    QCOMPARE(valueProp.metaType(), QMetaType::fromType<QString>());
    {
        MyGadget g;
        QString hello = QLatin1String("hello");
        QVERIFY(valueProp.writeOnGadget(&g, hello));
        QCOMPARE(g.m_value, QLatin1String("hello"));
        QCOMPARE(valueProp.readOnGadget(&g), QVariant(hello));
        QVERIFY(valueProp.resetOnGadget(&g));
        QCOMPARE(valueProp.readOnGadget(&g), QVariant(QLatin1String("reset")));
    }
}

struct CustomReadObject : QObject
{
    Q_OBJECT
};

struct CustomWriteObject : QObject
{
    Q_OBJECT
};

struct CustomWriteObjectChild : CustomWriteObject
{
    Q_OBJECT
};

struct TypeLazyRegistration : QObject
{
    Q_OBJECT
    Q_PROPERTY(CustomReadObject *read MEMBER _read)
    Q_PROPERTY(CustomWriteObject *write MEMBER _write)

    CustomReadObject *_read;
    CustomWriteObject *_write;

public:
    TypeLazyRegistration()
        : _read()
        , _write()
    {}
};

enum FreeEnum {
    FreeEnumValue1,
    FreeEnumValue2
};

namespace MySpace {
    enum NamespacedEnum {
        NamespacedEnumValue1,
        NamespacedEnumValue2
    };
};

namespace MyQtSpace {
    Q_NAMESPACE
    enum NamespacedEnum {
        NamespacedEnumValue1,
        NamespacedEnumValue2
    };
    Q_DECLARE_FLAGS(NamespacedFlags, NamespacedEnum)
    Q_FLAG_NS(NamespacedFlags)
};

namespace SeparateEnumNamespace {
    Q_NAMESPACE
    enum Enum {
        Value1,
        Value2
    };
    Q_ENUM_NS(Enum)
};
namespace SeparateFlagsNamespace {
    Q_NAMESPACE
    Q_DECLARE_FLAGS(Flags, SeparateEnumNamespace::Enum)
    Q_FLAG_NS(Flags)
}

class EnumFlagsTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TestEnum enumProperty READ enumProperty WRITE setEnumProperty)
    Q_PROPERTY(TestFlags flagProperty READ flagProperty WRITE setFlagProperty)
    Q_PROPERTY(UnsignedTestFlags unsignedFlagProperty READ unsignedFlagProperty WRITE setUnsignedFlagProperty)
    Q_PROPERTY(TestFlags64 flag64Property READ flag64Property WRITE setFlag64Property)
    Q_PROPERTY(UnsignedTestFlags64 unsignedFlag64Property READ unsignedFlag64Property WRITE setUnsignedFlag64Property)
    Q_PROPERTY(TestFlags64 storedFlag64Property MEMBER m_flags64)

    Q_PROPERTY(FreeEnum freeEnumProperty READ freeEnumProperty WRITE setFreeEnumProperty)
    Q_PROPERTY(MySpace::NamespacedEnum namespacedEnumProperty READ namespacedEnumProperty WRITE setNamespacedEnumProperty)
    Q_PROPERTY(MyQtSpace::NamespacedEnum qtNamespacedEnumProperty READ qtNamespacedEnumProperty WRITE setQtNamespacedEnumProperty)
    Q_PROPERTY(MyQtSpace::NamespacedFlags qtNamespacedFlagProperty READ qtNamespacedFlagProperty WRITE setQtNamespacedFlagProperty)

    Q_PROPERTY(SeparateEnumNamespace::Enum sepEnum READ sepEnum WRITE setSepEnum)
    Q_PROPERTY(SeparateFlagsNamespace::Flags sepFlags READ sepFlags WRITE setSepFlags)
public:
    enum TestEnum { e1, e2 };
    Q_ENUM(TestEnum)

    enum TestFlag { flag1 = 0x1, flag2 = 0x2 };
    Q_DECLARE_FLAGS(TestFlags, TestFlag)

    enum UnsignedTestFlag : uint { uflag1 = 0x1, uflag2 = 0x2 };
    Q_DECLARE_FLAGS(UnsignedTestFlags, UnsignedTestFlag)

    enum class TestFlag64 { flag1 = 0x1, flag2 = -1 };
    Q_DECLARE_FLAGS(TestFlags64, TestFlag64)

    enum class UnsignedTestFlag64 : qulonglong { uflag1 = 0x1, uflag2 = Q_UINT64_C(0x1234'5678'9abc'def0) };
    Q_DECLARE_FLAGS(UnsignedTestFlags64, UnsignedTestFlag64)
    Q_FLAG(UnsignedTestFlags64)

    using QObject::QObject;

    TestEnum enumProperty() const { return m_enum; }
    void setEnumProperty(TestEnum e) { m_enum = e; }

    TestFlags flagProperty() const { return m_flags; }
    void setFlagProperty(TestFlags f) { m_flags = f; }
    UnsignedTestFlags unsignedFlagProperty() const { return m_uflags; }
    void setUnsignedFlagProperty(UnsignedTestFlags f) { m_uflags = f; }

    TestFlags64 flag64Property() const { return m_flags64; }
    void setFlag64Property(TestFlags64 f) { m_flags64 = f; }
    UnsignedTestFlags64 unsignedFlag64Property() const { return m_uflags64; }
    void setUnsignedFlag64Property(UnsignedTestFlags64 f) { m_uflags64 = f; }

    FreeEnum freeEnumProperty() const { return m_freeEnum; }
    void setFreeEnumProperty(FreeEnum e) { m_freeEnum = e; }

    MySpace::NamespacedEnum namespacedEnumProperty() const { return m_namespacedEnum; }
    void setNamespacedEnumProperty(MySpace::NamespacedEnum e) { m_namespacedEnum = e; }

    MyQtSpace::NamespacedEnum qtNamespacedEnumProperty() const { return m_qtNamespaceEnum; }
    void setQtNamespacedEnumProperty(MyQtSpace::NamespacedEnum e) { m_qtNamespaceEnum = e; }

    MyQtSpace::NamespacedFlags qtNamespacedFlagProperty() const { return m_qtNamespaceFlags; }
    void setQtNamespacedFlagProperty(MyQtSpace::NamespacedFlags f) { m_qtNamespaceFlags = f; }

    SeparateEnumNamespace::Enum sepEnum() const { return m_sepEnum; }
    void setSepEnum(SeparateEnumNamespace::Enum e) { m_sepEnum = e; }

    SeparateFlagsNamespace::Flags sepFlags() const { return m_sepFlags; }
    void setSepFlags(SeparateFlagsNamespace::Flags f) { m_sepFlags = f; }

private:
    friend class tst_QMetaProperty;
    TestEnum m_enum = e1;
    TestFlags m_flags;
    UnsignedTestFlags m_uflags;
    TestFlags64 m_flags64;
    UnsignedTestFlags64 m_uflags64;

    FreeEnum m_freeEnum = FreeEnum::FreeEnumValue1;
    MySpace::NamespacedEnum m_namespacedEnum = MySpace::NamespacedEnumValue1;
    MyQtSpace::NamespacedEnum m_qtNamespaceEnum = MyQtSpace::NamespacedEnumValue1;
    MyQtSpace::NamespacedFlags m_qtNamespaceFlags;

    SeparateEnumNamespace::Enum m_sepEnum = SeparateEnumNamespace::Value1;
    SeparateFlagsNamespace::Flags m_sepFlags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EnumFlagsTester::TestFlags)

void tst_QMetaProperty::readAndWriteWithLazyRegistration()
{
    QVERIFY(!QMetaType::fromName("CustomReadObject*").isValid());
    QVERIFY(!QMetaType::fromName("CustomWriteObject*").isValid());

    TypeLazyRegistration o;
    QVERIFY(o.property("read").isValid());
    QVERIFY(QMetaType::fromName("CustomReadObject*").isValid());
    QVERIFY(!QMetaType::fromName("CustomWriteObject*").isValid());

    CustomWriteObjectChild data;
    QVariant value = QVariant::fromValue(&data); // this register CustomWriteObjectChild
    // check if base classes are not registered automatically, otherwise this test would be meaningless
    QVERIFY(!QMetaType::fromName("CustomWriteObject*").isValid());
    QVERIFY(o.setProperty("write", value));
    QVERIFY(QMetaType::fromName("CustomWriteObject*").isValid());
    QCOMPARE(o.property("write").value<CustomWriteObjectChild*>(), &data);
}

void tst_QMetaProperty::mapProperty()
{
    map.insert(5, 9);
    QVariant v1 = QVariant::fromValue(map);
    QVariant v = property("map");
    QVERIFY(v.isValid());
    QCOMPARE(map, (v.value<QMap<int,int> >()));
}

void tst_QMetaProperty::conversion()
{
    QMetaType::registerConverter<QString, CustomType>();
    QMetaType::registerConverter<CustomType, QString>();

    QString hello = QStringLiteral("Hello");

    // Write to a QString property using a CustomType in a QVariant
    QMetaProperty value7P = metaObject()->property(metaObject()->indexOfProperty("value7"));
    QVERIFY(value7P.isValid());
    QVERIFY(value7P.write(this, QVariant::fromValue(CustomType(hello))));
    QCOMPARE(value7, hello);

    // Write to a CustomType property using a QString in a QVariant
    QMetaProperty customP = metaObject()->property(metaObject()->indexOfProperty("custom"));
    QVERIFY(customP.isValid());
    QVERIFY(customP.write(this, hello));
    QCOMPARE(custom.str, hello);

    // Something that cannot be converted should fail
    QVERIFY(!customP.write(this, 45));
    QVERIFY(!customP.write(this, QVariant::fromValue(this)));
    QVERIFY(!value7P.write(this, QVariant::fromValue(this)));
    QVERIFY(!value7P.write(this, QVariant::fromValue<QObject*>(this)));

    // none of this should have changed the values
    QCOMPARE(value7, hello);
    QCOMPARE(custom.str, hello);

    // Empty variant should be converted to default object
    QVERIFY(customP.write(this, QVariant()));
    QCOMPARE(custom.str, QString());
    // or reset resetable
    QVERIFY(value7P.write(this, QVariant()));
    QCOMPARE(value7, QLatin1String("reset"));
}

void tst_QMetaProperty::enumsFlags()
{
    // QTBUG-83689, verify that enumerations and flags can be assigned from int,
    // which is important for Qt Widgets Designer.
    EnumFlagsTester t;

    auto mo = t.metaObject();

    const int enumIndex = mo->indexOfProperty("enumProperty");
    QVERIFY(enumIndex >= 0);
    auto enumProperty = mo->property(enumIndex);
    QVERIFY(enumProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(enumProperty.write(&t, QVariant(int(EnumFlagsTester::e2))));
    QCOMPARE(t.enumProperty(), EnumFlagsTester::e2);
    QVERIFY(enumProperty.enumerator().isValid()); // OK: Q_ENUM

    const int flagsIndex = mo->indexOfProperty("flagProperty");
    QVERIFY(flagsIndex >= 0);
    auto flagsProperty = mo->property(flagsIndex);
    QVERIFY(flagsProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(flagsProperty.write(&t, QVariant(int(EnumFlagsTester::flag2))));
    QCOMPARE(t.flagProperty(), EnumFlagsTester::flag2);
    QCOMPARE(flagsProperty.read(&t), QVariant(int(EnumFlagsTester::flag2)));
    QVERIFY(flagsProperty.write(&t, QVariant(uint(EnumFlagsTester::flag1))));
    QCOMPARE(t.flagProperty(), EnumFlagsTester::flag1);
    QCOMPARE(flagsProperty.read(&t), QVariant(int(EnumFlagsTester::flag1)));
    QVERIFY(!flagsProperty.enumerator().isValid()); // Not using Q_FLAG

    const int uflagsIndex = mo->indexOfProperty("unsignedFlagProperty");
    QVERIFY(uflagsIndex >= 0);
    auto uflagsProperty = mo->property(uflagsIndex);
    QVERIFY(uflagsProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(uflagsProperty.write(&t, QVariant(uint(EnumFlagsTester::uflag2))));
    QCOMPARE(t.unsignedFlagProperty(), EnumFlagsTester::uflag2);
    QCOMPARE(uflagsProperty.read(&t), QVariant(uint(EnumFlagsTester::uflag2)));
    QVERIFY(uflagsProperty.write(&t, QVariant(int(EnumFlagsTester::uflag1))));
    QCOMPARE(t.unsignedFlagProperty(), EnumFlagsTester::uflag1);
    QCOMPARE(uflagsProperty.read(&t), QVariant(uint(EnumFlagsTester::flag1)));
    QVERIFY(!uflagsProperty.enumerator().isValid()); // Not using Q_FLAG

    const int flags64Index = mo->indexOfProperty("flag64Property");
    QVERIFY(flags64Index >= 0);
    auto flags64Property = mo->property(flags64Index);
    QVERIFY(flags64Property.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(flags64Property.write(&t, QVariant(qlonglong(EnumFlagsTester::TestFlag64::flag2))));
    QCOMPARE(t.flag64Property(), EnumFlagsTester::TestFlag64::flag2);
    QCOMPARE(flags64Property.read(&t), QVariant(qlonglong(EnumFlagsTester::TestFlag64::flag2)));
    QVERIFY(flags64Property.write(&t, QVariant(uint(EnumFlagsTester::TestFlag64::flag1))));
    QCOMPARE(t.flag64Property(), EnumFlagsTester::TestFlag64::flag1);
    QCOMPARE(flags64Property.read(&t), QVariant(int(EnumFlagsTester::TestFlag64::flag1)));
    QVERIFY(!flags64Property.enumerator().isValid()); // Not using Q_FLAG

    const int sflags64Index = mo->indexOfProperty("storedFlag64Property");
    QVERIFY(sflags64Index >= 0);
    auto sflags64Property = mo->property(sflags64Index);
    QVERIFY(sflags64Property.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(sflags64Property.write(&t, QVariant(uint(EnumFlagsTester::TestFlag64::flag1))));
    QCOMPARE(t.m_flags64, EnumFlagsTester::TestFlag64::flag1);

    QCOMPARE(sflags64Property.read(&t), QVariant(uint(EnumFlagsTester::TestFlag64::flag1)));
    QVERIFY(sflags64Property.write(&t, QVariant(qlonglong(EnumFlagsTester::TestFlag64::flag2))));
    QCOMPARE(t.m_flags64, EnumFlagsTester::TestFlag64::flag2);
    QCOMPARE(sflags64Property.read(&t), QVariant(qulonglong(EnumFlagsTester::TestFlag64::flag2)));
    QVERIFY(!sflags64Property.enumerator().isValid()); // Not using Q_FLAG

    const int uflags64Index = mo->indexOfProperty("unsignedFlag64Property");
    QVERIFY(uflags64Index >= 0);
    auto uflags64Property = mo->property(uflags64Index);
    QVERIFY(uflags64Property.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(uflags64Property.write(&t, QVariant(qlonglong(EnumFlagsTester::UnsignedTestFlag64::uflag2))));
    QCOMPARE(t.unsignedFlag64Property(), EnumFlagsTester::UnsignedTestFlag64::uflag2);
    QCOMPARE(uflags64Property.read(&t), QVariant(qulonglong(EnumFlagsTester::UnsignedTestFlag64::uflag2)));
    QVERIFY(uflags64Property.write(&t, QVariant(int(EnumFlagsTester::UnsignedTestFlag64::uflag1))));
    QCOMPARE(t.unsignedFlag64Property(), EnumFlagsTester::UnsignedTestFlag64::uflag1);
    QCOMPARE(uflags64Property.read(&t), QVariant(uint(EnumFlagsTester::TestFlag64::flag1)));
    QVERIFY(uflags64Property.enumerator().isValid()); // Yes Q_FLAG

    const int freeEnumIndex = mo->indexOfProperty("freeEnumProperty");
    QVERIFY(freeEnumIndex >= 0);
    auto freeEnumProperty = mo->property(freeEnumIndex);
    QVERIFY(freeEnumProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(freeEnumProperty.write(&t, QVariant(FreeEnumValue2)));
    QCOMPARE(t.freeEnumProperty(), FreeEnumValue2);
    QVERIFY(!freeEnumProperty.enumerator().isValid()); // Not using Q_ENUM

    const int namespacedEnumIndex = mo->indexOfProperty("namespacedEnumProperty");
    QVERIFY(namespacedEnumIndex >= 0);
    auto namespacedEnumProperty = mo->property(namespacedEnumIndex);
    QVERIFY(namespacedEnumProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(namespacedEnumProperty.write(&t, QVariant(MySpace::NamespacedEnumValue2)));
    QCOMPARE(t.namespacedEnumProperty(), MySpace::NamespacedEnumValue2);
    QVERIFY(!namespacedEnumProperty.enumerator().isValid()); // Not using Q_NAMESPACE/Q_ENUM_NS

    const int qtNamespacedEnumIndex = mo->indexOfProperty("qtNamespacedEnumProperty");
    QVERIFY(qtNamespacedEnumIndex >= 0);
    auto qtNamespacedEnumProperty = mo->property(qtNamespacedEnumIndex);
    QVERIFY(qtNamespacedEnumProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(qtNamespacedEnumProperty.write(&t, QVariant(MyQtSpace::NamespacedEnumValue2)));
    QCOMPARE(t.qtNamespacedEnumProperty(), MyQtSpace::NamespacedEnumValue2);
    QVERIFY(qtNamespacedEnumProperty.enumerator().isValid()); // OK: Q_ENUM_NS

    const int qtNamespacedFlagIndex = mo->indexOfProperty("qtNamespacedFlagProperty");
    QVERIFY(qtNamespacedFlagIndex >= 0);
    auto qtNamespacedFlagProperty = mo->property(qtNamespacedFlagIndex);
    QVERIFY(qtNamespacedFlagProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(qtNamespacedFlagProperty.write(&t, QVariant(MyQtSpace::NamespacedFlags(MyQtSpace::NamespacedEnumValue2))));
    QCOMPARE(t.qtNamespacedFlagProperty(), MyQtSpace::NamespacedFlags(MyQtSpace::NamespacedEnumValue2));
    QVERIFY(qtNamespacedFlagProperty.enumerator().isValid()); // OK: Q_FLAG

    const int sepEnumIndex = mo->indexOfProperty("sepEnum");
    QVERIFY(sepEnumIndex >= 0);
    auto sepEnumProperty = mo->property(sepEnumIndex);
    QVERIFY(sepEnumProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(sepEnumProperty.write(&t, QVariant(SeparateEnumNamespace::Value2)));
    QCOMPARE(t.sepEnum(), SeparateEnumNamespace::Value2);
    QVERIFY(sepEnumProperty.enumerator().isValid()); // OK: Q_ENUM_NS

    const int sepFlagsIndex = mo->indexOfProperty("sepFlags");
    QVERIFY(sepFlagsIndex >= 0);
    auto sepFlagsProperty = mo->property(sepFlagsIndex);
    QVERIFY(sepFlagsProperty.metaType().flags().testFlag(QMetaType::IsEnumeration));
    QVERIFY(sepFlagsProperty.write(&t, QVariant(SeparateEnumNamespace::Value1)));
    QCOMPARE(t.sepFlags(), SeparateEnumNamespace::Value1);
    QVERIFY(!sepFlagsProperty.enumerator().isValid()); // NOK: the meta object is empty
}


void tst_QMetaProperty::notifySignalIndex()
{
    auto mo = this->metaObject();
    auto propIndex = mo->indexOfProperty("propWithInheritedSig");
    auto propWithInheritedSig = mo->property(propIndex);
    QVERIFY(propWithInheritedSig.isValid());
    QVERIFY(propWithInheritedSig.hasNotifySignal());
    QVERIFY(propWithInheritedSig.notifySignalIndex() != -1);
    QMetaMethod notifySignal = propWithInheritedSig.notifySignal();
    QVERIFY(notifySignal.isValid());
    QCOMPARE(notifySignal.name(), "baseSignal");
    QCOMPARE(notifySignal.parameterCount(), 1);
}

QTEST_MAIN(tst_QMetaProperty)
#include "tst_qmetaproperty.moc"
