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

#include <QtCore/qcborstream.h>
#include <QtTest>

#include <QtCore/private/qbytearray_p.h>

class tst_QCborStreamReader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase_data();
    void basics();
    void clear_data();
    void clear();
    void integers_data();
    void integers();
    void fixed_data();
    void fixed();
    void strings_data();
    void strings();
    void tags_data();
    void tags() { fixed(); }
    void emptyContainers_data();
    void emptyContainers();

    void arrays_data();
    void arrays();
    void maps_data() { arrays_data(); }
    void maps();
    void undefLengthArrays_data() { arrays_data(); }
    void undefLengthArrays();
    void undefLengthMaps_data() { arrays_data(); }
    void undefLengthMaps();

    void next_data() { arrays_data(); }
    void next();
    void validation_data();
    void validation();
    void hugeDeviceValidation_data();
    void hugeDeviceValidation();
    void recursionLimit_data();
    void recursionLimit();

    void addData_singleElement_data();
    void addData_singleElement();
    void addData_complex_data() { arrays_data(); }
    void addData_complex();

    void duplicatedData_data() { arrays_data(); }
    void duplicatedData();
    void extraData_data() { arrays_data(); }
    void extraData();
};

#define FOR_CBOR_TYPE(F) \
    F(QCborStreamReader::UnsignedInteger) \
    F(QCborStreamReader::NegativeInteger) \
    F(QCborStreamReader::ByteArray) \
    F(QCborStreamReader::String) \
    F(QCborStreamReader::Array) \
    F(QCborStreamReader::Map) \
    F(QCborStreamReader::Tag) \
    F(QCborStreamReader::SimpleType) \
    F(QCborStreamReader::Float16) \
    F(QCborStreamReader::Float) \
    F(QCborStreamReader::Double) \
    F(QCborStreamReader::Invalid)

QT_BEGIN_NAMESPACE
namespace QTest {
template<> char *toString<QCborStreamReader::Type>(const QCborStreamReader::Type &t)
{
    return qstrdup([=]() {
        switch (t) {
#define TYPE(t) \
        case t: return QT_STRINGIFY(t);
        FOR_CBOR_TYPE(TYPE)
#undef TYPE
        }
        return "<huh?>";
    }());
}
} // namespace QTest
QT_END_NAMESPACE

// Get the data from TinyCBOR (see src/3rdparty/tinycbor/tests/parser/data.cpp)
#include "data.cpp"

void tst_QCborStreamReader::initTestCase_data()
{
    QTest::addColumn<bool>("useDevice");
    QTest::newRow("QByteArray") << false;
    QTest::newRow("QBuffer") << true;
}

