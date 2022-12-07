// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QCoreApplication>
#include <QMetaType>
#include <QDBusArgument>
#include <QDBusMetaType>

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
struct Struct5                  // a{sa{sv}} - non-standard outer struct is used as a local
{                               // container, see marshalling operator below.
    QVariantMap m1;
    QVariantMap m2;
    QVariantMap m3;
};
struct Struct6                  // av - non-standard outer struct is used as a local container,
{                               // see marshalling operator below.
    QVariant v1;
    QVariant v2;
    QVariant v3;
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
Q_DECLARE_METATYPE(Struct5)
Q_DECLARE_METATYPE(Struct6)

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

QDBusArgument &operator<<(QDBusArgument &arg, const Struct5 &s)
{
    arg.beginMap(qMetaTypeId<QString>(), qMetaTypeId<QVariantMap>());

    arg.beginMapEntry();
    arg << QStringLiteral("map1") << s.m1;
    arg.endMapEntry();

    arg.beginMapEntry();
    arg << QStringLiteral("map2") << s.m2;
    arg.endMapEntry();

    arg.beginMapEntry();
    arg << QStringLiteral("map3") << s.m3;
    arg.endMapEntry();

    arg.endMap();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Struct6 &s)
{
    arg.beginArray(qMetaTypeId<QDBusVariant>());
    arg << QDBusVariant(s.v1) << QDBusVariant(s.v2) << QDBusVariant(s.v3);
    arg.endArray();
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
const QDBusArgument &operator>>(const QDBusArgument &arg, Struct5 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Struct6 &)
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
    qDBusRegisterMetaType<Struct5>();
    qDBusRegisterMetaType<Struct6>();
    qDBusRegisterMetaType<StringPair>();

    qDBusRegisterMetaType<QList<Struct1> >();
    qDBusRegisterMetaType<QList<Struct2> >();
    qDBusRegisterMetaType<QList<Struct3> >();
    qDBusRegisterMetaType<QList<Struct4> >();

#ifdef Q_CC_GNU_ONLY
    // GCC has a defect/extension (depending on your point of view) that allows
    // a template class with defaulted template parameters to match a Template
    // Template Parameter (TTP) with fewer template arguments. The call below
    // tries to use the template<template <typename> class Container, ...>
    // template functions qdbusargument.h
    qDBusRegisterMetaType<std::vector<Struct1> >();
#endif

    qDBusRegisterMetaType<Invalid0>();
    qDBusRegisterMetaType<Invalid1>();
    qDBusRegisterMetaType<Invalid2>();
    qDBusRegisterMetaType<Invalid3>();
    qDBusRegisterMetaType<Invalid4>();
    qDBusRegisterMetaType<Invalid5>();
    // don't register Invalid6
    qDBusRegisterMetaType<Invalid7>();

    qDBusRegisterMetaType<QList<Invalid0> >();

    intStringMap = qDBusRegisterMetaType<QMap<int, QString> >().id();
    stringStringMap = qDBusRegisterMetaType<QMap<QString, QString> >().id();
    stringStruct1Map = qDBusRegisterMetaType<QMap<QString, Struct1> >().id();
}

void tst_QDBusMetaType::staticTypes_data()
{
    QTest::addColumn<int>("typeId");
    QTest::addColumn<QString>("signature");

    QTest::newRow("uchar") << int(QMetaType::UChar) << "y";
    QTest::newRow("bool") << int(QMetaType::Bool) << "b";
    QTest::newRow("short") << int(QMetaType::Short) << "n";
    QTest::newRow("ushort") << int(QMetaType::UShort) << "q";
    QTest::newRow("int") << int(QMetaType::Int) << "i";
    QTest::newRow("uint") << int(QMetaType::UInt) << "u";
    QTest::newRow("qlonglong") << int(QMetaType::LongLong) << "x";
    QTest::newRow("qulonglong") << int(QMetaType::ULongLong) << "t";
    QTest::newRow("double") << int(QMetaType::Double) << "d";
    QTest::newRow("QString") << int(QMetaType::QString) << "s";
    QTest::newRow("QDBusObjectPath") << qMetaTypeId<QDBusObjectPath>() << "o";
    QTest::newRow("QDBusSignature") << qMetaTypeId<QDBusSignature>() << "g";
    QTest::newRow("QDBusVariant") << qMetaTypeId<QDBusVariant>() << "v";

    QTest::newRow("QByteArray") << int(QMetaType::QByteArray) << "ay";
    QTest::newRow("QStringList") << int(QMetaType::QStringList) << "as";
}

void tst_QDBusMetaType::dynamicTypes_data()
{
    QTest::addColumn<int>("typeId");
    QTest::addColumn<QString>("signature");

    QTest::newRow("QVariantList") << int(QMetaType::QVariantList) << "av";
    QTest::newRow("QVariantMap") << int(QMetaType::QVariantMap) << "a{sv}";
    QTest::newRow("QDate") << int(QMetaType::QDate) << "(iii)";
    QTest::newRow("QTime") << int(QMetaType::QTime) << "(iiii)";
    QTest::newRow("QDateTime") << int(QMetaType::QDateTime) << "((iii)(iiii)i)";
    QTest::newRow("QRect") << int(QMetaType::QRect) << "(iiii)";
    QTest::newRow("QRectF") << int(QMetaType::QRectF) << "(dddd)";
    QTest::newRow("QSize") << int(QMetaType::QSize) << "(ii)";
    QTest::newRow("QSizeF") << int(QMetaType::QSizeF) << "(dd)";
    QTest::newRow("QPoint") << int(QMetaType::QPoint) << "(ii)";
    QTest::newRow("QPointF") << int(QMetaType::QPointF) << "(dd)";
    QTest::newRow("QLine") << int(QMetaType::QLine) << "((ii)(ii))";
    QTest::newRow("QLineF") << int(QMetaType::QLineF) << "((dd)(dd))";

    QTest::newRow("Struct1") << qMetaTypeId<Struct1>() << "(s)";
    QTest::newRow("QList<Struct1>") << qMetaTypeId<QList<Struct1> >() << "a(s)";
#ifdef Q_CC_GNU_ONLY
    QTest::newRow("std::vector<Struct1>") << qMetaTypeId<std::vector<Struct1> >() << "a(s)";
#endif

    QTest::newRow("Struct2") << qMetaTypeId<Struct2>() << "(sos)";
    QTest::newRow("QList<Struct2>") << qMetaTypeId<QList<Struct2>>() << "a(sos)";

    QTest::newRow("QList<Struct3>") << qMetaTypeId<QList<Struct3>>() << "a(sas)";
    QTest::newRow("Struct3") << qMetaTypeId<Struct3>() << "(sas)";

    QTest::newRow("Struct4") << qMetaTypeId<Struct4>() << "(ssa(ss)sayasx)";
    QTest::newRow("QList<Struct4>") << qMetaTypeId<QList<Struct4>>() << "a(ssa(ss)sayasx)";

    QTest::newRow("Struct5") << qMetaTypeId<Struct5>() << "a{sa{sv}}";

    QTest::newRow("Struct6") << qMetaTypeId<Struct6>() << "av";

    QTest::newRow("QMap<int,QString>") << intStringMap << "a{is}";
    QTest::newRow("QMap<QString,QString>") << stringStringMap << "a{ss}";
    QTest::newRow("QMap<QString,Struct1>") << stringStruct1Map << "a{s(s)}";
}

void tst_QDBusMetaType::staticTypes()
{
    QFETCH(int, typeId);

    QString result = QDBusMetaType::typeToSignature(QMetaType(typeId));
    QTEST(result, "signature");
}

void tst_QDBusMetaType::dynamicTypes()
{
    // same test
    staticTypes();
}

void tst_QDBusMetaType::invalidTypes_data()
{
    QTest::addColumn<int>("typeId");
    QTest::addColumn<QString>("signature");

    QTest::newRow("Invalid0") << qMetaTypeId<Invalid0>() << "";
    QTest::newRow("Invalid1") << qMetaTypeId<Invalid1>() << "";
    QTest::newRow("Invalid2") << qMetaTypeId<Invalid2>() << "";
    QTest::newRow("Invalid3") << qMetaTypeId<Invalid3>() << "";
    QTest::newRow("Invalid4") << qMetaTypeId<Invalid4>() << "";
    QTest::newRow("Invalid5") << qMetaTypeId<Invalid5>() << "";
    QTest::newRow("Invalid6") << qMetaTypeId<Invalid6>() << "";
    QTest::newRow("Invalid7") << qMetaTypeId<Invalid7>() << "";

    QTest::newRow("QList<Invalid0>") << qMetaTypeId<QList<Invalid0>>() << "";

    QTest::newRow("long") << int(QMetaType::Long) << "";
    QTest::newRow("void*") << int(QMetaType::VoidStar) << "";
}

void tst_QDBusMetaType::invalidTypes()
{
    // same test
    if (qstrcmp(QTest::currentDataTag(), "Invalid0") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'Invalid0' produces invalid D-BUS signature '<empty>' (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid1") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'Invalid1' attempts to redefine basic D-BUS type 's' (QString) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid2") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'Invalid2' attempts to redefine basic D-BUS type 'o' (QDBusObjectPath) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid3") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'Invalid3' attempts to redefine basic D-BUS type 'as' (QStringList) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid4") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'Invalid4' attempts to redefine basic D-BUS type 'ay' (QByteArray) (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid5") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'Invalid5' produces invalid D-BUS signature 'ii' (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "Invalid7") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'Invalid7' produces invalid D-BUS signature '()' (Did you forget to call beginStructure() ?)");
    else if (qstrcmp(QTest::currentDataTag(), "QList<Invalid0>") == 0)
        QTest::ignoreMessage(QtWarningMsg, "QDBusMarshaller: type 'QList<Invalid0>' produces invalid D-BUS signature 'a' (Did you forget to call beginStructure() ?)");

    staticTypes();
    staticTypes();              // run twice: the error messages should be printed once only
}

QTEST_MAIN(tst_QDBusMetaType)

#include "tst_qdbusmetatype.moc"
