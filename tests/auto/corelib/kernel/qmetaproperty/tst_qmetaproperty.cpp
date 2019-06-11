/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
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


#include <QtTest/QtTest>

#include <qobject.h>
#include <qmetaobject.h>

struct CustomType
{
    int padding;
    QString str;
    CustomType(const QString &str = QString()) : str(str) {}
    operator QString() const { return str; }
    friend bool operator!=(const CustomType &a, const CustomType &b)
    { return a.str != b.str; }
};

Q_DECLARE_METATYPE(CustomType)

class tst_QMetaProperty : public QObject
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

private slots:
    void hasStdCppSet();
    void isConstant();
    void isFinal();
    void gadget();
    void readAndWriteWithLazyRegistration();
    void mapProperty();
    void conversion();

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

    QString value7;
    QMap<int, int> map;
    CustomType custom;
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

void tst_QMetaProperty::readAndWriteWithLazyRegistration()
{
    QCOMPARE(QMetaType::type("CustomReadObject*"), int(QMetaType::UnknownType));
    QCOMPARE(QMetaType::type("CustomWriteObject*"), int(QMetaType::UnknownType));

    TypeLazyRegistration o;
    QVERIFY(o.property("read").isValid());
    QVERIFY(QMetaType::type("CustomReadObject*") != QMetaType::UnknownType);
    QCOMPARE(QMetaType::type("CustomWriteObject*"), int(QMetaType::UnknownType));

    CustomWriteObjectChild data;
    QVariant value = QVariant::fromValue(&data); // this register CustomWriteObjectChild
    // check if base classes are not registered automatically, otherwise this test would be meaningless
    QCOMPARE(QMetaType::type("CustomWriteObject*"), int(QMetaType::UnknownType));
    QVERIFY(o.setProperty("write", value));
    QVERIFY(QMetaType::type("CustomWriteObject*") != QMetaType::UnknownType);
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

QTEST_MAIN(tst_QMetaProperty)
#include "tst_qmetaproperty.moc"
