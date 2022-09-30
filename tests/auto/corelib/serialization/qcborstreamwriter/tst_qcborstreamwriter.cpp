// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QTest>
#include <QCborStreamWriter>
#include <QBuffer>

class tst_QCborStreamWriter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase_data();
    void fixed_data();
    void fixed();
    void strings_data();
    void strings() { fixed(); }
    void nonAsciiStrings_data();
    void nonAsciiStrings();
    void arraysAndMaps_data();
    void arraysAndMaps() { fixed(); }
    void tags_data();
    void tags();
    void arrays_data() { tags_data(); }
    void arrays();
    void maps_data() { tags_data(); }
    void maps();
};

// Get the data from TinyCBOR (see src/3rdparty/tinycbor/tests/encoder/data.cpp)
typedef quint64 CborTag;
#include "data.cpp"

void encodeVariant(QCborStreamWriter &writer, const QVariant &v)
{
    int type = v.userType();
    switch (type) {
    case QMetaType::Int:
    case QMetaType::LongLong:
        return writer.append(v.toLongLong());

    case QMetaType::UInt:
    case QMetaType::ULongLong:
        return writer.append(v.toULongLong());

    case QMetaType::Bool:
        return writer.append(v.toBool());

    case QMetaType::UnknownType:
        return writer.appendUndefined();

    case QMetaType::VoidStar:
        return writer.append(nullptr);

    case QMetaType::Double:
        return writer.append(v.toDouble());

    case QMetaType::Float:
        return writer.append(v.toFloat());

    case QMetaType::QString:
        return writer.append(v.toString());

    case QMetaType::QByteArray:
        return writer.append(v.toByteArray());

    default:
        if (type == qMetaTypeId<NegativeInteger>())
            return writer.append(QCborNegativeInteger(v.value<NegativeInteger>().abs));
        if (type == qMetaTypeId<SimpleType>())
            return writer.append(QCborSimpleType(v.value<SimpleType>().type));
        if (type == qMetaTypeId<qfloat16>())
            return writer.append(v.value<qfloat16>());
        if (type == qMetaTypeId<Tag>()) {
            writer.append(QCborTag(v.value<Tag>().tag));
            return encodeVariant(writer, v.value<Tag>().tagged);
        }
        if (type == QMetaType::QVariantList || type == qMetaTypeId<IndeterminateLengthArray>()) {
            QVariantList list = v.toList();
            if (type == qMetaTypeId<IndeterminateLengthArray>()) {
                list = v.value<IndeterminateLengthArray>();
                writer.startArray();
            } else {
                writer.startArray(list.size());
            }
            for (const QVariant &v2 : std::as_const(list))
                encodeVariant(writer, v2);
            QVERIFY(writer.endArray());
            return;
        }
        if (type == qMetaTypeId<Map>() || type == qMetaTypeId<IndeterminateLengthMap>()) {
            Map map = v.value<Map>();
            if (type == qMetaTypeId<IndeterminateLengthMap>()) {
                map = v.value<IndeterminateLengthMap>();
                writer.startMap();
            } else {
                writer.startMap(map.size());
            }
            for (auto pair : std::as_const(map)) {
                encodeVariant(writer, pair.first);
                encodeVariant(writer, pair.second);
            }
            QVERIFY(writer.endMap());
            return;
        }
    }
    QFAIL("Shouldn't have got here");
}

void compare(const QVariant &input, const QByteArray &output)
{
    QFETCH_GLOBAL(bool, useDevice);

    if (useDevice) {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QCborStreamWriter writer(&buffer);
        encodeVariant(writer, input);
        QCOMPARE(buffer.data(), output);
    } else {
        QByteArray buffer;
        QCborStreamWriter writer(&buffer);
        encodeVariant(writer, input);
        QCOMPARE(buffer, output);
    }
}

void tst_QCborStreamWriter::initTestCase_data()
{
    QTest::addColumn<bool>("useDevice");
    QTest::newRow("QByteArray") << false;
    QTest::newRow("QIODevice") << true;
}

