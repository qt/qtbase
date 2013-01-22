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
#include <qcoreapplication.h>
#include <qmetatype.h>
#include <QtTest/QtTest>

#include <QtDBus/QtDBus>

class tst_QDBusMetaType: public QObject
{
    Q_OBJECT
public:
    int intStringMap;
    int stringStringMap;
    int stringStruct1Map;

private slots:
    void initTestCase();
    void staticTypes_data();
    void staticTypes();
    void dynamicTypes_data();
    void dynamicTypes();
    void invalidTypes_data();
    void invalidTypes();
};

typedef QPair<QString,QString> StringPair;

struct Struct1 { };             // (s)
struct Struct2 { };             // (sos)
struct Struct3 { };             // (sas)
struct Struct4                  // (ssa(ss)sayasx)
{
    QString m1;
    QString m2;
    QList<StringPair> m3;
    QString m4;
    QByteArray m5;
    QStringList m6;
    qlonglong m7;
};

struct Invalid0 { };            // empty
struct Invalid1 { };            // s
struct Invalid2 { };            // o
struct Invalid3 { };            // as
struct Invalid4 { };            // ay
struct Invalid5 { };            // ii
struct Invalid6 { };            // <invalid>
struct Invalid7 { };            // (<invalid>)

Q_DECLARE_METATYPE(Struct1)
Q_DECLARE_METATYPE(Struct2)
Q_DECLARE_METATYPE(Struct3)
Q_DECLARE_METATYPE(Struct4)
Q_DECLARE_METATYPE(StringPair)

Q_DECLARE_METATYPE(Invalid0)
Q_DECLARE_METATYPE(Invalid1)
Q_DECLARE_METATYPE(Invalid2)
Q_DECLARE_METATYPE(Invalid3)
Q_DECLARE_METATYPE(Invalid4)
Q_DECLARE_METATYPE(Invalid5)
Q_DECLARE_METATYPE(Invalid6)
Q_DECLARE_METATYPE(Invalid7)

typedef QMap<int, QString> IntStringMap;
typedef QMap<QString, QString> StringStringMap;
typedef QMap<QString, Struct1> StringStruct1Map;

Q_DECLARE_METATYPE(QVariant::Type)

QT_BEGIN_NAMESPACE
QDBusArgument &operator<<(QDBusArgument &arg, const Struct1 &)
{
    arg.beginStructure();
    arg << QString();
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Struct2 &)
{
    arg.beginStructure();
    arg << QString() << QDBusObjectPath() << QString();
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Struct3 &)
{
    arg.beginStructure();
    arg << QString() << QStringList();
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const StringPair &s)
{
    arg.beginStructure();
    arg << s.first << s.second;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Struct4 &s)
{
    arg.beginStructure();
    arg << s.m1 << s.m2 << s.m3 << s.m4 << s.m5 << s.m6 << s.m7;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Invalid0 &)
{
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Invalid1 &)
{
    arg << QString();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Invalid2 &)
{
    arg << QDBusObjectPath();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Invalid3 &)
{
    arg << QStringList();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Invalid4 &)
{
    arg << QByteArray();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Invalid5 &)
{
    arg << 1 << 2;
    return arg;
}

// no Invalid6

QDBusArgument &operator<<(QDBusArgument &arg, const Invalid7 &)
{
    arg.beginStructure();
    arg << Invalid0();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Struct1 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Struct2 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Struct3 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Struct4 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, StringPair &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid0 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid1 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid2 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid3 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid4 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid5 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid6 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Invalid7 &)
{ return arg; }
QT_END_NAMESPACE

