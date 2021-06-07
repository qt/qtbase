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
#include "tst_qvariant_common.h"

#include <QtCore/private/qmetaobjectbuilder_p.h>

void tst_QMetaType::constRefs()
{
    QCOMPARE(::qMetaTypeId<const int &>(), ::qMetaTypeId<int>());
    QCOMPARE(::qMetaTypeId<const QString &>(), ::qMetaTypeId<QString>());
    QCOMPARE(::qMetaTypeId<const CustomMovable &>(), ::qMetaTypeId<CustomMovable>());
    QCOMPARE(::qMetaTypeId<const QList<CustomMovable> &>(), ::qMetaTypeId<QList<CustomMovable> >());
    static_assert(::qMetaTypeId<const int &>() == ::qMetaTypeId<int>());
}

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
    QVERIFY((!QVariant::fromValue<From>(From()).template canConvert<To>()));
}

template<typename From, typename To>
void testCustomTypeConvertible()
{
    QVERIFY((hasRegisteredConverterFunction<From, To>()));
    QVERIFY((QVariant::fromValue<From>(From()).template canConvert<To>()));
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

void tst_QMetaType::convertConstNonConst()
{
    auto mtConstObj = QMetaType::fromType<QObject const*>();
    auto mtObj = QMetaType::fromType<QObject *>();
    auto mtConstDerived = QMetaType::fromType<Derived const*>();
    auto mtDerived = QMetaType::fromType<Derived *>();

    QVERIFY(QMetaType::canConvert(mtConstObj, mtObj));
    QVERIFY(QMetaType::canConvert(mtObj, mtConstObj)); // casting const away is allowed (but can lead to UB)
    QVERIFY(QMetaType::canConvert(mtConstDerived, mtObj));
    QVERIFY(QMetaType::canConvert(mtDerived, mtConstObj));
    QVERIFY(QMetaType::canConvert(mtObj, mtConstDerived));
}

void tst_QMetaType::compareCustomEqualOnlyType()
{
    QMetaType type = QMetaType::fromType<CustomEqualsOnlyType>();

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

    // check QMetaType::compare works/doesn't crash for equals only comparators
    auto cmp = type.compare(variant50.constData(), variant50.constData());
    QCOMPARE(cmp, QPartialOrdering::Unordered);
    bool equals = type.equals(variant50.constData(), variant50.constData());
    QVERIFY(equals);

    cmp = type.compare(variant100.constData(), variant100x.constData());
    QCOMPARE(cmp, QPartialOrdering::Unordered);
    equals = type.equals(variant100.constData(), variant100x.constData());
    QVERIFY(equals);

    cmp = type.compare(variant50.constData(), variant100.constData());
    QCOMPARE(cmp, QPartialOrdering::Unordered);
    equals = type.equals(variant50.constData(), variant100.constData());
    QVERIFY(!equals);

    //check QMetaType::equals for type w/o equals comparator being registered
    CustomMovable movable1;
    CustomMovable movable2;
    type = QMetaType::fromType<CustomMovable>();
    equals = type.equals(&movable1, &movable2);
}

void tst_QMetaType::customDebugStream()
{
    MessageHandlerCustom handler(::qMetaTypeId<CustomDebugStreamableType>());
    QVariant v1 = QVariant::fromValue(CustomDebugStreamableType());
    handler.expectedMessage = "QVariant(CustomDebugStreamableType, string-content)";
    qDebug() << v1;

    MessageHandlerCustom handler2(::qMetaTypeId<CustomDebugStreamableType2>());
    QMetaType::registerConverter<CustomDebugStreamableType2, QString>(&CustomDebugStreamableType2::toString);
    handler2.expectedMessage = "QVariant(CustomDebugStreamableType2, \"test\")";
    QVariant v2 = QVariant::fromValue(CustomDebugStreamableType2());
    qDebug() << v2;
}

void tst_QMetaType::unknownType()
{
    QMetaType invalid(QMetaType::UnknownType);
    QVERIFY(!invalid.create());
    QVERIFY(!invalid.sizeOf());
    QVERIFY(!invalid.metaObject());
    int buffer = 0xBAD;
    invalid.construct(&buffer);
    QCOMPARE(buffer, 0xBAD);
}

void tst_QMetaType::fromType()
{
    #define FROMTYPE_CHECK(MetaTypeName, MetaTypeId, RealType) \
        QCOMPARE(QMetaType::fromType<RealType>(), QMetaType(MetaTypeId)); \
        QVERIFY(QMetaType::fromType<RealType>() == QMetaType(MetaTypeId)); \
        QVERIFY(!(QMetaType::fromType<RealType>() != QMetaType(MetaTypeId))); \
        if (MetaTypeId != QMetaType::Void) \
            QCOMPARE(QMetaType::fromType<RealType>().id(), MetaTypeId);

    FOR_EACH_CORE_METATYPE(FROMTYPE_CHECK)

    QVERIFY(QMetaType::fromType<QString>() != QMetaType());
    QCOMPARE(QMetaType(), QMetaType());
    QCOMPARE(QMetaType(QMetaType::UnknownType), QMetaType());

    FROMTYPE_CHECK(_, ::qMetaTypeId<Whity<int>>(), Whity<int>)
    #undef FROMTYPE_CHECK
}

template<char X, typename T = void>
struct CharTemplate
{
    struct
    {
        int a;
    } x;

    union
    {
        int a;
    } y;
};

void tst_QMetaType::operatorEq_data()
{
    QTest::addColumn<QMetaType>("typeA");
    QTest::addColumn<QMetaType>("typeB");
    QTest::addColumn<bool>("eq");
    QTest::newRow("String") << QMetaType(QMetaType::QString)
                            << QMetaType::fromType<const QString &>() << true;
    QTest::newRow("void1")  << QMetaType(QMetaType::UnknownType) << QMetaType::fromType<void>()
                            << false;
    QTest::newRow("void2")  << QMetaType::fromType<const void>() << QMetaType::fromType<void>()
                            << true;
    QTest::newRow("list1")  << QMetaType::fromType<QList<const int *>>()
                            << QMetaType::fromType<QList<const int *>>() << true;
    QTest::newRow("list2")  << QMetaType::fromType<QList<const int *>>()
                            << QMetaType::fromType<QList<int *>>() << false;
    QTest::newRow("char1")  << QMetaType::fromType<CharTemplate<'>'>>()
                            << QMetaType::fromType<CharTemplate<'>', void>>() << true;
    QTest::newRow("annon1") << QMetaType::fromType<decltype(CharTemplate<'>'>::x)>()
                            << QMetaType::fromType<decltype(CharTemplate<'>'>::x)>() << true;
    QTest::newRow("annon2") << QMetaType::fromType<decltype(CharTemplate<'>'>::x)>()
                            << QMetaType::fromType<decltype(CharTemplate<'<'>::x)>() << false;
}

void tst_QMetaType::operatorEq()
{
    QFETCH(QMetaType, typeA);
    QFETCH(QMetaType, typeB);
    QFETCH(bool, eq);

    QCOMPARE(typeA == typeB, eq);
    QCOMPARE(typeB == typeA, eq);
    QCOMPARE(typeA != typeB, !eq);
    QCOMPARE(typeB != typeA, !eq);
}

class WithPrivateDTor {
    ~WithPrivateDTor(){};
};

struct WithDeletedDtor {
    ~WithDeletedDtor() = delete;
};

void tst_QMetaType::typesWithInaccessibleDTors()
{
    // should compile
    Q_UNUSED(QMetaType::fromType<WithPrivateDTor>());
    Q_UNUSED(QMetaType::fromType<WithDeletedDtor>());
}

void tst_QMetaType::voidIsNotUnknown()
{
    QMetaType voidType = QMetaType::fromType<void>();
    QMetaType voidType2 = QMetaType(QMetaType::Void);
    QCOMPARE(voidType, voidType2);
    QVERIFY(voidType != QMetaType(QMetaType::UnknownType));
}

void tst_QMetaType::typeNameNormalization()
{
    // check the we normalize types the right way
#define CHECK_TYPE_NORMALIZATION(Normalized, ...) \
    do { \
        /*QCOMPARE(QtPrivate::typenameHelper<Type>(), Normalized);*/ \
        QByteArray typeName = QMetaObject::normalizedType(#__VA_ARGS__); \
        QCOMPARE(typeName, Normalized); \
        typeName = QMetaType::fromType<__VA_ARGS__>().name(); \
        QCOMPARE(typeName, Normalized); \
    } while (0)

    CHECK_TYPE_NORMALIZATION("QList<QString*const>", QList<QString * const>);
    CHECK_TYPE_NORMALIZATION("QList<const QString*>", QList<const QString * >);
    CHECK_TYPE_NORMALIZATION("QList<const QString*const>", QList<const QString * const>);
    CHECK_TYPE_NORMALIZATION("QList<const QString*>", QList<QString const *>);
    CHECK_TYPE_NORMALIZATION("QList<signed char>", QList<signed char>);
    CHECK_TYPE_NORMALIZATION("QList<uint>", QList<unsigned>);
    CHECK_TYPE_NORMALIZATION("uint", uint);
    CHECK_TYPE_NORMALIZATION("QList<QHash<uint,QString*>>", QList<QHash<unsigned, QString *>>);
    CHECK_TYPE_NORMALIZATION("QList<qlonglong>", QList<qlonglong>);
    CHECK_TYPE_NORMALIZATION("QList<qulonglong>", QList<qulonglong>);
    CHECK_TYPE_NORMALIZATION("QList<qlonglong>", QList<long long>);
    CHECK_TYPE_NORMALIZATION("QList<qulonglong>", QList<unsigned long long>);
    CHECK_TYPE_NORMALIZATION("QList<qulonglong*>", QList<unsigned long long *>);
    CHECK_TYPE_NORMALIZATION("QList<ulong>", QList<long unsigned >);
#ifdef Q_CC_MSVC
    CHECK_TYPE_NORMALIZATION("qulonglong", __int64 unsigned);
#endif
    CHECK_TYPE_NORMALIZATION("std::pair<const QString&&,short>", QPair<const QString &&, signed short>);

    // The string based normalization doesn't handle aliases, QMetaType::fromType() does
//    CHECK_TYPE_NORMALIZATION("qulonglong", quint64);
    QCOMPARE(QMetaType::fromType<quint64>().name(), "qulonglong");

    // noramlizedType and metatype name agree
    {
        auto type = QMetaType::fromType<decltype(CharTemplate<'<'>::x)>();
        QCOMPARE(type.name(), QMetaObject::normalizedType(type.name()));
    }
    {
        auto type = QMetaType::fromType<decltype(CharTemplate<'<'>::y)>();
        QCOMPARE(type.name(), QMetaObject::normalizedType(type.name()));
    }
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
Q_DECLARE_METATYPE(UndefinedFunction4);