void tst_QCborStreamReader::basics()
{
    QFETCH_GLOBAL(bool, useDevice);
    QBuffer buffer;
    QCborStreamReader reader;

    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
        QVERIFY(reader.device() != nullptr);
    } else {
        QCOMPARE(reader.device(), nullptr);
    }

    QCOMPARE(reader.currentOffset(), 0);
    QCOMPARE(reader.lastError(), QCborError::EndOfFile);

    QCOMPARE(reader.type(), QCborStreamReader::Invalid);
    QVERIFY(!reader.isUnsignedInteger());
    QVERIFY(!reader.isNegativeInteger());
    QVERIFY(!reader.isByteArray());
    QVERIFY(!reader.isString());
    QVERIFY(!reader.isArray());
    QVERIFY(!reader.isContainer());
    QVERIFY(!reader.isMap());
    QVERIFY(!reader.isTag());
    QVERIFY(!reader.isSimpleType());
    QVERIFY(!reader.isBool());
    QVERIFY(!reader.isNull());
    QVERIFY(!reader.isUndefined());
    QVERIFY(!reader.isFloat16());
    QVERIFY(!reader.isFloat());
    QVERIFY(!reader.isDouble());
    QVERIFY(!reader.isValid());
    QVERIFY(reader.isInvalid());

    QCOMPARE(reader.containerDepth(), 0);
    QCOMPARE(reader.parentContainerType(), QCborStreamReader::Invalid);
    QVERIFY(!reader.hasNext());
    QVERIFY(!reader.next());
    QCOMPARE(reader.lastError(), QCborError::EndOfFile);

    QVERIFY(reader.isLengthKnown());    // well, it's not unknown
    QCOMPARE(reader.length(), ~0ULL);

    if (useDevice) {
        reader.reparse();
        QVERIFY(reader.device() != nullptr);
    } else {
        reader.addData(QByteArray());
        QCOMPARE(reader.device(), nullptr);
    }

    // nothing changes, we added nothing
    QCOMPARE(reader.currentOffset(), 0);
    QCOMPARE(reader.lastError(), QCborError::EndOfFile);

    QCOMPARE(reader.type(), QCborStreamReader::Invalid);
    QVERIFY(!reader.isUnsignedInteger());
    QVERIFY(!reader.isNegativeInteger());
    QVERIFY(!reader.isByteArray());
    QVERIFY(!reader.isString());
    QVERIFY(!reader.isArray());
    QVERIFY(!reader.isContainer());
    QVERIFY(!reader.isMap());
    QVERIFY(!reader.isTag());
    QVERIFY(!reader.isSimpleType());
    QVERIFY(!reader.isBool());
    QVERIFY(!reader.isNull());
    QVERIFY(!reader.isUndefined());
    QVERIFY(!reader.isFloat16());
    QVERIFY(!reader.isFloat());
    QVERIFY(!reader.isDouble());
    QVERIFY(!reader.isValid());
    QVERIFY(reader.isInvalid());

    QVERIFY(reader.isLengthKnown());    // well, it's not unknown
    QCOMPARE(reader.length(), ~0ULL);

    QCOMPARE(reader.containerDepth(), 0);
    QCOMPARE(reader.parentContainerType(), QCborStreamReader::Invalid);
    QVERIFY(!reader.hasNext());
    QVERIFY(!reader.next());

    reader.clear();
    QCOMPARE(reader.device(), nullptr);
    QCOMPARE(reader.currentOffset(), 0);
    QCOMPARE(reader.lastError(), QCborError::EndOfFile);
}

void tst_QCborStreamReader::clear_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QCborError>("firstError");
    QTest::addColumn<int>("offsetAfterSkip");
    QTest::newRow("invalid") << QByteArray(512, '\xff') << QCborError{QCborError::UnexpectedBreak} << 0;
    QTest::newRow("valid")   << QByteArray(512, '\0')   << QCborError{QCborError::NoError} << 0;
    QTest::newRow("skipped") << QByteArray(512, '\0')   << QCborError{QCborError::NoError} << 1;
}

void tst_QCborStreamReader::clear()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(QCborError, firstError);
    QFETCH(int, offsetAfterSkip);

    QBuffer buffer(&data);
    QCborStreamReader reader(data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    QCOMPARE(reader.isValid(), !firstError);
    QCOMPARE(reader.currentOffset(), 0);
    QCOMPARE(reader.lastError(), firstError);

    if (offsetAfterSkip) {
        reader.next();
        QVERIFY(!reader.isValid());
        QCOMPARE(reader.currentOffset(), 1);
        QCOMPARE(reader.lastError(), QCborError::NoError);
    }

    reader.clear();
    QCOMPARE(reader.device(), nullptr);
    QCOMPARE(reader.currentOffset(), 0);
    QCOMPARE(reader.lastError(), QCborError::EndOfFile);
}

void tst_QCborStreamReader::integers_data()
{
    addIntegers();
}

void tst_QCborStreamReader::integers()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(bool, isNegative);
    QFETCH(quint64, expectedRaw);
    QFETCH(qint64, expectedValue);
    QFETCH(bool, inInt64Range);
    quint64 absolute = (isNegative ? expectedRaw + 1 : expectedRaw);

    QBuffer buffer(&data);
    QCborStreamReader reader(useDevice ? QByteArray() : data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    QVERIFY(reader.isInteger());

    if (inInt64Range)
        QCOMPARE(reader.toInteger(), expectedValue);
    if (isNegative)
        QCOMPARE(quint64(reader.toNegativeInteger()), absolute);
    else
        QCOMPARE(reader.toUnsignedInteger(), absolute);
}

void escapedAppendTo(QString &result, const QByteArray &data)
{
    result += "h'" + QString::fromLatin1(data.toHex()) + '\'';
}

