/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest>

#include "qjsonarray.h"
#include "qjsonobject.h"
#include "qjsonvalue.h"
#include "qjsondocument.h"
#include "qregularexpression.h"
#include <limits>

#define INVALID_UNICODE "\xCE\xBA\xE1"
#define UNICODE_NON_CHARACTER "\xEF\xBF\xBF"
#define UNICODE_DJE "\320\202" // Character from the Serbian Cyrillic alphabet

class tst_QtJson: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testValueSimple();
    void testNumbers();
    void testNumbers_2();
    void testNumbers_3();
    void testNumbers_4();

    void testObjectSimple();
    void testObjectSmallKeys();
    void testObjectInsertCopies();
    void testArraySimple();
    void testArrayInsertCopies();
    void testValueObject();
    void testValueArray();
    void testObjectNested();
    void testArrayNested();
    void testArrayNestedEmpty();
    void testArrayComfortOperators();
    void testObjectNestedEmpty();

    void testValueRef();
    void testObjectIteration();
    void testArrayIteration();

    void testObjectFind();

    void testDocument();

    void nullValues();
    void nullArrays();
    void nullObject();
    void constNullObject();

    void keySorting_data();
    void keySorting();

    void undefinedValues();

    void fromVariant_data();
    void fromVariant();
    void fromVariantSpecial_data();
    void fromVariantSpecial();
    void toVariant_data();
    void toVariant();
    void fromVariantMap();
    void fromVariantHash();
    void toVariantMap();
    void toVariantHash();
    void toVariantList();

    void toJson();
    void toJsonSillyNumericValues();
    void toJsonLargeNumericValues();
    void fromJson();
    void fromJsonErrors();
    void fromBinary();
    void toAndFromBinary_data();
    void toAndFromBinary();
    void invalidBinaryData();
    void parseNumbers();
    void parseStrings();
    void parseDuplicateKeys();
    void testParser();

    void compactArray();
    void compactObject();

    void validation();

    void assignToDocument();

    void testDuplicateKeys();
    void testCompaction();
    void testDebugStream();
    void testCompactionError();

    void parseUnicodeEscapes();

    void assignObjects();
    void assignArrays();

    void testTrailingComma();
    void testDetachBug();
    void testJsonValueRefDefault();

    void valueEquals();
    void objectEquals_data();
    void objectEquals();
    void arrayEquals_data();
    void arrayEquals();
    void documentEquals_data();
    void documentEquals();

    void bom();
    void nesting();

    void longStrings();

    void arrayInitializerList();
    void objectInitializerList();

    void unicodeKeys();
    void garbageAtEnd();

    void removeNonLatinKey();
    void documentFromVariant();

    void parseErrorOffset_data();
    void parseErrorOffset();

    void implicitValueType();
    void implicitDocumentType();

    void streamSerializationQJsonDocument_data();
    void streamSerializationQJsonDocument();
    void streamSerializationQJsonArray_data();
    void streamSerializationQJsonArray();
    void streamSerializationQJsonObject_data();
    void streamSerializationQJsonObject();
    void streamSerializationQJsonValue_data();
    void streamSerializationQJsonValue();
    void streamSerializationQJsonValueEmpty();
    void streamVariantSerialization();
    void escapeSurrogateCodePoints_data();
    void escapeSurrogateCodePoints();

    void fromToVariantConversions_data();
    void fromToVariantConversions();

private:
    QString testDataDir;
};

void tst_QtJson::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("test.json")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
}

void tst_QtJson::testValueSimple()
{
    QJsonObject object;
    object.insert("number", 999.);
    QJsonArray array;
    for (int i = 0; i < 10; ++i)
        array.append((double)i);

    QJsonValue value(true);
    QCOMPARE(value.type(), QJsonValue::Bool);
    QCOMPARE(value.toDouble(), 0.);
    QCOMPARE(value.toString(), QString());
    QCOMPARE(value.toBool(), true);
    QCOMPARE(value.toObject(), QJsonObject());
    QCOMPARE(value.toArray(), QJsonArray());
    QCOMPARE(value.toDouble(99.), 99.);
    QCOMPARE(value.toString(QString("test")), QString("test"));
    QCOMPARE(value.toObject(object), object);
    QCOMPARE(value.toArray(array), array);

    value = 999.;
    QCOMPARE(value.type(), QJsonValue::Double);
    QCOMPARE(value.toDouble(), 999.);
    QCOMPARE(value.toString(), QString());
    QCOMPARE(value.toBool(), false);
    QCOMPARE(value.toBool(true), true);
    QCOMPARE(value.toObject(), QJsonObject());
    QCOMPARE(value.toArray(), QJsonArray());

    value = QLatin1String("test");
    QCOMPARE(value.toDouble(), 0.);
    QCOMPARE(value.toString(), QLatin1String("test"));
    QCOMPARE(value.toBool(), false);
    QCOMPARE(value.toObject(), QJsonObject());
    QCOMPARE(value.toArray(), QJsonArray());
}

void tst_QtJson::testNumbers()
{
    {
        int numbers[] = {
            0,
            -1,
            1,
            (1<<26),
            (1<<27),
            (1<<28),
            -(1<<26),
            -(1<<27),
            -(1<<28),
            (1<<26) - 1,
            (1<<27) - 1,
            (1<<28) - 1,
            -((1<<26) - 1),
            -((1<<27) - 1),
            -((1<<28) - 1)
        };
        int n = sizeof(numbers)/sizeof(int);

        QJsonArray array;
        for (int i = 0; i < n; ++i)
            array.append((double)numbers[i]);

        QByteArray serialized = QJsonDocument(array).toJson();
        QJsonDocument json = QJsonDocument::fromJson(serialized);
        QJsonArray array2 = json.array();

        QCOMPARE(array.size(), array2.size());
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(array.at(i).type(), QJsonValue::Double);
            QCOMPARE(array.at(i).toDouble(), (double)numbers[i]);
            QCOMPARE(array2.at(i).type(), QJsonValue::Double);
            QCOMPARE(array2.at(i).toDouble(), (double)numbers[i]);
        }
    }

    {
        qint64 numbers[] = {
            0,
            -1,
            1,
            (1ll<<54),
            (1ll<<55),
            (1ll<<56),
            -(1ll<<54),
            -(1ll<<55),
            -(1ll<<56),
            (1ll<<54) - 1,
            (1ll<<55) - 1,
            (1ll<<56) - 1,
            -((1ll<<54) - 1),
            -((1ll<<55) - 1),
            -((1ll<<56) - 1)
        };
        int n = sizeof(numbers)/sizeof(qint64);

        QJsonArray array;
        for (int i = 0; i < n; ++i)
            array.append((double)numbers[i]);

        QByteArray serialized = QJsonDocument(array).toJson();
        QJsonDocument json = QJsonDocument::fromJson(serialized);
        QJsonArray array2 = json.array();

        QCOMPARE(array.size(), array2.size());
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(array.at(i).type(), QJsonValue::Double);
            QCOMPARE(array.at(i).toDouble(), (double)numbers[i]);
            QCOMPARE(array2.at(i).type(), QJsonValue::Double);
            QCOMPARE(array2.at(i).toDouble(), (double)numbers[i]);
        }
    }

    {
        double numbers[] = {
            0,
            -1,
            1,
            double(1ll<<54),
            double(1ll<<55),
            double(1ll<<56),
            double(-(1ll<<54)),
            double(-(1ll<<55)),
            double(-(1ll<<56)),
            double((1ll<<54) - 1),
            double((1ll<<55) - 1),
            double((1ll<<56) - 1),
            double(-((1ll<<54) - 1)),
            double(-((1ll<<55) - 1)),
            double(-((1ll<<56) - 1)),
            1.1,
            0.1,
            -0.1,
            -1.1,
            1e200,
            -1e200
        };
        int n = sizeof(numbers)/sizeof(double);

        QJsonArray array;
        for (int i = 0; i < n; ++i)
            array.append(numbers[i]);

        QByteArray serialized = QJsonDocument(array).toJson();
        QJsonDocument json = QJsonDocument::fromJson(serialized);
        QJsonArray array2 = json.array();

        QCOMPARE(array.size(), array2.size());
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(array.at(i).type(), QJsonValue::Double);
            QCOMPARE(array.at(i).toDouble(), numbers[i]);
            QCOMPARE(array2.at(i).type(), QJsonValue::Double);
            QCOMPARE(array2.at(i).toDouble(), numbers[i]);
        }
    }

}

void tst_QtJson::testNumbers_2()
{
    // test cases from TC39 test suite for ECMAScript
    // http://hg.ecmascript.org/tests/test262/file/d067d2f0ca30/test/suite/ch08/8.5/8.5.1.js

    // Fill an array with 2 to the power of (0 ... -1075)
    double value = 1;
    double floatValues[1076], floatValues_1[1076];
    QJsonObject jObject;
    for (int power = 0; power <= 1075; power++) {
        floatValues[power] = value;
        jObject.insert(QString::number(power), QJsonValue(floatValues[power]));
        // Use basic math operations for testing, which are required to support 'gradual underflow' rather
        // than Math.pow etc..., which are defined as 'implementation dependent'.
        value = value * 0.5;
    }

    QJsonDocument jDocument1(jObject);
    QByteArray ba(jDocument1.toJson());

    QJsonDocument jDocument2(QJsonDocument::fromJson(ba));
    for (int power = 0; power <= 1075; power++) {
        floatValues_1[power] = jDocument2.object().value(QString::number(power)).toDouble();
#ifdef Q_OS_QNX
        if (power >= 970)
            QEXPECT_FAIL("", "See QTBUG-37066", Abort);
#endif
        QVERIFY2(floatValues[power] == floatValues_1[power], QString("floatValues[%1] != floatValues_1[%1]").arg(power).toLatin1());
    }

    // The last value is below min denorm and should round to 0, everything else should contain a value
    QVERIFY2(floatValues_1[1075] == 0, "Value after min denorm should round to 0");

    // Validate the last actual value is min denorm
    QVERIFY2(floatValues_1[1074] == 4.9406564584124654417656879286822e-324, QString("Min denorm value is incorrect: %1").arg(floatValues_1[1074]).toLatin1());

    // Validate that every value is half the value before it up to 1
    for (int index = 1074; index > 0; index--) {
        QVERIFY2(floatValues_1[index] != 0, QString("2**- %1 should not be 0").arg(index).toLatin1());

        QVERIFY2(floatValues_1[index - 1] == (floatValues_1[index] * 2), QString("Value should be double adjacent value at index %1").arg(index).toLatin1());
    }
}

void tst_QtJson::testNumbers_3()
{
    // test case from QTBUG-31926
    double d1 = 1.123451234512345;
    double d2 = 1.123451234512346;

    QJsonObject jObject;
    jObject.insert("d1", QJsonValue(d1));
    jObject.insert("d2", QJsonValue(d2));
    QJsonDocument jDocument1(jObject);
    QByteArray ba(jDocument1.toJson());

    QJsonDocument jDocument2(QJsonDocument::fromJson(ba));

    double d1_1(jDocument2.object().value("d1").toDouble());
    double d2_1(jDocument2.object().value("d2").toDouble());
    QVERIFY(d1_1 != d2_1);
}

void tst_QtJson::testNumbers_4()
{
    // no exponent notation used to print numbers between -2^64 and 2^64
    QJsonArray array;
    array << QJsonValue(+1000000000000000.0);
    array << QJsonValue(-1000000000000000.0);
    array << QJsonValue(+9007199254740992.0);
    array << QJsonValue(-9007199254740992.0);
    array << QJsonValue(+9223372036854775808.0);
    array << QJsonValue(-9223372036854775808.0);
    array << QJsonValue(+18446744073709551616.0);
    array << QJsonValue(-18446744073709551616.0);
    const QByteArray json(QJsonDocument(array).toJson());
    const QByteArray expected =
            "[\n"
            "    1000000000000000,\n"
            "    -1000000000000000,\n"
            "    9007199254740992,\n"
            "    -9007199254740992,\n"
            "    9223372036854776000,\n"
            "    -9223372036854776000,\n"
            "    18446744073709552000,\n"
            "    -18446744073709552000\n"
            "]\n";
    QCOMPARE(json, expected);
}

void tst_QtJson::testObjectSimple()
{
    QJsonObject object;
    object.insert("number", 999.);
    QCOMPARE(object.value("number").type(), QJsonValue::Double);
    QCOMPARE(object.value(QLatin1String("number")).toDouble(), 999.);
    object.insert("string", QString::fromLatin1("test"));
    QCOMPARE(object.value("string").type(), QJsonValue::String);
    QCOMPARE(object.value(QLatin1String("string")).toString(), QString("test"));
    object.insert("boolean", true);
    QCOMPARE(object.value("boolean").toBool(), true);
    QCOMPARE(object.value(QLatin1String("boolean")).toBool(), true);
    QJsonObject object2 = object;
    QJsonObject object3 = object;

    QStringList keys = object.keys();
    QVERIFY2(keys.contains("number"), "key number not found");
    QVERIFY2(keys.contains("string"), "key string not found");
    QVERIFY2(keys.contains("boolean"), "key boolean not found");

    // if we put a JsonValue into the JsonObject and retrieve
    // it, it should be identical.
    QJsonValue value(QLatin1String("foo"));
    object.insert("value", value);
    QCOMPARE(object.value("value"), value);

    int size = object.size();
    object.remove("boolean");
    QCOMPARE(object.size(), size - 1);
    QVERIFY2(!object.contains("boolean"), "key boolean should have been removed");

    QJsonValue taken = object.take("value");
    QCOMPARE(taken, value);
    QVERIFY2(!object.contains("value"), "key value should have been removed");

    QString before = object.value("string").toString();
    object.insert("string", QString::fromLatin1("foo"));
    QVERIFY2(object.value(QLatin1String("string")).toString() != before, "value should have been updated");

    // same tests again but with QStringView keys
    object2.insert(QStringView(u"value"), value);
    QCOMPARE(object2.value("value"), value);

    size = object2.size();
    object2.remove(QStringView(u"boolean"));
    QCOMPARE(object2.size(), size - 1);
    QVERIFY2(!object2.contains(QStringView(u"boolean")), "key boolean should have been removed");

    taken = object2.take(QStringView(u"value"));
    QCOMPARE(taken, value);
    QVERIFY2(!object2.contains("value"), "key value should have been removed");

    before = object2.value("string").toString();
    object2.insert(QStringView(u"string"), QString::fromLatin1("foo"));
    QVERIFY2(object2.value(QStringView(u"string")).toString() != before, "value should have been updated");

    // same tests again but with QLatin1String keys
    object3.insert(QLatin1String("value"), value);
    QCOMPARE(object3.value("value"), value);

    size = object3.size();
    object3.remove(QLatin1String("boolean"));
    QCOMPARE(object3.size(), size - 1);
    QVERIFY2(!object3.contains("boolean"), "key boolean should have been removed");

    taken = object3.take(QLatin1String("value"));
    QCOMPARE(taken, value);
    QVERIFY2(!object3.contains("value"), "key value should have been removed");

    before = object3.value("string").toString();
    object3.insert(QLatin1String("string"), QString::fromLatin1("foo"));
    QVERIFY2(object3.value(QLatin1String("string")).toString() != before, "value should have been updated");

    size = object.size();
    QJsonObject subobject;
    subobject.insert("number", 42);
    subobject.insert(QLatin1String("string"), QLatin1String("foobar"));
    object.insert("subobject", subobject);
    QCOMPARE(object.size(), size+1);
    QJsonValue subvalue = object.take(QLatin1String("subobject"));
    QCOMPARE(object.size(), size);
    QCOMPARE(subvalue.toObject(), subobject);
    // make object detach by modifying it many times
    for (int i = 0; i < 64; ++i)
        object.insert(QLatin1String("string"), QLatin1String("bar"));
    QCOMPARE(object.size(), size);
    QCOMPARE(subvalue.toObject(), subobject);
}