void tst_QDBusMetaType::initTestCase()
{
    qDBusRegisterMetaType<Struct1>();
    qDBusRegisterMetaType<Struct2>();
    qDBusRegisterMetaType<Struct3>();
    qDBusRegisterMetaType<Struct4>();
    qDBusRegisterMetaType<StringPair>();

    qDBusRegisterMetaType<QList<Struct1> >();
    qDBusRegisterMetaType<QList<Struct2> >();
    qDBusRegisterMetaType<QList<Struct3> >();
    qDBusRegisterMetaType<QList<Struct4> >();

    qDBusRegisterMetaType<Invalid0>();
    qDBusRegisterMetaType<Invalid1>();
    qDBusRegisterMetaType<Invalid2>();
    qDBusRegisterMetaType<Invalid3>();
    qDBusRegisterMetaType<Invalid4>();
    qDBusRegisterMetaType<Invalid5>();
    // don't register Invalid6
    qDBusRegisterMetaType<Invalid7>();

    qDBusRegisterMetaType<QList<Invalid0> >();

    intStringMap = qDBusRegisterMetaType<QMap<int, QString> >();
    stringStringMap = qDBusRegisterMetaType<QMap<QString, QString> >();
    stringStruct1Map = qDBusRegisterMetaType<QMap<QString, Struct1> >();
}

void tst_QDBusMetaType::staticTypes_data()
{
    QTest::addColumn<QVariant::Type>("typeId");
    QTest::addColumn<QString>("signature");

    QTest::newRow("uchar") << QVariant::Type(QMetaType::UChar) << "y";
    QTest::newRow("bool") << QVariant::Bool << "b";
    QTest::newRow("short") << QVariant::Type(QMetaType::Short) << "n";
    QTest::newRow("ushort") << QVariant::Type(QMetaType::UShort) << "q";
    QTest::newRow("int") << QVariant::Int << "i";
    QTest::newRow("uint") << QVariant::UInt << "u";
    QTest::newRow("qlonglong") << QVariant::LongLong << "x";
    QTest::newRow("qulonglong") << QVariant::ULongLong << "t";
    QTest::newRow("double") << QVariant::Double << "d";
    QTest::newRow("QString") << QVariant::String << "s";
    QTest::newRow("QDBusObjectPath") << QVariant::Type(qMetaTypeId<QDBusObjectPath>()) << "o";
    QTest::newRow("QDBusSignature") << QVariant::Type(qMetaTypeId<QDBusSignature>()) << "g";
    QTest::newRow("QDBusVariant") << QVariant::Type(qMetaTypeId<QDBusVariant>()) << "v";

    QTest::newRow("QByteArray") << QVariant::ByteArray << "ay";
    QTest::newRow("QStringList") << QVariant::StringList << "as";
}

void tst_QDBusMetaType::dynamicTypes_data()
{
    QTest::addColumn<QVariant::Type>("typeId");
    QTest::addColumn<QString>("signature");

    QTest::newRow("QVariantList") << QVariant::List << "av";
    QTest::newRow("QVariantMap") << QVariant::Map << "a{sv}";
    QTest::newRow("QDate") << QVariant::Date << "(iii)";
    QTest::newRow("QTime") << QVariant::Time << "(iiii)";
    QTest::newRow("QDateTime") << QVariant::DateTime << "((iii)(iiii)i)";
    QTest::newRow("QRect") << QVariant::Rect << "(iiii)";
    QTest::newRow("QRectF") << QVariant::RectF << "(dddd)";
    QTest::newRow("QSize") << QVariant::Size << "(ii)";
    QTest::newRow("QSizeF") << QVariant::SizeF << "(dd)";
    QTest::newRow("QPoint") << QVariant::Point << "(ii)";
    QTest::newRow("QPointF") << QVariant::PointF << "(dd)";
    QTest::newRow("QLine") << QVariant::Line << "((ii)(ii))";
    QTest::newRow("QLineF") << QVariant::LineF << "((dd)(dd))";

    QTest::newRow("Struct1") << QVariant::Type(qMetaTypeId<Struct1>()) << "(s)";
    QTest::newRow("QList<Struct1>") << QVariant::Type(qMetaTypeId<QList<Struct1> >()) << "a(s)";

    QTest::newRow("Struct2") << QVariant::Type(qMetaTypeId<Struct2>()) << "(sos)";
    QTest::newRow("QList<Struct2>") << QVariant::Type(qMetaTypeId<QList<Struct2> >()) << "a(sos)";

    QTest::newRow("QList<Struct3>") << QVariant::Type(qMetaTypeId<QList<Struct3> >()) << "a(sas)";
    QTest::newRow("Struct3") << QVariant::Type(qMetaTypeId<Struct3>()) << "(sas)";

    QTest::newRow("Struct4") << QVariant::Type(qMetaTypeId<Struct4>()) << "(ssa(ss)sayasx)";
    QTest::newRow("QList<Struct4>") << QVariant::Type(qMetaTypeId<QList<Struct4> >()) << "a(ssa(ss)sayasx)";

    QTest::newRow("QMap<int,QString>") << QVariant::Type(intStringMap) << "a{is}";
    QTest::newRow("QMap<QString,QString>") << QVariant::Type(stringStringMap) << "a{ss}";
    QTest::newRow("QMap<QString,Struct1>") << QVariant::Type(stringStruct1Map) << "a{s(s)}";
}