void escapedAppendTo(QString &result, const QString &data)
{
    result += '"';
    for (int i = 0; i <= data.size(); i += 245) {
        // hopefully we won't have a surrogate pair split here
        QScopedArrayPointer<char> escaped(QTest::toPrettyUnicode(data.mid(i, 245)));
        QLatin1String s(escaped.data() + 1);    // skip opening "
        s.chop(1);                              // drop the closing "
        result += s;
    }
    result += '"';
}

template <typename S, QCborStreamReader::StringResult<S> (QCborStreamReader:: *Decoder)()>
static QString parseOneString_helper(QCborStreamReader &reader)
{
    QString result;
    bool parens = !reader.isLengthKnown();
    if (parens)
        result += '(';

    auto r = (reader.*Decoder)();
    const char *comma = "";
    while (r.status == QCborStreamReader::Ok) {
        result += comma;
        escapedAppendTo(result, r.data);

        r = (reader.*Decoder)();
        comma = ", ";
    }

    if (r.status == QCborStreamReader::Error)
        return QString();

    if (parens)
        result += ')';
    return result;
}

static QString parseOneByteArray(QCborStreamReader &reader)
{
    return parseOneString_helper<QByteArray, &QCborStreamReader::readByteArray>(reader);
}

static QString parseOneString(QCborStreamReader &reader)
{
    return parseOneString_helper<QString, &QCborStreamReader::readString>(reader);
}

static QString makeNegativeString(QCborNegativeInteger n)
{
    return n == QCborNegativeInteger(0) ?
                QString("-18446744073709551616") :
                QString("-%1").arg(quint64(n));
}

template <typename T> static inline bool canConvertTo(double v)
{
    // The [conv.fpint] (7.10 Floating-integral conversions) section of the
    // standard says only exact conversions are guaranteed. Converting
    // integrals to floating-point with loss of precision has implementation-
    // defined behavior whether the next higher or next lower is returned;
    // converting FP to integral is UB if it can't be represented.;
    Q_STATIC_ASSERT(std::numeric_limits<T>::is_integer);

    double supremum = ldexp(1, std::numeric_limits<T>::digits);
    if (v >= supremum)
        return false;

    if (v < std::numeric_limits<T>::min()) // either zero or a power of two, so it's exact
        return false;

    // we're in range
    return v == floor(v);
}

static QString makeFpString(double v)
{
    if (canConvertTo<qint64>(v))
        return QString::number(qint64(v)) + '.';
    if (canConvertTo<quint64>(v))
        return QString::number(quint64(v)) + '.';

    QString s = QString::number(v, 'g', std::numeric_limits<double>::digits10 + 2);
    if (!s.contains('.') && !s.contains('e') && !qIsInf(v) && !qIsNaN(v))
        s += '.';
    return s;
}

static QString makeFpString(float v)
{
    if (qIsInf(v))
        return v > 0 ? "inf" : "-inf";
    if (qIsNaN(v))
        return "nan";
    return makeFpString(double(v)) + 'f';
}

static QString makeFpString(qfloat16 v)
{
    if (qIsInf(v))
        return v > 0 ? "inf" : "-inf";
    if (qIsNaN(v))
        return "nan";
    return makeFpString(double(v)) + "f16";
}

static QString parseOne(QCborStreamReader &reader)
{
    QString result;

    switch (reader.type()) {
    case QCborStreamReader::UnsignedInteger:
        result = QString::number(reader.toUnsignedInteger());
        break;
    case QCborStreamReader::NegativeInteger:
        result = makeNegativeString(reader.toNegativeInteger());
        break;
    case QCborStreamReader::ByteArray:
        return parseOneByteArray(reader);
    case QCborStreamReader::String:
        return parseOneString(reader);
    case QCborStreamReader::Array:
    case QCborStreamReader::Map: {
        const char *delimiters = (reader.isArray() ? "[]" : "{}");
        result += delimiters[0];
        reader.enterContainer();

        QLatin1String comma("");
        while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
            result += comma + parseOne(reader);
            comma = QLatin1String(", ");

            if (reader.parentContainerType() == QCborStreamReader::Map
                    && reader.lastError() == QCborError::NoError)
                result += ": " + parseOne(reader);
        }

        if (reader.isValid())
            return QString();
        if (reader.lastError() != QCborError::NoError)
            return QString();
        reader.leaveContainer();
        result += delimiters[1];
        return result;
    }
    case QCborStreamReader::Tag: {
        QCborTag tag = reader.toTag();
        if (!reader.next())
            return QString();
        return QString("%1(%2)").arg(quint64(tag)).arg(parseOne(reader));
    }
    case QCborStreamReader::SimpleType:
        switch (reader.toSimpleType()) {
        case QCborSimpleType::False:
            result = QStringLiteral("false");
            break;
        case QCborSimpleType::True:
            result = QStringLiteral("true");
            break;
        case QCborSimpleType::Null:
            result = QStringLiteral("null");
            break;
        case QCborSimpleType::Undefined:
            result = QStringLiteral("undefined");
            break;
        default:
            result = QString("simple(%1)").arg(quint8(reader.toSimpleType()));
            break;
        }
        break;
    case QCborStreamReader::Float16:
        result = makeFpString(reader.toFloat16());
        break;
    case QCborStreamReader::Float:
        result = makeFpString(reader.toFloat());
        break;
    case QCborStreamReader::Double:
        result = makeFpString(reader.toDouble());
        break;
    case QCborStreamReader::Invalid:
        return QStringLiteral("<invalid>");
    }

    if (!reader.next())
        return QString();
    return result;
}