void tst_QtJson::testObjectSmallKeys()
{
    QJsonObject data1;
    data1.insert(QStringLiteral("1"), 123.);
    QVERIFY(data1.contains(QStringLiteral("1")));
    QCOMPARE(data1.value(QStringLiteral("1")).toDouble(), (double)123);
    data1.insert(QStringLiteral("12"), 133.);
    QCOMPARE(data1.value(QStringLiteral("12")).toDouble(), (double)133);
    QVERIFY(data1.contains(QStringLiteral("12")));
    data1.insert(QStringLiteral("123"), 323.);
    QCOMPARE(data1.value(QStringLiteral("12")).toDouble(), (double)133);
    QVERIFY(data1.contains(QStringLiteral("123")));
    QCOMPARE(data1.value(QStringLiteral("123")).type(), QJsonValue::Double);
    QCOMPARE(data1.value(QStringLiteral("123")).toDouble(), (double)323);
    QCOMPARE(data1.constEnd() - data1.constBegin(), 3);
    QCOMPARE(data1.end() - data1.begin(), 3);
}

void tst_QtJson::testObjectInsertCopies()
{
    {
        QJsonObject obj;
        obj["prop1"] = "TEST";
        QCOMPARE(obj.size(), 1);
        QCOMPARE(obj.value("prop1"), "TEST");

        obj["prop2"] = obj.value("prop1");
        QCOMPARE(obj.size(), 2);
        QCOMPARE(obj.value("prop1"), "TEST");
        QCOMPARE(obj.value("prop2"), "TEST");
    }
    {
        // see QTBUG-83366
        QJsonObject obj;
        obj["value"] = "TEST";
        QCOMPARE(obj.size(), 1);
        QCOMPARE(obj.value("value"), "TEST");

        obj["prop2"] = obj.value("value");
        QCOMPARE(obj.size(), 2);
        QCOMPARE(obj.value("value"), "TEST");
        QCOMPARE(obj.value("prop2"), "TEST");
    }
    {
        QJsonObject obj;
        obj["value"] = "TEST";
        QCOMPARE(obj.size(), 1);
        QCOMPARE(obj.value("value"), "TEST");

        // same as previous, but this is a QJsonValueRef
        QJsonValueRef rv = obj["prop2"];
        rv = obj["value"];
        QCOMPARE(obj.size(), 2);
        QCOMPARE(obj.value("value"), "TEST");
        QCOMPARE(obj.value("prop2"), "TEST");
    }
    {
        QJsonObject obj;
        obj["value"] = "TEST";
        QCOMPARE(obj.size(), 1);
        QCOMPARE(obj.value("value"), "TEST");

        // same as previous, but this is a QJsonValueRef
        QJsonValueRef rv = obj["value"];
        obj["prop2"] = rv;
        QCOMPARE(obj.size(), 2);
        QCOMPARE(obj.value("value"), "TEST");
        QEXPECT_FAIL("", "QTBUG-83398: design flaw: the obj[] call invalidates the QJsonValueRef", Continue);
        QCOMPARE(obj.value("prop2"), "TEST");
    }
    {
        QJsonObject obj;
        obj["value"] = "TEST";
        QCOMPARE(obj.size(), 1);
        QCOMPARE(obj.value("value"), "TEST");

        QJsonValueRef v = obj["value"];
        QJsonObject obj2 = obj;
        obj.insert("prop2", v);
        QCOMPARE(obj.size(), 2);
        QCOMPARE(obj.value("value"), "TEST");
        QCOMPARE(obj.value("prop2"), "TEST");
        QCOMPARE(obj2.size(), 1);
        QCOMPARE(obj2.value("value"), "TEST");
    }
}

void tst_QtJson::testArraySimple()
{
    QJsonArray array;
    array.append(999.);
    array.append(QString::fromLatin1("test"));
    array.append(true);

    QJsonValue val = array.at(0);
    QCOMPARE(array.at(0).toDouble(), 999.);
    QCOMPARE(array.at(1).toString(), QString("test"));
    QCOMPARE(array.at(2).toBool(), true);
    QCOMPARE(array.size(), 3);

    // if we put a JsonValue into the JsonArray and retrieve
    // it, it should be identical.
    QJsonValue value(QLatin1String("foo"));
    array.append(value);
    QCOMPARE(array.at(3), value);

    int size = array.size();
    array.removeAt(2);
    --size;
    QCOMPARE(array.size(), size);

    QJsonValue taken = array.takeAt(0);
    --size;
    QCOMPARE(taken.toDouble(), 999.);
    QCOMPARE(array.size(), size);

    // check whether null values work
    array.append(QJsonValue());
    ++size;
    QCOMPARE(array.size(), size);
    QCOMPARE(array.last().type(), QJsonValue::Null);
    QCOMPARE(array.last(), QJsonValue());

    QCOMPARE(array.first().type(), QJsonValue::String);
    QCOMPARE(array.first(), QJsonValue(QLatin1String("test")));

    array.prepend(false);
    QCOMPARE(array.first().type(), QJsonValue::Bool);
    QCOMPARE(array.first(), QJsonValue(false));

    QCOMPARE(array.at(-1), QJsonValue(QJsonValue::Undefined));
    QCOMPARE(array.at(array.size()), QJsonValue(QJsonValue::Undefined));

    array.replace(0, -555.);
    QCOMPARE(array.first().type(), QJsonValue::Double);
    QCOMPARE(array.first(), QJsonValue(-555.));
    QCOMPARE(array.at(1).type(), QJsonValue::String);
    QCOMPARE(array.at(1), QJsonValue(QLatin1String("test")));
}

void tst_QtJson::testArrayInsertCopies()
{
    {
        QJsonArray array;
        array.append("TEST");
        QCOMPARE(array.size(), 1);
        QCOMPARE(array.at(0), "TEST");

        array.append(array.at(0));
        QCOMPARE(array.size(), 2);
        QCOMPARE(array.at(0), "TEST");
        QCOMPARE(array.at(1), "TEST");
    }
    {
        QJsonArray array;
        array.append("TEST");
        QCOMPARE(array.size(), 1);
        QCOMPARE(array.at(0), "TEST");

        array.prepend(array.at(0));
        QCOMPARE(array.size(), 2);
        QCOMPARE(array.at(0), "TEST");
        QCOMPARE(array.at(1), "TEST");
    }
}

void tst_QtJson::testValueObject()
{
    QJsonObject object;
    object.insert("number", 999.);
    object.insert("string", QLatin1String("test"));
    object.insert("boolean", true);

    QJsonValue value(object);

    // if we don't modify the original JsonObject, toObject()
    // on the JsonValue should return the same object (non-detached).
    QCOMPARE(value.toObject(), object);

    // if we modify the original object, it should detach
    object.insert("test", QJsonValue(QLatin1String("test")));
    QVERIFY2(value.toObject() != object, "object should have detached");
}

void tst_QtJson::testValueArray()
{
    QJsonArray array;
    array.append(999.);
    array.append(QLatin1String("test"));
    array.append(true);

    QJsonValue value(array);

    // if we don't modify the original JsonArray, toArray()
    // on the JsonValue should return the same object (non-detached).
    QCOMPARE(value.toArray(), array);

    // if we modify the original array, it should detach
    array.append(QLatin1String("test"));
    QVERIFY2(value.toArray() != array, "array should have detached");
}

void tst_QtJson::testObjectNested()
{
    QJsonObject inner, outer;
    inner.insert("number", 999.);
    outer.insert("nested", inner);

    // if we don't modify the original JsonObject, value()
    // should return the same object (non-detached).
    QJsonObject value = outer.value("nested").toObject();
    QCOMPARE(value, inner);
    QCOMPARE(value.value("number").toDouble(), 999.);

    // if we modify the original object, it should detach and not
    // affect the nested object
    inner.insert("number", 555.);
    value = outer.value("nested").toObject();
    QVERIFY2(inner.value("number").toDouble() != value.value("number").toDouble(),
             "object should have detached");

    // array in object
    QJsonArray array;
    array.append(123.);
    array.append(456.);
    outer.insert("array", array);
    QCOMPARE(outer.value("array").toArray(), array);
    QCOMPARE(outer.value("array").toArray().at(1).toDouble(), 456.);

    // two deep objects
    QJsonObject twoDeep;
    twoDeep.insert("boolean", true);
    inner.insert("nested", twoDeep);
    outer.insert("nested", inner);
    QCOMPARE(outer.value("nested").toObject().value("nested").toObject(), twoDeep);
    QCOMPARE(outer.value("nested").toObject().value("nested").toObject().value("boolean").toBool(),
             true);
}

void tst_QtJson::testArrayNested()
{
    QJsonArray inner, outer;
    inner.append(999.);
    outer.append(inner);

    // if we don't modify the original JsonArray, value()
    // should return the same array (non-detached).
    QJsonArray value = outer.at(0).toArray();
    QCOMPARE(value, inner);
    QCOMPARE(value.at(0).toDouble(), 999.);

    // if we modify the original array, it should detach and not
    // affect the nested array
    inner.append(555.);
    value = outer.at(0).toArray();
    QVERIFY2(inner.size() != value.size(), "array should have detached");

    // objects in arrays
    QJsonObject object;
    object.insert("boolean", true);
    outer.append(object);
    QCOMPARE(outer.last().toObject(), object);
    QCOMPARE(outer.last().toObject().value("boolean").toBool(), true);

    // two deep arrays
    QJsonArray twoDeep;
    twoDeep.append(QJsonValue(QString::fromLatin1("nested")));
    inner.append(twoDeep);
    outer.append(inner);
    QCOMPARE(outer.last().toArray().last().toArray(), twoDeep);
    QCOMPARE(outer.last().toArray().last().toArray().at(0).toString(), QString("nested"));
}

void tst_QtJson::testArrayNestedEmpty()
{
    QJsonObject object;
    QJsonArray inner;
    object.insert("inner", inner);
    QJsonValue val = object.value("inner");
    QJsonArray value = object.value("inner").toArray();
    QVERIFY(QJsonDocument(value).isArray());
    QCOMPARE(value.size(), 0);
    QCOMPARE(value, inner);
    QCOMPARE(value.size(), 0);
    object.insert("count", 0.);
    QCOMPARE(object.value("inner").toArray().size(), 0);
    QVERIFY(object.value("inner").toArray().isEmpty());
    QJsonDocument(object).toBinaryData();
    QCOMPARE(object.value("inner").toArray().size(), 0);
}

void tst_QtJson::testObjectNestedEmpty()
{
    QJsonObject object;
    QJsonObject inner;
    QJsonObject inner2;
    object.insert("inner", inner);
    object.insert("inner2", inner2);
    QJsonObject value = object.value("inner").toObject();
    QVERIFY(QJsonDocument(value).isObject());
    QCOMPARE(value.size(), 0);
    QCOMPARE(value, inner);
    QCOMPARE(value.size(), 0);
    object.insert("count", 0.);
    QCOMPARE(object.value("inner").toObject().size(), 0);
    QCOMPARE(object.value("inner").type(), QJsonValue::Object);
    QJsonDocument(object).toBinaryData();
    QVERIFY(object.value("inner").toObject().isEmpty());
    QVERIFY(object.value("inner2").toObject().isEmpty());
    QJsonDocument doc = QJsonDocument::fromBinaryData(QJsonDocument(object).toBinaryData());
    QVERIFY(!doc.isNull());
    QJsonObject reconstituted(doc.object());
    QCOMPARE(reconstituted.value("inner").toObject().size(), 0);
    QCOMPARE(reconstituted.value("inner").type(), QJsonValue::Object);
    QCOMPARE(reconstituted.value("inner2").type(), QJsonValue::Object);
}

void tst_QtJson::testArrayComfortOperators()
{
    QJsonArray first;
    first.append(123.);
    first.append(QLatin1String("foo"));

    QJsonArray second = QJsonArray() << 123. << QLatin1String("foo");
    QCOMPARE(first, second);

    first = first + QLatin1String("bar");
    second += QLatin1String("bar");
    QCOMPARE(first, second);
}

void tst_QtJson::testValueRef()
{
    QJsonArray array;
    array.append(1.);
    array.append(2.);
    array.append(3.);
    array.append(4);
    array.append(4.1);
    array[1] = false;

    QCOMPARE(array.size(), 5);
    QCOMPARE(array.at(0).toDouble(), 1.);
    QCOMPARE(array.at(2).toDouble(), 3.);
    QCOMPARE(array.at(3).toInt(), 4);
    QCOMPARE(array.at(4).toInt(), 0);
    QCOMPARE(array.at(1).type(), QJsonValue::Bool);
    QCOMPARE(array.at(1).toBool(), false);

    QJsonObject object;
    object[QLatin1String("key")] = true;
    QCOMPARE(object.size(), 1);
    object.insert(QLatin1String("null"), QJsonValue());
    QCOMPARE(object.value(QLatin1String("null")), QJsonValue());
    object[QLatin1String("null")] = 100.;
    QCOMPARE(object.value(QLatin1String("null")).type(), QJsonValue::Double);
    QJsonValue val = qAsConst(object)[QLatin1String("null")];
    QCOMPARE(val.toDouble(), 100.);
    QCOMPARE(object.size(), 2);

    array[1] = array[2] = object[QLatin1String("key")] = 42;
    QCOMPARE(array[1], array[2]);
    QCOMPARE(array[2], object[QLatin1String("key")]);
    QCOMPARE(object.value(QLatin1String("key")), QJsonValue(42));
}

