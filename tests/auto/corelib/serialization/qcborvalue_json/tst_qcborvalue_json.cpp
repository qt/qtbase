/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
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

Q_DECLARE_METATYPE(QCborValue)

class tst_QCborValue_Json : public QObject
{
    Q_OBJECT

private slots:
    void toVariant_data();
    void toVariant();
    void toJson_data() { toVariant_data(); }
    void toJson();
    void taggedByteArrayToJson_data();
    void taggedByteArrayToJson();

    void fromVariant_data() { toVariant_data(); }
    void fromVariant();
    void fromJson_data();
    void fromJson();

    void nonStringKeysInMaps_data();
    void nonStringKeysInMaps();
};

void tst_QCborValue_Json::toVariant_data()
{
    QTest::addColumn<QCborValue>("v");
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QJsonValue>("json");
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUuid uuid = QUuid::createUuid();

    QMetaEnum me = QMetaEnum::fromType<QCborValue::Type>();
    auto add = [me](const QCborValue &v, const QVariant &exp, const QJsonValue &json) {
        auto addRow = [=]() -> QTestData & {
            const char *typeString = me.valueToKey(v.type());
            if (v.type() == QCborValue::Integer)
                return QTest::addRow("Integer:%lld", exp.toLongLong());
            if (v.type() == QCborValue::Double)
                return QTest::addRow("Double:%g", exp.toDouble());
            if (v.type() == QCborValue::ByteArray || v.type() == QCborValue::String)
                return QTest::addRow("%s:%d", typeString, exp.toString().size());
            if (v.type() >= 0x10000)
                return QTest::newRow(exp.typeName());
            return QTest::newRow(typeString);
        };
        addRow() << v << exp << json;
    };

    // good JSON matching:
    add(QCborValue(), QVariant(), QJsonValue::Null);
    add(nullptr, QVariant::fromValue(nullptr), QJsonValue::Null);
    add(false, false, false);
    add(true, true, true);
    add(0, 0, 0);
    add(1, 1, 1);
    add(-1, -1, -1);
    add(0., 0., 0.);
    add(1.25, 1.25, 1.25);
    add(-1.25, -1.25, -1.25);
    add("Hello", "Hello", "Hello");

    // converts to string in JSON:
    add(QByteArray("Hello"), QByteArray("Hello"), "SGVsbG8");
    add(QCborValue(dt), dt, dt.toString(Qt::ISODateWithMs));
    add(QCborValue(QUrl("http://example.com/{q}")), QUrl("http://example.com/{q}"),
        "http://example.com/%7Bq%7D");      // note the encoded form in JSON
    add(QCborValue(QRegularExpression(".")), QRegularExpression("."), ".");
    add(QCborValue(uuid), uuid, uuid.toString(QUuid::WithoutBraces));

    // not valid in JSON
    QTest::newRow("simpletype") << QCborValue(QCborSimpleType(255))
                                << QVariant::fromValue(QCborSimpleType(255))
                                << QJsonValue("simple(255)");
    QTest::newRow("Double:inf") << QCborValue(qInf())
                                << QVariant(qInf())
                                << QJsonValue();
    QTest::newRow("Double:-inf") << QCborValue(-qInf())
                                 << QVariant(-qInf())
                                 << QJsonValue();
    QTest::newRow("Double:nan") << QCborValue(qQNaN())
                                << QVariant(qQNaN())
                                << QJsonValue();

    // large integral values lose precision in JSON
    QTest::newRow("Integer:max") << QCborValue(std::numeric_limits<qint64>::max())
                                 << QVariant(std::numeric_limits<qint64>::max())
                                 << QJsonValue(std::numeric_limits<qint64>::max());
    QTest::newRow("Integer:min") << QCborValue(std::numeric_limits<qint64>::min())
                                 << QVariant(std::numeric_limits<qint64>::min())
                                 << QJsonValue(std::numeric_limits<qint64>::min());

    // empty arrays and maps
    add(QCborArray(), QVariantList(), QJsonArray());
    add(QCborMap(), QVariantMap(), QJsonObject());
}