static QString parse(QCborStreamReader &reader, const QByteArray &data)
{
    qint64 oldPos = 0;
    if (QIODevice *dev = reader.device())
        oldPos = dev->pos();

    QString r = parseOne(reader);
    if (r.isEmpty())
        return r;

    if (reader.currentOffset() - oldPos != data.size())
        r = QString("Number of parsed bytes (%1) not expected (%2)")
                .arg(reader.currentOffset()).arg(data.size());
    if (QIODevice *dev = reader.device()) {
        if (dev->pos() - oldPos != data.size())
            r = QString("QIODevice not advanced (%1) as expected (%2)")
                    .arg(dev->pos()).arg(data.size());
    }

    return r;
}

bool parseNonRecursive(QString &result, bool &printingStringChunks, QCborStreamReader &reader)
{
    while (reader.lastError() == QCborError::NoError) {
        if (!reader.hasNext()) {
            if (result.endsWith(", "))
                result.chop(2);
            if (reader.containerDepth() == 0)
                return true;
            result += reader.parentContainerType() == QCborStreamReader::Map ? "}, " : "], ";
            reader.leaveContainer();
            continue;
        }

        switch (reader.type()) {
        case QCborStreamReader::UnsignedInteger:
            result += QString::number(reader.toUnsignedInteger());
            break;
        case QCborStreamReader::NegativeInteger:
            result += makeNegativeString(reader.toNegativeInteger());
            break;
        case QCborStreamReader::ByteArray:
        case QCborStreamReader::String: {
            QCborStreamReader::StringResultCode status;
            if (!printingStringChunks && !reader.isLengthKnown()) {
                printingStringChunks = true;
                result += '(';
            }
            if (reader.isByteArray()) {
                auto r = reader.readByteArray();
                status = r.status;
                if (r.status == QCborStreamReader::Ok)
                    escapedAppendTo(result, r.data);
            } else {
                auto r = reader.readString();
                status = r.status;
                if (r.status == QCborStreamReader::Ok)
                    escapedAppendTo(result, r.data);
            }

            if (status == QCborStreamReader::EndOfString) {
                if (result.endsWith(", "))
                    result.chop(2);
                if (printingStringChunks)
                    result += ')';
                result += ", ";
                printingStringChunks = false;
            }
            if (status == QCborStreamReader::Ok && printingStringChunks)
                result += ", ";

            continue;
        }
        case QCborStreamReader::Array:
            result += '[';
            reader.enterContainer();
            continue;
        case QCborStreamReader::Map:
            result += '{';
            reader.enterContainer();
            continue;
        case QCborStreamReader::Tag:
            result += QString("Tag:%1:").arg(quint64(reader.toTag()));
            reader.next();
            continue;       // skip the comma
        case QCborStreamReader::SimpleType:
            switch (reader.toSimpleType()) {
            case QCborSimpleType::False:
                result += QStringLiteral("false");
                break;
            case QCborSimpleType::True:
                result += QStringLiteral("true");
                break;
            case QCborSimpleType::Null:
                result += QStringLiteral("null");
                break;
            case QCborSimpleType::Undefined:
                result += QStringLiteral("undefined");
                break;
            default:
                result += QString("simple(%1)").arg(quint8(reader.toSimpleType()));
                break;
            }
            break;
        case QCborStreamReader::Float16:
            result += makeFpString(reader.toFloat16());
            break;
        case QCborStreamReader::Float:
            result += makeFpString(reader.toFloat());
            break;
        case QCborStreamReader::Double:
            result += makeFpString(reader.toDouble());
            break;
        case QCborStreamReader::Invalid:
            break;
        }

        reader.next();
        result += ", ";
    }
    return false;
};


