/****************************************************************************
**
** Copyright (C) 2020 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qcborvalue.h>
#include <QtTest>

#include <QtCore/private/qbytearray_p.h>

Q_DECLARE_METATYPE(QCborValue)
Q_DECLARE_METATYPE(QCborValue::EncodingOptions)

class tst_QCborValue : public QObject
{
    Q_OBJECT

private slots:
    void basics_data();
    void basics();
    void tagged_data() { basics_data(); }
    void tagged();
    void extendedTypes_data();
    void extendedTypes();
    void copyCompare_data() { basics_data(); }
    void copyCompare();

    void arrayDefaultInitialization();
    void arrayEmptyInitializerList();
    void arrayEmptyDetach();
    void arrayInitializerList();
    void arrayMutation();
    void arrayPrepend();
    void arrayInsertRemove_data() { basics_data(); }
    void arrayInsertRemove();
    void arrayInsertTagged_data() { basics_data(); }
    void arrayInsertTagged();
    void arrayStringElements();
    void arraySelfAssign_data() { basics_data(); }
    void arraySelfAssign();

    void mapDefaultInitialization();
    void mapEmptyInitializerList();
    void mapEmptyDetach();
    void mapSimpleInitializerList();
    void mapMutation();
    void mapStringValues();
    void mapStringKeys();
    void mapInsertRemove_data() { basics_data(); }
    void mapInsertRemove();
    void mapInsertTagged_data() { basics_data(); }
    void mapInsertTagged();
    void mapSelfAssign_data() { basics_data(); }
    void mapSelfAssign();
    void mapComplexKeys_data() { basics_data(); }
    void mapComplexKeys();

    void sorting();

    void toCbor_data();
    void toCbor();
    void toCborStreamWriter_data() { toCbor_data(); }
    void toCborStreamWriter();
    void fromCbor_data();
    void fromCbor();
    void fromCborStreamReaderByteArray_data() { fromCbor_data(); }
    void fromCborStreamReaderByteArray();
    void fromCborStreamReaderIODevice_data() { fromCbor_data(); }
    void fromCborStreamReaderIODevice();
    void validation_data();
    void validation();
    void hugeDeviceValidation_data();
    void hugeDeviceValidation();
    void recursionLimit_data();
    void recursionLimit();
    void toDiagnosticNotation_data();
    void toDiagnosticNotation();
};

// Get the validation data from TinyCBOR (see src/3rdparty/tinycbor/tests/parser/data.cpp)
#include "data.cpp"

struct SimpleTypeWrapper
{
    // QCborSimpleType is an enum, so QVariant knows how to convert it to
    // integer and we don't want it to do that.
    SimpleTypeWrapper(QCborSimpleType type = {}) : st(type) {}
    QCborSimpleType st;
};
Q_DECLARE_METATYPE(SimpleTypeWrapper)

void tst_QCborValue::basics_data()
{
    QTest::addColumn<QCborValue::Type>("type");
    QTest::addColumn<QCborValue>("v");
    QTest::addColumn<QVariant>("expectedValue");
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUuid uuid = QUuid::createUuid();

    QMetaEnum me = QMetaEnum::fromType<QCborValue::Type>();
    auto add = [me](QCborValue::Type t, const QCborValue &v, const QVariant &exp) {
        auto addRow = [=]() -> QTestData & {
            const char *typeString = me.valueToKey(t);
            if (t == QCborValue::Integer)
                return QTest::addRow("Integer:%lld", exp.toLongLong());
            if (t == QCborValue::Double)
                return QTest::addRow("Double:%g", exp.toDouble());
            if (t == QCborValue::ByteArray || t == QCborValue::String)
                return QTest::addRow("%s:%d", typeString, exp.toString().size());
            return QTest::newRow(typeString);
        };
        addRow() << t << v << exp;
    };
    auto st = [](QCborSimpleType t) { return QVariant::fromValue<SimpleTypeWrapper>(t); };

    add(QCborValue::Undefined, QCborValue(), st(QCborSimpleType::Undefined));
    add(QCborValue::Null, QCborValue::Null, st(QCborSimpleType::Null));
    QTest::newRow("nullptr") << QCborValue::Null << QCborValue(nullptr)
                             << st(QCborSimpleType::Null);
    add(QCborValue::False, false, st(QCborSimpleType::False));
    QTest::newRow("false") << QCborValue::False << QCborValue(QCborValue::False)
                           << st(QCborSimpleType::False);
    add(QCborValue::True, true, st(QCborSimpleType::True));
    QTest::newRow("true") << QCborValue::True << QCborValue(QCborValue::True)
                          << st(QCborSimpleType::True);
    QTest::newRow("simpletype") << QCborValue::Type(QCborValue::SimpleType + 255)
                                << QCborValue(QCborSimpleType(255))
                                << st(QCborSimpleType(255));
    add(QCborValue::Integer, 0, 0);
    add(QCborValue::Integer, 1, 1);
    add(QCborValue::Integer, -1, -1);
    add(QCborValue::Integer, std::numeric_limits<qint64>::min(), std::numeric_limits<qint64>::min());
    add(QCborValue::Integer, std::numeric_limits<qint64>::max(), std::numeric_limits<qint64>::max());
    add(QCborValue::Double, 0., 0.);
    add(QCborValue::Double, 1.25, 1.25);
    add(QCborValue::Double, -1.25, -1.25);
    add(QCborValue::Double, qInf(), qInf());
    add(QCborValue::Double, -qInf(), -qInf());
    add(QCborValue::Double, qQNaN(), qQNaN());
    add(QCborValue::ByteArray, QByteArray("Hello"), QByteArray("Hello"));
    add(QCborValue::ByteArray, QByteArray(), QByteArray());
    add(QCborValue::String, "Hello", "Hello");
    add(QCborValue::String, QLatin1String(), QString());
    add(QCborValue::DateTime, QCborValue(dt), dt);
    add(QCborValue::Url, QCborValue(QUrl("http://example.com")), QUrl("http://example.com"));
    add(QCborValue::RegularExpression, QCborValue(QRegularExpression("^.*$")), QRegularExpression("^.*$"));
    add(QCborValue::Uuid, QCborValue(uuid), uuid);

    // empty arrays and maps
    add(QCborValue::Array, QCborArray(), QVariantList());
    add(QCborValue::Map, QCborMap(), QVariantMap());
}

static void basicTypeCheck(QCborValue::Type type, const QCborValue &v, const QVariant &expectedValue)
{
    bool isSimpleType = (expectedValue.userType() == qMetaTypeId<SimpleTypeWrapper>());
    QCborSimpleType st = expectedValue.value<SimpleTypeWrapper>().st;

    QCOMPARE(v.type(), type);
    QCOMPARE(v.isInteger(), type == QCborValue::Integer);
    QCOMPARE(v.isByteArray(), type == QCborValue::ByteArray);
    QCOMPARE(v.isString(), type == QCborValue::String);
    QCOMPARE(v.isArray(), type == QCborValue::Array);
    QCOMPARE(v.isMap(), type == QCborValue::Map);
    QCOMPARE(v.isFalse(), type == QCborValue::False);
    QCOMPARE(v.isTrue(), type == QCborValue::True);
    QCOMPARE(v.isBool(), type == QCborValue::False || type == QCborValue::True);
    QCOMPARE(v.isNull(), type == QCborValue::Null);
    QCOMPARE(v.isUndefined(), type == QCborValue::Undefined);
    QCOMPARE(v.isDouble(), type == QCborValue::Double);
    QCOMPARE(v.isDateTime(), type == QCborValue::DateTime);
    QCOMPARE(v.isUrl(), type == QCborValue::Url);
    QCOMPARE(v.isUuid(), type == QCborValue::Uuid);
    QCOMPARE(v.isInvalid(), type == QCborValue::Invalid);
    QCOMPARE(v.isContainer(), type == QCborValue::Array || type == QCborValue::Map);
    QCOMPARE(v.isSimpleType(), isSimpleType);
    QCOMPARE(v.isSimpleType(QCborSimpleType::False), st == QCborSimpleType::False);
    QCOMPARE(v.isSimpleType(QCborSimpleType::True), st == QCborSimpleType::True);
    QCOMPARE(v.isSimpleType(QCborSimpleType::Null), st == QCborSimpleType::Null);
    QCOMPARE(v.isSimpleType(QCborSimpleType::Undefined), st == QCborSimpleType::Undefined);
    QCOMPARE(v.isSimpleType(QCborSimpleType(255)), st == QCborSimpleType(255));

    if (v.isInteger()) {
        QCOMPARE(v.toInteger(), expectedValue.toLongLong());
        QCOMPARE(v.toDouble(), 0. + expectedValue.toLongLong());
    } else {
        QCOMPARE(v.toInteger(), qint64(expectedValue.toDouble()));
        QCOMPARE(v.toDouble(), expectedValue.toDouble());
    }
    QCOMPARE(v.toBool(true), st != QCborSimpleType::False);
    QCOMPARE(v.toBool(), st == QCborSimpleType::True);
    if (st == QCborSimpleType::Undefined)
        QCOMPARE(v.toSimpleType(QCborSimpleType::Null), QCborSimpleType::Undefined);
    else if (isSimpleType)
        QCOMPARE(v.toSimpleType(), st);
    else
        QCOMPARE(v.toSimpleType(), QCborSimpleType::Undefined);

#define CMP(expr, T, validexpr)    \
    if (expectedValue.userType() == qMetaTypeId<T>()) \
        QCOMPARE(expr, expectedValue.value<T>()); \
    else \
        QVERIFY(validexpr)
    CMP(v.toByteArray(), QByteArray, v.toByteArray().isNull());
    CMP(v.toString(), QString, v.toString().isNull());
    CMP(v.toDateTime(), QDateTime, !v.toDateTime().isValid());
    CMP(v.toUrl(), QUrl, !v.toUrl().isValid());
    CMP(v.toRegularExpression(), QRegularExpression, v.toRegularExpression().pattern().isNull());
    CMP(v.toUuid(), QUuid, v.toUuid().isNull());
#undef CMP

    QVERIFY(v.toArray().isEmpty());
    QVERIFY(v.toMap().isEmpty());

    QVERIFY(v["Hello"].isUndefined());
    QVERIFY(v[0].isUndefined());
}

void tst_QCborValue::basics()
{
    QFETCH(QCborValue::Type, type);
    QFETCH(QCborValue, v);
    QFETCH(QVariant, expectedValue);

    basicTypeCheck(type, v, expectedValue);
}

void tst_QCborValue::tagged()
{
    QFETCH(QCborValue::Type, type);
    QFETCH(QCborValue, v);
    QFETCH(QVariant, expectedValue);

    // make it tagged
    QCborValue tagged(QCborKnownTags::Signature, v);
    QVERIFY(tagged.isTag());
    QCOMPARE(tagged.tag(), QCborTag(QCborKnownTags::Signature));

    // shouldn't compare equal
    QVERIFY(tagged != v);
    QVERIFY(v != tagged);

    // ensure we can reach the original value
    basicTypeCheck(type, tagged.taggedValue(), expectedValue);
    QVERIFY(tagged.taggedValue() == v);
    QVERIFY(v == tagged.taggedValue());

    // nested tagging should work too
    QCborValue tagged2(QCborKnownTags::EncodedCbor, tagged);
    QVERIFY(tagged2.isTag());
    QCOMPARE(tagged2.tag(), QCborTag(QCborKnownTags::EncodedCbor));

    QVERIFY(tagged2 != tagged);
    QVERIFY(tagged != tagged2);

    QVERIFY(tagged2.taggedValue() == tagged);
    QVERIFY(tagged == tagged2.taggedValue());
    QVERIFY(tagged2.taggedValue().taggedValue() == v);
    QVERIFY(v == tagged2.taggedValue().taggedValue());
}

void tst_QCborValue::extendedTypes_data()
{
    QTest::addColumn<QCborValue>("extended");
    QTest::addColumn<QCborValue>("tagged");
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUuid uuid = QUuid::createUuid();

    QTest::newRow("DateTime") << QCborValue(dt)
                              << QCborValue(QCborKnownTags::DateTimeString, dt.toString(Qt::ISODateWithMs));
    QTest::newRow("Url:Empty") << QCborValue(QUrl())
                               << QCborValue(QCborKnownTags::Url, QString());
    QTest::newRow("Url:Authority") << QCborValue(QUrl("https://example.com"))
                                   << QCborValue(QCborKnownTags::Url, "https://example.com");
    QTest::newRow("Url:Path") << QCborValue(QUrl("file:///tmp/none"))
                              << QCborValue(QCborKnownTags::Url, "file:///tmp/none");
    QTest::newRow("Url:QueryFragment") << QCborValue(QUrl("whatever:?a=b&c=d#e"))
                                       << QCborValue(QCborKnownTags::Url, "whatever:?a=b&c=d#e");
    QTest::newRow("Regex:Empty") << QCborValue(QRegularExpression())
                                 << QCborValue(QCborKnownTags::RegularExpression, QString());
    QTest::newRow("Regex") << QCborValue(QRegularExpression("^.*$"))
                           << QCborValue(QCborKnownTags::RegularExpression, QString("^.*$"));
    QTest::newRow("Uuid") << QCborValue(uuid)
                          << QCborValue(QCborKnownTags::Uuid, uuid.toRfc4122());
}

void tst_QCborValue::extendedTypes()
{
    QFETCH(QCborValue, extended);
    QFETCH(QCborValue, tagged);
    QVERIFY(extended.isTag());
    QVERIFY(tagged.isTag());
    QVERIFY(extended == tagged);
    QVERIFY(tagged == extended);

    QCOMPARE(extended.tag(), tagged.tag());
    QCOMPARE(extended.taggedValue(), tagged.taggedValue());
}

void tst_QCborValue::copyCompare()
{
    QFETCH(QCborValue, v);
    QCborValue other = v;
    other = v;
    v = other;

    QCOMPARE(v.compare(other), 0);
    QCOMPARE(v, other);
    QVERIFY(!(v != other));
    QVERIFY(!(v < other));
#if 0 && QT_HAS_INCLUDE(<compare>)
    QVERIFY(v <= other);
    QVERIFY(v >= other);
    QVERIFY(!(v > other));
#endif

    if (v.isUndefined())
        other = nullptr;
    else
        other = {};
    QVERIFY(v.type() != other.type());
    QVERIFY(!(v == other));
    QVERIFY(v != other);

    // they're different types, so they can't compare equal
    QVERIFY(v.compare(other) != 0);
    QVERIFY((v < other) || (other < v));
}

void tst_QCborValue::arrayDefaultInitialization()
{
    QCborArray a;
    QVERIFY(a.isEmpty());
    QCOMPARE(a.size(), 0);
    QVERIFY(!a.contains(0));
    QVERIFY(!a.contains(-1));
    QVERIFY(!a.contains(false));
    QVERIFY(!a.contains(true));
    QVERIFY(!a.contains(nullptr));
    QVERIFY(!a.contains({}));
    QVERIFY(!a.contains(1.0));
    QVERIFY(!a.contains(QByteArray("Hello")));
    QVERIFY(!a.contains("Hello"));
    QVERIFY(!a.contains(QCborArray()));
    QVERIFY(!a.contains(QCborMap()));
    QVERIFY(!a.contains(QCborValue(QDateTime::currentDateTimeUtc())));
    QVERIFY(!a.contains(QCborValue(QUrl("http://example.com"))));
    QVERIFY(!a.contains(QCborValue(QUuid::createUuid())));

    QVERIFY(a.at(0).isUndefined());
    QCOMPARE(a.constBegin(), a.constEnd());

    QVERIFY(a == a);
    QVERIFY(a == QCborArray());
    QVERIFY(QCborArray() == a);

    QCborValue v(a);
    QVERIFY(v.isArray());
    QVERIFY(!v.isMap());
    QVERIFY(!v.isTag());
    QVERIFY(v[0].isUndefined());

    QCborArray a2 = v.toArray();
    QVERIFY(a2.isEmpty());
    QCOMPARE(a2, a);
}

void tst_QCborValue::mapDefaultInitialization()
{
    QCborMap m;
    QVERIFY(m.isEmpty());
    QCOMPARE(m.size(), 0);
    QVERIFY(m.keys().isEmpty());
    QVERIFY(!m.contains(0));
    QVERIFY(!m.contains(-1));
    QVERIFY(!m.contains(false));
    QVERIFY(!m.contains(true));
    QVERIFY(!m.contains(QCborValue::Null));
    QVERIFY(!m.contains({}));
    QVERIFY(!m.contains(1.0));
    QVERIFY(!m.contains(QLatin1String("Hello")));
    QVERIFY(!m.contains(QStringLiteral("Hello")));
    QVERIFY(!m.contains(QCborValue(QByteArray("Hello"))));
    QVERIFY(!m.contains(QCborArray()));
    QVERIFY(!m.contains(QCborMap()));
    QVERIFY(!m.contains(QCborValue(QDateTime::currentDateTimeUtc())));
    QVERIFY(!m.contains(QCborValue(QUrl("http://example.com"))));
    QVERIFY(!m.contains(QCborValue(QUuid::createUuid())));

    QVERIFY(m.value(0).isUndefined());
    QVERIFY(m.value(QLatin1String("Hello")).isUndefined());
    QVERIFY(m.value(QStringLiteral("Hello")).isUndefined());
    QVERIFY(m.value(QCborValue()).isUndefined());
#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    QVERIFY(m.value("Hello").isUndefined());
#endif

    QVERIFY(m == m);
    QVERIFY(m == QCborMap{});
    QVERIFY(QCborMap{} == m);

    QCborValue v(m);
    QVERIFY(v.isMap());
    QVERIFY(!v.isArray());
    QVERIFY(!v.isTag());
    QVERIFY(v[0].isUndefined());
    QVERIFY(v[QLatin1String("Hello")].isUndefined());
    QVERIFY(v["Hello"].isUndefined());

    QCborMap m2 = v.toMap();
    QVERIFY(m2.isEmpty());
    QCOMPARE(m2.size(), 0);
    QCOMPARE(m2, m);
}

void tst_QCborValue::arrayEmptyInitializerList()
{
    QCborArray a{};
    QVERIFY(a.isEmpty());
    QCOMPARE(a.size(), 0);
    QVERIFY(a == a);
    QVERIFY(a == QCborArray());
    QVERIFY(QCborArray() == a);
}

void tst_QCborValue::mapEmptyInitializerList()
{
    QCborMap m{};
    QVERIFY(m.isEmpty());
    QCOMPARE(m.size(), 0);
    QVERIFY(m == m);
    QVERIFY(m == QCborMap{});
    QVERIFY(QCborMap{} == m);
}

void tst_QCborValue::arrayEmptyDetach()
{
    QCborArray a;
    QCOMPARE(a.begin(), a.end());
    QVERIFY(a.isEmpty());
    QCOMPARE(a.size(), 0);

    QVERIFY(a == a);
    QVERIFY(a == QCborArray());
    QVERIFY(QCborArray() == a);

    QCborValue v(a);
    QVERIFY(v.isArray());
    QVERIFY(!v.isMap());
    QVERIFY(!v.isTag());

    QCborArray a2 = v.toArray();
    QVERIFY(a2.isEmpty());
    QCOMPARE(a2, a);
}

void tst_QCborValue::mapEmptyDetach()
{
    QCborMap m;
    QCOMPARE(m.begin(), m.end());
    QVERIFY(m.isEmpty());
    QCOMPARE(m.size(), 0);

    QVERIFY(m == m);
    QVERIFY(m == QCborMap{});
    QVERIFY(QCborMap{} == m);

    QCborValue v(m);
    QVERIFY(v.isMap());
    QVERIFY(!v.isArray());
    QVERIFY(!v.isTag());

    QCborMap m2 = v.toMap();
    QVERIFY(m2.isEmpty());
    QCOMPARE(m2, m);
}

void tst_QCborValue::arrayInitializerList()
{
    QCborArray a{0, -1, false, true, nullptr, {}, 1.0};
    QVERIFY(!a.isEmpty());
    QCOMPARE(a.size(), 7);
    QCOMPARE(a.at(0), QCborValue(0));
    QCOMPARE(a.at(1), QCborValue(-1));
    QCOMPARE(a.at(2), QCborValue(QCborValue::False));
    QCOMPARE(a.at(3), QCborValue(QCborValue::True));
    QCOMPARE(a.at(4), QCborValue(QCborValue::Null));
    QCOMPARE(a.at(5), QCborValue(QCborValue::Undefined));
    QCOMPARE(a.at(6), QCborValue(1.0));

    QVERIFY(a == a);
    QVERIFY(a != QCborArray{});
    QVERIFY(QCborArray{} != a);
    QVERIFY(a == QCborArray({0, -1, false, true, nullptr, {}, 1.0}));

    QCborValue v = a;
    QCOMPARE(v[0], QCborValue(0));
    QCOMPARE(v[1], QCborValue(-1));
    QCOMPARE(v[2], QCborValue(QCborValue::False));
    QCOMPARE(v[3], QCborValue(QCborValue::True));
    QCOMPARE(v[4], QCborValue(QCborValue::Null));
    QCOMPARE(v[5], QCborValue(QCborValue::Undefined));
    QCOMPARE(v[6], QCborValue(1.0));

    QVERIFY(a.contains(0));
    QVERIFY(a.contains(-1));
    QVERIFY(a.contains(false));
    QVERIFY(a.contains(true));
    QVERIFY(a.contains(nullptr));
    QVERIFY(a.contains({}));
    QVERIFY(a.contains(1.0));
    QVERIFY(!a.contains(QByteArray("Hello")));
    QVERIFY(!a.contains("Hello"));
    QVERIFY(!a.contains(QCborArray()));
    QVERIFY(!a.contains(QCborMap()));
    QVERIFY(!a.contains(QCborValue(QDateTime::currentDateTimeUtc())));
    QVERIFY(!a.contains(QCborValue(QUrl("http://example.com"))));
    QVERIFY(!a.contains(QCborValue(QUuid::createUuid())));

    // iterators
    auto it = a.constBegin();
    auto end = a.constEnd();
    QCOMPARE(end - it, 7);
    QCOMPARE(it + 7, end);
    QVERIFY(it->isInteger());
    QCOMPARE(*it, QCborValue(0));
    QCOMPARE(it[1], QCborValue(-1));
    QCOMPARE(*(it + 2), QCborValue(false));
    it += 3;
    QCOMPARE(*it, QCborValue(true));
    ++it;
    QCOMPARE(*it, QCborValue(nullptr));
    it++;
    QCOMPARE(*it, QCborValue());
    --end;
    QCOMPARE(*end, QCborValue(1.0));
    end--;
    QCOMPARE(it, end);

    // range for
    int i = 0;
    for (const QCborValue &v : qAsConst(a)) {
        QVERIFY(!v.isInvalid());
        QCOMPARE(v.isUndefined(), i == 5); // 6th element is Undefined
        ++i;
    }
    QCOMPARE(i, a.size());
}

void tst_QCborValue::mapSimpleInitializerList()
{
    QCborMap m{{0, 0}, {1, 0}, {2, "Hello"}, {"Hello", 2}, {3, QLatin1String("World")}, {QLatin1String("World"), 3}};
    QCOMPARE(m.size(), 6);
    QVERIFY(m == m);
    QVERIFY(m != QCborMap{});
    QVERIFY(QCborMap{} != m);
    QVERIFY(m == QCborMap({{0, 0}, {1, 0}, {2, "Hello"}, {"Hello", 2}, {3, QLatin1String("World")}, {QLatin1String("World"), 3}}));

    QCborValue vmap = m;
    {
        QVERIFY(m.contains(0));
        QCborValue v = m.value(0);
        QVERIFY(v.isInteger());
        QCOMPARE(v.toInteger(), 0);
        QCOMPARE(vmap[0], v);
    }
    {
        QVERIFY(m.contains(1));
        QCborValue v = m.value(1);
        QVERIFY(v.isInteger());
        QCOMPARE(v.toInteger(), 0);
        QCOMPARE(vmap[1], v);
    }
    {
        QVERIFY(m.contains(2));
        QCborValue v = m.value(2);
        QVERIFY(v.isString());
        QCOMPARE(v.toString(), "Hello");
        QCOMPARE(vmap[2], v);
    }
    {
        QVERIFY(m.contains(3));
        QCborValue v = m.value(3);
        QVERIFY(v.isString());
        QCOMPARE(v.toString(), "World");
        QCOMPARE(vmap[3], v);
    }
    {
        QVERIFY(m.contains(QStringLiteral("Hello")));
        QCborValue v = m.value(QLatin1String("Hello"));
        QVERIFY(v.isInteger());
        QCOMPARE(v.toInteger(), 2);
        QCOMPARE(vmap[QStringLiteral("Hello")], v);
    }
    {
        QVERIFY(m.contains(QLatin1String("World")));
        QCborValue v = m.value(QStringLiteral("World"));
        QVERIFY(v.isInteger());
        QCOMPARE(v.toInteger(), 3);
        QCOMPARE(vmap[QLatin1String("World")], v);
    }

    QVERIFY(!m.contains(QCborValue::Null));
    QVERIFY(!m.contains(QCborValue()));
    QVERIFY(!m.contains(QCborValue(1.0)));  // Important: 1.0 does not match 1
    QVERIFY(!m.contains(QCborValue(QByteArray("Hello"))));
    QVERIFY(!m.contains(QCborArray()));
    QVERIFY(!m.contains(QCborMap()));
    QVERIFY(!m.contains(QCborValue(QDateTime::currentDateTimeUtc())));
    QVERIFY(!m.contains(QCborValue(QUrl("http://example.com"))));
    QVERIFY(!m.contains(QCborValue(QUuid::createUuid())));

    // iterators (QCborMap is not sorted)
    auto it = m.constBegin();
    auto end = m.constEnd();
    QCOMPARE(end - it, 6);
    QCOMPARE(it + 6, end);
    QCOMPARE(it.key(), QCborValue(0));
    QCOMPARE(it.value(), QCborValue(0));
    QVERIFY(it->isInteger());
    ++it;
    QCOMPARE(it.key(), QCborValue(1));
    QCOMPARE(it.value(), QCborValue(0));
    QCOMPARE((it + 1).key(), QCborValue(2));
    QVERIFY((it + 1)->isString());
    QCOMPARE((it + 1)->toString(), "Hello");
    it += 2;
    QCOMPARE(it.key(), QCborValue("Hello"));
    QVERIFY(it->isInteger());
    it++;
    QCOMPARE(it.key(), QCborValue(3));
    QVERIFY(it->isString());
    QCOMPARE(it.value().toString(), "World");
    --end;
    QCOMPARE(end.key(), QCborValue("World"));
    QCOMPARE(end.value(), QCborValue(3));
    end--;
    QCOMPARE(it, end);

    // range for
    int i = 0;
    for (auto pair : qAsConst(m)) {
        QVERIFY(!pair.first.isUndefined());
        QVERIFY(!pair.second.isUndefined());
        ++i;
    }
    QCOMPARE(i, m.size());
}

void tst_QCborValue::arrayMutation()
{
    QCborArray a{42};
    {
        QCborValueRef v = a[0];
        QVERIFY(!a.isEmpty());
        QVERIFY(v.isInteger());
        QCOMPARE(v.toInteger(), 42);

        // now mutate the list
        v = true;
        QVERIFY(v.isBool());
        QVERIFY(v.isTrue());
        QVERIFY(a.at(0).isTrue());
        QVERIFY(a.at(0) == v);
        QVERIFY(v == a.at(0));
    }

    QVERIFY(a == a);
    QVERIFY(a == QCborArray{true});

    QCborArray a2 = a;
    a.append(nullptr);
    QCOMPARE(a.size(), 2);
    QCOMPARE(a2.size(), 1);

    // self-insertion
    a2.append(a2);
    QCOMPARE(a2.size(), 2);
    QCOMPARE(a2.last().toArray().size(), 1);

    QCborValueRef v = a[0];
    QVERIFY(v.isTrue());
    v = 2.5;
    QVERIFY(v.isDouble());
    QVERIFY(a.first().isDouble());
    QVERIFY(a.last().isNull());
    QVERIFY(a2.first().isTrue());

    a2 = a;
    auto it = a.begin();    // detaches again
    auto end = a.end();
    QCOMPARE(end - it, 2);
    QCOMPARE(it + 2, end);
    QCOMPARE(*it, QCborValue(2.5));
    QCOMPARE(*++it, QCborValue(nullptr));
    QVERIFY(a2 == a);
    QVERIFY(a == a2);

    *it = -1;
    QCOMPARE(*it, QCborValue(-1));
    QCOMPARE(a.at(1), QCborValue(-1));
    QCOMPARE(a2.at(1), QCborValue(nullptr));
    QCOMPARE(++it, end);
}

void tst_QCborValue::mapMutation()
{
    QCborMap m;
    QVERIFY(m.isEmpty());

    {
        QCborValueRef v = m[42];
        QCOMPARE(m.size(), 1);
        QVERIFY(v.isUndefined());

        // now mutate the list
        v = true;
        QVERIFY(v.isBool());
        QVERIFY(v.isTrue());
        QVERIFY(m.begin()->isTrue());
        QVERIFY(m.begin().value() == v);
        QVERIFY(v == m.begin().value());
    }

    QVERIFY(m == QCborMap({{42, true}}));
    QVERIFY(QCborMap({{42, true}}) == m);

    QCborMap m2 = m;
    m.insert({nullptr, nullptr});
    QCOMPARE(m.size(), 2);
    QCOMPARE(m2.size(), 1);

    QCborValueRef v = m[42];
    QVERIFY(v.isTrue());
    v = 2.5;
    QVERIFY(v.isDouble());
    QVERIFY(m.begin()->isDouble());
    QVERIFY((m.end() - 1)->isNull());
    QVERIFY(m2.begin()->isTrue());

    m2 = m;
    auto it = m.begin();    // detaches again
    auto end = m.end();
    QCOMPARE(end - it, 2);
    QCOMPARE(it + 2, end);
    QCOMPARE(it.key(), QCborValue(42));
    QCOMPARE(it.value(), QCborValue(2.5));
    QCOMPARE((++it).value(), QCborValue(nullptr));
    QCOMPARE(it.key(), QCborValue(nullptr));
    QVERIFY(m2 == m);
    QVERIFY(m == m2);

    it.value() = -1;
    QCOMPARE(it.key(), QCborValue(nullptr));
    QCOMPARE(it.value(), QCborValue(-1));
    QCOMPARE((m.end() - 1)->toInteger(), -1);
    QVERIFY((m2.end() - 1)->isNull());
    QCOMPARE(++it, end);
}

void tst_QCborValue::arrayPrepend()
{
    QCborArray a;
    a.prepend(0);
    a.prepend(nullptr);
    QCOMPARE(a.at(1), QCborValue(0));
    QCOMPARE(a.at(0), QCborValue(nullptr));
    QCOMPARE(a.size(), 2);
}

void tst_QCborValue::arrayInsertRemove()
{
    QFETCH(QCborValue, v);
    QCborArray a;
    a.append(42);
    a.append(v);
    a.insert(1, QCborValue(nullptr));
    QCOMPARE(a.at(0), QCborValue(42));
    QCOMPARE(a.at(1), QCborValue(nullptr));
    QCOMPARE(a.at(2), v);

    // remove 42
    a.removeAt(0);
    QCOMPARE(a.size(), 2);
    QCOMPARE(a.at(0), QCborValue(nullptr));
    QCOMPARE(a.at(1), v);

    auto it = a.begin();
    it = a.erase(it);   // removes nullptr
    QCOMPARE(a.size(), 1);
    QCOMPARE(a.at(0), v);

    it = a.erase(it);
    QVERIFY(a.isEmpty());
    QCOMPARE(it, a.end());

    // reinsert the element so we can take it
    a.append(v);
    QCOMPARE(a.takeAt(0), v);
    QVERIFY(a.isEmpty());
}

void tst_QCborValue::arrayStringElements()
{
    QCborArray a{"Hello"};
    a.append(QByteArray("Hello"));
    a.append(QLatin1String("World"));
    QVERIFY(a == a);
    QVERIFY(a == QCborArray({QLatin1String("Hello"),
                             QByteArray("Hello"), QStringLiteral("World")}));

    QCborValueRef r1 = a[0];
    QCOMPARE(r1.toString(), "Hello");
    QCOMPARE(r1.operator QCborValue(), QCborValue("Hello"));
    QVERIFY(r1 == QCborValue("Hello"));

    QCborValue v2 = a.at(1);
    QCOMPARE(v2.toByteArray(), QByteArray("Hello"));
    QCOMPARE(v2, QCborValue(QByteArray("Hello")));

    // v2 must continue to be valid after the entry getting removed
    a.removeAt(1);
    QCOMPARE(v2.toByteArray(), QByteArray("Hello"));
    QCOMPARE(v2, QCborValue(QByteArray("Hello")));

    v2 = a.at(1);
    QCOMPARE(v2.toString(), "World");
    QCOMPARE(v2, QCborValue("World"));

    QCOMPARE(a.takeAt(1).toString(), "World");
    QCOMPARE(a.takeAt(0).toString(), "Hello");
    QVERIFY(a.isEmpty());
}

void tst_QCborValue::mapStringValues()
{
    QCborMap m{{0, "Hello"}};
    m.insert({1, QByteArray("Hello")});
    m.insert({2, QLatin1String("World")});
    QVERIFY(m == m);

    QCborValueRef r1 = m[0];
    QCOMPARE(r1.toString(), "Hello");
    QCOMPARE(r1.operator QCborValue(), QCborValue("Hello"));
    QVERIFY(r1 == QCborValue("Hello"));

    QCborValue v2 = m.value(1);
    QCOMPARE(v2.toByteArray(), QByteArray("Hello"));
    QCOMPARE(v2, QCborValue(QByteArray("Hello")));

    // v2 must continue to be valid after the entry getting removed
    m.erase(m.constFind(1));
    QCOMPARE(v2.toByteArray(), QByteArray("Hello"));
    QCOMPARE(v2, QCborValue(QByteArray("Hello")));

    v2 = (m.begin() + 1).value();
    QCOMPARE(v2.toString(), "World");
    QCOMPARE(v2, QCborValue("World"));

    QCOMPARE(m.extract(m.begin() + 1).toString(), "World");
    QCOMPARE(m.take(0).toString(), "Hello");
    QVERIFY(m.isEmpty());
}

void tst_QCborValue::mapStringKeys()
{
    QCborMap m{{QLatin1String("Hello"), 1}, {QStringLiteral("World"), 2}};
    QCOMPARE(m.value(QStringLiteral("Hello")), QCborValue(1));
    QCOMPARE(m.value(QLatin1String("World")), QCborValue(2));

    QCborMap m2 = m;
    QVERIFY(m2 == m);
    QVERIFY(m == m2);

    m.insert({QByteArray("foo"), "bar"});
    QCOMPARE(m.size(), 3);
    QCOMPARE(m2.size(), 2);
    QVERIFY(m2 != m);
    QVERIFY(m != m2);

    QVERIFY(m2.value(QCborValue(QByteArray("foo"))).isUndefined());
    QVERIFY(m.value(QCborValue(QLatin1String("foo"))).isUndefined());
    QCOMPARE(m.value(QCborValue(QByteArray("foo"))).toString(), "bar");
}

void tst_QCborValue::mapInsertRemove()
{
    QFETCH(QCborValue, v);
    QCborMap m{{1, v}};

    m.remove(1);
    QVERIFY(m.isEmpty());
    QVERIFY(!m.contains(1));

    m.insert(2, v);
    QVERIFY(m.contains(2));
    QVERIFY(m[2] == v);
    QVERIFY(v == m[2]);

    auto it = m.find(2);
    it = m.erase(it);
    QVERIFY(m.isEmpty());

    // creates m[2] and m[42] just by referencing them
    m[2];
    QCborValueRef r = m[42];
    QCOMPARE(m.size(), 2);

    r = v;
    it = m.find(42);
    QVERIFY(it.value() == v);
    QVERIFY(v == it.value());
    QVERIFY(it.value() == r);
    QVERIFY(r == it.value());

    QCOMPARE(m.extract(it), v);
    QVERIFY(!m.contains(42));

    m[2] = v;
    QCOMPARE(m.take(2), v);
    QVERIFY(m.take(2).isUndefined());
    QVERIFY(m.isEmpty());
}

void tst_QCborValue::arrayInsertTagged()
{
    QFETCH(QCborValue, v);

    // make it tagged
    QCborValue tagged(QCborKnownTags::Signature, v);

    QCborArray a{tagged};
    a.insert(1, tagged);
    QCOMPARE(a.size(), 2);
    QCOMPARE(a.at(0), tagged);
    QCOMPARE(a.at(1), tagged);
    QCOMPARE(a.at(0).taggedValue(), v);
    QCOMPARE(a.at(1).taggedValue(), v);
    QCOMPARE(a.takeAt(0).taggedValue(), v);
    QCOMPARE(a.takeAt(0).taggedValue(), v);
    QVERIFY(a.isEmpty());
}

void tst_QCborValue::mapInsertTagged()
{
    QFETCH(QCborValue, v);

    // make it tagged
    QCborValue tagged(QCborKnownTags::Signature, v);

    QCborMap m{{11, tagged}};
    m.insert({-21, tagged});
    QCOMPARE(m.size(), 2);
    QCOMPARE(m.constBegin().value(), tagged);
    QCOMPARE(m.value(-21), tagged);
    QCOMPARE(m.value(11).taggedValue(), v);
    QCOMPARE((m.end() - 1).value().taggedValue(), v);
    QCOMPARE(m.extract(m.end() - 1).taggedValue(), v);
    QVERIFY(!m.contains(-21));
    QCOMPARE(m.take(11).taggedValue(), v);
    QVERIFY(m.isEmpty());
}

void tst_QCborValue::arraySelfAssign()
{
    QFETCH(QCborValue, v);
    QCborArray a;

    a = {v};

    // Test 1: QCborValue created first, so
    // QCborArray::insert() detaches
    {
        a.append(a);
        QCOMPARE(a.size(), 2);
        QCOMPARE(a.last().toArray().size(), 1);
    }

    a = {v};

    // Test 2: QCborValueRef created first
    {
        a.append(36);
        auto it = a.end() - 1;
        *it = a;

        QCOMPARE(a.size(), 2);
        QCOMPARE(it->toArray().size(), 2);
        QCOMPARE(it->toArray().last(), QCborValue(36));
    }
}

void tst_QCborValue::mapSelfAssign()
{
    QFETCH(QCborValue, v);
    QCborMap m;

    m = {{0, v}};
    QCOMPARE(m.size(), 1);

    // Test 1: create a QCborValue first
    // in this case, QCborMap::operator[] detaches first
    {
        QCborValue vm = m;
        m[1] = vm;      // self-assign
        QCOMPARE(m.size(), 2);
        QCOMPARE(m.value(0), v);

        QCborMap m2 = m.value(1).toMap();
        // there mustn't be an element with key 1
        QCOMPARE(m2.size(), 1);
        QCOMPARE(m2.value(0), v);
        QVERIFY(!m2.contains(1));
    }

    m = {{0, v}};

    // Test 2: create the QCborValueRef first
    // in this case, there's no opportunity to detach
    {
        QCborValueRef rv = m[1];
        rv = m;     // self-assign (implicit QCborValue creation)
        QCOMPARE(m.size(), 2);
        QCOMPARE(m.value(0), v);

        QCborMap m2 = m.value(1).toMap();
        // there must be an element with key 1
        QCOMPARE(m2.size(), 2);
        QCOMPARE(m2.value(0), v);
        QVERIFY(m2.contains(1));
        QCOMPARE(m2.value(1), QCborValue());
    }

    m = {{0, v}};

    // Test 3: don't force creation of either before
    // in this case, it's up to the compiler to choose
    {
        m[1] = m;   // self-assign
        QCOMPARE(m.size(), 2);

        QCborMap m2 = m.value(1).toMap();
        QVERIFY(m2.size() == 1 || m2.size() == 2);
    }

    m = {{0, v}};

    // Test 4: self-assign as key
    // in this scase, QCborMap::operator[] must detach
    {
        m[m] = v;
        QCOMPARE(m.size(), 2);

        auto it = m.constEnd() - 1;
        QCOMPARE(it.value(), v);
        QCOMPARE(it.key(), QCborMap({{0, v}}));
    }
}

void tst_QCborValue::mapComplexKeys()
{
    QFETCH(QCborValue, v);
    QCborValue tagged(QCborKnownTags::Signature, v);

    QCborMap m{{42, true}, {v, 42}, {-3, nullptr}};
    QCOMPARE(m.size(), 3);
    QVERIFY(m.contains(42));
    QVERIFY(m.contains(-3));
    QVERIFY(m.contains(v));
    QVERIFY(!m.contains(tagged));

    auto it = m.constFind(v);
    QVERIFY(it != m.constEnd());
    QVERIFY(it.key() == v);
    QVERIFY(v == it.key());
    QCOMPARE(it.value().toInteger(), 42);

    QCborArray a{0, 1, 2, 3, v};
    m[a] = 1;
    QCOMPARE(m.size(), 4);
    QCOMPARE((m.constEnd() - 1).value(), QCborValue(1));
    if (v != QCborValue(QCborValue::Array))
        QVERIFY(!m.contains(QCborArray{}));
    QVERIFY(!m.contains(QCborArray{0}));
    QVERIFY(!m.contains(QCborArray{0, 1}));
    QVERIFY(!m.contains(QCborArray{0, 1, 2}));
    QVERIFY(!m.contains(QCborArray{0, 1, 2, 4}));
    QVERIFY(!m.contains(QCborArray{0, 1, 2, 3, v, 4}));

    it = m.constFind(QCborArray{0, 1, 2, 3, v});
    QVERIFY(it != m.constEnd());
    QCOMPARE(it.key(), a);
    QCOMPARE(it.value(), QCborValue(1));

    m[m] = 1;   // assign itself as a key -- this necessarily detaches before
    QCOMPARE(m.size(), 5);
    QCOMPARE((m.end() - 1).value(), 1);
    QCOMPARE((m.end() - 1).key().toMap().size(), 4);

    QCborValue mv(m);
    if (v.isInteger()) {
        // we should be able to find using the overloads too
        QCOMPARE(m[v.toInteger()].toInteger(), 42);
        QCOMPARE(mv[v.toInteger()].toInteger(), 42);
    } else if (v.isString()) {
        // ditto
        QCOMPARE(m[v.toString()].toInteger(), 42);
        QCOMPARE(mv[v.toString()].toInteger(), 42);

        // basics_data() strings are Latin1
        QByteArray latin1 = v.toString().toLatin1();
        Q_ASSERT(v.toString() == QString::fromLatin1(latin1));
        QCOMPARE(m[QLatin1String(latin1)].toInteger(), 42);
    }

    m.remove(v);
    QVERIFY(!m.contains(v));
    QVERIFY(!m.contains(tagged));

    QCborValueRef r = m[tagged];
    QVERIFY(!m.contains(v));
    QVERIFY(m.contains(tagged));
    r = 47;
    QCOMPARE(m[tagged].toInteger(), 47);
    QCOMPARE(m.take(tagged).toInteger(), 47);
    QVERIFY(!m.contains(tagged));
}

void tst_QCborValue::sorting()
{
    QCborValue vundef, vnull(nullptr);
    QCborValue vtrue(true), vfalse(false);
    QCborValue vint1(1), vint2(2);
    QCborValue vneg1(-1), vneg2(-2);
    QCborValue vba2(QByteArray("Hello")), vba3(QByteArray("World")), vba1(QByteArray("foo"));
    QCborValue vs2("Hello"), vs3("World"), vs1("foo");
    QCborValue va1(QCborValue::Array), va2(QCborArray{1}), va3(QCborArray{0, 0});
    QCborValue vm1(QCborValue::Map), vm2(QCborMap{{1, 0}}), vm3(QCborMap{{0, 0}, {1, 0}});
    QCborValue vdt1(QDateTime::fromMSecsSinceEpoch(0, Qt::UTC)), vdt2(QDateTime::currentDateTimeUtc());
    QCborValue vtagged1(QCborKnownTags::UnixTime_t, 0), vtagged2(QCborKnownTags::UnixTime_t, 0.0),
            vtagged3(QCborKnownTags::Signature, 0), vtagged4(QCborTag(-2), 0), vtagged5(QCborTag(-1), 0);
    QCborValue vurl1(QUrl("https://example.net")), vurl2(QUrl("https://example.com/"));
    QCborValue vuuid1{QUuid()}, vuuid2(QUuid::createUuid());
    QCborValue vsimple1(QCborSimpleType(1)), vsimple32(QCborSimpleType(32)), vsimple255(QCborSimpleType(255));
    QCborValue vdouble1(1.5), vdouble2(qInf());
    QCborValue vndouble1(-1.5), vndouble2(-qInf());

#define CHECK_ORDER(v1, v2) \
    QVERIFY(v1 < v2); \
    QVERIFY(!(v2 < v2))

    // intra-type comparisons
    CHECK_ORDER(vfalse, vtrue);
    CHECK_ORDER(vsimple1, vsimple32);
    CHECK_ORDER(vsimple32, vsimple255);
    CHECK_ORDER(vint1, vint2);
    CHECK_ORDER(vdouble1, vdouble2);
    CHECK_ORDER(vndouble1, vndouble2);
    // note: shorter length sorts first
    CHECK_ORDER(vba1, vba2);
    CHECK_ORDER(vba2, vba3);
    CHECK_ORDER(vs1, vs2);
    CHECK_ORDER(vs2, vs3);
    CHECK_ORDER(va1, va2);
    CHECK_ORDER(va2, va3);
    CHECK_ORDER(vm1, vm2);
    CHECK_ORDER(vm2, vm3);
    CHECK_ORDER(vdt1, vdt2);
    CHECK_ORDER(vtagged1, vtagged2);
    CHECK_ORDER(vtagged2, vtagged3);
    CHECK_ORDER(vtagged3, vtagged4);
    CHECK_ORDER(vtagged4, vtagged5);
    CHECK_ORDER(vurl1, vurl2);
    CHECK_ORDER(vuuid1, vuuid2);

    // surprise 1: CBOR sorts integrals by absolute value
    CHECK_ORDER(vneg1, vneg2);

    // surprise 2: CBOR sorts negatives after positives (sign+magnitude)
    CHECK_ORDER(vint2, vneg1);
    QVERIFY(vint2.toInteger() > vneg1.toInteger());
    CHECK_ORDER(vdouble2, vndouble1);
    QVERIFY(vdouble2.toDouble() > vndouble1.toDouble());

    // inter-type comparisons
    CHECK_ORDER(vneg2, vba1);
    CHECK_ORDER(vba3, vs1);
    CHECK_ORDER(vs3, va1);
    CHECK_ORDER(va2, vm1);
    CHECK_ORDER(vm2, vdt1);
    CHECK_ORDER(vdt2, vtagged1);
    CHECK_ORDER(vtagged2, vurl1);
    CHECK_ORDER(vurl1, vuuid1);
    CHECK_ORDER(vuuid2, vtagged3);
    CHECK_ORDER(vtagged4, vsimple1);
    CHECK_ORDER(vsimple1, vfalse);
    CHECK_ORDER(vtrue, vnull);
    CHECK_ORDER(vnull, vundef);
    CHECK_ORDER(vundef, vsimple32);
    CHECK_ORDER(vsimple255, vdouble1);

    // which shows all doubles sorted after integrals
    CHECK_ORDER(vint2, vdouble1);
    QVERIFY(vint2.toInteger() > vdouble1.toDouble());
#undef CHECK_ORDER
}

static void addCommonCborData()
{
    // valid for both decoding and encoding
    QTest::addColumn<QCborValue>("v");
    QTest::addColumn<QByteArray>("result");
    QTest::addColumn<QCborValue::EncodingOptions>("options");
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUuid uuid = QUuid::createUuid();
    QCborValue::EncodingOptions noxfrm = QCborValue::NoTransformation;

    // integrals
    QTest::newRow("Integer:0") << QCborValue(0) << raw("\x00") << noxfrm;
    QTest::newRow("Integer:1") << QCborValue(1) << raw("\x01") << noxfrm;
    QTest::newRow("Integer:-1") << QCborValue(-1) << raw("\x20") << noxfrm;
    QTest::newRow("Integer:INT64_MAX") << QCborValue(std::numeric_limits<qint64>::max())
                                       << raw("\x1b\x7f\xff\xff\xff""\xff\xff\xff\xff")
                                       << noxfrm;
    QTest::newRow("Integer:INT64_MIN") << QCborValue(std::numeric_limits<qint64>::min())
                                       << raw("\x3b\x7f\xff\xff\xff""\xff\xff\xff\xff")
                                       << noxfrm;

    QTest::newRow("simple0") << QCborValue(QCborValue::SimpleType) << raw("\xe0") << noxfrm;
    QTest::newRow("simple1") << QCborValue(QCborSimpleType(1)) << raw("\xe1") << noxfrm;
    QTest::newRow("simple255") << QCborValue(QCborSimpleType(255)) << raw("\xf8\xff") << noxfrm;
    QTest::newRow("Undefined") << QCborValue() << raw("\xf7") << noxfrm;
    QTest::newRow("Null") << QCborValue(nullptr) << raw("\xf6") << noxfrm;
    QTest::newRow("True") << QCborValue(true) << raw("\xf5") << noxfrm;
    QTest::newRow("False") << QCborValue(false) << raw("\xf4") << noxfrm;
    QTest::newRow("simple32") << QCborValue(QCborSimpleType(32)) << raw("\xf8\x20") << noxfrm;
    QTest::newRow("simple255") << QCborValue(QCborSimpleType(255)) << raw("\xf8\xff") << noxfrm;

    QTest::newRow("Double:0") << QCborValue(0.) << raw("\xfb\0\0\0\0""\0\0\0\0") << noxfrm;
    QTest::newRow("Double:1.5") << QCborValue(1.5) << raw("\xfb\x3f\xf8\0\0""\0\0\0\0") << noxfrm;
    QTest::newRow("Double:-1.5") << QCborValue(-1.5) << raw("\xfb\xbf\xf8\0\0""\0\0\0\0") << noxfrm;
    QTest::newRow("Double:INT64_MAX+1") << QCborValue(std::numeric_limits<qint64>::max() + 1.)
                                            << raw("\xfb\x43\xe0\0\0""\0\0\0\0") << noxfrm;
    QTest::newRow("Double:maxintegralfp") << QCborValue(18446744073709551616.0 - 2048)
                                          << raw("\xfb\x43\xef\xff\xff""\xff\xff\xff\xff")
                                          << noxfrm;
    QTest::newRow("Double:minintegralfp") << QCborValue(-18446744073709551616.0 + 2048)
                                          << raw("\xfb\xc3\xef\xff\xff""\xff\xff\xff\xff")
                                          << noxfrm;
    QTest::newRow("Double:inf") << QCborValue(qInf()) << raw("\xfb\x7f\xf0\0\0""\0\0\0\0") << noxfrm;
    QTest::newRow("Double:-inf") << QCborValue(-qInf()) << raw("\xfb\xff\xf0\0""\0\0\0\0\0") << noxfrm;
    QTest::newRow("Double:nan") << QCborValue(qQNaN()) << raw("\xfb\x7f\xf8\0\0""\0\0\0\0") << noxfrm;

    QTest::newRow("Float:0") << QCborValue(0.) << raw("\xfa\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat);
    QTest::newRow("Float:1.5") << QCborValue(1.5) << raw("\xfa\x3f\xc0\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat);
    QTest::newRow("Float:-1.5") << QCborValue(-1.5) << raw("\xfa\xbf\xc0\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat);
    QTest::newRow("Float:inf") << QCborValue(qInf()) << raw("\xfa\x7f\x80\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat);
    QTest::newRow("Float:-inf") << QCborValue(-qInf()) << raw("\xfa\xff\x80\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat);
    QTest::newRow("Float:nan") << QCborValue(qQNaN()) << raw("\xfa\x7f\xc0\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat);

    QTest::newRow("Float16:0") << QCborValue(0.) << raw("\xf9\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat16);
    QTest::newRow("Float16:1.5") << QCborValue(1.5) << raw("\xf9\x3e\0") << QCborValue::EncodingOptions(QCborValue::UseFloat16);
    QTest::newRow("Float16:-1.5") << QCborValue(-1.5) << raw("\xf9\xbe\0") << QCborValue::EncodingOptions(QCborValue::UseFloat16);
    QTest::newRow("Float16:inf") << QCborValue(qInf()) << raw("\xf9\x7c\0") << QCborValue::EncodingOptions(QCborValue::UseFloat16);
    QTest::newRow("Float16:-inf") << QCborValue(-qInf()) << raw("\xf9\xfc\0") << QCborValue::EncodingOptions(QCborValue::UseFloat16);
    QTest::newRow("Float16:nan") << QCborValue(qQNaN()) << raw("\xf9\x7e\0") << QCborValue::EncodingOptions(QCborValue::UseFloat16);

    // out of range of qint64, but in range for CBOR, so these do get converted
    // to integrals on write and back to double on read
    QTest::newRow("UseInteger:INT64_MAX+1") << QCborValue(std::numeric_limits<qint64>::max() + 1.)
                                            << raw("\x1b\x80\0\0\0""\0\0\0\0")
                                            << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:maxintegralfp") << QCborValue(18446744073709551616.0 - 2048)
                                              << raw("\x1b\xff\xff\xff\xff""\xff\xff\xf8\0")
                                              << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:minintegralfp") << QCborValue(-18446744073709551616.0 + 2048)
                                              << raw("\x3b\xff\xff\xff\xff""\xff\xff\xf7\xff")
                                              << QCborValue::EncodingOptions(QCborValue::UseIntegers);

    QTest::newRow("ByteArray:Empty") << QCborValue(QByteArray()) << raw("\x40") << noxfrm;
    QTest::newRow("ByteArray") << QCborValue(QByteArray("Hello")) << raw("\x45Hello") << noxfrm;
    QTest::newRow("ByteArray:WithNull") << QCborValue(raw("\0\1\2\xff")) << raw("\x44\0\1\2\xff") << noxfrm;

    QTest::newRow("String:Empty") << QCborValue(QString()) << raw("\x60") << noxfrm;
    QTest::newRow("String:UsAscii") << QCborValue("Hello") << raw("\x65Hello") << noxfrm;
    QTest::newRow("String:Latin1") << QCborValue(QLatin1String("R\xe9sum\xe9"))
                                     << raw("\x68R\xc3\xa9sum\xc3\xa9") << noxfrm;
    QTest::newRow("String:Unicode") << QCborValue(QStringLiteral(u"éś α €"))
                                     << raw("\x6b\xc3\xa9\xc5\x9b \xce\xb1 \xe2\x82\xac") << noxfrm;

    QTest::newRow("DateTime") << QCborValue(dt)             // this is UTC
                              << "\xc0\x78\x18" + dt.toString(Qt::ISODateWithMs).toLatin1()
                              << noxfrm;
    QTest::newRow("DateTime-UTC") << QCborValue(QDateTime({2018, 1, 1}, {9, 0, 0}, Qt::UTC))
                                  << raw("\xc0\x78\x18" "2018-01-01T09:00:00.000Z")
                                  << noxfrm;
    QTest::newRow("DateTime-Local") << QCborValue(QDateTime({2018, 1, 1}, {9, 0, 0}, Qt::LocalTime))
                                    << raw("\xc0\x77" "2018-01-01T09:00:00.000")
                                    << noxfrm;
    QTest::newRow("DateTime+01:00") << QCborValue(QDateTime({2018, 1, 1}, {9, 0, 0}, Qt::OffsetFromUTC, 3600))
                                    << raw("\xc0\x78\x1d" "2018-01-01T09:00:00.000+01:00")
                                    << noxfrm;
    QTest::newRow("Url:Empty") << QCborValue(QUrl()) << raw("\xd8\x20\x60") << noxfrm;
    QTest::newRow("Url") << QCborValue(QUrl("HTTPS://example.com/{%30%31}?q=%3Ca+b%20%C2%A9%3E&%26"))
                         << raw("\xd8\x20\x78\x27" "https://example.com/{01}?q=<a+b \xC2\xA9>&%26")
                         << noxfrm;
    QTest::newRow("Regex:Empty") << QCborValue(QRegularExpression()) << raw("\xd8\x23\x60") << noxfrm;
    QTest::newRow("Regex") << QCborValue(QRegularExpression("^.*$"))
                           << raw("\xd8\x23\x64" "^.*$") << noxfrm;
    QTest::newRow("Uuid") << QCborValue(uuid) << raw("\xd8\x25\x50") + uuid.toRfc4122() << noxfrm;

    // empty arrays and maps
    QTest::newRow("Array") << QCborValue(QCborArray()) << raw("\x80") << noxfrm;
    QTest::newRow("Map") << QCborValue(QCborMap()) << raw("\xa0") << noxfrm;

    QTest::newRow("Tagged:ByteArray") << QCborValue(QCborKnownTags::PositiveBignum, raw("\1\0\0\0\0""\0\0\0\0"))
                                      << raw("\xc2\x49\1\0\0\0\0""\0\0\0\0") << noxfrm;
    QTest::newRow("Tagged:Array") << QCborValue(QCborKnownTags::Decimal, QCborArray{-2, 27315})
                                  << raw("\xc4\x82\x21\x19\x6a\xb3") << noxfrm;
}

void tst_QCborValue::toCbor_data()
{
    addCommonCborData();

    // The rest of these tests are conversions whose decoding does not yield
    // back the same QCborValue.

    // Signalling NaN get normalized to quiet ones
    QTest::newRow("Double:snan") << QCborValue(qSNaN()) << raw("\xfb\x7f\xf8\0""\0\0\0\0\0") << QCborValue::EncodingOptions();
    QTest::newRow("Float:snan") << QCborValue(qSNaN()) << raw("\xfa\x7f\xc0\0\0") << QCborValue::EncodingOptions(QCborValue::UseFloat);
    QTest::newRow("Float16:snan") << QCborValue(qSNaN()) << raw("\xf9\x7e\0") << QCborValue::EncodingOptions(QCborValue::UseFloat16);

    // Floating point written as integers are read back as integers
    QTest::newRow("UseInteger:0") << QCborValue(0.) << raw("\x00") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:1") << QCborValue(1.) << raw("\x01") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:-1") << QCborValue(-1.) << raw("\x20") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:INT64_MIN") << QCborValue(std::numeric_limits<qint64>::min() + 0.)
                                          << raw("\x3b\x7f\xff\xff\xff""\xff\xff\xff\xff")
                                          << QCborValue::EncodingOptions(QCborValue::UseIntegers);

    // but obviously non-integral or out of range floating point stay FP
    QTest::newRow("UseInteger:1.5") << QCborValue(1.5) << raw("\xfb\x3f\xf8\0\0""\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:-1.5") << QCborValue(-1.5) << raw("\xfb\xbf\xf8\0\0""\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:inf") << QCborValue(qInf()) << raw("\xfb\x7f\xf0\0\0""\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:-inf") << QCborValue(-qInf()) << raw("\xfb\xff\xf0\0""\0\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:nan") << QCborValue(qQNaN()) << raw("\xfb\x7f\xf8\0\0""\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:2^64") << QCborValue(18446744073709551616.0) << raw("\xfb\x43\xf0\0\0""\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
    QTest::newRow("UseInteger:-2^65") << QCborValue(-2 * 18446744073709551616.0) << raw("\xfb\xc4\0\0\0""\0\0\0\0") << QCborValue::EncodingOptions(QCborValue::UseIntegers);
}

void tst_QCborValue::toCbor()
{
    QFETCH(QCborValue, v);
    QFETCH(QByteArray, result);
    QFETCH(QCborValue::EncodingOptions, options);

    QCOMPARE(v.toCbor(options), result);

    // in maps and arrays
    QCOMPARE(QCborArray{v}.toCborValue().toCbor(options), "\x81" + result);
    QCOMPARE(QCborArray({v, v}).toCborValue().toCbor(options),
             "\x82" + result + result);
    QCOMPARE(QCborMap({{1, v}}).toCborValue().toCbor(options),
             "\xa1\x01" + result);

    // tagged
    QCborValue t(QCborKnownTags::Signature, v);
    QCOMPARE(t.toCbor(options), "\xd9\xd9\xf7" + result);
    QCOMPARE(QCborArray({t, t}).toCborValue().toCbor(options),
             "\x82\xd9\xd9\xf7" + result + "\xd9\xd9\xf7" + result);
    QCOMPARE(QCborMap({{1, t}}).toCborValue().toCbor(options),
             "\xa1\x01\xd9\xd9\xf7" + result);
}

void tst_QCborValue::toCborStreamWriter()
{
    QFETCH(QCborValue, v);
    QFETCH(QByteArray, result);
    QFETCH(QCborValue::EncodingOptions, options);

    QByteArray output;
    QBuffer buffer(&output);
    buffer.open(QIODevice::WriteOnly);
    QCborStreamWriter writer(&buffer);

    v.toCbor(writer, options);
    QCOMPARE(buffer.pos(), result.size());
    QCOMPARE(output, result);
}

void tst_QCborValue::fromCbor_data()
{
    addCommonCborData();

    // chunked strings
    QTest::newRow("ByteArray:Chunked") << QCborValue(QByteArray("Hello"))
                                        << raw("\x5f\x43Hel\x42lo\xff");
    QTest::newRow("ByteArray:Chunked:Empty") << QCborValue(QByteArray()) << raw("\x5f\xff");
    QTest::newRow("String:Chunked") << QCborValue("Hello")
                                    << raw("\x7f\x63Hel\x62lo\xff");
    QTest::newRow("String:Chunked:Empty") << QCborValue(QString())
                                    << raw("\x7f\xff");

    QTest::newRow("DateTime:NoMilli") << QCborValue(QDateTime::fromSecsSinceEpoch(1515565477, Qt::UTC))
                                      << raw("\xc0\x74" "2018-01-10T06:24:37Z");
    QTest::newRow("UnixTime_t:Integer") << QCborValue(QDateTime::fromSecsSinceEpoch(1515565477, Qt::UTC))
                                        << raw("\xc1\x1a\x5a\x55\xb1\xa5");
    QTest::newRow("UnixTime_t:Double") << QCborValue(QDateTime::fromMSecsSinceEpoch(1515565477125, Qt::UTC))
                                       << raw("\xc1\xfb\x41\xd6\x95\x6c""\x69\x48\x00\x00");

    QTest::newRow("Url:NotNormalized") << QCborValue(QUrl("https://example.com/\xc2\xa9 "))
                                       << raw("\xd8\x20\x78\x1dHTTPS://EXAMPLE.COM/%c2%a9%20");

    QTest::newRow("Uuid:Zero") << QCborValue(QUuid()) << raw("\xd8\x25\x40");
    QTest::newRow("Uuid:TooShort") << QCborValue(QUuid::fromRfc4122(raw("\1\2\3\4""\4\3\2\0""\0\0\0\0""\0\0\0\0")))
                                   << raw("\xd8\x25\x47" "\1\2\3\4\4\3\2");
    QTest::newRow("Uuid:TooLong") << QCborValue(QUuid::fromRfc4122(raw("\1\2\3\4""\4\3\2\0""\0\0\0\0""\0\0\0\1")))
                                   << raw("\xd8\x25\x51" "\1\2\3\4""\4\3\2\0""\0\0\0\0""\0\0\0\1""\2");
}

void fromCbor_common(void (*doCheck)(const QCborValue &, const QByteArray &))
{
    QFETCH(QCborValue, v);
    QFETCH(QByteArray, result);

    doCheck(v, result);
    if (QTest::currentTestFailed())
        return;

    // in an array
    doCheck(QCborArray{v}, "\x81" + result);
    if (QTest::currentTestFailed())
        return;

    doCheck(QCborArray{v, v}, "\x82" + result + result);
    if (QTest::currentTestFailed())
        return;

    // in a map
    doCheck(QCborMap{{1, v}}, "\xa1\1" + result);
    if (QTest::currentTestFailed())
        return;

    // undefined-length arrays and maps
    doCheck(QCborArray{v}, "\x9f" + result + "\xff");
    if (QTest::currentTestFailed())
        return;
    doCheck(QCborArray{v, v}, "\x9f" + result + result + "\xff");
    if (QTest::currentTestFailed())
        return;
    doCheck(QCborMap{{1, v}}, "\xbf\1" + result + "\xff");
    if (QTest::currentTestFailed())
        return;

    // tagged
    QCborValue t(QCborKnownTags::Signature, v);
    doCheck(t, "\xd9\xd9\xf7" + result);
    if (QTest::currentTestFailed())
        return;

    // in an array
    doCheck(QCborArray{t}, "\x81\xd9\xd9\xf7" + result);
    if (QTest::currentTestFailed())
        return;

    doCheck(QCborArray{t, t}, "\x82\xd9\xd9\xf7" + result + "\xd9\xd9\xf7" + result);
    if (QTest::currentTestFailed())
        return;

    // in a map
    doCheck(QCborMap{{1, t}}, "\xa1\1\xd9\xd9\xf7" + result);
    if (QTest::currentTestFailed())
        return;
}

void tst_QCborValue::fromCbor()
{
    auto doCheck = [](const QCborValue &v, const QByteArray &result) {
        QCborParserError error;
        QCborValue decoded = QCborValue::fromCbor(result, &error);
        QVERIFY2(error.error == QCborError(), qPrintable(error.errorString()));
        QCOMPARE(error.offset, result.size());
        QVERIFY(decoded == v);
        QVERIFY(v == decoded);
    };

    fromCbor_common(doCheck);
}

void tst_QCborValue::fromCborStreamReaderByteArray()
{
    auto doCheck = [](const QCborValue &expected, const QByteArray &data) {
        QCborStreamReader reader(data);
        QCborValue decoded = QCborValue::fromCbor(reader);
        QCOMPARE(reader.lastError(), QCborError());
        QCOMPARE(reader.currentOffset(), data.size());
        QVERIFY(decoded == expected);
        QVERIFY(expected == decoded);
    };

    fromCbor_common(doCheck);
}

void tst_QCborValue::fromCborStreamReaderIODevice()
{
    auto doCheck = [](const QCborValue &expected, const QByteArray &data) {
        QBuffer buffer;
        buffer.setData(data);
        buffer.open(QIODevice::ReadOnly);
        QCborStreamReader reader(&buffer);
        QCborValue decoded = QCborValue::fromCbor(reader);
        QCOMPARE(reader.lastError(), QCborError());
        QCOMPARE(reader.currentOffset(), data.size());
        QVERIFY(decoded == expected);
        QVERIFY(expected == decoded);
        QCOMPARE(buffer.pos(), reader.currentOffset());
    };

    fromCbor_common(doCheck);
}

#include "../cborlargedatavalidation.cpp"

void tst_QCborValue::validation_data()
{
    // Add QCborStreamReader-specific limitations due to use of QByteArray and
    // QString, which are allocated by QArrayData::allocate().
    const qsizetype MaxInvalid = std::numeric_limits<QByteArray::size_type>::max();
    const qsizetype MinInvalid = MaxByteArraySize + 1;
    addValidationColumns();
    addValidationData(MinInvalid);
    addValidationLargeData(MinInvalid, MaxInvalid);

    // These tests say we have arrays and maps with very large item counts.
    // They are meant to ensure we don't pre-allocate a lot of memory
    // unnecessarily and possibly crash the application. The actual number of
    // elements in the stream is only 2, so we should get an unexpected EOF
    // error. QCborValue internally uses 16 bytes per element, so we get to
    // 2 GB at 2^27 elements.
    QTest::addRow("very-large-array-no-overflow") << raw("\x9a\x07\xff\xff\xff" "\0\0") << 0 << CborErrorUnexpectedEOF;
    QTest::addRow("very-large-array-overflow1") << raw("\x9a\x40\0\0\0" "\0\0") << 0 << CborErrorUnexpectedEOF;

    // this makes sure we don't accidentally clip to 32-bit: sending 2^32+2 elements
    QTest::addRow("very-large-array-overflow2") << raw("\x9b\0\0\0\1""\0\0\0\2" "\0\0") << 0 << CborErrorDataTooLarge;
}

void tst_QCborValue::validation()
{
    QFETCH(QByteArray, data);
    QFETCH(CborError, expectedError);
    QCborError error = { QCborError::Code(expectedError) };

    QCborParserError parserError;
    QCborValue decoded = QCborValue::fromCbor(data, &parserError);
    QCOMPARE(parserError.error, error);

    if (data.startsWith('\x81')) {
        // decode without the array prefix
        char *ptr = const_cast<char *>(data.constData());
        QByteArray mid = QByteArray::fromRawData(ptr + 1, data.size() - 1);
        decoded = QCborValue::fromCbor(mid, &parserError);
        QCOMPARE(parserError.error, error);
    }
}

void tst_QCborValue::hugeDeviceValidation_data()
{
    addValidationHugeDevice(MaxByteArraySize + 1, MaxStringSize + 1);
}

void tst_QCborValue::hugeDeviceValidation()
{
    QFETCH(QSharedPointer<QIODevice>, device);
    QFETCH(CborError, expectedError);
    QCborError error = { QCborError::Code(expectedError) };

    device->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    QCborStreamReader reader(device.data());
    QCborValue decoded = QCborValue::fromCbor(reader);
    QCOMPARE(reader.lastError(), error);
}

void tst_QCborValue::recursionLimit_data()
{
    constexpr int RecursionAttempts = 4096;
    QTest::addColumn<QByteArray>("data");
    QByteArray arrays(RecursionAttempts, char(0x81));
    QByteArray _arrays(RecursionAttempts, char(0x9f));
    QByteArray maps(RecursionAttempts, char(0xa1));
    QByteArray _maps(RecursionAttempts, char(0xbf));
    QByteArray tags(RecursionAttempts, char(0xc0));

    QTest::newRow("array-nesting-too-deep") << arrays;
    QTest::newRow("_array-nesting-too-deep") << _arrays;
    QTest::newRow("map-nesting-too-deep") << maps;
    QTest::newRow("_map-nesting-too-deep") << _maps;
    QTest::newRow("tag-nesting-too-deep") << tags;

    QByteArray mixed(5 * RecursionAttempts, Qt::Uninitialized);
    char *ptr = mixed.data();
    for (int i = 0; i < RecursionAttempts; ++i) {
        quint8 type = qBound(quint8(QCborStreamReader::Array), quint8(i & 0x80), quint8(QCborStreamReader::Tag));
        quint8 additional_info = i & 0x1f;
        if (additional_info == 0x1f)
            (void)additional_info;      // leave it
        else if (additional_info > 0x1a)
            additional_info = 0x1a;
        else if (additional_info < 1)
            additional_info = 1;

        *ptr++ = type | additional_info;
        if (additional_info == 0x18) {
            *ptr++ = uchar(i);
        } else if (additional_info == 0x19) {
            qToBigEndian(ushort(i), ptr);
            ptr += 2;
        } else if (additional_info == 0x1a) {
            qToBigEndian(uint(i), ptr);
            ptr += 4;
        }
    }

    QTest::newRow("mixed-nesting-too-deep") << mixed;
}

void tst_QCborValue::recursionLimit()
{
    QFETCH(QByteArray, data);

    QCborParserError error;
    QCborValue decoded = QCborValue::fromCbor(data, &error);
    QCOMPARE(error.error, QCborError::NestingTooDeep);
}

void tst_QCborValue::toDiagnosticNotation_data()
{
    QTest::addColumn<QCborValue>("v");
    QTest::addColumn<int>("opts");
    QTest::addColumn<QString>("expected");
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUuid uuid = QUuid::createUuid();

    QMetaEnum me = QMetaEnum::fromType<QCborValue::Type>();
    auto add = [me](const QCborValue &v, const QString &exp) {
        auto addRow = [=](const char *prefix) -> QTestData & {
            QCborValue::Type t = v.type();
            if (t == QCborValue::Integer)
                return QTest::addRow("%sInteger:%lld", prefix, v.toInteger());
            if (t == QCborValue::Double)
                return QTest::addRow("%sDouble:%g", prefix, v.toDouble());
            if (t == QCborValue::ByteArray)
                return QTest::addRow("%sByteArray:%d", prefix, v.toByteArray().size());
            if (t == QCborValue::String)
                return QTest::addRow("%sString:%d", prefix, v.toString().size());

            QByteArray typeString = me.valueToKey(t);
            Q_ASSERT(!typeString.isEmpty());
            return QTest::newRow(prefix + typeString);
        };
        addRow("") << v << int(QCborValue::DiagnosticNotationOptions{}) << exp;
        addRow("LW:") << v << int(QCborValue::LineWrapped) << exp;
        addRow("Array:") << QCborValue(QCborArray{v}) << int(QCborValue::DiagnosticNotationOptions{}) << '[' + exp + ']';
        addRow("Mapped:") << QCborValue(QCborMap{{2, v}}) << int(QCborValue::DiagnosticNotationOptions{}) << "{2: " + exp + '}';
        addRow("Mapping:") << QCborValue(QCborMap{{v, 2}}) << int(QCborValue::DiagnosticNotationOptions{}) << '{' + exp + ": 2}";
    };

    // empty arrays and maps
    QTest::newRow("EmptyArray")
            << QCborValue(QCborArray()) << int(QCborValue::DiagnosticNotationOptions{})
            << "[]";
    QTest::newRow("EmptyMap")
            << QCborValue(QCborMap()) << int(QCborValue::DiagnosticNotationOptions{})
            << "{}";

    add(QCborValue(), "undefined");
    add(QCborValue::Null, "null");
    add(false, "false");
    add(true, "true");
    add(QCborSimpleType(0), "simple(0)");
    QTest::newRow("SimpleType-255")
            << QCborValue(QCborSimpleType(255)) << int(QCborValue::DiagnosticNotationOptions{})
            << "simple(255)";
    add(0, "0");
    add(1, "1");
    add(-1, "-1");
    add(std::numeric_limits<qint64>::min(), QString::number(std::numeric_limits<qint64>::min()));
    add(std::numeric_limits<qint64>::max(), QString::number(std::numeric_limits<qint64>::max()));
    add(0., "0.0");
    add(1.25, "1.25");
    add(-1.25, "-1.25");
    add(qInf(), "inf");
    add(-qInf(), "-inf");
    add(qQNaN(), "nan");
    add(QByteArray(), "h''");
    add(QByteArray("Hello"), "h'48656c6c6f'");
    add(QLatin1String(), QLatin1String("\"\""));
    add("Hello", "\"Hello\"");
    add("\"Hello\\World\"", "\"\\\"Hello\\\\World\\\"\"");
    add(QCborValue(dt), "0(\"" + dt.toString(Qt::ISODateWithMs) + "\")");
    add(QCborValue(QUrl("http://example.com")), "32(\"http://example.com\")");
    add(QCborValue(QRegularExpression("^.*$")), "35(\"^.*$\")");
    add(QCborValue(uuid), "37(h'" + uuid.toString(QUuid::Id128) + "')");

    // arrays and maps with more than one element
    QTest::newRow("2Array")
            << QCborValue(QCborArray{0, 1}) << int(QCborValue::DiagnosticNotationOptions{})
            << "[0, 1]";
    QTest::newRow("2Map")
            << QCborValue(QCborMap{{0, 1}, {"foo", "bar"}}) << int(QCborValue::DiagnosticNotationOptions{})
            << "{0: 1, \"foo\": \"bar\"}";

    // line wrapping in arrays and maps
    QTest::newRow("LW:EmptyArray")
            << QCborValue(QCborArray()) << int(QCborValue::LineWrapped)
            << "[\n]";
    QTest::newRow("LW:EmptyMap")
            << QCborValue(QCborMap()) << int(QCborValue::LineWrapped)
            << "{\n}";
    QTest::newRow("LW:Array:Integer:0")
            << QCborValue(QCborArray{0}) << int(QCborValue::LineWrapped)
            << "[\n    0\n]";
    QTest::newRow("LW:Array:String:5")
            << QCborValue(QCborArray{"Hello"}) << int(QCborValue::LineWrapped)
            << "[\n    \"Hello\"\n]";
    QTest::newRow("LW:Map:0-0")
            << QCborValue(QCborMap{{0, 0}}) << int(QCborValue::LineWrapped)
            << "{\n    0: 0\n}";
    QTest::newRow("LW:Map:String:5")
            << QCborValue(QCborMap{{0, "Hello"}}) << int(QCborValue::LineWrapped)
            << "{\n    0: \"Hello\"\n}";
    QTest::newRow("LW:2Array")
            << QCborValue(QCborArray{0, 1}) << int(QCborValue::LineWrapped)
            << "[\n    0,\n    1\n]";
    QTest::newRow("LW:2Map")
            << QCborValue(QCborMap{{0, 0}, {"foo", "bar"}}) << int(QCborValue::LineWrapped)
            << "{\n    0: 0,\n    \"foo\": \"bar\"\n}";

    // nested arrays and maps
    QTest::newRow("Array:EmptyArray")
            << QCborValue(QCborArray() << QCborArray()) << int(QCborValue::DiagnosticNotationOptions{})
            << "[[]]";
    QTest::newRow("Array:EmptyMap")
            << QCborValue(QCborArray() << QCborMap()) << int(QCborValue::DiagnosticNotationOptions{})
            << "[{}]";
    QTest::newRow("LW:Array:EmptyArray")
            << QCborValue(QCborArray() << QCborArray()) << int(QCborValue::LineWrapped)
            << "[\n    [\n    ]\n]";
    QTest::newRow("LW:Array:EmptyMap")
            << QCborValue(QCborArray() << QCborMap()) << int(QCborValue::LineWrapped)
            << "[\n    {\n    }\n]";
    QTest::newRow("LW:Array:2Array")
            << QCborValue(QCborArray() << QCborArray{0, 1}) << int(QCborValue::LineWrapped)
            << "[\n    [\n        0,\n        1\n    ]\n]";
    QTest::newRow("LW:Map:2Array")
            << QCborValue(QCborMap{{0, QCborArray{0, 1}}}) << int(QCborValue::LineWrapped)
            << "{\n    0: [\n        0,\n        1\n    ]\n}";
    QTest::newRow("LW:Map:2Map")
            << QCborValue(QCborMap{{-1, QCborMap{{0, 0}, {"foo", "bar"}}}}) << int(QCborValue::LineWrapped)
            << "{\n    -1: {\n        0: 0,\n        \"foo\": \"bar\"\n    }\n}";

    // string escaping
    QTest::newRow("String:escaping")
            << QCborValue("\1\a\b\t\f\r\n\v\x1f\x7f \"\xc2\xa0\xe2\x82\xac\xf0\x90\x80\x80\\\"")
            << int(QCborValue::DiagnosticNotationOptions{})
            << "\"\\u0001\\a\\b\\t\\f\\r\\n\\v\\u001F\\u007F \\\"\\u00A0\\u20AC\\U00010000\\\\\\\"\"";

    // extended formatting for byte arrays
    QTest::newRow("Extended:ByteArray:0")
            << QCborValue(QByteArray()) << int(QCborValue::ExtendedFormat)
            << "h''";
    QTest::newRow("Extended:ByteArray:5")
            << QCborValue(QByteArray("Hello")) << int(QCborValue::ExtendedFormat)
            << "h'48 65 6c 6c 6f'";
    QTest::newRow("Extended:ByteArray:Base64url")
            << QCborValue(QCborKnownTags::ExpectedBase64url, QByteArray("\xff\xef"))
            << int(QCborValue::ExtendedFormat) << "21(b64'_-8')";
    QTest::newRow("Extended:ByteArray:Base64")
            << QCborValue(QCborKnownTags::ExpectedBase64, QByteArray("\xff\xef"))
            << int(QCborValue::ExtendedFormat) << "22(b64'/+8=')";

    // formatting applies through arrays too
    QTest::newRow("Extended:Array:ByteArray:Base64url")
            << QCborValue(QCborKnownTags::ExpectedBase64url, QCborArray{QByteArray("\xff\xef")})
            << int(QCborValue::ExtendedFormat) << "21([b64'_-8'])";
    // and only the innermost applies
    QTest::newRow("ByteArray:multiple-tags")
            << QCborValue(QCborKnownTags::ExpectedBase64url,
                          QCborArray{QCborValue(QCborKnownTags::ExpectedBase16, QByteArray("Hello")),
                          QByteArray("\xff\xef")})
            << int(QCborValue::ExtendedFormat) << "21([23(h'48 65 6c 6c 6f'), b64'_-8'])";
}

void tst_QCborValue::toDiagnosticNotation()
{
    QFETCH(QCborValue, v);
    QFETCH(QString, expected);
    QFETCH(int, opts);

    QString result = v.toDiagnosticNotation(QCborValue::DiagnosticNotationOptions(opts));
    QCOMPARE(result, expected);
}

QTEST_MAIN(tst_QCborValue)

#include "tst_qcborvalue.moc"