void tst_QCborValue_Json::toVariant()
{
    QFETCH(QCborValue, v);
    QFETCH(QVariant, variant);

    if (qIsNaN(variant.toDouble())) {
        // because NaN != NaN, QVariant(NaN) != QVariant(NaN), so we
        // only need to compare the classification
        QVERIFY(qIsNaN(v.toVariant().toDouble()));

        // the rest of this function depends on the variant comparison
        return;
    }

    QCOMPARE(v.toVariant(), variant);
    if (variant.isValid()) {
        QVariant variant2 = QVariant::fromValue(v);
        QVERIFY(variant2.canConvert(variant.userType()));
        QVERIFY(variant2.convert(variant.userType()));
        QCOMPARE(variant2, variant);
    }

    // tags get ignored:
    QCOMPARE(QCborValue(QCborKnownTags::Signature, v).toVariant(), variant);

    // make arrays with this item
    QCOMPARE(QCborArray({v}).toVariantList(), QVariantList({variant}));
    QCOMPARE(QCborArray({v, v}).toVariantList(), QVariantList({variant, variant}));

    // and maps
    QCOMPARE(QCborMap({{"foo", v}}).toVariantMap(), QVariantMap({{"foo", variant}}));
    QCOMPARE(QCborMap({{"foo", v}}).toVariantHash(), QVariantHash({{"foo", variant}}));

    // finally, mixed
    QCOMPARE(QCborArray{QCborMap({{"foo", v}})}.toVariantList(),
             QVariantList{QVariantMap({{"foo", variant}})});
}

void tst_QCborValue_Json::toJson()
{
    QFETCH(QCborValue, v);
    QFETCH(QJsonValue, json);

    QCOMPARE(v.toJsonValue(), json);
    QCOMPARE(QVariant::fromValue(v).toJsonValue(), json);

    // most tags get ignored:
    QCOMPARE(QCborValue(QCborKnownTags::Signature, v).toJsonValue(), json);

    // make arrays with this item
    QCOMPARE(QCborArray({v}).toJsonArray(), QJsonArray({json}));
    QCOMPARE(QCborArray({v, v}).toJsonArray(), QJsonArray({json, json}));

    // and maps
    QCOMPARE(QCborMap({{"foo", v}}).toJsonObject(), QJsonObject({{"foo", json}}));

    // finally, mixed
    QCOMPARE(QCborArray{QCborMap({{"foo", v}})}.toJsonArray(),
             QJsonArray{QJsonObject({{"foo", json}})});
}

void tst_QCborValue_Json::taggedByteArrayToJson_data()
{
    QTest::addColumn<QCborValue>("v");
    QTest::addColumn<QJsonValue>("json");

    QByteArray data("\xff\x01");
    QTest::newRow("base64url") << QCborValue(QCborKnownTags::ExpectedBase64url, data) << QJsonValue("_wE");
    QTest::newRow("base64") << QCborValue(QCborKnownTags::ExpectedBase64, data) << QJsonValue("/wE=");
    QTest::newRow("hex") << QCborValue(QCborKnownTags::ExpectedBase16, data) << QJsonValue("ff01");
}

void tst_QCborValue_Json::taggedByteArrayToJson()
{
    QFETCH(QCborValue, v);
    QFETCH(QJsonValue, json);

    QCOMPARE(v.toJsonValue(), json);
    QCOMPARE(QCborArray({v}).toJsonArray(), QJsonArray({json}));
}

void tst_QCborValue_Json::fromVariant()
{
    QFETCH(QCborValue, v);
    QFETCH(QVariant, variant);

    QCOMPARE(QCborValue::fromVariant(variant), v);
    QCOMPARE(variant.value<QCborValue>(), v);

    // try arrays
    QCOMPARE(QCborArray::fromVariantList({variant}), QCborArray{v});
    QCOMPARE(QCborArray::fromVariantList({variant, variant}), QCborArray({v, v}));

    if (variant.type() == QVariant::String) {
        QString s = variant.toString();
        QCOMPARE(QCborArray::fromStringList({s}), QCborArray{v});
        QCOMPARE(QCborArray::fromStringList({s, s}), QCborArray({v, v}));
    }

    // maps...
    QVariantMap map{{"foo", variant}};
    QCOMPARE(QCborMap::fromVariantMap(map), QCborMap({{"foo", v}}));
    QCOMPARE(QCborMap::fromVariantHash({{"foo", variant}}), QCborMap({{"foo", v}}));

    // nested
    QVariantMap outer{{"bar", QVariantList{0, map, true}}};
    QCOMPARE(QCborMap::fromVariantMap(outer),
             QCborMap({{"bar", QCborArray{0, QCborMap{{"foo", v}}, true}}}));
}

