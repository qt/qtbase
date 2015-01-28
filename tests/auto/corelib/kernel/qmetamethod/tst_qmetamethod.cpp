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

class tst_QMetaMethod : public QObject
{
    Q_OBJECT

private slots:
    void method_data();
    void method();

    void invalidMethod();

    void comparisonOperators();

    void fromSignal();

    void gadget();
};

struct CustomType { };
Q_DECLARE_METATYPE(CustomType)

struct CustomUnregisteredType { };

Q_DECLARE_METATYPE(QMetaMethod::Access)
Q_DECLARE_METATYPE(QMetaMethod::MethodType)

class MethodTestObject : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE MethodTestObject();
    Q_INVOKABLE MethodTestObject(int constructorIntArg);
    Q_INVOKABLE MethodTestObject(qreal constructorQRealArg);
    Q_INVOKABLE MethodTestObject(const QString &constructorQStringArg);
    Q_INVOKABLE MethodTestObject(CustomType constructorCustomTypeArg);
    Q_INVOKABLE MethodTestObject(CustomUnregisteredType constructorCustomUnregisteredTypeArg);
    Q_INVOKABLE MethodTestObject(bool boolArg, int intArg, uint uintArg,
                                 qlonglong longlongArg, qulonglong ulonglongArg,
                                 double doubleArg, long longArg, short shortArg,
                                 char charArg, ulong ulongArg, ushort ushortArg,
                                 uchar ucharArg, float floatArg);
    Q_INVOKABLE MethodTestObject(bool, int);

    Q_INVOKABLE void voidInvokable();
    Q_INVOKABLE void voidInvokableInt(int voidInvokableIntArg);
    Q_INVOKABLE void voidInvokableQReal(qreal voidInvokableQRealArg);
    Q_INVOKABLE void voidInvokableQString(const QString &voidInvokableQStringArg);
    Q_INVOKABLE void voidInvokableCustomType(CustomType voidInvokableCustomTypeArg);
    Q_INVOKABLE void voidInvokableCustomUnregisteredType(CustomUnregisteredType voidInvokableCustomUnregisteredTypeArg);
    Q_INVOKABLE bool boolInvokable();
    Q_INVOKABLE qreal qrealInvokable();
    Q_INVOKABLE QString qstringInvokable();
    Q_INVOKABLE CustomType customTypeInvokable();
    Q_INVOKABLE CustomUnregisteredType customUnregisteredTypeInvokable();
    Q_INVOKABLE QVariant qvariantInvokableBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat(
        bool boolArg, int intArg, uint uintArg, qlonglong longlongArg, qulonglong ulonglongArg, double doubleArg,
        long longArg, short shortArg, char charArg, ulong ulongArg, ushort ushortArg, uchar ucharArg, float floatArg);
    Q_INVOKABLE void voidInvokableNoParameterNames(bool, int);
public slots:
    void voidSlot();
    void voidSlotInt(int voidSlotIntArg);
    void voidSlotQReal(qreal voidSlotQRealArg);
    void voidSlotQString(const QString &voidSlotQStringArg);
    void voidSlotCustomType(CustomType voidSlotCustomTypeArg);
    void voidSlotCustomUnregisteredType(CustomUnregisteredType voidSlotCustomUnregisteredTypeArg);
    bool boolSlot();
    qreal qrealSlot();
    QString qstringSlot();
    CustomType customTypeSlot();
    CustomUnregisteredType customUnregisteredTypeSlot();
    QVariant qvariantSlotBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat(
        bool boolArg, int intArg, uint uintArg, qlonglong longlongArg, qulonglong ulonglongArg, double doubleArg,
        long longArg, short shortArg, char charArg, ulong ulongArg, ushort ushortArg, uchar ucharArg, float floatArg);
    void voidSlotNoParameterNames(bool, int);