void tst_QCborStreamWriter::fixed_data()
{
    addColumns();
    addFixedData();
}

void tst_QCborStreamWriter::fixed()
{
    QFETCH(QVariant, input);
    QFETCH(QByteArray, output);
    compare(input, output);
}

void tst_QCborStreamWriter::strings_data()
{
    addColumns();
    addStringsData();
}

void tst_QCborStreamWriter::nonAsciiStrings_data()
{
    QTest::addColumn<QByteArray>("output");
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("isLatin1");

    QByteArray latin1 = "Résumé";
    QTest::newRow("shortlatin1")
            << ("\x68" + latin1) << QString::fromUtf8(latin1) << true;

    // replicate it 5 times (total 40 bytes)
    latin1 += latin1 + latin1 + latin1 + latin1;
    QTest::newRow("longlatin1")
            << ("\x78\x28" + latin1) << QString::fromUtf8(latin1) << true;

    QByteArray nonlatin1 = "Χαίρετε";
    QTest::newRow("shortnonlatin1")
            << ("\x6e" + nonlatin1) << QString::fromUtf8(nonlatin1) << false;

    // replicate it 4 times (total 56 bytes)
    nonlatin1 = nonlatin1 + nonlatin1 + nonlatin1 + nonlatin1;
    QTest::newRow("longnonlatin1")
            << ("\x78\x38" + nonlatin1) << QString::fromUtf8(nonlatin1) << false;
}

void tst_QCborStreamWriter::nonAsciiStrings()
{
    QFETCH(QByteArray, output);
    QFETCH(QString, input);
    QFETCH(bool, isLatin1);
    QFETCH_GLOBAL(bool, useDevice);

    // will be wrong if !isLatin1
    QByteArray latin1 = input.toLatin1();

    if (useDevice) {
        {
            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly);
            QCborStreamWriter writer(&buffer);
            writer.append(input);
            QCOMPARE(buffer.data(), output);
        }

        if (isLatin1) {
            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly);
            QCborStreamWriter writer(&buffer);
            writer.append(QLatin1String(latin1.constData(), latin1.size()));
            QCOMPARE(buffer.data(), output);
        }
    } else {
        {
            QByteArray buffer;
            QCborStreamWriter writer(&buffer);
            encodeVariant(writer, input);
            QCOMPARE(buffer, output);
        }

        if (isLatin1) {
            QByteArray buffer;
            QCborStreamWriter writer(&buffer);
            writer.append(QLatin1String(latin1.constData(), latin1.size()));
            QCOMPARE(buffer, output);
        }
    }
}

void tst_QCborStreamWriter::arraysAndMaps_data()
{
    addColumns();
    addArraysAndMaps();
}

void tst_QCborStreamWriter::tags_data()
{
    addColumns();
    addFixedData();
    addStringsData();
    addArraysAndMaps();
}

void tst_QCborStreamWriter::tags()
{
    QFETCH(QVariant, input);
    QFETCH(QByteArray, output);

    compare(QVariant::fromValue(Tag{1, input}), "\xc1" + output);
}

void tst_QCborStreamWriter::arrays()
{
    QFETCH(QVariant, input);
    QFETCH(QByteArray, output);

    compare(make_list(input), "\x81" + output);
    if (QTest::currentTestFailed())
        return;

    compare(make_list(input, input), "\x82" + output + output);
    if (QTest::currentTestFailed())
        return;

    // nested lists
    compare(make_list(make_list(input)), "\x81\x81" + output);
    if (QTest::currentTestFailed())
        return;

    compare(make_list(make_list(input), make_list(input)), "\x82\x81" + output + "\x81" + output);
}

void tst_QCborStreamWriter::maps()
{
    QFETCH(QVariant, input);
    QFETCH(QByteArray, output);

    compare(make_map({{1, input}}), "\xa1\1" + output);
    if (QTest::currentTestFailed())
        return;

    compare(make_map({{1, input}, {input, 24}}), "\xa2\1" + output + output + "\x18\x18");
}

QTEST_MAIN(tst_QCborStreamWriter)

#include "tst_qcborstreamwriter.moc"