void tst_QCborValue_Json::fromJson_data()
{
    QTest::addColumn<QCborValue>("v");
    QTest::addColumn<QJsonValue>("json");

    QTest::newRow("null") << QCborValue(QCborValue::Null) << QJsonValue(QJsonValue::Null);
    QTest::newRow("false") << QCborValue(false) << QJsonValue(false);
    QTest::newRow("true") << QCborValue(true) << QJsonValue(true);
    QTest::newRow("0") << QCborValue(0) << QJsonValue(0.);
    QTest::newRow("1") << QCborValue(1) << QJsonValue(1);
    QTest::newRow("1.5") << QCborValue(1.5) << QJsonValue(1.5);
    QTest::newRow("string") << QCborValue("Hello") << QJsonValue("Hello");
    QTest::newRow("array") << QCborValue(QCborValue::Array) << QJsonValue(QJsonValue::Array);
    QTest::newRow("map") << QCborValue(QCborValue::Map) << QJsonValue(QJsonValue::Object);
}

void tst_QCborValue_Json::fromJson()
{
    QFETCH(QCborValue, v);
    QFETCH(QJsonValue, json);

    QCOMPARE(QCborValue::fromJsonValue(json), v);
    QCOMPARE(QVariant(json).value<QCborValue>(), v);
    QCOMPARE(QCborArray::fromJsonArray({json}), QCborArray({v}));
    QCOMPARE(QCborArray::fromJsonArray({json, json}), QCborArray({v, v}));
    QCOMPARE(QCborMap::fromJsonObject({{"foo", json}}), QCborMap({{"foo", v}}));

    // confirm we can roundtrip back to JSON
    QCOMPARE(QCborValue::fromJsonValue(json).toJsonValue(), json);
}

void tst_QCborValue_Json::nonStringKeysInMaps_data()
{
    QTest::addColumn<QCborValue>("key");
    QTest::addColumn<QString>("converted");

    auto add = [](const char *str, const QCborValue &v) {
        QTest::newRow(str) << v << str;
    };
    add("0", 0);
    add("-1", -1);
    add("false", false);
    add("true", true);
    add("null", nullptr);
    add("undefined", {});   // should this be ""?
    add("simple(255)", QCborSimpleType(255));
    add("2.5", 2.5);

    QByteArray data("\xff\x01");
    QTest::newRow("bytearray") << QCborValue(data) << "_wE";
    QTest::newRow("base64url") << QCborValue(QCborKnownTags::ExpectedBase64url, data) << "_wE";
    QTest::newRow("base64") << QCborValue(QCborKnownTags::ExpectedBase64, data) << "/wE=";
    QTest::newRow("hex") << QCborValue(QCborKnownTags::ExpectedBase16, data) << "ff01";

    QTest::newRow("emptyarray") << QCborValue(QCborValue::Array) << "[]";
    QTest::newRow("emptymap") << QCborValue(QCborValue::Map) << "{}";
    QTest::newRow("array") << QCborValue(QCborArray{1, true, 2.5, "Hello"})
                           << "[1, true, 2.5, \"Hello\"]";
    QTest::newRow("map") << QCborValue(QCborMap{{"Hello", 0}, {0, "Hello"}})
                         << "{\"Hello\": 0, 0: \"Hello\"}";

    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUrl url("https://example.com");
    QUuid uuid = QUuid::createUuid();
    QTest::newRow("QDateTime") << QCborValue(dt) << dt.toString(Qt::ISODateWithMs);
    QTest::newRow("QUrl") << QCborValue(url) << url.toString(QUrl::FullyEncoded);
    QTest::newRow("QRegularExpression") << QCborValue(QRegularExpression(".*")) << ".*";
    QTest::newRow("QUuid") << QCborValue(uuid) << uuid.toString(QUuid::WithoutBraces);
}

void tst_QCborValue_Json::nonStringKeysInMaps()
{
    QFETCH(QCborValue, key);
    QFETCH(QString, converted);

    QCborMap m;
    m.insert(key, 0);

    {
        QVariantMap vm = m.toVariantMap();
        auto it = vm.begin();
        QVERIFY(it != vm.end());
        QCOMPARE(it.key(), converted);
        QCOMPARE(it.value(), 0);
        QCOMPARE(++it, vm.end());
    }

    {
        QJsonObject o = m.toJsonObject();
        auto it = o.begin();
        QVERIFY(it != o.end());
        QCOMPARE(it.key(), converted);
        QCOMPARE(it.value(), 0);
        QCOMPARE(++it, o.end());
    }
}

QTEST_MAIN(tst_QCborValue_Json)

#include "tst_qcborvalue_json.moc"