signals:
    void voidSignal();
    void voidSignalVoid(void);
    void voidSignalInt(int voidSignalIntArg);
    void voidSignalQReal(qreal voidSignalQRealArg);
    void voidSignalQString(const QString &voidSignalQStringArg);
    void voidSignalCustomType(CustomType voidSignalCustomTypeArg);
    void voidSignalCustomUnregisteredType(CustomUnregisteredType voidSignalCustomUnregisteredTypeArg);
    bool boolSignal();
    qreal qrealSignal();
    QString qstringSignal();
    CustomType customTypeSignal();
    CustomUnregisteredType customUnregisteredTypeSignal();
    QVariant qvariantSignalBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat(
        bool boolArg, int intArg, uint uintArg, qlonglong longlongArg, qulonglong ulonglongArg, double doubleArg,
        long longArg, short shortArg, char charArg, ulong ulongArg, ushort ushortArg, uchar ucharArg, float floatArg);
    void voidSignalNoParameterNames(bool, int);
};

MethodTestObject::MethodTestObject() {}
MethodTestObject::MethodTestObject(int) {}
MethodTestObject::MethodTestObject(qreal) {}
MethodTestObject::MethodTestObject(const QString &) {}
MethodTestObject::MethodTestObject(CustomType) {}
MethodTestObject::MethodTestObject(CustomUnregisteredType) {}
MethodTestObject::MethodTestObject(bool, int, uint, qlonglong, qulonglong,
                                   double, long, short, char, ulong, ushort,
                                   uchar, float) {}
MethodTestObject::MethodTestObject(bool, int) {}

void MethodTestObject::voidInvokable() {}
void MethodTestObject::voidInvokableInt(int) {}
void MethodTestObject::voidInvokableQReal(qreal) {}
void MethodTestObject::voidInvokableQString(const QString &) {}
void MethodTestObject::voidInvokableCustomType(CustomType) {}
void MethodTestObject::voidInvokableCustomUnregisteredType(CustomUnregisteredType) {}
bool MethodTestObject::boolInvokable() { return true; }
qreal MethodTestObject::qrealInvokable() { return 1.0; }
QString MethodTestObject::qstringInvokable() { return QString(); }
CustomType MethodTestObject::customTypeInvokable() { return CustomType(); }
CustomUnregisteredType MethodTestObject::customUnregisteredTypeInvokable()
{
    return CustomUnregisteredType();
}
QVariant MethodTestObject::qvariantInvokableBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat(
    bool, int, uint, qlonglong, qulonglong, double, long, short, char, ulong, ushort, uchar, float)
{
    return QVariant();
}
void MethodTestObject::voidInvokableNoParameterNames(bool, int) {}

void MethodTestObject::voidSlot() {}
void MethodTestObject::voidSlotInt(int) {}
void MethodTestObject::voidSlotQReal(qreal) {}
void MethodTestObject::voidSlotQString(const QString &) {}
void MethodTestObject::voidSlotCustomType(CustomType) {}
void MethodTestObject::voidSlotCustomUnregisteredType(CustomUnregisteredType) {}
bool MethodTestObject::boolSlot() { return true; }
qreal MethodTestObject::qrealSlot() { return 1.0; }
QString MethodTestObject::qstringSlot() { return QString(); }
CustomType MethodTestObject::customTypeSlot() { return CustomType(); }
CustomUnregisteredType MethodTestObject::customUnregisteredTypeSlot()
{
    return CustomUnregisteredType();
}
QVariant MethodTestObject::qvariantSlotBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat(
    bool, int, uint, qlonglong, qulonglong, double, long, short, char, ulong, ushort, uchar, float)
{
    return QVariant();
}
void MethodTestObject::voidSlotNoParameterNames(bool, int) {}

