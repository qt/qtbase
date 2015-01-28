/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 Olivier Goffart <ogoffart@woboq.com>
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

#include <qobject.h>
#include <qmetaobject.h>

class tst_QMetaProperty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(EnumType value WRITE setValue READ getValue)
    Q_PROPERTY(EnumType value2 WRITE set_value READ get_value)
    Q_PROPERTY(int value8 READ value8)
    Q_PROPERTY(int value9 READ value9 CONSTANT)
    Q_PROPERTY(int value10 READ value10 FINAL)

private slots:
    void hasStdCppSet();
    void isConstant();
    void isFinal();
    void gadget();
    void readAndWriteWithLazyRegistration();

public:
    enum EnumType { EnumType1 };

    void setValue(EnumType) {}
    EnumType getValue() const { return EnumType1; }
    void set_value(EnumType) {}
    EnumType get_value() const { return EnumType1; }

    int value8() const { return 1; }
    int value9() const { return 1; }
    int value10() const { return 1; }
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
    void resetValue() { m_value = QLatin1Literal("reset"); }
};

void tst_QMetaProperty::gadget()
{
    const QMetaObject *mo = &MyGadget::staticMetaObject;
    QMetaProperty valueProp = mo->property(mo->indexOfProperty("value"));
    QVERIFY(valueProp.isValid());
    {
        MyGadget g;
        QString hello = QLatin1Literal("hello");
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


QTEST_MAIN(tst_QMetaProperty)
#include "tst_qmetaproperty.moc"