static QString &removeIndicators(QString &str)
{
    // remove any CBOR encoding indicators from the string, since parseOne above
    // doesn't produce them
    static QRegularExpression rx("_(\\d+)? ?");
    return str.replace(rx, QString());
}

void tst_QCborStreamReader::fixed_data()
{
    addColumns();
    addFixedData();
}

void tst_QCborStreamReader::fixed()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    QBuffer buffer(&data);
    QCborStreamReader reader(useDevice ? QByteArray() : data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    QCOMPARE(parse(reader, data), expected);

    // verify that we can re-read
    reader.reset();
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    QCOMPARE(parse(reader, data), expected);
}

void tst_QCborStreamReader::strings_data()
{
    addColumns();
    addStringsData();
}

void tst_QCborStreamReader::strings()
{
    fixed();
    if (QTest::currentTestFailed())
        return;

    // Extra string checks:
    // We'll compare the reads using readString() and readByteArray()
    // (henceforth "control data" because fixed() above tested them) with those
    // obtained with readStringChunk().

    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    QFETCH_GLOBAL(bool, useDevice);
    bool isChunked = expected.startsWith('(');

    QBuffer buffer(&data), controlBuffer(&data);
    QCborStreamReader reader(data), controlReader(data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        controlBuffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
        controlReader.setDevice(&controlBuffer);
    }
    QVERIFY(reader.isString() || reader.isByteArray());
    QCOMPARE(reader.isLengthKnown(), !isChunked);

    if (!isChunked)
        QCOMPARE(reader.currentStringChunkSize(), qsizetype(reader.length()));

    int chunks = 0;
    forever {
        QCborStreamReader::StringResult<QByteArray> controlData;
        if (reader.isString()) {
            auto r = controlReader.readString();
            controlData.data = r.data.toUtf8();
            controlData.status =  r.status;
        } else {
            controlData = controlReader.readByteArray();
        }
        QVERIFY(controlData.status != QCborStreamReader::Error);

        for (int i = 0; i < 10; ++i) {
            // this call must work several times with the same result
            QCOMPARE(reader.currentStringChunkSize(), controlData.data.size());
        }

        QByteArray chunk(controlData.data.size(), Qt::Uninitialized);
        auto r = reader.readStringChunk(chunk.data(), chunk.size());
        QCOMPARE(r.status, controlData.status);
        if (r.status == QCborStreamReader::Ok)
            QCOMPARE(r.data, controlData.data.size());
        else
            QCOMPARE(r.data, 0);
        QCOMPARE(chunk, controlData.data);

        if (r.status == QCborStreamReader::EndOfString)
            break;
        ++chunks;
    }

    if (!isChunked)
        QCOMPARE(chunks, 1);
}

void tst_QCborStreamReader::tags_data()
{
    addColumns();
    addTagsData();
}

void tst_QCborStreamReader::emptyContainers_data()
{
    addColumns();
    addEmptyContainersData();
}

void tst_QCborStreamReader::emptyContainers()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    QBuffer buffer(&data);
    QCborStreamReader reader(data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    if (reader.isLengthKnown())
        QCOMPARE(reader.length(), 0U);
    QCOMPARE(parse(reader, data), expected);

    // verify that we can re-read
    reader.reset();
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    if (reader.isLengthKnown())
        QCOMPARE(reader.length(), 0U);
    QCOMPARE(parse(reader, data), expected);
}

void tst_QCborStreamReader::arrays_data()
{
    addColumns();
    addFixedData();
    addStringsData();
    addTagsData();
    addEmptyContainersData();
}