void tst_QtJson::testObjectIteration()
{
    QJsonObject object;

    for (QJsonObject::iterator it = object.begin(); it != object.end(); ++it)
        QVERIFY(false);

    const QString property = "kkk";
    object.insert(property, 11);
    object.take(property);
    for (QJsonObject::iterator it = object.begin(); it != object.end(); ++it)
        QVERIFY(false);

    for (int i = 0; i < 10; ++i)
        object[QString::number(i)] = (double)i;

    QCOMPARE(object.size(), 10);

    QCOMPARE(object.begin()->toDouble(), object.constBegin()->toDouble());

    for (QJsonObject::iterator it = object.begin(); it != object.end(); ++it) {
        QJsonValue value = it.value();
        QCOMPARE((double)it.key().toInt(), value.toDouble());
    }

    {
        QJsonObject object2 = object;
        QCOMPARE(object, object2);

        QJsonValue val = *object2.begin();
        object2.erase(object2.begin());
        QCOMPARE(object.size(), 10);
        QCOMPARE(object2.size(), 9);

        for (QJsonObject::const_iterator it = object2.constBegin(); it != object2.constEnd(); ++it) {
            QJsonValue value = it.value();
            QVERIFY(it.value() != val);
            QCOMPARE((double)it.key().toInt(), value.toDouble());
        }
    }

    {
        QJsonObject object2 = object;
        QCOMPARE(object, object2);

        QJsonObject::iterator it = object2.find(QString::number(5));
        QJsonValue val = *it;
        object2.erase(it);
        QCOMPARE(object.size(), 10);
        QCOMPARE(object2.size(), 9);

        for (QJsonObject::const_iterator it = object2.constBegin(); it != object2.constEnd(); ++it) {
            QJsonValue value = it.value();
            QVERIFY(it.value() != val);
            QCOMPARE((double)it.key().toInt(), value.toDouble());
        }
    }

    {
        QJsonObject::Iterator it = object.begin();
        it += 5;
        QCOMPARE(QJsonValue(it.value()).toDouble(), 5.);
        it -= 3;
        QCOMPARE(QJsonValue(it.value()).toDouble(), 2.);
        QJsonObject::Iterator it2 = it + 5;
        QCOMPARE(QJsonValue(it2.value()).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(QJsonValue(it2.value()).toDouble(), 1.);
    }

    {
        QJsonObject::ConstIterator it = object.constBegin();
        it += 5;
        QCOMPARE(QJsonValue(it.value()).toDouble(), 5.);
        it -= 3;
        QCOMPARE(QJsonValue(it.value()).toDouble(), 2.);
        QJsonObject::ConstIterator it2 = it + 5;
        QCOMPARE(QJsonValue(it2.value()).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(QJsonValue(it2.value()).toDouble(), 1.);
    }

    QJsonObject::Iterator it = object.begin();
    while (!object.isEmpty())
        it = object.erase(it);
    QCOMPARE(object.size() , 0);
    QCOMPARE(it, object.end());
}

void tst_QtJson::testArrayIteration()
{
    QJsonArray array;
    for (int i = 0; i < 10; ++i)
        array.append(i);

    QCOMPARE(array.size(), 10);

    int i = 0;
    for (QJsonArray::iterator it = array.begin(); it != array.end(); ++it, ++i) {
        QJsonValue value = (*it);
        QCOMPARE((double)i, value.toDouble());
    }

    QCOMPARE(array.begin()->toDouble(), array.constBegin()->toDouble());

    {
        QJsonArray array2 = array;
        QCOMPARE(array, array2);

        QJsonValue val = *array2.begin();
        array2.erase(array2.begin());
        QCOMPARE(array.size(), 10);
        QCOMPARE(array2.size(), 9);

        i = 1;
        for (QJsonArray::const_iterator it = array2.constBegin(); it != array2.constEnd(); ++it, ++i) {
            QJsonValue value = (*it);
            QCOMPARE((double)i, value.toDouble());
        }
    }

    {
        QJsonArray::Iterator it = array.begin();
        it += 5;
        QCOMPARE(QJsonValue((*it)).toDouble(), 5.);
        it -= 3;
        QCOMPARE(QJsonValue((*it)).toDouble(), 2.);
        QJsonArray::Iterator it2 = it + 5;
        QCOMPARE(QJsonValue(*it2).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(QJsonValue(*it2).toDouble(), 1.);
    }

    {
        QJsonArray::ConstIterator it = array.constBegin();
        it += 5;
        QCOMPARE(QJsonValue((*it)).toDouble(), 5.);
        it -= 3;
        QCOMPARE(QJsonValue((*it)).toDouble(), 2.);
        QJsonArray::ConstIterator it2 = it + 5;
        QCOMPARE(QJsonValue(*it2).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(QJsonValue(*it2).toDouble(), 1.);
    }

    QJsonArray::Iterator it = array.begin();
    while (!array.isEmpty())
        it = array.erase(it);
    QCOMPARE(array.size() , 0);
    QCOMPARE(it, array.end());
}

void tst_QtJson::testObjectFind()
{
    QJsonObject object;
    for (int i = 0; i < 10; ++i)
        object[QString::number(i)] = i;

    QCOMPARE(object.size(), 10);

    QJsonObject::iterator it = object.find(QLatin1String("1"));
    QCOMPARE((*it).toDouble(), 1.);
    it = object.find(QString("11"));
    QCOMPARE((*it).type(), QJsonValue::Undefined);
    QCOMPARE(it, object.end());

    QJsonObject::const_iterator cit = object.constFind(QLatin1String("1"));
    QCOMPARE((*cit).toDouble(), 1.);
    cit = object.constFind(QString("11"));
    QCOMPARE((*it).type(), QJsonValue::Undefined);
    QCOMPARE(it, object.end());
}

void tst_QtJson::testDocument()
{
    QJsonDocument doc;
    QCOMPARE(doc.isEmpty(), true);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), false);

    QJsonObject object;
    doc.setObject(object);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), true);

    object.insert(QLatin1String("Key"), QLatin1String("Value"));
    doc.setObject(object);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), true);
    QCOMPARE(doc.object(), object);
    QCOMPARE(doc.array(), QJsonArray());

    doc = QJsonDocument();
    QCOMPARE(doc.isEmpty(), true);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), false);

    QJsonArray array;
    doc.setArray(array);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), true);
    QCOMPARE(doc.isObject(), false);

    array.append(QLatin1String("Value"));
    doc.setArray(array);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), true);
    QCOMPARE(doc.isObject(), false);
    QCOMPARE(doc.array(), array);
    QCOMPARE(doc.object(), QJsonObject());

    QJsonObject outer;
    outer.insert(QLatin1String("outerKey"), 22);
    QJsonObject inner;
    inner.insert(QLatin1String("innerKey"), 42);
    outer.insert(QLatin1String("innter"), inner);
    QJsonArray innerArray;
    innerArray.append(23);
    outer.insert(QLatin1String("innterArray"), innerArray);

    QJsonDocument doc2(outer.value(QLatin1String("innter")).toObject());
    QVERIFY(doc2.object().contains(QLatin1String("innerKey")));
    QCOMPARE(doc2.object().value(QLatin1String("innerKey")), QJsonValue(42));

    QJsonDocument doc3;
    doc3.setObject(outer.value(QLatin1String("innter")).toObject());
    QCOMPARE(doc3.isArray(), false);
    QCOMPARE(doc3.isObject(), true);
    QVERIFY(doc3.object().contains(QString("innerKey")));
    QCOMPARE(doc3.object().value(QLatin1String("innerKey")), QJsonValue(42));

    QJsonDocument doc4(outer.value(QLatin1String("innterArray")).toArray());
    QCOMPARE(doc4.isArray(), true);
    QCOMPARE(doc4.isObject(), false);
    QCOMPARE(doc4.array().size(), 1);
    QCOMPARE(doc4.array().at(0), QJsonValue(23));

    QJsonDocument doc5;
    doc5.setArray(outer.value(QLatin1String("innterArray")).toArray());
    QCOMPARE(doc5.isArray(), true);
    QCOMPARE(doc5.isObject(), false);
    QCOMPARE(doc5.array().size(), 1);
    QCOMPARE(doc5.array().at(0), QJsonValue(23));
}

void tst_QtJson::nullValues()
{
    QJsonArray array;
    array.append(QJsonValue());

    QCOMPARE(array.size(), 1);
    QCOMPARE(array.at(0), QJsonValue());

    QJsonObject object;
    object.insert(QString("key"), QJsonValue());
    QCOMPARE(object.contains(QLatin1String("key")), true);
    QCOMPARE(object.size(), 1);
    QCOMPARE(object.value(QString("key")), QJsonValue());
}

void tst_QtJson::nullArrays()
{
    QJsonArray nullArray;
    QJsonArray nonNull;
    nonNull.append(QLatin1String("bar"));

    QCOMPARE(nullArray, QJsonArray());
    QVERIFY(nullArray != nonNull);
    QVERIFY(nonNull != nullArray);

    QCOMPARE(nullArray.size(), 0);
    QCOMPARE(nullArray.takeAt(0), QJsonValue(QJsonValue::Undefined));
    QCOMPARE(nullArray.first(), QJsonValue(QJsonValue::Undefined));
    QCOMPARE(nullArray.last(), QJsonValue(QJsonValue::Undefined));
    nullArray.removeAt(0);
    nullArray.removeAt(-1);

    nullArray.append(QString("bar"));
    nullArray.removeAt(0);

    QCOMPARE(nullArray.size(), 0);
    QCOMPARE(nullArray.takeAt(0), QJsonValue(QJsonValue::Undefined));
    QCOMPARE(nullArray.first(), QJsonValue(QJsonValue::Undefined));
    QCOMPARE(nullArray.last(), QJsonValue(QJsonValue::Undefined));
    nullArray.removeAt(0);
    nullArray.removeAt(-1);
}

void tst_QtJson::nullObject()
{
    QJsonObject nullObject;
    QJsonObject nonNull;
    nonNull.insert(QLatin1String("foo"), QLatin1String("bar"));

    QCOMPARE(nullObject, QJsonObject());
    QVERIFY(nullObject != nonNull);
    QVERIFY(nonNull != nullObject);

    QCOMPARE(nullObject.size(), 0);
    QCOMPARE(nullObject.keys(), QStringList());
    nullObject.remove("foo");
    QCOMPARE(nullObject, QJsonObject());
    QCOMPARE(nullObject.take("foo"), QJsonValue(QJsonValue::Undefined));
    QCOMPARE(nullObject.contains("foo"), false);

    nullObject.insert("foo", QString("bar"));
    nullObject.remove("foo");

    QCOMPARE(nullObject.size(), 0);
    QCOMPARE(nullObject.keys(), QStringList());
    nullObject.remove("foo");
    QCOMPARE(nullObject, QJsonObject());
    QCOMPARE(nullObject.take("foo"), QJsonValue(QJsonValue::Undefined));
    QCOMPARE(nullObject.contains("foo"), false);
}

void tst_QtJson::constNullObject()
{
    const QJsonObject nullObject;
    QJsonObject nonNull;
    nonNull.insert(QLatin1String("foo"), QLatin1String("bar"));

    QCOMPARE(nullObject, QJsonObject());
    QVERIFY(nullObject != nonNull);
    QVERIFY(nonNull != nullObject);

    QCOMPARE(nullObject.size(), 0);
    QCOMPARE(nullObject.keys(), QStringList());
    QCOMPARE(nullObject, QJsonObject());
    QCOMPARE(nullObject.contains("foo"), false);
    QCOMPARE(nullObject["foo"], QJsonValue(QJsonValue::Undefined));
}

void tst_QtJson::keySorting_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<QStringList>("sortedKeys");

    QStringList list = {"A", "B"};
    QTest::newRow("sorted-ascii-2") << R"({ "A": false, "B": true })" << list;
    const char *json = "{ \"B\": true, \"A\": false }";
    QTest::newRow("unsorted-ascii-2") << json << list;

    list = QStringList{"A", "B", "C", "D", "E"};
    QTest::newRow("sorted-ascii-5") << R"({"A": 1, "B": 2, "C": 3, "D": 4, "E": 5})" << list;
    QTest::newRow("unsorted-ascii-5") << R"({"A": 1, "C": 3, "D": 4, "B": 2, "E": 5})" << list;
    QTest::newRow("inverse-sorted-ascii-5") << R"({"E": 5, "D": 4, "C": 3, "B": 2, "A": 1})" << list;

    list = QStringList{"á", "é", "í", "ó", "ú"};
    QTest::newRow("sorted-latin1") << R"({"á": 1, "é": 2, "í": 3, "ó": 4, "ú": 5})" << list;
    QTest::newRow("unsorted-latin1") << R"({"á": 1, "í": 3, "ó": 4, "é": 2, "ú": 5})" << list;
    QTest::newRow("inverse-sorted-latin1") << R"({"ú": 5, "ó": 4, "í": 3, "é": 2, "á": 1})" << list;

    QTest::newRow("sorted-escaped-latin1") << R"({"\u00e1": 1, "\u00e9": 2, "\u00ed": 3, "\u00f3": 4, "\u00fa": 5})" << list;
    QTest::newRow("unsorted-escaped-latin1") << R"({"\u00e1": 1, "\u00ed": 3, "\u00f3": 4, "\u00e9": 2, "\u00fa": 5})" << list;
    QTest::newRow("inverse-sorted-escaped-latin1") << R"({"\u00fa": 5, "\u00f3": 4, "\u00ed": 3, "\u00e9": 2, "\u00e1": 1})" << list;

    list = QStringList{"A", "α", "Я", "€", "测"};
    QTest::newRow("sorted") << R"({"A": 1, "α": 2, "Я": 3, "€": 4, "测": 5})" << list;
    QTest::newRow("unsorted") << R"({"A": 1, "Я": 3, "€": 4, "α": 2, "测": 5})" << list;
    QTest::newRow("inverse-sorted") << R"({"测": 5, "€": 4, "Я": 3, "α": 2, "A": 1})" << list;

    QTest::newRow("sorted-escaped") << R"({"A": 1, "\u03b1": 2, "\u042f": 3, "\u20ac": 4, "\u6d4b": 5})" << list;
    QTest::newRow("unsorted-escaped") << R"({"A": 1, "\u042f": 3, "\u20ac": 4, "\u03b1": 2, "\u6d4b": 5})" << list;
    QTest::newRow("inverse-sorted-escaped") << R"({"\u6d4b": 5, "\u20ac": 4, "\u042f": 3, "\u03b1": 2, "A": 1})" << list;
}

void tst_QtJson::keySorting()
{
    QFETCH(QString, json);
    QFETCH(QStringList, sortedKeys);
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());

    QCOMPARE(doc.isObject(), true);

    QJsonObject o = doc.object();
    QCOMPARE(o.size(), sortedKeys.size());
    QCOMPARE(o.keys(), sortedKeys);
    QJsonObject::const_iterator it = o.constBegin();
    QStringList::const_iterator it2 = sortedKeys.constBegin();
    for ( ; it != o.constEnd(); ++it, ++it2)
        QCOMPARE(it.key(), *it2);
}

void tst_QtJson::undefinedValues()
{
    QJsonObject object;
    object.insert("Key", QJsonValue(QJsonValue::Undefined));
    QCOMPARE(object.size(), 0);
    object["Key"] = QJsonValue(QJsonValue::Undefined);
    QCOMPARE(object.size(), 0);

    object.insert("Key", QLatin1String("Value"));
    QCOMPARE(object.size(), 1);
    QCOMPARE(object.value("Key").type(), QJsonValue::String);
    QCOMPARE(object.value("foo").type(), QJsonValue::Undefined);
    object.insert("Key", QJsonValue(QJsonValue::Undefined));
    QCOMPARE(object.size(), 0);
    QCOMPARE(object.value("Key").type(), QJsonValue::Undefined);

    QJsonArray array;
    array.append(QJsonValue(QJsonValue::Undefined));
    QCOMPARE(array.size(), 1);
    QCOMPARE(array.at(0).type(), QJsonValue::Null);

    QCOMPARE(array.at(1).type(), QJsonValue::Undefined);
    QCOMPARE(array.at(-1).type(), QJsonValue::Undefined);
}

void tst_QtJson::fromVariant_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QJsonValue>("jsonvalue");

    bool boolValue = true;
    int intValue = -1;
    uint uintValue = 1;
    long long longlongValue = -2;
    unsigned long long ulonglongValue = 2;
    float floatValue = 3.3f;
    double doubleValue = 4.4;
    QString stringValue("str");

    QStringList stringList;
    stringList.append(stringValue);
    stringList.append("str2");
    QJsonArray jsonArray_string;
    jsonArray_string.append(stringValue);
    jsonArray_string.append("str2");

    QVariantList variantList;
    variantList.append(boolValue);
    variantList.append(floatValue);
    variantList.append(doubleValue);
    variantList.append(stringValue);
    variantList.append(stringList);
    variantList.append(QVariant::fromValue(nullptr));
    variantList.append(QVariant());
    QJsonArray jsonArray_variant;
    jsonArray_variant.append(boolValue);
    jsonArray_variant.append(floatValue);
    jsonArray_variant.append(doubleValue);
    jsonArray_variant.append(stringValue);
    jsonArray_variant.append(jsonArray_string);
    jsonArray_variant.append(QJsonValue(QJsonValue::Null));
    jsonArray_variant.append(QJsonValue());

    QVariantMap variantMap;
    variantMap["bool"] = boolValue;
    variantMap["float"] = floatValue;
    variantMap["string"] = stringValue;
    variantMap["array"] = variantList;
    variantMap["null"] = QVariant::fromValue(nullptr);
    variantMap["default"] = QVariant();
    QVariantHash variantHash;
    variantHash["bool"] = boolValue;
    variantHash["float"] = floatValue;
    variantHash["string"] = stringValue;
    variantHash["array"] = variantList;
    variantHash["null"] = QVariant::fromValue(nullptr);
    variantHash["default"] = QVariant();
    QJsonObject jsonObject;
    jsonObject["bool"] = boolValue;
    jsonObject["float"] = floatValue;
    jsonObject["string"] = stringValue;
    jsonObject["array"] = jsonArray_variant;
    jsonObject["null"] = QJsonValue::Null;
    jsonObject["default"] = QJsonValue();

    QTest::newRow("default") << QVariant() <<  QJsonValue(QJsonValue::Null);
    QTest::newRow("nullptr") << QVariant::fromValue(nullptr) <<  QJsonValue(QJsonValue::Null);
    QTest::newRow("bool") << QVariant(boolValue) <<  QJsonValue(boolValue);
    QTest::newRow("int") << QVariant(intValue) <<  QJsonValue(intValue);
    QTest::newRow("uint") << QVariant(uintValue) <<  QJsonValue(static_cast<double>(uintValue));
    QTest::newRow("longlong") << QVariant(longlongValue) <<  QJsonValue(longlongValue);
    QTest::newRow("ulonglong") << QVariant(ulonglongValue) <<  QJsonValue(static_cast<double>(ulonglongValue));
    QTest::newRow("float") << QVariant(floatValue) <<  QJsonValue(floatValue);
    QTest::newRow("double") << QVariant(doubleValue) <<  QJsonValue(doubleValue);
    QTest::newRow("string") << QVariant(stringValue) <<  QJsonValue(stringValue);
    QTest::newRow("stringList") << QVariant(stringList) <<  QJsonValue(jsonArray_string);
    QTest::newRow("variantList") << QVariant(variantList) <<  QJsonValue(jsonArray_variant);
    QTest::newRow("variantMap") << QVariant(variantMap) <<  QJsonValue(jsonObject);
    QTest::newRow("variantHash") << QVariant(variantHash) <<  QJsonValue(jsonObject);
}