void tst_QDBusMetaType::staticTypes()
{
    QFETCH(QVariant::Type, typeId);

    QString result = QDBusMetaType::typeToSignature(typeId);
    QTEST(result, "signature");
}

void tst_QDBusMetaType::dynamicTypes()
{
    // same test
    staticTypes();
}

void tst_QDBusMetaType::invalidTypes_data()
{
    QTest::addColumn<QVariant::Type>("typeId");
    QTest::addColumn<QString>("signature");

    QTest::newRow("Invalid0") << QVariant::Type(qMetaTypeId<Invalid0>()) << "";
    QTest::newRow("Invalid1") << QVariant::Type(qMetaTypeId<Invalid1>()) << "";
    QTest::newRow("Invalid2") << QVariant::Type(qMetaTypeId<Invalid2>()) << "";
    QTest::newRow("Invalid3") << QVariant::Type(qMetaTypeId<Invalid3>()) << "";
    QTest::newRow("Invalid4") << QVariant::Type(qMetaTypeId<Invalid4>()) << "";
    QTest::newRow("Invalid5") << QVariant::Type(qMetaTypeId<Invalid5>()) << "";
    QTest::newRow("Invalid6") << QVariant::Type(qMetaTypeId<Invalid6>()) << "";
    QTest::newRow("Invalid7") << QVariant::Type(qMetaTypeId<Invalid7>()) << "";

    QTest::newRow("QList<Invalid0>") << QVariant::Type(qMetaTypeId<QList<Invalid0> >()) << "";

    QTest::newRow("long") << QVariant::Type(QMetaType::Long) << "";
    QTest::newRow("void*") << QVariant::Type(QMetaType::VoidStar) << "";
}

void tst_QDBusMetaType::invalidTypes()
{
    // same test
    if (qstrcmp(QTest::currentDataTag(), "Invalid0") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `Invalid0' produces invalid D-BUS signature `<empty>' (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid1") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `Invalid1' attempts to redefine basic D-BUS type 's' (QString) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid2") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `Invalid2' attempts to redefine basic D-BUS type 'o' (QDBusObjectPath) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid3") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `Invalid3' attempts to redefine basic D-BUS type 'as' (QStringList) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid4") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `Invalid4' attempts to redefine basic D-BUS type 'ay' (QByteArray) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid5") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `Invalid5' produces invalid D-BUS signature `ii' (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid7") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `Invalid7' produces invalid D-BUS signature `()' (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "QList<Invalid0>") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type `QList<Invalid0>' produces invalid D-BUS signature `a' (Did you forget to call beginStructure() ?)");

    staticTypes();
    staticTypes();              // run twice: the error messages should be printed once only
}

QTEST_MAIN(tst_QDBusMetaType)

#include "tst_qdbusmetatype.moc"