static void checkContainer(int len, const QByteArray &data, const QString &expected)
{
    QFETCH_GLOBAL(bool, useDevice);

    QByteArray copy = data;
    QBuffer buffer(&copy);
    QCborStreamReader reader(data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    if (len >= 0) {
        QVERIFY(reader.isLengthKnown());
        QCOMPARE(reader.length(), uint(len));
    }
    QCOMPARE(parse(reader, data), expected);

    // verify that we can re-read
    reader.reset();
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    if (len >= 0) {
        QVERIFY(reader.isLengthKnown());
        QCOMPARE(reader.length(), uint(len));
    }
    QCOMPARE(parse(reader, data), expected);
}

void tst_QCborStreamReader::arrays()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    checkContainer(1, '\x81' + data, '[' + expected + ']');
    if (QTest::currentTestFailed())
        return;

    checkContainer(2, '\x82' + data + data, '[' + expected + ", " + expected + ']');
}

void tst_QCborStreamReader::maps()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    // int keys
    checkContainer(1, "\xa1\1" + data, "{1: " + expected + '}');
    if (QTest::currentTestFailed())
        return;

    checkContainer(2, "\xa2\1" + data + '\x20' + data,
                   "{1: " + expected + ", -1: " + expected + '}');
    if (QTest::currentTestFailed())
        return;

    // string keys
    checkContainer(1, "\xa1\x65Hello" + data, "{\"Hello\": " + expected + '}');
    if (QTest::currentTestFailed())
        return;

    checkContainer(2, "\xa2\x65World" + data + "\x65Hello" + data,
                   "{\"World\": " + expected + ", \"Hello\": " + expected + '}');
}

void tst_QCborStreamReader::undefLengthArrays()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    checkContainer(-1, '\x9f' + data + '\xff', '[' + expected + ']');
    if (QTest::currentTestFailed())
        return;

    checkContainer(-2, '\x9f' + data + data + '\xff', '[' + expected + ", " + expected + ']');
}

void tst_QCborStreamReader::undefLengthMaps()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    // int keys
    checkContainer(-1, "\xbf\1" + data + '\xff', "{1: " + expected + '}');
    if (QTest::currentTestFailed())
        return;

    checkContainer(-2, "\xbf\1" + data + '\x20' + data + '\xff',
                   "{1: " + expected + ", -1: " + expected + '}');
    if (QTest::currentTestFailed())
        return;

    // string keys
    checkContainer(-1, "\xbf\x65Hello" + data + '\xff', "{\"Hello\": " + expected + '}');
    if (QTest::currentTestFailed())
        return;

    checkContainer(-2, "\xbf\x65World" + data + "\x65Hello" + data + '\xff',
                   "{\"World\": " + expected + ", \"Hello\": " + expected + '}');
}

void tst_QCborStreamReader::next()
{
    QFETCH(QByteArray, data);

    auto doit = [](QByteArray data) {
        QFETCH_GLOBAL(bool, useDevice);

        QBuffer buffer(&data);
        QCborStreamReader reader(data);
        if (useDevice) {
            buffer.open(QIODevice::ReadOnly);
            reader.setDevice(&buffer);
        }
        return reader.next();
    };

    QVERIFY(doit('\x81' + data));
    QVERIFY(doit('\x82' + data + data));
    QVERIFY(doit('\x9f' + data + '\xff'));
    QVERIFY(doit("\x81\x9f" + data + '\xff'));
    QVERIFY(doit("\x9f\x81" + data + '\xff'));

    QVERIFY(doit("\xa1\1" + data));
    QVERIFY(doit("\xa2\1" + data + '\x20' + data));
    QVERIFY(doit("\xbf\1" + data + '\xff'));
    QVERIFY(doit("\xbf\x9f\1\xff\x9f" + data + "\xff\xff"));
}

#include "../cborlargedatavalidation.cpp"

void tst_QCborStreamReader::validation_data()
{
    // Add QCborStreamReader-specific limitations due to use of QByteArray and
    // QString, which are allocated by QArrayData::allocate().
    const qsizetype MaxInvalid = std::numeric_limits<QByteArray::size_type>::max();
    const qsizetype MinInvalid = MaxByteArraySize + 1;

    addValidationColumns();
    addValidationData(MinInvalid);
    addValidationLargeData(MinInvalid, MaxInvalid);
}