// replaces QVariant() with QVariant(nullptr)
static QVariant normalizedVariant(const QVariant &v)
{
    switch (v.userType()) {
    case QMetaType::UnknownType:
        return QVariant::fromValue(nullptr);
    case QMetaType::QVariantList: {
        const QVariantList in = v.toList();
        QVariantList out;
        out.reserve(in.size());
        for (const QVariant &v : in)
            out << normalizedVariant(v);
        return out;
    }
    case QMetaType::QVariantMap: {
        const QVariantMap in = v.toMap();
        QVariantMap out;
        for (auto it = in.begin(); it != in.end(); ++it)
            out.insert(it.key(), normalizedVariant(it.value()));
        return out;
    }
    case QMetaType::QVariantHash: {
        const QVariantHash in = v.toHash();
        QVariantHash out;
        for (auto it = in.begin(); it != in.end(); ++it)
            out.insert(it.key(), normalizedVariant(it.value()));
        return out;
    }

    default:
        return v;
    }
}

void tst_QtJson::fromVariant()
{
    QFETCH( QVariant, variant );
    QFETCH( QJsonValue, jsonvalue );

    QCOMPARE(QJsonValue::fromVariant(variant), jsonvalue);
    QCOMPARE(normalizedVariant(variant).toJsonValue(), jsonvalue);
}

void tst_QtJson::fromVariantSpecial_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QJsonValue>("jsonvalue");

    // Qt types with special encoding
    QTest::newRow("url") << QVariant(QUrl("https://example.com/\xc2\xa9 "))
                         << QJsonValue("https://example.com/%C2%A9%20");
    QTest::newRow("uuid") << QVariant(QUuid(0x40c01df6, 0x1ad5, 0x4762, 0x9c, 0xfe, 0xfd, 0xba, 0xfa, 0xb5, 0xde, 0xf8))
                          << QJsonValue("40c01df6-1ad5-4762-9cfe-fdbafab5def8");
}

void tst_QtJson::fromVariantSpecial()
{
    QFETCH( QVariant, variant );
    QFETCH( QJsonValue, jsonvalue );

    QCOMPARE(QJsonValue::fromVariant(variant), jsonvalue);
}

void tst_QtJson::toVariant_data()
{
    fromVariant_data();
}

void tst_QtJson::toVariant()
{
    QFETCH( QVariant, variant );
    QFETCH( QJsonValue, jsonvalue );

    QCOMPARE(jsonvalue.toVariant(), normalizedVariant(variant));
}

void tst_QtJson::fromVariantMap()
{
    QVariantMap map;
    map.insert(QLatin1String("key1"), QLatin1String("value1"));
    map.insert(QLatin1String("key2"), QLatin1String("value2"));
    QJsonObject object = QJsonObject::fromVariantMap(map);
    QCOMPARE(object.size(), 2);
    QCOMPARE(object.value(QLatin1String("key1")), QJsonValue(QLatin1String("value1")));
    QCOMPARE(object.value(QLatin1String("key2")), QJsonValue(QLatin1String("value2")));

    QVariantList list;
    list.append(true);
    list.append(QVariant());
    list.append(999.);
    list.append(QLatin1String("foo"));
    map.insert("list", list);
    object = QJsonObject::fromVariantMap(map);
    QCOMPARE(object.size(), 3);
    QCOMPARE(object.value(QLatin1String("key1")), QJsonValue(QLatin1String("value1")));
    QCOMPARE(object.value(QLatin1String("key2")), QJsonValue(QLatin1String("value2")));
    QCOMPARE(object.value(QLatin1String("list")).type(), QJsonValue::Array);
    QJsonArray array = object.value(QLatin1String("list")).toArray();
    QCOMPARE(array.size(), 4);
    QCOMPARE(array.at(0).type(), QJsonValue::Bool);
    QCOMPARE(array.at(0).toBool(), true);
    QCOMPARE(array.at(1).type(), QJsonValue::Null);
    QCOMPARE(array.at(2).type(), QJsonValue::Double);
    QCOMPARE(array.at(2).toDouble(), 999.);
    QCOMPARE(array.at(3).type(), QJsonValue::String);
    QCOMPARE(array.at(3).toString(), QLatin1String("foo"));
}

void tst_QtJson::fromVariantHash()
{
    QVariantHash map;
    map.insert(QLatin1String("key1"), QLatin1String("value1"));
    map.insert(QLatin1String("key2"), QLatin1String("value2"));
    QJsonObject object = QJsonObject::fromVariantHash(map);
    QCOMPARE(object.size(), 2);
    QCOMPARE(object.value(QLatin1String("key1")), QJsonValue(QLatin1String("value1")));
    QCOMPARE(object.value(QLatin1String("key2")), QJsonValue(QLatin1String("value2")));
}

void tst_QtJson::toVariantMap()
{
    QCOMPARE(QMetaType::Type(QJsonValue(QJsonObject()).toVariant().type()), QMetaType::QVariantMap); // QTBUG-32524

    QJsonObject object;
    QVariantMap map = object.toVariantMap();
    QVERIFY(map.isEmpty());

    object.insert("Key", QString("Value"));
    object.insert("null", QJsonValue());
    QJsonArray array;
    array.append(true);
    array.append(999.);
    array.append(QLatin1String("string"));
    array.append(QJsonValue::Null);
    object.insert("Array", array);

    map = object.toVariantMap();

    QCOMPARE(map.size(), 3);
    QCOMPARE(map.value("Key"), QVariant(QString("Value")));
    QCOMPARE(map.value("null"), QVariant::fromValue(nullptr));
    QCOMPARE(map.value("Array").type(), QVariant::List);
    QVariantList list = map.value("Array").toList();
    QCOMPARE(list.size(), 4);
    QCOMPARE(list.at(0), QVariant(true));
    QCOMPARE(list.at(1), QVariant(999.));
    QCOMPARE(list.at(2), QVariant(QLatin1String("string")));
    QCOMPARE(list.at(3), QVariant::fromValue(nullptr));
}

void tst_QtJson::toVariantHash()
{
    QJsonObject object;
    QVariantHash hash = object.toVariantHash();
    QVERIFY(hash.isEmpty());

    object.insert("Key", QString("Value"));
    object.insert("null", QJsonValue::Null);
    QJsonArray array;
    array.append(true);
    array.append(999.);
    array.append(QLatin1String("string"));
    array.append(QJsonValue::Null);
    object.insert("Array", array);

    hash = object.toVariantHash();

    QCOMPARE(hash.size(), 3);
    QCOMPARE(hash.value("Key"), QVariant(QString("Value")));
    QCOMPARE(hash.value("null"), QVariant::fromValue(nullptr));
    QCOMPARE(hash.value("Array").type(), QVariant::List);
    QVariantList list = hash.value("Array").toList();
    QCOMPARE(list.size(), 4);
    QCOMPARE(list.at(0), QVariant(true));
    QCOMPARE(list.at(1), QVariant(999.));
    QCOMPARE(list.at(2), QVariant(QLatin1String("string")));
    QCOMPARE(list.at(3), QVariant::fromValue(nullptr));
}

void tst_QtJson::toVariantList()
{
    QCOMPARE(QMetaType::Type(QJsonValue(QJsonArray()).toVariant().type()), QMetaType::QVariantList); // QTBUG-32524

    QJsonArray array;
    QVariantList list = array.toVariantList();
    QVERIFY(list.isEmpty());

    array.append(QString("Value"));
    array.append(QJsonValue());
    QJsonArray inner;
    inner.append(true);
    inner.append(999.);
    inner.append(QLatin1String("string"));
    inner.append(QJsonValue());
    array.append(inner);

    list = array.toVariantList();

    QCOMPARE(list.size(), 3);
    QCOMPARE(list[0], QVariant(QString("Value")));
    QCOMPARE(list[1], QVariant::fromValue(nullptr));
    QCOMPARE(list[2].type(), QVariant::List);
    QVariantList vlist = list[2].toList();
    QCOMPARE(vlist.size(), 4);
    QCOMPARE(vlist.at(0), QVariant(true));
    QCOMPARE(vlist.at(1), QVariant(999.));
    QCOMPARE(vlist.at(2), QVariant(QLatin1String("string")));
    QCOMPARE(vlist.at(3), QVariant::fromValue(nullptr));
}

void tst_QtJson::toJson()
{
    // Test QJsonDocument::Indented format
    {
        QJsonObject object;
        object.insert("\\Key\n", QString("Value"));
        object.insert("null", QJsonValue());
        QJsonArray array;
        array.append(true);
        array.append(999.);
        array.append(QLatin1String("string"));
        array.append(QJsonValue());
        array.append(QLatin1String("\\\a\n\r\b\tabcABC\""));
        object.insert("Array", array);

        QByteArray json = QJsonDocument(object).toJson();

        QByteArray expected =
                "{\n"
                "    \"Array\": [\n"
                "        true,\n"
                "        999,\n"
                "        \"string\",\n"
                "        null,\n"
                "        \"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"\n"
                "    ],\n"
                "    \"\\\\Key\\n\": \"Value\",\n"
                "    \"null\": null\n"
                "}\n";
        QCOMPARE(json, expected);

        QJsonDocument doc;
        doc.setObject(object);
        json = doc.toJson();
        QCOMPARE(json, expected);

        doc.setArray(array);
        json = doc.toJson();
        expected =
                "[\n"
                "    true,\n"
                "    999,\n"
                "    \"string\",\n"
                "    null,\n"
                "    \"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"\n"
                "]\n";
        QCOMPARE(json, expected);
    }

    // Test QJsonDocument::Compact format
    {
        QJsonObject object;
        object.insert("\\Key\n", QString("Value"));
        object.insert("null", QJsonValue());
        QJsonArray array;
        array.append(true);
        array.append(999.);
        array.append(QLatin1String("string"));
        array.append(QJsonValue());
        array.append(QLatin1String("\\\a\n\r\b\tabcABC\""));
        object.insert("Array", array);

        QByteArray json = QJsonDocument(object).toJson(QJsonDocument::Compact);
        QByteArray expected =
                "{\"Array\":[true,999,\"string\",null,\"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"],\"\\\\Key\\n\":\"Value\",\"null\":null}";
        QCOMPARE(json, expected);

        QJsonDocument doc;
        doc.setObject(object);
        json = doc.toJson(QJsonDocument::Compact);
        QCOMPARE(json, expected);

        doc.setArray(array);
        json = doc.toJson(QJsonDocument::Compact);
        expected = "[true,999,\"string\",null,\"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"]";
        QCOMPARE(json, expected);
    }
}

void tst_QtJson::toJsonSillyNumericValues()
{
    QJsonObject object;
    QJsonArray array;
    array.append(QJsonValue(std::numeric_limits<double>::infinity()));  // encode to: null
    array.append(QJsonValue(-std::numeric_limits<double>::infinity())); // encode to: null
    array.append(QJsonValue(std::numeric_limits<double>::quiet_NaN())); // encode to: null
    object.insert("Array", array);

    QByteArray json = QJsonDocument(object).toJson();

    QByteArray expected =
            "{\n"
            "    \"Array\": [\n"
            "        null,\n"
            "        null,\n"
            "        null\n"
            "    ]\n"
            "}\n";

    QCOMPARE(json, expected);

    QJsonDocument doc;
    doc.setObject(object);
    json = doc.toJson();
    QCOMPARE(json, expected);
}

void tst_QtJson::toJsonLargeNumericValues()
{
    QJsonObject object;
    QJsonArray array;
    array.append(QJsonValue(1.234567)); // actual precision bug in Qt 5.0.0
    array.append(QJsonValue(1.7976931348623157e+308)); // JS Number.MAX_VALUE
    array.append(QJsonValue(5e-324));                  // JS Number.MIN_VALUE
    array.append(QJsonValue(std::numeric_limits<double>::min()));
    array.append(QJsonValue(std::numeric_limits<double>::max()));
    array.append(QJsonValue(std::numeric_limits<double>::epsilon()));
    array.append(QJsonValue(std::numeric_limits<double>::denorm_min()));
    array.append(QJsonValue(0.0));
    array.append(QJsonValue(-std::numeric_limits<double>::min()));
    array.append(QJsonValue(-std::numeric_limits<double>::max()));
    array.append(QJsonValue(-std::numeric_limits<double>::epsilon()));
    array.append(QJsonValue(-std::numeric_limits<double>::denorm_min()));
    array.append(QJsonValue(-0.0));
    array.append(QJsonValue(9007199254740992LL));  // JS Number max integer
    array.append(QJsonValue(-9007199254740992LL)); // JS Number min integer
    object.insert("Array", array);

    QByteArray json = QJsonDocument(object).toJson();

    QByteArray expected =
            "{\n"
            "    \"Array\": [\n"
            "        1.234567,\n"
            "        1.7976931348623157e+308,\n"
#ifdef QT_NO_DOUBLECONVERSION // "shortest" double conversion is not very short then
            "        4.9406564584124654e-324,\n"
            "        2.2250738585072014e-308,\n"
            "        1.7976931348623157e+308,\n"
            "        2.2204460492503131e-16,\n"
            "        4.9406564584124654e-324,\n"
            "        0,\n"
            "        -2.2250738585072014e-308,\n"
            "        -1.7976931348623157e+308,\n"
            "        -2.2204460492503131e-16,\n"
            "        -4.9406564584124654e-324,\n"
#else
            "        5e-324,\n"
            "        2.2250738585072014e-308,\n"
            "        1.7976931348623157e+308,\n"
            "        2.220446049250313e-16,\n"
            "        5e-324,\n"
            "        0,\n"
            "        -2.2250738585072014e-308,\n"
            "        -1.7976931348623157e+308,\n"
            "        -2.220446049250313e-16,\n"
            "        -5e-324,\n"
#endif
            "        0,\n"
            "        9007199254740992,\n"
            "        -9007199254740992\n"
            "    ]\n"
            "}\n";

#ifdef Q_OS_QNX
    QEXPECT_FAIL("", "See QTBUG-37066", Continue);
#endif
    QCOMPARE(json, expected);

    QJsonDocument doc;
    doc.setObject(object);
    json = doc.toJson();
#ifdef Q_OS_QNX
    QEXPECT_FAIL("", "See QTBUG-37066", Continue);
#endif
    QCOMPARE(json, expected);
}