void tst_QMetaMethod::method_data()
{
    QTest::addColumn<QByteArray>("signature");
    QTest::addColumn<int>("returnType");
    QTest::addColumn<QByteArray>("returnTypeName");
    QTest::addColumn<QList<int> >("parameterTypes");
    QTest::addColumn<QList<QByteArray> >("parameterTypeNames");
    QTest::addColumn<QList<QByteArray> >("parameterNames");
    QTest::addColumn<QMetaMethod::Access>("access");
    QTest::addColumn<QMetaMethod::MethodType>("methodType");

    QTest::newRow("voidSignal")
            << QByteArray("voidSignal()")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidInvokable")
            << QByteArray("voidInvokable()")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("voidSlot")
            << QByteArray("voidSlot()")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("MethodTestObject()")
            << QByteArray("MethodTestObject()")
            << int(QMetaType::UnknownType) << QByteArray("")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Constructor;

    QTest::newRow("voidSignalVoid")
            << QByteArray("voidSignalVoid()")
            << int(QMetaType::Void) << QByteArray("void")
            << QList<int>()
            << QList<QByteArray>()
            << QList<QByteArray>()
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidSignalInt")
            << QByteArray("voidSignalInt(int)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("voidSignalIntArg"))
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidInvokableInt")
            << QByteArray("voidInvokableInt(int)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("voidInvokableIntArg"))
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("voidSlotInt")
            << QByteArray("voidSlotInt(int)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("voidSlotIntArg"))
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("MethodTestObject(int)")
            << QByteArray("MethodTestObject(int)")
            << int(QMetaType::UnknownType) << QByteArray("")
            << (QList<int>() << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("constructorIntArg"))
            << QMetaMethod::Public
            << QMetaMethod::Constructor;

    QTest::newRow("voidSignalQReal")
            << QByteArray("voidSignalQReal(qreal)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << qMetaTypeId<qreal>())
            << (QList<QByteArray>() << QByteArray("qreal"))
            << (QList<QByteArray>() << QByteArray("voidSignalQRealArg"))
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidInvokableQReal")
            << QByteArray("voidInvokableQReal(qreal)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << qMetaTypeId<qreal>())
            << (QList<QByteArray>() << QByteArray("qreal"))
            << (QList<QByteArray>() << QByteArray("voidInvokableQRealArg"))
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("voidSlotQReal")
            << QByteArray("voidSlotQReal(qreal)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << qMetaTypeId<qreal>())
            << (QList<QByteArray>() << QByteArray("qreal"))
            << (QList<QByteArray>() << QByteArray("voidSlotQRealArg"))
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("MethodTestObject(qreal)")
            << QByteArray("MethodTestObject(qreal)")
            << int(QMetaType::UnknownType) << QByteArray("")
            << (QList<int>() << qMetaTypeId<qreal>())
            << (QList<QByteArray>() << QByteArray("qreal"))
            << (QList<QByteArray>() << QByteArray("constructorQRealArg"))
            << QMetaMethod::Public
            << QMetaMethod::Constructor;

    QTest::newRow("voidSignalQString")
            << QByteArray("voidSignalQString(QString)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::QString))
            << (QList<QByteArray>() << QByteArray("QString"))
            << (QList<QByteArray>() << QByteArray("voidSignalQStringArg"))
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidInvokableQString")
            << QByteArray("voidInvokableQString(QString)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::QString))
            << (QList<QByteArray>() << QByteArray("QString"))
            << (QList<QByteArray>() << QByteArray("voidInvokableQStringArg"))
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("voidSlotQString")
            << QByteArray("voidSlotQString(QString)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::QString))
            << (QList<QByteArray>() << QByteArray("QString"))
            << (QList<QByteArray>() << QByteArray("voidSlotQStringArg"))
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("MethodTestObject(QString)")
            << QByteArray("MethodTestObject(QString)")
            << int(QMetaType::UnknownType) << QByteArray("")
            << (QList<int>() << int(QMetaType::QString))
            << (QList<QByteArray>() << QByteArray("QString"))
            << (QList<QByteArray>() << QByteArray("constructorQStringArg"))
            << QMetaMethod::Public
            << QMetaMethod::Constructor;

    QTest::newRow("voidSignalCustomType")
            << QByteArray("voidSignalCustomType(CustomType)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << qMetaTypeId<CustomType>())
            << (QList<QByteArray>() << QByteArray("CustomType"))
            << (QList<QByteArray>() << QByteArray("voidSignalCustomTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidInvokableCustomType")
            << QByteArray("voidInvokableCustomType(CustomType)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << qMetaTypeId<CustomType>())
            << (QList<QByteArray>() << QByteArray("CustomType"))
            << (QList<QByteArray>() << QByteArray("voidInvokableCustomTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("voidSlotCustomType")
            << QByteArray("voidSlotCustomType(CustomType)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << qMetaTypeId<CustomType>())
            << (QList<QByteArray>() << QByteArray("CustomType"))
            << (QList<QByteArray>() << QByteArray("voidSlotCustomTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("MethodTestObject(CustomType)")
            << QByteArray("MethodTestObject(CustomType)")
            << int(QMetaType::UnknownType) << QByteArray("")
            << (QList<int>() << qMetaTypeId<CustomType>())
            << (QList<QByteArray>() << QByteArray("CustomType"))
            << (QList<QByteArray>() << QByteArray("constructorCustomTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Constructor;

    QTest::newRow("voidSignalCustomUnregisteredType")
            << QByteArray("voidSignalCustomUnregisteredType(CustomUnregisteredType)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << 0)
            << (QList<QByteArray>() << QByteArray("CustomUnregisteredType"))
            << (QList<QByteArray>() << QByteArray("voidSignalCustomUnregisteredTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidInvokableCustomUnregisteredType")
            << QByteArray("voidInvokableCustomUnregisteredType(CustomUnregisteredType)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << 0)
            << (QList<QByteArray>() << QByteArray("CustomUnregisteredType"))
            << (QList<QByteArray>() << QByteArray("voidInvokableCustomUnregisteredTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("voidSlotCustomUnregisteredType")
            << QByteArray("voidSlotCustomUnregisteredType(CustomUnregisteredType)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << 0)
            << (QList<QByteArray>() << QByteArray("CustomUnregisteredType"))
            << (QList<QByteArray>() << QByteArray("voidSlotCustomUnregisteredTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("MethodTestObject(CustomUnregisteredType)")
            << QByteArray("MethodTestObject(CustomUnregisteredType)")
            << int(QMetaType::UnknownType) << QByteArray("")
            << (QList<int>() << 0)
            << (QList<QByteArray>() << QByteArray("CustomUnregisteredType"))
            << (QList<QByteArray>() << QByteArray("constructorCustomUnregisteredTypeArg"))
            << QMetaMethod::Public
            << QMetaMethod::Constructor;

    QTest::newRow("boolSignal")
            << QByteArray("boolSignal()")
            << int(QMetaType::Bool) << QByteArray("bool")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("boolInvokable")
            << QByteArray("boolInvokable()")
            << int(QMetaType::Bool) << QByteArray("bool")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("boolSlot")
            << QByteArray("boolSlot()")
            << int(QMetaType::Bool) << QByteArray("bool")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("qrealSignal")
            << QByteArray("qrealSignal()")
            << int(QMetaType::QReal) << QByteArray("qreal")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("qrealInvokable")
            << QByteArray("qrealInvokable()")
            << int(QMetaType::QReal) << QByteArray("qreal")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("qrealSlot")
            << QByteArray("qrealSlot()")
            << int(QMetaType::QReal) << QByteArray("qreal")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("qstringSignal")
            << QByteArray("qstringSignal()")
            << int(QMetaType::QString) << QByteArray("QString")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("qstringInvokable")
            << QByteArray("qstringInvokable()")
            << int(QMetaType::QString) << QByteArray("QString")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("qstringSlot")
            << QByteArray("qstringSlot()")
            << int(QMetaType::QString) << QByteArray("QString")
            << (QList<int>())
            << (QList<QByteArray>())
            << (QList<QByteArray>())
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    {
        QList<int> parameterTypes = QList<int>()
                << int(QMetaType::Bool) << int(QMetaType::Int) << int(QMetaType::UInt)
                << int(QMetaType::LongLong) << int(QMetaType::ULongLong) << int(QMetaType::Double)
                << int(QMetaType::Long) << int(QMetaType::Short) << int(QMetaType::Char)
                << int(QMetaType::ULong) << int(QMetaType::UShort) << int(QMetaType::UChar)
                << int(QMetaType::Float);
        QList<QByteArray> parameterTypeNames = QList<QByteArray>()
                << QByteArray("bool") << QByteArray("int") << QByteArray("uint")
                << QByteArray("qlonglong") << QByteArray("qulonglong") << QByteArray("double")
                << QByteArray("long") << QByteArray("short") << QByteArray("char") << QByteArray("ulong")
                << QByteArray("ushort") << QByteArray("uchar") << QByteArray("float");
        QList<QByteArray> parameterNames = QList<QByteArray>()
                << QByteArray("boolArg") << QByteArray("intArg") << QByteArray("uintArg")
                << QByteArray("longlongArg") << QByteArray("ulonglongArg") << QByteArray("doubleArg")
                << QByteArray("longArg") << QByteArray("shortArg") << QByteArray("charArg")
                << QByteArray("ulongArg") << QByteArray("ushortArg") << QByteArray("ucharArg")
                << QByteArray("floatArg");

        QTest::newRow("qvariantSignalBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat")
                << QByteArray("qvariantSignalBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat("
                              "bool,int,uint,qlonglong,qulonglong,double,long,short,char,ulong,ushort,uchar,float)")
                << int(QMetaType::QVariant) << QByteArray("QVariant")
                << parameterTypes << parameterTypeNames << parameterNames
                << QMetaMethod::Public
                << QMetaMethod::Signal;

        QTest::newRow("qvariantInvokableBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat")
                << QByteArray("qvariantInvokableBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat("
                              "bool,int,uint,qlonglong,qulonglong,double,long,short,char,ulong,ushort,uchar,float)")
                << int(QMetaType::QVariant) << QByteArray("QVariant")
                << parameterTypes << parameterTypeNames << parameterNames
                << QMetaMethod::Public
                << QMetaMethod::Method;

        QTest::newRow("qvariantSlotBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat")
                << QByteArray("qvariantSlotBoolIntUIntLonglongULonglongDoubleLongShortCharUlongUshortUcharFloat("
                              "bool,int,uint,qlonglong,qulonglong,double,long,short,char,ulong,ushort,uchar,float)")
                << int(QMetaType::QVariant) << QByteArray("QVariant")
                << parameterTypes << parameterTypeNames << parameterNames
                << QMetaMethod::Public
                << QMetaMethod::Slot;

        QTest::newRow("MethodTestObject(bool,int,uint,qlonglong,qulonglong,double,long,short,char,ulong,ushort,uchar,float)")
                << QByteArray("MethodTestObject(bool,int,uint,qlonglong,qulonglong,double,long,short,char,ulong,ushort,uchar,float)")
                << int(QMetaType::UnknownType) << QByteArray("")
                << parameterTypes << parameterTypeNames << parameterNames
                << QMetaMethod::Public
                << QMetaMethod::Constructor;
    }

    QTest::newRow("voidSignalNoParameterNames")
            << QByteArray("voidSignalNoParameterNames(bool,int)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::Bool) << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("bool") << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("") << QByteArray(""))
            << QMetaMethod::Public
            << QMetaMethod::Signal;

    QTest::newRow("voidInvokableNoParameterNames")
            << QByteArray("voidInvokableNoParameterNames(bool,int)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::Bool) << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("bool") << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("") << QByteArray(""))
            << QMetaMethod::Public
            << QMetaMethod::Method;

    QTest::newRow("voidSlotNoParameterNames")
            << QByteArray("voidSlotNoParameterNames(bool,int)")
            << int(QMetaType::Void) << QByteArray("void")
            << (QList<int>() << int(QMetaType::Bool) << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("bool") << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("") << QByteArray(""))
            << QMetaMethod::Public
            << QMetaMethod::Slot;

    QTest::newRow("MethodTestObject(bool,int)")
            << QByteArray("MethodTestObject(bool,int)")
            << int(QMetaType::UnknownType) << QByteArray("")
            << (QList<int>() << int(QMetaType::Bool) << int(QMetaType::Int))
            << (QList<QByteArray>() << QByteArray("bool") << QByteArray("int"))
            << (QList<QByteArray>() << QByteArray("") << QByteArray(""))
            << QMetaMethod::Public
            << QMetaMethod::Constructor;
}

void tst_QMetaMethod::method()
{
    QFETCH(QByteArray, signature);
    QFETCH(int, returnType);
    QFETCH(QByteArray, returnTypeName);
    QFETCH(QList<int>, parameterTypes);
    QFETCH(QList<QByteArray>, parameterTypeNames);
    QFETCH(QList<QByteArray>, parameterNames);
    QFETCH(QMetaMethod::MethodType, methodType);
    QFETCH(QMetaMethod::Access, access);

    QVERIFY(parameterTypes.size() == parameterTypeNames.size());
    QVERIFY(parameterTypes.size() == parameterNames.size());

    const QMetaObject *mo = &MethodTestObject::staticMetaObject;
    int index = (methodType == QMetaMethod::Constructor)
                ? mo->indexOfConstructor(signature) : mo->indexOfMethod(signature);
    QVERIFY(index != -1);
    QMetaMethod method = (methodType == QMetaMethod::Constructor)
                         ? mo->constructor(index) : mo->method(index);
    QVERIFY(method.isValid());
    QCOMPARE(method.methodType(), methodType);
    QCOMPARE(method.access(), access);

    QVERIFY(!method.methodSignature().isEmpty());
    if (method.methodSignature() != signature) {
        // QMetaMethod should always produce a semantically equivalent signature
        int signatureIndex = (methodType == QMetaMethod::Constructor)
                ? mo->indexOfConstructor(method.methodSignature())
                : mo->indexOfMethod(method.methodSignature());
        QCOMPARE(signatureIndex, index);
    }

    QByteArray computedName = signature.left(signature.indexOf('('));
    QCOMPARE(method.name(), computedName);

    QCOMPARE(method.tag(), "");
    QCOMPARE(method.returnType(), returnType);
    QVERIFY(method.typeName() != 0);
    if (QByteArray(method.typeName()) != returnTypeName) {
        // QMetaMethod should always produce a semantically equivalent typename
        QCOMPARE(QMetaType::type(method.typeName()), QMetaType::type(returnTypeName));
    }

    if (method.parameterTypes() != parameterTypeNames) {
        // QMetaMethod should always produce semantically equivalent typenames
        QList<QByteArray> actualTypeNames = method.parameterTypes();
        QCOMPARE(actualTypeNames.size(), parameterTypeNames.size());
        for (int i = 0; i < parameterTypeNames.size(); ++i) {
            QCOMPARE(QMetaType::type(actualTypeNames.at(i)),
                     QMetaType::type(parameterTypeNames.at(i)));
        }
    }
    QCOMPARE(method.parameterNames(), parameterNames);

    QCOMPARE(method.parameterCount(), parameterTypes.size());
    for (int i = 0; i < parameterTypes.size(); ++i)
        QCOMPARE(method.parameterType(i), parameterTypes.at(i));

    {
        QVector<int> actualParameterTypes(parameterTypes.size());
        method.getParameterTypes(actualParameterTypes.data());
        for (int i = 0; i < parameterTypes.size(); ++i)
            QCOMPARE(actualParameterTypes.at(i), parameterTypes.at(i));
    }

    // Bogus indexes
    QCOMPARE(method.parameterType(-1), 0);
    QCOMPARE(method.parameterType(parameterTypes.size()), 0);
}

void tst_QMetaMethod::invalidMethod()
{
    QMetaMethod method;
    QVERIFY(!method.isValid());

    QMetaMethod method2 = staticMetaObject.method(staticMetaObject.methodCount());
    QVERIFY(!method2.isValid());

    QMetaMethod method3 = staticMetaObject.method(-1);
    QVERIFY(!method3.isValid());
}

void tst_QMetaMethod::comparisonOperators()
{
    static const QMetaObject *mo = &MethodTestObject::staticMetaObject;
    for (int x = 0; x < 2; ++x) {
        int count = x ? mo->constructorCount() : mo->methodCount();
        for (int i = 0; i < count; ++i) {
            QMetaMethod method = x ? mo->constructor(i) : mo->method(i);
            const QMetaObject *methodMo = method.enclosingMetaObject();
            for (int j = 0; j < count; ++j) {
                QMetaMethod other = x ? mo->constructor(j) : mo->method(j);
                bool expectedEqual = ((methodMo == other.enclosingMetaObject())
                                      && (i == j));
                QCOMPARE(method == other, expectedEqual);
                QCOMPARE(method != other, !expectedEqual);
                QCOMPARE(other == method, expectedEqual);
                QCOMPARE(other != method, !expectedEqual);
            }

            QVERIFY(method != QMetaMethod());
            QVERIFY(QMetaMethod() != method);
            QVERIFY(!(method == QMetaMethod()));
            QVERIFY(!(QMetaMethod() == method));
        }
    }

    // Constructors and normal methods with identical index should not
    // compare equal
    for (int i = 0; i < qMin(mo->methodCount(), mo->constructorCount()); ++i) {
        QMetaMethod method = mo->method(i);
        QMetaMethod constructor = mo->constructor(i);
        QVERIFY(method != constructor);
        QVERIFY(!(method == constructor));
    }
}

void tst_QMetaMethod::fromSignal()
{
#define FROMSIGNAL_HELPER(ObjectType, Name, Arguments)  { \
        const QMetaObject *signalMeta = &ObjectType::staticMetaObject; \
        QCOMPARE(QMetaMethod::fromSignal(&ObjectType::Name), \
            signalMeta->method(signalMeta->indexOfSignal(QMetaObject::normalizedSignature(#Name #Arguments)))); \
    }

    FROMSIGNAL_HELPER(MethodTestObject, voidSignal, ())
    FROMSIGNAL_HELPER(MethodTestObject, voidSignalQString, (const QString&))
    FROMSIGNAL_HELPER(QObject, destroyed, (QObject*))
    FROMSIGNAL_HELPER(QObject, objectNameChanged, (const QString &))

    // Inherited from QObject
    FROMSIGNAL_HELPER(MethodTestObject, destroyed, (QObject*))
    FROMSIGNAL_HELPER(MethodTestObject, objectNameChanged, (const QString &))

    // Methods that are not signals; fromSignal should return invalid method
    FROMSIGNAL_HELPER(MethodTestObject, voidSlot, ())
    FROMSIGNAL_HELPER(QObject, deleteLater, ())

#undef FROMSIGNAL_HELPER
}

class MyGadget {
    Q_GADGET
public:
    QString m_value;
    Q_INVOKABLE void setValue(const QString &value) { m_value = value; }
    Q_INVOKABLE QString getValue() { return m_value; }
};

void tst_QMetaMethod::gadget()
{
    int idx;

    idx = MyGadget::staticMetaObject.indexOfMethod("setValue(QString)");
    QVERIFY(idx >= 0);
    QMetaMethod setValueMethod = MyGadget::staticMetaObject.method(idx);
    QVERIFY(setValueMethod.isValid());

    idx = MyGadget::staticMetaObject.indexOfMethod("getValue()");
    QVERIFY(idx >= 0);
    QMetaMethod getValueMethod = MyGadget::staticMetaObject.method(idx);
    QVERIFY(getValueMethod.isValid());

    {
        MyGadget gadget;
        QString string;

        QVERIFY(getValueMethod.invokeOnGadget(&gadget, Q_RETURN_ARG(QString, string)));
        QCOMPARE(string, gadget.m_value);

        QVERIFY(setValueMethod.invokeOnGadget(&gadget, Q_ARG(QString, QLatin1String("hello"))));
        QCOMPARE(gadget.m_value, QLatin1String("hello"));

        QVERIFY(getValueMethod.invokeOnGadget(&gadget, Q_RETURN_ARG(QString, string)));
        QCOMPARE(string, gadget.m_value);
    }

    {
        // Call with null should not crash
        MyGadget *gadget = Q_NULLPTR;
        QString string;
        QVERIFY(!setValueMethod.invokeOnGadget(gadget, Q_ARG(QString, QLatin1String("hi"))));
        QVERIFY(!getValueMethod.invokeOnGadget(gadget, Q_RETURN_ARG(QString, string)));
    }
}


QTEST_MAIN(tst_QMetaMethod)
#include "tst_qmetamethod.moc"