void tst_QCborStreamReader::validation()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(CborError, expectedError);
    QCborError error = { QCborError::Code(expectedError) };

    QBuffer buffer(&data);
    QCborStreamReader reader(data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    parse(reader, data);
    QCOMPARE(reader.lastError(), error);

    // next() should fail
    reader.reset();
    QVERIFY(!reader.next());
    QCOMPARE(reader.lastError(), error);
}

void tst_QCborStreamReader::hugeDeviceValidation_data()
{
    addValidationHugeDevice(MaxByteArraySize + 1, MaxStringSize + 1);
}

void tst_QCborStreamReader::hugeDeviceValidation()
{
    QFETCH_GLOBAL(bool, useDevice);
    if (!useDevice)
        return;

    QFETCH(QSharedPointer<QIODevice>, device);
    QFETCH(CborError, expectedError);
    QCborError error = { QCborError::Code(expectedError) };

    device->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    QCborStreamReader reader(device.data());

    QVERIFY(parseOne(reader).isEmpty());
    QCOMPARE(reader.lastError(), error);

    // next() should fail
    reader.reset();
    QVERIFY(!reader.next());
    QCOMPARE(reader.lastError(), error);
}

static const int Recursions = 3;
void tst_QCborStreamReader::recursionLimit_data()
{
    static const int recursions = Recursions + 2;
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("array") << QByteArray(recursions, '\x81') + '\x20';
    QTest::newRow("_array") << QByteArray(recursions, '\x9f') + '\x20' + QByteArray(recursions, '\xff');

    QByteArray data;
    for (int i = 0; i < recursions; ++i)
        data += "\xa1\x65Hello";
    data += '\2';
    QTest::newRow("map-recursive-values") << data;

    data.clear();
    for (int i = 0; i < recursions; ++i)
        data += "\xbf\x65World";
    data += '\2';
    for (int i = 0; i < recursions; ++i)
        data += "\xff";
    QTest::newRow("_map-recursive-values") << data;

    data = QByteArray(recursions, '\xa1');
    data += '\2';
    for (int i = 0; i < recursions; ++i)
        data += "\x7f\x64quux\xff";
    QTest::newRow("map-recursive-keys") << data;

    data = QByteArray(recursions, '\xbf');
    data += '\2';
    for (int i = 0; i < recursions; ++i)
        data += "\1\xff";
    QTest::newRow("_map-recursive-keys") << data;

    data.clear();
    for (int i = 0; i < recursions / 2; ++i)
        data += "\x81\xa1\1";
    data += '\2';
    QTest::newRow("mixed") << data;
}

void tst_QCborStreamReader::recursionLimit()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);

    data.prepend('\x81');
    QBuffer buffer(&data);
    QCborStreamReader reader(data);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }

    // verify that it works normally:
    QVERIFY(reader.enterContainer());
    QVERIFY(reader.next());
    QVERIFY(!reader.hasNext());

    reader.reset();
    QVERIFY(reader.enterContainer());
    QVERIFY(!reader.next(Recursions));
    QCOMPARE(reader.lastError(), QCborError::NestingTooDeep);
}

void tst_QCborStreamReader::addData_singleElement_data()
{
    addColumns();
    addFixedData();
    addNonChunkedStringsData();
}

void tst_QCborStreamReader::addData_singleElement()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    QByteArray growing;
    QBuffer buffer(&growing);
    QCborStreamReader reader;
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    for (int i = 0; i < data.size() - 1; ++i) {
        // add one byte from the data
        if (useDevice) {
            growing.append(data.at(i));
            reader.reparse();
        } else {
            reader.addData(data.constData() + i, 1);
        }

        parse(reader, data);
        QCOMPARE(reader.lastError(), QCborError::EndOfFile);
    }

    // add the last byte
    if (useDevice) {
        growing.append(data.right(1));
        reader.reparse();
    } else {
        reader.addData(data.right(1));
    }
    QCOMPARE(reader.lastError(), QCborError::NoError);
    QCOMPARE(parse(reader, data), expected);
}