void tst_QtJson::fromJson()
{
    {
        QByteArray json = "[\n    true\n]\n";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        QJsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        QCOMPARE(array.at(0).type(), QJsonValue::Bool);
        QCOMPARE(array.at(0).toBool(), true);
        QCOMPARE(doc.toJson(), json);
    }
    {
        //regression test: test if unicode_control_characters are correctly decoded
        QByteArray json = "[\n    \"" UNICODE_NON_CHARACTER "\"\n]\n";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        QJsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        QCOMPARE(array.at(0).type(), QJsonValue::String);
        QCOMPARE(array.at(0).toString(), QString::fromUtf8(UNICODE_NON_CHARACTER));
        QCOMPARE(doc.toJson(), json);
    }
    {
        QByteArray json = "[]";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        QJsonArray array = doc.array();
        QCOMPARE(array.size(), 0);
    }
    {
        QByteArray json = "{}";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        QJsonObject object = doc.object();
        QCOMPARE(object.size(), 0);
    }
    {
        QByteArray json = "{\n    \"Key\": true\n}\n";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        QJsonObject object = doc.object();
        QCOMPARE(object.size(), 1);
        QCOMPARE(object.value("Key"), QJsonValue(true));
        QCOMPARE(doc.toJson(), json);
    }
    {
        QByteArray json = "[ null, true, false, \"Foo\", 1, [], {} ]";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        QJsonArray array = doc.array();
        QCOMPARE(array.size(), 7);
        QCOMPARE(array.at(0).type(), QJsonValue::Null);
        QCOMPARE(array.at(1).type(), QJsonValue::Bool);
        QCOMPARE(array.at(1).toBool(), true);
        QCOMPARE(array.at(2).type(), QJsonValue::Bool);
        QCOMPARE(array.at(2).toBool(), false);
        QCOMPARE(array.at(3).type(), QJsonValue::String);
        QCOMPARE(array.at(3).toString(), QLatin1String("Foo"));
        QCOMPARE(array.at(4).type(), QJsonValue::Double);
        QCOMPARE(array.at(4).toDouble(), 1.);
        QCOMPARE(array.at(5).type(), QJsonValue::Array);
        QCOMPARE(array.at(5).toArray().size(), 0);
        QCOMPARE(array.at(6).type(), QJsonValue::Object);
        QCOMPARE(array.at(6).toObject().size(), 0);
    }
    {
        QByteArray json = "{ \"0\": null, \"1\": true, \"2\": false, \"3\": \"Foo\", \"4\": 1, \"5\": [], \"6\": {} }";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        QJsonObject object = doc.object();
        QCOMPARE(object.size(), 7);
        QCOMPARE(object.value("0").type(), QJsonValue::Null);
        QCOMPARE(object.value("1").type(), QJsonValue::Bool);
        QCOMPARE(object.value("1").toBool(), true);
        QCOMPARE(object.value("2").type(), QJsonValue::Bool);
        QCOMPARE(object.value("2").toBool(), false);
        QCOMPARE(object.value("3").type(), QJsonValue::String);
        QCOMPARE(object.value("3").toString(), QLatin1String("Foo"));
        QCOMPARE(object.value("4").type(), QJsonValue::Double);
        QCOMPARE(object.value("4").toDouble(), 1.);
        QCOMPARE(object.value("5").type(), QJsonValue::Array);
        QCOMPARE(object.value("5").toArray().size(), 0);
        QCOMPARE(object.value("6").type(), QJsonValue::Object);
        QCOMPARE(object.value("6").toObject().size(), 0);
    }
    {
        QByteArray compactJson = "{\"Array\": [true,999,\"string\",null,\"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"],\"\\\\Key\\n\": \"Value\",\"null\": null}";
        QJsonDocument doc = QJsonDocument::fromJson(compactJson);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        QJsonObject object = doc.object();
        QCOMPARE(object.size(), 3);
        QCOMPARE(object.value("\\Key\n").isString(), true);
        QCOMPARE(object.value("\\Key\n").toString(), QString("Value"));
        QCOMPARE(object.value("null").isNull(), true);
        QCOMPARE(object.value("Array").isArray(), true);
        QJsonArray array = object.value("Array").toArray();
        QCOMPARE(array.size(), 5);
        QCOMPARE(array.at(0).isBool(), true);
        QCOMPARE(array.at(0).toBool(), true);
        QCOMPARE(array.at(1).isDouble(), true);
        QCOMPARE(array.at(1).toDouble(), 999.);
        QCOMPARE(array.at(2).isString(), true);
        QCOMPARE(array.at(2).toString(), QLatin1String("string"));
        QCOMPARE(array.at(3).isNull(), true);
        QCOMPARE(array.at(4).isString(), true);
        QCOMPARE(array.at(4).toString(), QLatin1String("\\\a\n\r\b\tabcABC\""));
    }
}

void tst_QtJson::fromJsonErrors()
{
    {
        QJsonParseError error;
        QByteArray json = "{\n    \n\n";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::UnterminatedObject);
        QCOMPARE(error.offset, 8);
    }
    {
        QJsonParseError error;
        QByteArray json = "{\n    \"key\" 10\n";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::MissingNameSeparator);
        QCOMPARE(error.offset, 13);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    \n\n";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::UnterminatedArray);
        QCOMPARE(error.offset, 8);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n   1, true\n\n";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::UnterminatedArray);
        QCOMPARE(error.offset, 14);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n  1 true\n\n";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::MissingValueSeparator);
        QCOMPARE(error.offset, 7);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    nul";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalValue);
        QCOMPARE(error.offset, 7);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    nulzz";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalValue);
        QCOMPARE(error.offset, 10);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    tru";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalValue);
        QCOMPARE(error.offset, 7);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    trud]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalValue);
        QCOMPARE(error.offset, 10);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    fal";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalValue);
        QCOMPARE(error.offset, 7);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    falsd]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalValue);
        QCOMPARE(error.offset, 11);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    11111";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::TerminationByNumber);
        QCOMPARE(error.offset, 11);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    -1E10000]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalNumber);
        QCOMPARE(error.offset, 14);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    -1e-10000]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalNumber);
        QCOMPARE(error.offset, 15);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    \"\\u12\"]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalEscapeSequence);
        QCOMPARE(error.offset, 11);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    \"foo" INVALID_UNICODE "bar\"]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalUTF8String);
        QCOMPARE(error.offset, 12);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    \"";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::UnterminatedString);
        QCOMPARE(error.offset, 8);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    \"c" UNICODE_DJE "a\\u12\"]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalEscapeSequence);
        QCOMPARE(error.offset, 15);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    \"c" UNICODE_DJE "a" INVALID_UNICODE "bar\"]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::IllegalUTF8String);
        QCOMPARE(error.offset, 13);
    }
    {
        QJsonParseError error;
        QByteArray json = "[\n    \"c" UNICODE_DJE "a ]";
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, QJsonParseError::UnterminatedString);
        QCOMPARE(error.offset, 14);
    }
}

void tst_QtJson::fromBinary()
{
    QFile file(testDataDir + "/test.json");
    file.open(QFile::ReadOnly);
    QByteArray testJson = file.readAll();

    QJsonDocument doc = QJsonDocument::fromJson(testJson);
    QJsonDocument outdoc = QJsonDocument::fromBinaryData(doc.toBinaryData());
    QVERIFY(!outdoc.isNull());
    QCOMPARE(doc, outdoc);

    QFile bfile(testDataDir + "/test.bjson");
    bfile.open(QFile::ReadOnly);
    QByteArray binary = bfile.readAll();

    QJsonDocument bdoc = QJsonDocument::fromBinaryData(binary);
    QVERIFY(!bdoc.isNull());
    QCOMPARE(doc.toVariant(), bdoc.toVariant());
    QCOMPARE(doc, bdoc);
}

void tst_QtJson::toAndFromBinary_data()
{
    QTest::addColumn<QString>("filename");
    QTest::newRow("test.json") << (testDataDir + "/test.json");
    QTest::newRow("test2.json") << (testDataDir + "/test2.json");
}

void tst_QtJson::toAndFromBinary()
{
    QFETCH(QString, filename);
    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QVERIFY(!doc.isNull());
    QJsonDocument outdoc = QJsonDocument::fromBinaryData(doc.toBinaryData());
    QVERIFY(!outdoc.isNull());
    QCOMPARE(doc, outdoc);
}

void tst_QtJson::invalidBinaryData()
{
    QDir dir(testDataDir + "/invalidBinaryData");
    QFileInfoList files = dir.entryInfoList();
    for (int i = 0; i < files.size(); ++i) {
        if (!files.at(i).isFile())
            continue;
        QFile file(files.at(i).filePath());
        file.open(QIODevice::ReadOnly);
        QByteArray bytes = file.readAll();
        bytes.squeeze();
        QJsonDocument document = QJsonDocument::fromRawData(bytes.constData(), bytes.size());
        QVERIFY(document.isNull());
    }
}

void tst_QtJson::parseNumbers()
{
    {
        // test number parsing
        struct Numbers {
            const char *str;
            int n;
        };
        Numbers numbers [] = {
            { "0", 0 },
            { "1", 1 },
            { "10", 10 },
            { "-1", -1 },
            { "100000", 100000 },
            { "-999", -999 }
        };
        int size = sizeof(numbers)/sizeof(Numbers);
        for (int i = 0; i < size; ++i) {
            QByteArray json = "[ ";
            json += numbers[i].str;
            json += " ]";
            QJsonDocument doc = QJsonDocument::fromJson(json);
            QVERIFY(!doc.isEmpty());
            QCOMPARE(doc.isArray(), true);
            QCOMPARE(doc.isObject(), false);
            QJsonArray array = doc.array();
            QCOMPARE(array.size(), 1);
            QJsonValue val = array.at(0);
            QCOMPARE(val.type(), QJsonValue::Double);
            QCOMPARE(val.toDouble(), (double)numbers[i].n);
        }
    }
    {
        // test number parsing
        struct Numbers {
            const char *str;
            double n;
        };
        Numbers numbers [] = {
            { "0", 0 },
            { "1", 1 },
            { "10", 10 },
            { "-1", -1 },
            { "100000", 100000 },
            { "-999", -999 },
            { "1.1", 1.1 },
            { "1e10", 1e10 },
            { "-1.1", -1.1 },
            { "-1e10", -1e10 },
            { "-1E10", -1e10 },
            { "1.1e10", 1.1e10 },
            { "1.1e308", 1.1e308 },
            { "-1.1e308", -1.1e308 },
            { "1.1e-308", 1.1e-308 },
            { "-1.1e-308", -1.1e-308 },
            { "1.1e+308", 1.1e+308 },
            { "-1.1e+308", -1.1e+308 },
            { "1.e+308", 1.e+308 },
            { "-1.e+308", -1.e+308 }
        };
        int size = sizeof(numbers)/sizeof(Numbers);
        for (int i = 0; i < size; ++i) {
            QByteArray json = "[ ";
            json += numbers[i].str;
            json += " ]";
            QJsonDocument doc = QJsonDocument::fromJson(json);
#ifdef Q_OS_QNX
            if (0 == QString::compare(numbers[i].str, "1.1e-308"))
                QEXPECT_FAIL("", "See QTBUG-37066", Abort);
#endif
            QVERIFY(!doc.isEmpty());
            QCOMPARE(doc.isArray(), true);
            QCOMPARE(doc.isObject(), false);
            QJsonArray array = doc.array();
            QCOMPARE(array.size(), 1);
            QJsonValue val = array.at(0);
            QCOMPARE(val.type(), QJsonValue::Double);
            QCOMPARE(val.toDouble(), numbers[i].n);
        }
    }
}

void tst_QtJson::parseStrings()
{
    const char *strings [] =
    {
        "Foo",
        "abc\\\"abc",
        "abc\\\\abc",
        "abc\\babc",
        "abc\\fabc",
        "abc\\nabc",
        "abc\\rabc",
        "abc\\tabc",
        "abc\\u0019abc",
        "abc" UNICODE_DJE "abc",
        UNICODE_NON_CHARACTER
    };
    int size = sizeof(strings)/sizeof(const char *);

    for (int i = 0; i < size; ++i) {
        QByteArray json = "[\n    \"";
        json += strings[i];
        json += "\"\n]\n";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        QJsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        QJsonValue val = array.at(0);
        QCOMPARE(val.type(), QJsonValue::String);

        QCOMPARE(doc.toJson(), json);
    }

    struct Pairs {
        const char *in;
        const char *out;
    };
    Pairs pairs [] = {
        { "abc\\/abc", "abc/abc" },
        { "abc\\u0402abc", "abc" UNICODE_DJE "abc" },
        { "abc\\u0065abc", "abceabc" },
        { "abc\\uFFFFabc", "abc" UNICODE_NON_CHARACTER "abc" }
    };
    size = sizeof(pairs)/sizeof(Pairs);

    for (int i = 0; i < size; ++i) {
        QByteArray json = "[\n    \"";
        json += pairs[i].in;
        json += "\"\n]\n";
        QByteArray out = "[\n    \"";
        out += pairs[i].out;
        out += "\"\n]\n";
        QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        QJsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        QJsonValue val = array.at(0);
        QCOMPARE(val.type(), QJsonValue::String);

        QCOMPARE(doc.toJson(), out);
    }

}

void tst_QtJson::parseDuplicateKeys()
{
    const char *json = "{ \"B\": true, \"A\": null, \"B\": false }";

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QCOMPARE(doc.isObject(), true);

    QJsonObject o = doc.object();
    QCOMPARE(o.size(), 2);
    QJsonObject::const_iterator it = o.constBegin();
    QCOMPARE(it.key(), QLatin1String("A"));
    QCOMPARE(it.value(), QJsonValue());
    ++it;
    QCOMPARE(it.key(), QLatin1String("B"));
    QCOMPARE(it.value(), QJsonValue(false));
}

void tst_QtJson::testParser()
{
    QFile file(testDataDir + "/test.json");
    file.open(QFile::ReadOnly);
    QByteArray testJson = file.readAll();

    QJsonDocument doc = QJsonDocument::fromJson(testJson);
    QVERIFY(!doc.isEmpty());
}

void tst_QtJson::compactArray()
{
    QJsonArray array;
    array.append(QLatin1String("First Entry"));
    array.append(QLatin1String("Second Entry"));
    array.append(QLatin1String("Third Entry"));
    QJsonDocument doc(array);
    int s =  doc.toBinaryData().size();
    array.removeAt(1);
    doc.setArray(array);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             QByteArray("[\n"
                        "    \"First Entry\",\n"
                        "    \"Third Entry\"\n"
                        "]\n"));

    array.removeAt(0);
    doc.setArray(array);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             QByteArray("[\n"
                        "    \"Third Entry\"\n"
                        "]\n"));

    array.removeAt(0);
    doc.setArray(array);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             QByteArray("[\n"
                        "]\n"));

}

void tst_QtJson::compactObject()
{
    QJsonObject object;
    object.insert(QLatin1String("Key1"), QLatin1String("First Entry"));
    object.insert(QLatin1String("Key2"), QLatin1String("Second Entry"));
    object.insert(QLatin1String("Key3"), QLatin1String("Third Entry"));
    QJsonDocument doc(object);
    int s =  doc.toBinaryData().size();
    object.remove(QLatin1String("Key2"));
    doc.setObject(object);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             QByteArray("{\n"
                        "    \"Key1\": \"First Entry\",\n"
                        "    \"Key3\": \"Third Entry\"\n"
                        "}\n"));

    object.remove(QLatin1String("Key1"));
    doc.setObject(object);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             QByteArray("{\n"
                        "    \"Key3\": \"Third Entry\"\n"
                        "}\n"));

    object.remove(QLatin1String("Key3"));
    doc.setObject(object);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             QByteArray("{\n"
                        "}\n"));

}

void tst_QtJson::validation()
{
    // this basically tests that we don't crash on corrupt data
    QFile file(testDataDir + "/test.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray testJson = file.readAll();
    QVERIFY(!testJson.isEmpty());

    QJsonDocument doc = QJsonDocument::fromJson(testJson);
    QVERIFY(!doc.isNull());

    QByteArray binary = doc.toBinaryData();

    // only test the first 1000 bytes. Testing the full file takes too long
    for (int i = 0; i < 1000; ++i) {
        QByteArray corrupted = binary;
        corrupted[i] = char(0xff);
        QJsonDocument doc = QJsonDocument::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        QByteArray json = doc.toJson();
    }


    QFile file2(testDataDir + "/test3.json");
    file2.open(QFile::ReadOnly);
    testJson = file2.readAll();
    QVERIFY(!testJson.isEmpty());

    doc = QJsonDocument::fromJson(testJson);
    QVERIFY(!doc.isNull());

    binary = doc.toBinaryData();

    for (int i = 0; i < binary.size(); ++i) {
        QByteArray corrupted = binary;
        corrupted[i] = char(0xff);
        QJsonDocument doc = QJsonDocument::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        QByteArray json = doc.toJson();

        corrupted = binary;
        corrupted[i] = 0x00;
        doc = QJsonDocument::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        json = doc.toJson();
    }
}

void tst_QtJson::assignToDocument()
{
    {
        const char *json = "{ \"inner\": { \"key\": true } }";
        QJsonDocument doc = QJsonDocument::fromJson(json);

        QJsonObject o = doc.object();
        QJsonValue inner = o.value("inner");

        QJsonDocument innerDoc(inner.toObject());

        QVERIFY(innerDoc != doc);
        QCOMPARE(innerDoc.object(), inner.toObject());
    }
    {
        const char *json = "[ [ true ] ]";
        QJsonDocument doc = QJsonDocument::fromJson(json);

        QJsonArray a = doc.array();
        QJsonValue inner = a.at(0);

        QJsonDocument innerDoc(inner.toArray());

        QVERIFY(innerDoc != doc);
        QCOMPARE(innerDoc.array(), inner.toArray());
    }
}


void tst_QtJson::testDuplicateKeys()
{
    QJsonObject obj;
    obj.insert(QLatin1String("foo"), QLatin1String("bar"));
    obj.insert(QLatin1String("foo"), QLatin1String("zap"));
    QCOMPARE(obj.size(), 1);
    QCOMPARE(obj.value(QLatin1String("foo")).toString(), QLatin1String("zap"));
}

void tst_QtJson::testCompaction()
{
    // modify object enough times to trigger compactionCounter
    // and make sure the data is still valid
    QJsonObject obj;
    for (int i = 0; i < 33; ++i) {
        obj.remove(QLatin1String("foo"));
        obj.insert(QLatin1String("foo"), QLatin1String("bar"));
    }
    QCOMPARE(obj.size(), 1);
    QCOMPARE(obj.value(QLatin1String("foo")).toString(), QLatin1String("bar"));

    QJsonDocument doc = QJsonDocument::fromBinaryData(QJsonDocument(obj).toBinaryData());
    QVERIFY(!doc.isNull());
    QVERIFY(!doc.isEmpty());
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), true);
    QCOMPARE(doc.object(), obj);
}

void tst_QtJson::testDebugStream()
{
    {
        // QJsonObject

        QJsonObject object;
        QTest::ignoreMessage(QtDebugMsg, "QJsonObject()");
        qDebug() << object;

        object.insert(QLatin1String("foo"), QLatin1String("bar"));
        QTest::ignoreMessage(QtDebugMsg, "QJsonObject({\"foo\":\"bar\"})");
        qDebug() << object;
    }

    {
        // QJsonArray

        QJsonArray array;
        QTest::ignoreMessage(QtDebugMsg, "QJsonArray()");
        qDebug() << array;

        array.append(1);
        array.append(QLatin1String("foo"));
        QTest::ignoreMessage(QtDebugMsg, "QJsonArray([1,\"foo\"])");
        qDebug() << array;
    }

    {
        // QJsonDocument

        QJsonDocument doc;
        QTest::ignoreMessage(QtDebugMsg, "QJsonDocument()");
        qDebug() << doc;

        QJsonObject object;
        object.insert(QLatin1String("foo"), QLatin1String("bar"));
        doc.setObject(object);
        QTest::ignoreMessage(QtDebugMsg, "QJsonDocument({\"foo\":\"bar\"})");
        qDebug() << doc;

        QJsonArray array;
        array.append(1);
        array.append(QLatin1String("foo"));
        QTest::ignoreMessage(QtDebugMsg, "QJsonDocument([1,\"foo\"])");
        doc.setArray(array);
        qDebug() << doc;
    }

    {
        // QJsonValue

        QJsonValue value;

        QTest::ignoreMessage(QtDebugMsg, "QJsonValue(null)");
        qDebug() << value;

        value = QJsonValue(true); // bool
        QTest::ignoreMessage(QtDebugMsg, "QJsonValue(bool, true)");
        qDebug() << value;

        value = QJsonValue((double)4.2); // double
        QTest::ignoreMessage(QtDebugMsg, "QJsonValue(double, 4.2)");
        qDebug() << value;

        value = QJsonValue((int)42); // int
        QTest::ignoreMessage(QtDebugMsg, "QJsonValue(double, 42)");
        qDebug() << value;

        value = QJsonValue(QLatin1String("foo")); // string
        QTest::ignoreMessage(QtDebugMsg, "QJsonValue(string, \"foo\")");
        qDebug() << value;

        QJsonArray array;
        array.append(1);
        array.append(QLatin1String("foo"));
        value = QJsonValue(array); // array
        QTest::ignoreMessage(QtDebugMsg, "QJsonValue(array, QJsonArray([1,\"foo\"]))");
        qDebug() << value;

        QJsonObject object;
        object.insert(QLatin1String("foo"), QLatin1String("bar"));
        value = QJsonValue(object); // object
        QTest::ignoreMessage(QtDebugMsg, "QJsonValue(object, QJsonObject({\"foo\":\"bar\"}))");
        qDebug() << value;
    }
}

void tst_QtJson::testCompactionError()
{
    QJsonObject schemaObject;
    schemaObject.insert("_Type", QLatin1String("_SchemaType"));
    schemaObject.insert("name", QLatin1String("Address"));
    schemaObject.insert("schema", QJsonObject());
    {
        QJsonObject content(schemaObject);
        QJsonDocument doc(content);
        QVERIFY(!doc.isNull());
        QByteArray hash = QCryptographicHash::hash(doc.toBinaryData(), QCryptographicHash::Md5).toHex();
        schemaObject.insert("_Version", QString::fromLatin1(hash.constData(), hash.size()));
    }

    QJsonObject schema;
    schema.insert("streetNumber", schema.value("number").toObject());
    schemaObject.insert("schema", schema);
    {
        QJsonObject content(schemaObject);
        content.remove("_Uuid");
        content.remove("_Version");
        QJsonDocument doc(content);
        QVERIFY(!doc.isNull());
        QByteArray hash = QCryptographicHash::hash(doc.toBinaryData(), QCryptographicHash::Md5).toHex();
        schemaObject.insert("_Version", QString::fromLatin1(hash.constData(), hash.size()));
    }
}

void tst_QtJson::parseUnicodeEscapes()
{
    const QByteArray json = "[ \"A\\u00e4\\u00C4\" ]";

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonArray array = doc.array();

    QString result = QLatin1String("A");
    result += QChar(0xe4);
    result += QChar(0xc4);

    QCOMPARE(array.first().toString(), result);
}

void tst_QtJson::assignObjects()
{
    const char *json =
            "[ { \"Key\": 1 }, { \"Key\": 2 } ]";

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonArray array = doc.array();

    QJsonObject object = array.at(0).toObject();
    QCOMPARE(object.value("Key").toDouble(), 1.);

    object = array.at(1).toObject();
    QCOMPARE(object.value("Key").toDouble(), 2.);
}

void tst_QtJson::assignArrays()
{
    const char *json =
            "[ [ 1 ], [ 2 ] ]";

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonArray array = doc.array();

    QJsonArray inner = array.at(0).toArray()  ;
    QCOMPARE(inner.at(0).toDouble(), 1.);

    inner= array.at(1).toArray();
    QCOMPARE(inner.at(0).toDouble(), 2.);
}

void tst_QtJson::testTrailingComma()
{
    const char *jsons[] = { "{ \"Key\": 1, }", "[ { \"Key\": 1 }, ]" };

    for (unsigned i = 0; i < sizeof(jsons)/sizeof(jsons[0]); ++i) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsons[i], &error);
        QCOMPARE(error.error, QJsonParseError::MissingObject);
    }
}

void tst_QtJson::testDetachBug()
{
    QJsonObject dynamic;
    QJsonObject embedded;

    QJsonObject local;

    embedded.insert("Key1", QString("Value1"));
    embedded.insert("Key2", QString("Value2"));
    dynamic.insert(QStringLiteral("Bogus"), QString("bogusValue"));
    dynamic.insert("embedded", embedded);
    local = dynamic.value("embedded").toObject();

    dynamic.remove("embedded");

    QCOMPARE(local.keys().size(),2);
    local.remove("Key1");
    local.remove("Key2");
    QCOMPARE(local.keys().size(), 0);

    local.insert("Key1", QString("anotherValue"));
    QCOMPARE(local.keys().size(), 1);
}

void tst_QtJson::valueEquals()
{
    QCOMPARE(QJsonValue(), QJsonValue());
    QVERIFY(QJsonValue() != QJsonValue(QJsonValue::Undefined));
    QVERIFY(QJsonValue() != QJsonValue(true));
    QVERIFY(QJsonValue() != QJsonValue(1.));
    QVERIFY(QJsonValue() != QJsonValue(QJsonArray()));
    QVERIFY(QJsonValue() != QJsonValue(QJsonObject()));

    QCOMPARE(QJsonValue(true), QJsonValue(true));
    QVERIFY(QJsonValue(true) != QJsonValue(false));
    QVERIFY(QJsonValue(true) != QJsonValue(QJsonValue::Undefined));
    QVERIFY(QJsonValue(true) != QJsonValue());
    QVERIFY(QJsonValue(true) != QJsonValue(1.));
    QVERIFY(QJsonValue(true) != QJsonValue(QJsonArray()));
    QVERIFY(QJsonValue(true) != QJsonValue(QJsonObject()));

    QCOMPARE(QJsonValue(1), QJsonValue(1));
    QVERIFY(QJsonValue(1) != QJsonValue(2));
    QCOMPARE(QJsonValue(1), QJsonValue(1.));
    QVERIFY(QJsonValue(1) != QJsonValue(1.1));
    QVERIFY(QJsonValue(1) != QJsonValue(QJsonValue::Undefined));
    QVERIFY(QJsonValue(1) != QJsonValue());
    QVERIFY(QJsonValue(1) != QJsonValue(true));
    QVERIFY(QJsonValue(1) != QJsonValue(QJsonArray()));
    QVERIFY(QJsonValue(1) != QJsonValue(QJsonObject()));

    QCOMPARE(QJsonValue(1.), QJsonValue(1.));
    QVERIFY(QJsonValue(1.) != QJsonValue(2.));
    QVERIFY(QJsonValue(1.) != QJsonValue(QJsonValue::Undefined));
    QVERIFY(QJsonValue(1.) != QJsonValue());
    QVERIFY(QJsonValue(1.) != QJsonValue(true));
    QVERIFY(QJsonValue(1.) != QJsonValue(QJsonArray()));
    QVERIFY(QJsonValue(1.) != QJsonValue(QJsonObject()));

    QCOMPARE(QJsonValue(QJsonArray()), QJsonValue(QJsonArray()));
    QJsonArray nonEmptyArray;
    nonEmptyArray.append(true);
    QVERIFY(QJsonValue(QJsonArray()) != nonEmptyArray);
    QVERIFY(QJsonValue(QJsonArray()) != QJsonValue(QJsonValue::Undefined));
    QVERIFY(QJsonValue(QJsonArray()) != QJsonValue());
    QVERIFY(QJsonValue(QJsonArray()) != QJsonValue(true));
    QVERIFY(QJsonValue(QJsonArray()) != QJsonValue(1.));
    QVERIFY(QJsonValue(QJsonArray()) != QJsonValue(QJsonObject()));

    QCOMPARE(QJsonValue(QJsonObject()), QJsonValue(QJsonObject()));
    QJsonObject nonEmptyObject;
    nonEmptyObject.insert("Key", true);
    QVERIFY(QJsonValue(QJsonObject()) != nonEmptyObject);
    QVERIFY(QJsonValue(QJsonObject()) != QJsonValue(QJsonValue::Undefined));
    QVERIFY(QJsonValue(QJsonObject()) != QJsonValue());
    QVERIFY(QJsonValue(QJsonObject()) != QJsonValue(true));
    QVERIFY(QJsonValue(QJsonObject()) != QJsonValue(1.));
    QVERIFY(QJsonValue(QJsonObject()) != QJsonValue(QJsonArray()));

    QCOMPARE(QJsonValue("foo"), QJsonValue(QLatin1String("foo")));
    QCOMPARE(QJsonValue("foo"), QJsonValue(QString("foo")));
    QCOMPARE(QJsonValue("\x66\x6f\x6f"), QJsonValue(QString("foo")));
    QCOMPARE(QJsonValue("\x62\x61\x72"), QJsonValue("bar"));
    QCOMPARE(QJsonValue(UNICODE_NON_CHARACTER), QJsonValue(QString(UNICODE_NON_CHARACTER)));
    QCOMPARE(QJsonValue(UNICODE_DJE), QJsonValue(QString(UNICODE_DJE)));
    QCOMPARE(QJsonValue("\xc3\xa9"), QJsonValue(QString("\xc3\xa9")));
}

void tst_QtJson::objectEquals_data()
{
    QTest::addColumn<QJsonObject>("left");
    QTest::addColumn<QJsonObject>("right");
    QTest::addColumn<bool>("result");

    QTest::newRow("two defaults") << QJsonObject() << QJsonObject() << true;

    QJsonObject object1;
    object1.insert("property", 1);
    QJsonObject object2;
    object2["property"] = 1;
    QJsonObject object3;
    object3.insert("property1", 1);
    object3.insert("property2", 2);

    QTest::newRow("the same object (1 vs 2)") << object1 << object2 << true;
    QTest::newRow("the same object (3 vs 3)") << object3 << object3 << true;
    QTest::newRow("different objects (2 vs 3)") << object2 << object3 << false;
    QTest::newRow("object vs default") << object1 << QJsonObject() << false;

    QJsonObject empty;
    empty.insert("property", 1);
    empty.take("property");
    QTest::newRow("default vs empty") << QJsonObject() << empty << true;
    QTest::newRow("empty vs empty") << empty << empty << true;
    QTest::newRow("object vs empty") << object1 << empty << false;

    QJsonObject referencedEmpty;
    referencedEmpty["undefined"];
    QTest::newRow("referenced empty vs referenced empty") << referencedEmpty << referencedEmpty << true;
    QTest::newRow("referenced empty vs object") << referencedEmpty << object1 << false;

    QJsonObject referencedObject1;
    referencedObject1.insert("property", 1);
    referencedObject1["undefined"];
    QJsonObject referencedObject2;
    referencedObject2.insert("property", 1);
    referencedObject2["aaaaaaaaa"]; // earlier then "property"
    referencedObject2["zzzzzzzzz"]; // after "property"
    QTest::newRow("referenced object vs default") << referencedObject1 << QJsonObject() << false;
    QTest::newRow("referenced object vs referenced object") << referencedObject1 << referencedObject1 << true;
    QTest::newRow("referenced object vs object (different)") << referencedObject1 << object3 << false;
}

void tst_QtJson::objectEquals()
{
    QFETCH(QJsonObject, left);
    QFETCH(QJsonObject, right);
    QFETCH(bool, result);

    QCOMPARE(left == right, result);
    QCOMPARE(right == left, result);

    // invariants checks
    QCOMPARE(left, left);
    QCOMPARE(right, right);
    QCOMPARE(left != right, !result);
    QCOMPARE(right != left, !result);

    // The same but from QJsonValue perspective
    QCOMPARE(QJsonValue(left) == QJsonValue(right), result);
    QCOMPARE(QJsonValue(left) != QJsonValue(right), !result);
    QCOMPARE(QJsonValue(right) == QJsonValue(left), result);
    QCOMPARE(QJsonValue(right) != QJsonValue(left), !result);

    // The same, but from a QJsonDocument perspective
    QCOMPARE(QJsonDocument(left) == QJsonDocument(right), result);
    QCOMPARE(QJsonDocument(left) != QJsonDocument(right), !result);
    QCOMPARE(QJsonDocument(right) == QJsonDocument(left), result);
    QCOMPARE(QJsonDocument(right) != QJsonDocument(left), !result);
}