void tst_QCborStreamReader::addData_complex()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    // transform tags (parseNonRecursive can't produce the usual form)
    expected.replace(QRegularExpression(R"/((\d+)\(([^)]+)\))/"), "Tag:\\1:\\2");

    auto doit = [](const QByteArray &data) {
        QFETCH_GLOBAL(bool, useDevice);

        // start with one byte
        int added = 1;
        QByteArray growing = data.left(added);
        QBuffer buffer(&growing);
        QCborStreamReader reader(growing);
        if (useDevice) {
            buffer.open(QIODevice::ReadOnly);
            reader.setDevice(&buffer);
        }

        QString result;
        bool printingStringChunks = false;
        forever {
            if (parseNonRecursive(result, printingStringChunks, reader))
                return result;
            if (reader.lastError() != QCborError::EndOfFile)
                return reader.lastError().toString();

            while (reader.lastError() == QCborError::EndOfFile) {
                // add more data
                if (added == data.size())
                    return QStringLiteral("Couldn't parse even with all data");

                if (useDevice) {
                    growing.append(data.at(added));
                    reader.reparse();
                } else {
                    reader.addData(data.constData() + added, 1);
                }
                ++added;
            }
        }
    };

    // plain:
    QCOMPARE(doit(data), expected);

    // in an array
    QCOMPARE(doit('\x81' + data), '[' + expected + ']');
    QCOMPARE(doit('\x82' + data + data), '[' + expected + ", " + expected + ']');

    QCOMPARE(doit('\x9f' + data + '\xff'), '[' + expected + ']');
    QCOMPARE(doit('\x9f' + data + data + '\xff'), '[' + expected + ", " + expected + ']');

    // in a map
    QCOMPARE(doit("\xa1\x01" + data), "{1, " + expected + '}');
    QCOMPARE(doit("\xa1\x65Hello" + data), "{\"Hello\", " + expected + '}');
    QCOMPARE(doit("\xa1\x7f\x65Hello\x65World\xff" + data), "{(\"Hello\", \"World\"), " + expected + '}');
    QCOMPARE(doit("\xa2\x01" + data + "\x65Hello" + data),
             "{1, " + expected + ", \"Hello\", " + expected + '}');

    QCOMPARE(doit("\xbf\x01" + data + '\xff'), "{1, " + expected + '}');

    // mixed
    QCOMPARE(doit("\xbf\x01\x81" + data + '\xff'), "{1, [" + expected + "]}");
    QCOMPARE(doit("\xbf\x01\x82" + data + data + '\xff'),
             "{1, [" + expected + ", " + expected + "]}");
    QCOMPARE(doit("\xbf\x01\x9f" + data + data + "\xff\xff"),
             "{1, [" + expected + ", " + expected + "]}");
}

void tst_QCborStreamReader::duplicatedData()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    // double the data up
    QByteArray doubledata = data + data;

    QBuffer buffer(&doubledata);
    QCborStreamReader reader(doubledata);
    if (useDevice) {
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    QVERIFY(reader.isValid());
    QCOMPARE(reader.lastError(), QCborError::NoError);
    QCOMPARE(parse(reader, data), expected);     // yes, data

    QVERIFY(reader.currentOffset() < doubledata.size());
    if (useDevice) {
        reader.setDevice(&buffer);
        QVERIFY(reader.isValid());
        QCOMPARE(reader.lastError(), QCborError::NoError);
        QCOMPARE(parse(reader, data), expected);
        QCOMPARE(buffer.pos(), doubledata.size());
    } else {
        // there's no reader.setData()
    }
}

void tst_QCborStreamReader::extraData()
{
    QFETCH_GLOBAL(bool, useDevice);
    QFETCH(QByteArray, data);
    QFETCH(QString, expected);
    removeIndicators(expected);

    QByteArray extension(9, '\0');

    // stress test everything with extra bytes (just one byte changing;
    // TinyCBOR used to have a bug where the next byte got sometimes read)
    for (int c = '\0'; c < 0x100; ++c) {
        extension[0] = c;
        QByteArray extendeddata = data + extension;

        QBuffer buffer(&extendeddata);
        QCborStreamReader reader(extendeddata);
        if (useDevice) {
            buffer.open(QIODevice::ReadOnly);
            reader.setDevice(&buffer);
        }
        QVERIFY(reader.isValid());
        QCOMPARE(reader.lastError(), QCborError::NoError);
        QCOMPARE(parse(reader, data), expected);     // yes, data

        // if we were a parser, we could parse the next payload
        if (useDevice)
            QCOMPARE(buffer.readAll(), extension);
    }
}


QTEST_MAIN(tst_QCborStreamReader)

#include "tst_qcborstreamreader.moc"