void tst_QtJson::arrayEquals_data()
{
    QTest::addColumn<QJsonArray>("left");
    QTest::addColumn<QJsonArray>("right");
    QTest::addColumn<bool>("result");

    QTest::newRow("two defaults") << QJsonArray() << QJsonArray() << true;

    QJsonArray array1;
    array1.append(1);
    QJsonArray array2;
    array2.append(2111);
    array2[0] = 1;
    QJsonArray array3;
    array3.insert(0, 1);
    array3.insert(1, 2);

    QTest::newRow("the same array (1 vs 2)") << array1 << array2 << true;
    QTest::newRow("the same array (3 vs 3)") << array3 << array3 << true;
    QTest::newRow("different arrays (2 vs 3)") << array2 << array3 << false;
    QTest::newRow("array vs default") << array1 << QJsonArray() << false;

    QJsonArray empty;
    empty.append(1);
    empty.takeAt(0);
    QTest::newRow("default vs empty") << QJsonArray() << empty << true;
    QTest::newRow("empty vs default") << empty << QJsonArray() << true;
    QTest::newRow("empty vs empty") << empty << empty << true;
    QTest::newRow("array vs empty") << array1 << empty << false;
}

void tst_QtJson::arrayEquals()
{
    QFETCH(QJsonArray, left);
    QFETCH(QJsonArray, right);
    QFETCH(bool, result);

    QCOMPARE(left == right, result);
    QCOMPARE(right == left, result);

    // invariants checks
    QCOMPARE(left, left);
    QCOMPARE(right, right);
    QCOMPARE(left != right, !result);
    QCOMPARE(right != left, !result);

    // The same but from QJsonValue perspective
    QCOMPARE(QJsonValue(left) == QJsonValue(right), result);
    QCOMPARE(QJsonValue(left) != QJsonValue(right), !result);
    QCOMPARE(QJsonValue(right) == QJsonValue(left), result);
    QCOMPARE(QJsonValue(right) != QJsonValue(left), !result);

    // The same but from QJsonDocument perspective
    QCOMPARE(QJsonDocument(left) == QJsonDocument(right), result);
    QCOMPARE(QJsonDocument(left) != QJsonDocument(right), !result);
    QCOMPARE(QJsonDocument(right) == QJsonDocument(left), result);
    QCOMPARE(QJsonDocument(right) != QJsonDocument(left), !result);
}

void tst_QtJson::documentEquals_data()
{
    QTest::addColumn<QJsonDocument>("left");
    QTest::addColumn<QJsonDocument>("right");
    QTest::addColumn<bool>("result");

    QTest::newRow("two defaults") << QJsonDocument() << QJsonDocument() << true;

    QJsonDocument emptyobj(QJsonObject{});
    QJsonDocument emptyarr(QJsonArray{});
    QTest::newRow("emptyarray vs default") << emptyarr << QJsonDocument() << false;
    QTest::newRow("emptyobject vs default") << emptyobj << QJsonDocument() << false;
    QTest::newRow("emptyarray vs emptyobject") << emptyarr << emptyobj << false;

    QJsonDocument array1(QJsonArray{1});
    QJsonDocument array2(QJsonArray{2});
    QTest::newRow("emptyarray vs emptyarray") << emptyarr << emptyarr << true;
    QTest::newRow("emptyarray vs array") << emptyarr << array1 << false;
    QTest::newRow("array vs array") << array1 << array1 << true;
    QTest::newRow("array vs otherarray") << array1 << array2 << false;

    QJsonDocument object1(QJsonObject{{"hello", "world"}});
    QJsonDocument object2(QJsonObject{{"hello", 2}});
    QTest::newRow("emptyobject vs emptyobject") << emptyobj << emptyobj << true;
    QTest::newRow("emptyobject vs object") << emptyobj << object1 << false;
    QTest::newRow("object vs object") << object1 << object1 << true;
    QTest::newRow("object vs otherobject") << object1 << object2 << false;

    QTest::newRow("object vs array") << array1 << object1 << false;
}

void tst_QtJson::documentEquals()
{
    QFETCH(QJsonDocument, left);
    QFETCH(QJsonDocument, right);
    QFETCH(bool, result);

    QCOMPARE(left == right, result);
    QCOMPARE(right == left, result);

    // invariants checks
    QCOMPARE(left, left);
    QCOMPARE(right, right);
    QCOMPARE(left != right, !result);
    QCOMPARE(right != left, !result);
}

void tst_QtJson::bom()
{
    QFile file(testDataDir + "/bom.json");
    file.open(QFile::ReadOnly);
    QByteArray json = file.readAll();

    // Import json document into a QJsonDocument
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);

    QVERIFY(!doc.isNull());
    QCOMPARE(error.error, QJsonParseError::NoError);
}

void tst_QtJson::nesting()
{
    // check that we abort parsing too deeply nested json documents.
    // this is to make sure we don't crash because the parser exhausts the
    // stack.

    const char *array_data =
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]";

    QByteArray json(array_data);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);

    QVERIFY(!doc.isNull());
    QCOMPARE(error.error, QJsonParseError::NoError);

    json.prepend('[');
    json.append(']');
    doc = QJsonDocument::fromJson(json, &error);

    QVERIFY(doc.isNull());
    QCOMPARE(error.error, QJsonParseError::DeepNesting);

    json = QByteArray("true ");

    for (int i = 0; i < 1024; ++i) {
        json.prepend("{ \"Key\": ");
        json.append(" }");
    }

    doc = QJsonDocument::fromJson(json, &error);

    QVERIFY(!doc.isNull());
    QCOMPARE(error.error, QJsonParseError::NoError);

    json.prepend('[');
    json.append(']');
    doc = QJsonDocument::fromJson(json, &error);

    QVERIFY(doc.isNull());
    QCOMPARE(error.error, QJsonParseError::DeepNesting);

}

void tst_QtJson::longStrings()
{
    // test around 15 and 16 bit boundaries, as these are limits
    // in the data structures (for Latin1String in qjson_p.h)
    QString s(0x7ff0, 'a');
    QByteArray ba(0x7ff0, 'a');
    ba.append(0x8010 - 0x7ff0, 'c');
    for (int i = 0x7ff0; i < 0x8010; i++) {
        s.append(QLatin1Char('c'));

        QMap <QString, QVariant> map;
        map["key"] = s;

        /* Create a QJsonDocument from the QMap ... */
        QJsonDocument d1 = QJsonDocument::fromVariant(QVariant(map));
        /* ... and a QByteArray from the QJsonDocument */
        QByteArray a1 = d1.toJson();

        /* Create a QJsonDocument from the QByteArray ... */
        QJsonDocument d2 = QJsonDocument::fromJson(a1);
        /* ... and a QByteArray from the QJsonDocument */
        QByteArray a2 = d2.toJson();
        QCOMPARE(a1, a2);

        // Test long keys
        QJsonObject o1, o2;
        o1[s] = 42;
        o2[QLatin1String(ba.data(), i + 1)] = 42;
        d1.setObject(o1);
        d2.setObject(o2);
        a1 = d1.toJson();
        a2 = d2.toJson();
        QCOMPARE(a1, a2);
    }

    s = QString(0xfff0, 'a');
    ba = QByteArray(0xfff0, 'a');
    ba.append(0x10010 - 0xfff0, 'c');
    for (int i = 0xfff0; i < 0x10010; i++) {
        s.append(QLatin1Char('c'));

        QMap <QString, QVariant> map;
        map["key"] = s;

        /* Create a QJsonDocument from the QMap ... */
        QJsonDocument d1 = QJsonDocument::fromVariant(QVariant(map));
        /* ... and a QByteArray from the QJsonDocument */
        QByteArray a1 = d1.toJson();

        /* Create a QJsonDocument from the QByteArray ... */
        QJsonDocument d2 = QJsonDocument::fromJson(a1);
        /* ... and a QByteArray from the QJsonDocument */
        QByteArray a2 = d2.toJson();
        QCOMPARE(a1, a2);

        // Test long keys
        QJsonObject o1, o2;
        o1[s] = 42;
        o2[QLatin1String(ba.data(), i + 1)] = 42;
        d1.setObject(o1);
        d2.setObject(o2);
        a1 = d1.toJson();
        a2 = d2.toJson();
        QCOMPARE(a1, a2);
    }
}

void tst_QtJson::testJsonValueRefDefault()
{
    QJsonObject empty;

    QCOMPARE(empty["n/a"].toString(), QString());
    QCOMPARE(empty["n/a"].toString("default"), QStringLiteral("default"));

    QCOMPARE(empty["n/a"].toBool(), false);
    QCOMPARE(empty["n/a"].toBool(true), true);

    QCOMPARE(empty["n/a"].toInt(), 0);
    QCOMPARE(empty["n/a"].toInt(42), 42);

    QCOMPARE(empty["n/a"].toDouble(), 0.0);
    QCOMPARE(empty["n/a"].toDouble(42.0), 42.0);
}

void tst_QtJson::arrayInitializerList()
{
    QVERIFY(QJsonArray{}.isEmpty());
    QCOMPARE(QJsonArray{"one"}.count(), 1);
    QCOMPARE(QJsonArray{1}.count(), 1);

    {
        QJsonArray a{1.3, "hello", 0};
        QCOMPARE(QJsonValue(a[0]), QJsonValue(1.3));
        QCOMPARE(QJsonValue(a[1]), QJsonValue("hello"));
        QCOMPARE(QJsonValue(a[2]), QJsonValue(0));
        QCOMPARE(a.count(), 3);
    }
    {
        QJsonObject o;
        o["property"] = 1;
        QJsonArray a1 {o};
        QCOMPARE(a1.count(), 1);
        QCOMPARE(a1[0].toObject(), o);

        QJsonArray a2 {o, 23};
        QCOMPARE(a2.count(), 2);
        QCOMPARE(a2[0].toObject(), o);
        QCOMPARE(QJsonValue(a2[1]), QJsonValue(23));

        QJsonArray a3 { a1, o, a2 };
        QCOMPARE(QJsonValue(a3[0]), QJsonValue(a1));
        QCOMPARE(QJsonValue(a3[1]), QJsonValue(o));
        QCOMPARE(QJsonValue(a3[2]), QJsonValue(a2));

        QJsonArray a4 { 1, QJsonArray{1,2,3}, QJsonArray{"hello", 2}, QJsonObject{{"one", 1}} };
        QCOMPARE(a4.count(), 4);
        QCOMPARE(QJsonValue(a4[0]), QJsonValue(1));

        {
            QJsonArray a41 = a4[1].toArray();
            QJsonArray a42 = a4[2].toArray();
            QJsonObject a43 = a4[3].toObject();
            QCOMPARE(a41.count(), 3);
            QCOMPARE(a42.count(), 2);
            QCOMPARE(a43.count(), 1);

            QCOMPARE(QJsonValue(a41[2]), QJsonValue(3));
            QCOMPARE(QJsonValue(a42[1]), QJsonValue(2));
            QCOMPARE(QJsonValue(a43["one"]), QJsonValue(1));
        }
    }
}

void tst_QtJson::objectInitializerList()
{
    QVERIFY(QJsonObject{}.isEmpty());

    {   // one property
        QJsonObject one {{"one", 1}};
        QCOMPARE(one.count(), 1);
        QVERIFY(one.contains("one"));
        QCOMPARE(QJsonValue(one["one"]), QJsonValue(1));
    }
    {   // two properties
        QJsonObject two {
                           {"one", 1},
                           {"two", 2}
                        };
        QCOMPARE(two.count(), 2);
        QVERIFY(two.contains("one"));
        QVERIFY(two.contains("two"));
        QCOMPARE(QJsonValue(two["one"]), QJsonValue(1));
        QCOMPARE(QJsonValue(two["two"]), QJsonValue(2));
    }
    {   // nested object
        QJsonObject object{{"nested", QJsonObject{{"innerProperty", 2}}}};
        QCOMPARE(object.count(), 1);
        QVERIFY(object.contains("nested"));
        QVERIFY(object["nested"].isObject());

        QJsonObject nested = object["nested"].toObject();
        QCOMPARE(QJsonValue(nested["innerProperty"]), QJsonValue(2));
    }
    {   // nested array
        QJsonObject object{{"nested", QJsonArray{"innerValue", 2.1, "bum cyk cyk"}}};
        QCOMPARE(object.count(), 1);
        QVERIFY(object.contains("nested"));
        QVERIFY(object["nested"].isArray());

        QJsonArray nested = object["nested"].toArray();
        QCOMPARE(nested.count(), 3);
        QCOMPARE(QJsonValue(nested[0]), QJsonValue("innerValue"));
        QCOMPARE(QJsonValue(nested[1]), QJsonValue(2.1));
    }
}

void tst_QtJson::unicodeKeys()
{
    QByteArray json = "{"
                      "\"x\\u2090_1\": \"hello_1\","
                      "\"y\\u2090_2\": \"hello_2\","
                      "\"T\\u2090_3\": \"hello_3\","
                      "\"xyz_4\": \"hello_4\","
                      "\"abc_5\": \"hello_5\""
                      "}";

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QJsonObject o = doc.object();

    const auto keys = o.keys();
    QCOMPARE(keys.size(), 5);
    for (const QString &key : keys) {
        QString suffix = key.mid(key.indexOf(QLatin1Char('_')));
        QCOMPARE(o[key].toString(), QString("hello") + suffix);
    }
}

void tst_QtJson::garbageAtEnd()
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson("{},", &error);
    QCOMPARE(error.error, QJsonParseError::GarbageAtEnd);
    QCOMPARE(error.offset, 2);
    QVERIFY(doc.isEmpty());

    doc = QJsonDocument::fromJson("{}    ", &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(!doc.isEmpty());
}

void tst_QtJson::removeNonLatinKey()
{
    const QString nonLatinKeyName = QString::fromUtf8("Атрибут100500");

    QJsonObject sourceObject;

    sourceObject.insert("code", 1);
    sourceObject.remove("code");

    sourceObject.insert(nonLatinKeyName, 1);

    const QByteArray json = QJsonDocument(sourceObject).toJson();
    const QJsonObject restoredObject = QJsonDocument::fromJson(json).object();

    QCOMPARE(sourceObject.keys(), restoredObject.keys());
    QVERIFY(sourceObject.contains(nonLatinKeyName));
    QVERIFY(restoredObject.contains(nonLatinKeyName));
}

void tst_QtJson::documentFromVariant()
{
    // Test the valid forms of QJsonDocument::fromVariant.

    QString string = QStringLiteral("value");

    QStringList strList;
    strList.append(string);

    QJsonDocument da1 = QJsonDocument::fromVariant(QVariant(strList));
    QVERIFY(da1.isArray());

    QVariantList list;
    list.append(string);

    QJsonDocument da2 = QJsonDocument::fromVariant(list);
    QVERIFY(da2.isArray());

    // As JSON arrays they should be equal.
    QCOMPARE(da1.array(), da2.array());


    QMap <QString, QVariant> map;
    map["key"] = string;

    QJsonDocument do1 = QJsonDocument::fromVariant(QVariant(map));
    QVERIFY(do1.isObject());

    QHash <QString, QVariant> hash;
    hash["key"] = string;

    QJsonDocument do2 = QJsonDocument::fromVariant(QVariant(hash));
    QVERIFY(do2.isObject());

    // As JSON objects they should be equal.
    QCOMPARE(do1.object(), do2.object());
}

void tst_QtJson::parseErrorOffset_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<int>("errorOffset");

    QTest::newRow("Trailing comma in object") << QByteArray("{ \"value\": false, }") << 19;
    QTest::newRow("Trailing comma in object plus whitespace") << QByteArray("{ \"value\": false, }    ") << 19;
    QTest::newRow("Trailing comma in array") << QByteArray("[ false, ]") << 10;
    QTest::newRow("Trailing comma in array plus whitespace") << QByteArray("[ false, ]    ") << 10;
    QTest::newRow("Missing value in object") << QByteArray("{ \"value\": , } ") << 12;
    QTest::newRow("Missing value in array") << QByteArray("[ \"value\" , , ] ") << 13;
    QTest::newRow("Leading comma in object") << QByteArray("{ ,  \"value\": false}") << 3;
    QTest::newRow("Leading comma in array") << QByteArray("[ ,  false]") << 3;
    QTest::newRow("Stray ,") << QByteArray("  ,  ") << 3;
    QTest::newRow("Stray [") << QByteArray("  [  ") << 5;
    QTest::newRow("Stray }") << QByteArray("  }  ") << 3;
}

void tst_QtJson::parseErrorOffset()
{
    QFETCH(QByteArray, json);
    QFETCH(int, errorOffset);

    QJsonParseError error;
    QJsonDocument::fromJson(json, &error);

    QVERIFY(error.error != QJsonParseError::NoError);
    QCOMPARE(error.offset, errorOffset);
}

void tst_QtJson::implicitValueType()
{
    QJsonObject rootObject{
        {"object", QJsonObject{{"value", 42}}},
        {"array", QJsonArray{665, 666, 667}}
    };

    QJsonValue objectValue = rootObject["object"];
    QCOMPARE(objectValue["value"].toInt(), 42);
    QCOMPARE(objectValue["missingValue"], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(objectValue[123], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(objectValue["missingValue"].toInt(123), 123);

    QJsonValue arrayValue = rootObject["array"];
    QCOMPARE(arrayValue[1].toInt(), 666);
    QCOMPARE(arrayValue[-1], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(arrayValue["asObject"], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(arrayValue[-1].toInt(123), 123);

    const QJsonObject constObject = rootObject;
    QCOMPARE(constObject["object"]["value"].toInt(), 42);
    QCOMPARE(constObject["array"][1].toInt(), 666);

    QJsonValue objectAsValue(rootObject);
    QCOMPARE(objectAsValue["object"]["value"].toInt(), 42);
    QCOMPARE(objectAsValue["array"][1].toInt(), 666);
}

void tst_QtJson::implicitDocumentType()
{
    QJsonDocument emptyDocument;
    QCOMPARE(emptyDocument["asObject"], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(emptyDocument[123], QJsonValue(QJsonValue::Undefined));

    QJsonDocument objectDocument(QJsonObject{{"value", 42}});
    QCOMPARE(objectDocument["value"].toInt(), 42);
    QCOMPARE(objectDocument["missingValue"], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(objectDocument[123], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(objectDocument["missingValue"].toInt(123), 123);

    QJsonDocument arrayDocument(QJsonArray{665, 666, 667});
    QCOMPARE(arrayDocument[1].toInt(), 666);
    QCOMPARE(arrayDocument[-1], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(arrayDocument["asObject"], QJsonValue(QJsonValue::Undefined));
    QCOMPARE(arrayDocument[-1].toInt(123), 123);
}

void tst_QtJson::streamSerializationQJsonDocument_data()
{
    QTest::addColumn<QJsonDocument>("document");
    QTest::newRow("empty") << QJsonDocument();
    QTest::newRow("object") << QJsonDocument(QJsonObject{{"value", 42}});
}

void tst_QtJson::streamSerializationQJsonDocument()
{
    // Check interface only, implementation is tested through to and from
    // json functions.
    QByteArray buffer;
    QFETCH(QJsonDocument, document);
    QJsonDocument output;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    save << document;
    QDataStream load(buffer);
    load >> output;
    QCOMPARE(output, document);
}

void tst_QtJson::streamSerializationQJsonArray_data()
{
    QTest::addColumn<QJsonArray>("array");
    QTest::newRow("empty") << QJsonArray();
    QTest::newRow("values") << QJsonArray{665, 666, 667};
}

void tst_QtJson::streamSerializationQJsonArray()
{
    // Check interface only, implementation is tested through to and from
    // json functions.
    QByteArray buffer;
    QFETCH(QJsonArray, array);
    QJsonArray output;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    save << array;
    QDataStream load(buffer);
    load >> output;
    QCOMPARE(output, array);
}

void tst_QtJson::streamSerializationQJsonObject_data()
{
    QTest::addColumn<QJsonObject>("object");
    QTest::newRow("empty") << QJsonObject();
    QTest::newRow("non-empty") << QJsonObject{{"foo", 665}, {"bar", 666}};
}

void tst_QtJson::streamSerializationQJsonObject()
{
    // Check interface only, implementation is tested through to and from
    // json functions.
    QByteArray buffer;
    QFETCH(QJsonObject, object);
    QJsonObject output;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    save << object;
    QDataStream load(buffer);
    load >> output;
    QCOMPARE(output, object);
}

void tst_QtJson::streamSerializationQJsonValue_data()
{
    QTest::addColumn<QJsonValue>("value");
    QTest::newRow("double") << QJsonValue{665};
    QTest::newRow("bool") << QJsonValue{true};
    QTest::newRow("string") << QJsonValue{QStringLiteral("bum")};
    QTest::newRow("array") << QJsonValue{QJsonArray{12,1,5,6,7}};
    QTest::newRow("object") << QJsonValue{QJsonObject{{"foo", 665}, {"bar", 666}}};
    // test json escape sequence
    QTest::newRow("array with 0xD800") << QJsonValue(QJsonArray{QString(0xD800)});
    QTest::newRow("array with 0xDF06,0xD834") << QJsonValue(QJsonArray{QString(0xDF06).append(0xD834)});
}

void tst_QtJson::streamSerializationQJsonValue()
{
    QByteArray buffer;
    QFETCH(QJsonValue, value);
    QJsonValue output;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    save << value;
    QDataStream load(buffer);
    load >> output;
    QCOMPARE(output, value);
}

void tst_QtJson::streamSerializationQJsonValueEmpty()
{
    QByteArray buffer;
    {
        QJsonValue undef{QJsonValue::Undefined};
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << undef;
        QDataStream load(buffer);
        QJsonValue output;
        load >> output;
        QVERIFY(output.isUndefined());
    }
    {
        QJsonValue null{QJsonValue::Null};
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << null;
        QDataStream load(buffer);
        QJsonValue output;
        load >> output;
        QVERIFY(output.isNull());
    }
}

void tst_QtJson::streamVariantSerialization()
{
    // Check interface only, implementation is tested through to and from
    // json functions.
    QByteArray buffer;
    {
        QJsonDocument objectDoc(QJsonArray{665, 666, 667});
        QVariant output;
        QVariant variant(objectDoc);
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << variant;
        QDataStream load(buffer);
        load >> output;
        QCOMPARE(output.userType(), QMetaType::QJsonDocument);
        QCOMPARE(output.toJsonDocument(), objectDoc);
    }
    {
        QJsonArray array{665, 666, 667};
        QVariant output;
        QVariant variant(array);
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << variant;
        QDataStream load(buffer);
        load >> output;
        QCOMPARE(output.userType(), QMetaType::QJsonArray);
        QCOMPARE(output.toJsonArray(), array);
    }
    {
        QJsonObject obj{{"foo", 42}};
        QVariant output;
        QVariant variant(obj);
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << variant;
        QDataStream load(buffer);
        load >> output;
        QCOMPARE(output.userType(), QMetaType::QJsonObject);
        QCOMPARE(output.toJsonObject(), obj);
    }
    {
        QJsonValue value{42};
        QVariant output;
        QVariant variant(value);
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << variant;
        QDataStream load(buffer);
        load >> output;
        QCOMPARE(output.userType(), QMetaType::QJsonValue);
        QCOMPARE(output.toJsonValue(), value);
    }
}

void tst_QtJson::escapeSurrogateCodePoints_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<QByteArray>("escStr");
    QTest::newRow("0xD800") << QString(0xD800) << QByteArray("\\ud800");
    QTest::newRow("0xDF06,0xD834") << QString(0xDF06).append(0xD834) << QByteArray("\\udf06\\ud834");
}

void tst_QtJson::escapeSurrogateCodePoints()
{
    QFETCH(QString, str);
    QFETCH(QByteArray, escStr);
    QJsonArray array;
    array.append(str);
    QByteArray buffer;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    save << array;
    // verify the buffer has escaped values
    QVERIFY(buffer.contains(escStr));
}

void tst_QtJson::fromToVariantConversions_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QJsonValue>("json");
    QTest::addColumn<QVariant>("jsonToVariant");

    QByteArray utf8("\xC4\x90\xC4\x81\xC5\xA3\xC3\xA2"); // Đāţâ
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUuid uuid = QUuid::createUuid();

    constexpr double maxDouble = std::numeric_limits<double>::max();
    constexpr double minDouble = std::numeric_limits<double>::min();

    QTest::newRow("default")     << QVariant() << QJsonValue(QJsonValue::Null)
                                 << QVariant::fromValue(nullptr);
    QTest::newRow("nullptr")     << QVariant::fromValue(nullptr) << QJsonValue(QJsonValue::Null)
                                 << QVariant::fromValue(nullptr);
    QTest::newRow("bool")        << QVariant(true) << QJsonValue(true) << QVariant(true);
    QTest::newRow("int pos")     << QVariant(123) << QJsonValue(123) << QVariant(qlonglong(123));
    QTest::newRow("int neg")     << QVariant(-123) << QJsonValue(-123) << QVariant(qlonglong(-123));
    QTest::newRow("int big pos") << QVariant((1ll << 52) +1) << QJsonValue((1ll << 52) + 1)
                                 << QVariant(qlonglong((1ll << 52) + 1));
    QTest::newRow("int big neg") << QVariant(-(1ll << 52) + 1) << QJsonValue(-(1ll << 52) + 1)
                                 << QVariant(qlonglong(-(1ll << 52) + 1));
    QTest::newRow("double pos")  << QVariant(123.) << QJsonValue(123.) << QVariant(qlonglong(123.));
    QTest::newRow("double neg")  << QVariant(-123.) << QJsonValue(-123.)
                                 << QVariant(qlonglong(-123.));
    QTest::newRow("double big")  << QVariant(maxDouble - 1000) << QJsonValue(maxDouble - 1000)
                                 << QVariant(maxDouble - 1000);
    QTest::newRow("double max")  << QVariant(maxDouble) << QJsonValue(maxDouble)
                                 << QVariant(maxDouble);
    QTest::newRow("double min")  << QVariant(minDouble) << QJsonValue(minDouble)
                                 << QVariant(minDouble);
    QTest::newRow("double big neg")  << QVariant(1000 - maxDouble) << QJsonValue(1000 - maxDouble)
                                     << QVariant(1000 - maxDouble);
    QTest::newRow("double max neg")  << QVariant(-maxDouble) << QJsonValue(-maxDouble)
                                     << QVariant(-maxDouble);
    QTest::newRow("double min neg")  << QVariant(-minDouble) << QJsonValue(-minDouble)
                                     << QVariant(-minDouble);

    QTest::newRow("string null")     << QVariant(QString()) << QJsonValue(QString())
                                     << QVariant(QString());
    QTest::newRow("string empty")    << QVariant(QString("")) << QJsonValue(QString(""))
                                     << QVariant(QString(""));
    QTest::newRow("string ascii")    << QVariant(QString("Data")) << QJsonValue(QString("Data"))
                                     << QVariant(QString("Data"));
    QTest::newRow("string utf8")     << QVariant(QString(utf8)) << QJsonValue(QString(utf8))
                                     << QVariant(QString(utf8));

    QTest::newRow("bytearray null")  << QVariant(QByteArray()) << QJsonValue(QJsonValue::Null)
                                     << QVariant::fromValue(nullptr);
    QTest::newRow("bytearray empty") << QVariant(QByteArray()) << QJsonValue(QJsonValue::Null)
                                     << QVariant::fromValue(nullptr);
    QTest::newRow("bytearray ascii") << QVariant(QByteArray("Data")) << QJsonValue(QString("Data"))
                                     << QVariant(QString("Data"));
    QTest::newRow("bytearray utf8")  << QVariant(utf8) << QJsonValue(QString(utf8))
                                     << QVariant(QString(utf8));

    QTest::newRow("datetime") << QVariant(dt) << QJsonValue(dt.toString(Qt::ISODateWithMs))
                              << QVariant(dt.toString(Qt::ISODateWithMs));
    QTest::newRow("url")      << QVariant(QUrl("http://example.com/{q}"))
                              << QJsonValue("http://example.com/%7Bq%7D")
                              << QVariant(QString("http://example.com/%7Bq%7D"));
    QTest::newRow("uuid")     << QVariant(QUuid(uuid))
                              << QJsonValue(uuid.toString(QUuid::WithoutBraces))
                              << QVariant(uuid.toString(QUuid::WithoutBraces));
    QTest::newRow("regexp")   << QVariant(QRegularExpression(".")) << QJsonValue(QJsonValue::Null)
                              << QVariant::fromValue(nullptr);

    QTest::newRow("inf")      << QVariant(qInf()) << QJsonValue(QJsonValue::Null)
                              << QVariant::fromValue(nullptr);
    QTest::newRow("-inf")     << QVariant(-qInf()) << QJsonValue(QJsonValue::Null)
                              << QVariant::fromValue(nullptr);
    QTest::newRow("NaN")      << QVariant(qQNaN()) << QJsonValue(QJsonValue::Null)
                              << QVariant::fromValue(nullptr);
}

void tst_QtJson::fromToVariantConversions()
{
    QFETCH(QVariant, variant);
    QFETCH(QJsonValue, json);
    QFETCH(QVariant, jsonToVariant);

    QVariant variantFromJson(json);
    QVariant variantFromJsonArray(QJsonArray { json });
    QVariant variantFromJsonObject(QVariantMap { { "foo", variant } });

    QJsonObject object { QPair<QString, QJsonValue>("foo", json) };

    // QJsonValue <> QVariant
    {
        QCOMPARE(QJsonValue::fromVariant(variant), json);

        // test the same for QVariant from QJsonValue/QJsonArray/QJsonObject
        QCOMPARE(QJsonValue::fromVariant(variantFromJson), json);
        QCOMPARE(QJsonValue::fromVariant(variantFromJsonArray), QJsonArray { json });
        QCOMPARE(QJsonValue::fromVariant(variantFromJsonObject), object);

        // QJsonValue to variant
        QCOMPARE(json.toVariant(), jsonToVariant);
        QCOMPARE(json.toVariant().userType(), jsonToVariant.userType());

        // variant to QJsonValue
        QCOMPARE(QVariant(json).toJsonValue(), json);
    }

    // QJsonArray <> QVariantList
    {
        QCOMPARE(QJsonArray::fromVariantList(QVariantList { variant }), QJsonArray { json });

        // test the same for QVariantList from QJsonValue/QJsonArray/QJsonObject
        QCOMPARE(QJsonArray::fromVariantList(QVariantList { variantFromJson }),
                 QJsonArray { json });
        QCOMPARE(QJsonArray::fromVariantList(QVariantList { variantFromJsonArray }),
                 QJsonArray {{ QJsonArray { json } }});
        QCOMPARE(QJsonArray::fromVariantList(QVariantList { variantFromJsonObject }),
                 QJsonArray { object });

        // QJsonArray to variant
        QCOMPARE(QJsonArray { json }.toVariantList(), QVariantList { jsonToVariant });
        // variant to QJsonArray
        QCOMPARE(QVariant(QJsonArray { json }).toJsonArray(), QJsonArray { json });
    }

    // QJsonObject <> QVariantMap
    {
        QCOMPARE(QJsonObject::fromVariantMap(QVariantMap { { "foo", variant } }), object);

        // test the same for QVariantMap from QJsonValue/QJsonArray/QJsonObject
        QCOMPARE(QJsonObject::fromVariantMap(QVariantMap { { "foo", variantFromJson } }), object);

        QJsonObject nestedArray { QPair<QString, QJsonArray>("bar", QJsonArray { json }) };
        QJsonObject nestedObject { QPair<QString, QJsonObject>("bar", object) };
        QCOMPARE(QJsonObject::fromVariantMap(QVariantMap { { "bar", variantFromJsonArray } }),
                 nestedArray);
        QCOMPARE(QJsonObject::fromVariantMap(QVariantMap { { "bar", variantFromJsonObject } }),
                 nestedObject);

        // QJsonObject to variant
        QCOMPARE(object.toVariantMap(), QVariantMap({ { "foo", jsonToVariant } }));
        // variant to QJsonObject
        QCOMPARE(QVariant(object).toJsonObject(), object);
    }
}

QTEST_MAIN(tst_QtJson)
#include "tst_qtjson.moc"
