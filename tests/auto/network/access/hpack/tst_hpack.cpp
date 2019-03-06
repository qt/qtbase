/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 Governikus GmbH & Co. KG.
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

#include <QtTest/QtTest>

#include <QtNetwork/private/bitstreams_p.h>
#include <QtNetwork/private/hpack_p.h>

#include <QtCore/qbytearray.h>

#include <cstdlib>
#include <vector>
#include <string>

QT_USE_NAMESPACE

using namespace HPack;

class tst_Hpack: public QObject
{
    Q_OBJECT

public:
    tst_Hpack();
private Q_SLOTS:
    void bitstreamConstruction();
    void bitstreamWrite();
    void bitstreamReadWrite();
    void bitstreamCompression();
    void bitstreamErrors();

    void lookupTableConstructor();

    void lookupTableStatic();
    void lookupTableDynamic();

    void hpackEncodeRequest_data();
    void hpackEncodeRequest();
    void hpackDecodeRequest_data();
    void hpackDecodeRequest();

    void hpackEncodeResponse_data();
    void hpackEncodeResponse();
    void hpackDecodeResponse_data();
    void hpackDecodeResponse();

    // TODO: more-more-more tests needed!

private:
    void hpackEncodeRequest(bool withHuffman);
    void hpackEncodeResponse(bool withHuffman);

    HttpHeader header1;
    std::vector<uchar> buffer1;
    BitOStream request1;

    HttpHeader header2;
    std::vector<uchar> buffer2;
    BitOStream request2;

    HttpHeader header3;
    std::vector<uchar> buffer3;
    BitOStream request3;
};

using StreamError = BitIStream::Error;

tst_Hpack::tst_Hpack()
    : request1(buffer1),
      request2(buffer2),
      request3(buffer3)
{
}

void tst_Hpack::bitstreamConstruction()
{
    const uchar bytes[] = {0xDE, 0xAD, 0xBE, 0xEF};
    const int size = int(sizeof bytes);

    // Default ctors:
    std::vector<uchar> buffer;
    {
        const BitOStream out(buffer);
        QVERIFY(out.bitLength() == 0);
        QVERIFY(out.byteLength() == 0);

        const BitIStream in;
        QVERIFY(in.bitLength() == 0);
        QVERIFY(in.streamOffset() == 0);
        QVERIFY(in.error() == StreamError::NoError);
    }

    // Create istream with some data:
    {
        BitIStream in(bytes, bytes + size);
        QVERIFY(in.bitLength() == size * 8);
        QVERIFY(in.streamOffset() == 0);
        QVERIFY(in.error() == StreamError::NoError);
        // 'Read' some data back:
        for (int i = 0; i < size; ++i) {
            uchar bitPattern = 0;
            const auto bitsRead = in.peekBits(quint64(i * 8), 8, &bitPattern);
            QVERIFY(bitsRead == 8);
            QVERIFY(bitPattern == bytes[i]);
        }
    }

    // Copy ctors:
    {
        // Ostreams - copy is disabled.
        // Istreams:
        const BitIStream in1;
        const BitIStream in2(in1);
        QVERIFY(in2.bitLength() == in1.bitLength());
        QVERIFY(in2.streamOffset() == in1.streamOffset());
        QVERIFY(in2.error() == StreamError::NoError);

        const BitIStream in3(bytes, bytes + size);
        const BitIStream in4(in3);
        QVERIFY(in4.bitLength() == in3.bitLength());
        QVERIFY(in4.streamOffset() == in3.streamOffset());
        QVERIFY(in4.error() == StreamError::NoError);
    }
}

void tst_Hpack::bitstreamWrite()
{
    // Known representations,
    // https://http2.github.io/http2-spec/compression.html.
    // 5.1 Integer Representation

    // Test bit/byte lengths of the
    // resulting data:
    std::vector<uchar> buffer;
    BitOStream out(buffer);
    out.write(3);
    // 11, fits into 8-bit prefix:
    QVERIFY(out.bitLength() == 8);
    QVERIFY(out.byteLength() == 1);
    QVERIFY(out.begin()[0] == 3);

    out.clear();
    QVERIFY(out.bitLength() == 0);
    QVERIFY(out.byteLength() == 0);

    // This number does not fit into 8-bit
    // prefix we'll need 2 bytes:
    out.write(256);
    QVERIFY(out.byteLength() == 2);
    QVERIFY(out.bitLength() == 16);
    QVERIFY(out.begin()[0] == 0xff);
    QVERIFY(out.begin()[1] == 1);

    out.clear();

    // See 5.2   String Literal Representation.

    // We use Huffman code,
    // char 'a' has a prefix code 00011 (5 bits)
    out.write(QByteArray("aaa", 3), true);
    QVERIFY(out.byteLength() == 3);
    QVERIFY(out.bitLength() == 24);
    // Now we must have in our stream:
    // 10000010 | 00011000| 11000111
    const uchar *encoded = out.begin();
    QVERIFY(encoded[0] == 0x82);
    QVERIFY(encoded[1] == 0x18);
    QVERIFY(encoded[2] == 0xC7);
    // TODO: add more tests ...
}

void tst_Hpack::bitstreamReadWrite()
{
    // We can write into the bit stream:
    // 1) bit patterns
    // 2) integers (see HPACK, 5.1)
    // 3) string (see HPACK, 5.2)
    std::vector<uchar> buffer;
    BitOStream out(buffer);
    out.writeBits(0xf, 3);
    QVERIFY(out.byteLength() == 1);
    QVERIFY(out.bitLength() == 3);

    // Now, read it back:
    {
        BitIStream in(out.begin(), out.end());
        uchar bitPattern = 0;
        const auto bitsRead = in.peekBits(0, 3, &bitPattern);
        // peekBits pack into the most significant byte/bit:
        QVERIFY(bitsRead == 3);
        QVERIFY((bitPattern >> 5) == 7);
    }

    const quint32 testInt = 133;
    out.write(testInt);

    // This integer does not fit into the current 5-bit prefix,
    // so byteLength == 2.
    QVERIFY(out.byteLength() == 2);
    const auto bitLength = out.bitLength();
    QVERIFY(bitLength > 3);

    // Now, read it back:
    {
        BitIStream in(out.begin(), out.end());
        in.skipBits(3); // Bit pattern
        quint32 value = 0;
        QVERIFY(in.read(&value));
        QVERIFY(in.error() == StreamError::NoError);
        QCOMPARE(value, testInt);
    }

    const QByteArray testString("ABCDE", 5);
    out.write(testString, true); // Compressed
    out.write(testString, false); // Non-compressed
    QVERIFY(out.byteLength() > 2);
    QVERIFY(out.bitLength() > bitLength);

    // Now, read it back:
    {
        BitIStream in(out.begin(), out.end());
        in.skipBits(bitLength); // Bit pattern and integer
        QByteArray value;
        // Read compressed string first ...
        QVERIFY(in.read(&value));
        QCOMPARE(value, testString);
        QCOMPARE(in.error(), StreamError::NoError);
        // Now non-compressed ...
        QVERIFY(in.read(&value));
        QCOMPARE(value, testString);
        QCOMPARE(in.error(), StreamError::NoError);
    }
}

void tst_Hpack::bitstreamCompression()
{
    // Similar to bitstreamReadWrite but
    // writes/reads a lot of mixed strings/integers.
    std::vector<std::string> strings;
    std::vector<quint32> integers;
    std::vector<bool> isA; // integer or string.
    const std::string bytes("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789()[]/*");
    const unsigned nValues = 100000;

    quint64 totalStringBytes = 0;
    std::vector<uchar> buffer;
    BitOStream out(buffer);
    for (unsigned i = 0; i < nValues; ++i) {
        const bool isString = QRandomGenerator::global()->bounded(1000) > 500;
        isA.push_back(isString);
        if (!isString) {
            integers.push_back(QRandomGenerator::global()->bounded(1000u));
            out.write(integers.back());
        } else {
            const auto start = QRandomGenerator::global()->bounded(uint(bytes.length()) / 2);
            auto end = start * 2;
            if (!end)
                end = unsigned(bytes.length() / 2);
            strings.push_back(bytes.substr(start, end - start));
            const auto &s = strings.back();
            totalStringBytes += s.size();
            QByteArray data(s.c_str(), int(s.size()));
            const bool compressed(QRandomGenerator::global()->bounded(1000) > 500);
            out.write(data, compressed);
        }
    }

    qDebug() << "Compressed(?) byte length:" << out.byteLength()
             << "total string bytes:" << totalStringBytes;
    qDebug() << "total integer bytes (for quint32):" << integers.size() * sizeof(quint32);

    QVERIFY(out.byteLength() > 0);
    QVERIFY(out.bitLength() > 0);

    BitIStream in(out.begin(), out.end());

    for (unsigned i = 0, iS = 0, iI = 0; i < nValues; ++i) {
        if (isA[i]) {
            QByteArray data;
            QVERIFY(in.read(&data));
            QCOMPARE(in.error(), StreamError::NoError);
            QCOMPARE(data.toStdString(), strings[iS]);
            ++iS;
        } else {
            quint32 value = 0;
            QVERIFY(in.read(&value));
            QCOMPARE(in.error(), StreamError::NoError);
            QCOMPARE(value, integers[iI]);
            ++iI;
        }
    }
}

void tst_Hpack::bitstreamErrors()
{
    {
        BitIStream in;
        quint32 val = 0;
        QVERIFY(!in.read(&val));
        QCOMPARE(in.error(), StreamError::NotEnoughData);
    }
    {
        // Integer in a stream, that does not fit into quint32.
        const uchar bytes[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        BitIStream in(bytes, bytes + sizeof bytes);
        quint32 val = 0;
        QVERIFY(!in.read(&val));
        QCOMPARE(in.error(), StreamError::InvalidInteger);
    }
    {
        const uchar byte = 0x82; // 1 - Huffman compressed, 2 - the (fake) byte length.
        BitIStream in(&byte, &byte + 1);
        QByteArray val;
        QVERIFY(!in.read(&val));
        QCOMPARE(in.error(), StreamError::NotEnoughData);
    }
}

void tst_Hpack::lookupTableConstructor()
{
    {
        FieldLookupTable nonIndexed(4096, false);
        QVERIFY(nonIndexed.dynamicDataSize() == 0);
        QVERIFY(nonIndexed.numberOfDynamicEntries() == 0);
        QVERIFY(nonIndexed.numberOfStaticEntries() != 0);
        QVERIFY(nonIndexed.numberOfStaticEntries() == nonIndexed.numberOfEntries());
        // Now we add some fake field and verify what 'non-indexed' means ... no search
        // by name.
        QVERIFY(nonIndexed.prependField("custom-key", "custom-value"));
        // 54: 10 + 12 in name/value pair above + 32 required by HPACK specs ...
        QVERIFY(nonIndexed.dynamicDataSize() == 54);
        QVERIFY(nonIndexed.numberOfDynamicEntries() == 1);
        QCOMPARE(nonIndexed.numberOfEntries(), nonIndexed.numberOfStaticEntries() + 1);
        // Should fail to find it (invalid index 0) - search is disabled.
        QVERIFY(nonIndexed.indexOf("custom-key", "custom-value") == 0);
    }
    {
        // "key" + "value" == 8 bytes, + 32 (HPACK's requirement) == 40.
        // Let's ask for a max-size 32 so that entry does not fit:
        FieldLookupTable nonIndexed(32, false);
        QVERIFY(nonIndexed.prependField("key", "value"));
        QVERIFY(nonIndexed.numberOfEntries() == nonIndexed.numberOfStaticEntries());
        QVERIFY(nonIndexed.indexOf("key", "value") == 0);
    }
    {
        FieldLookupTable indexed(4096, true);
        QVERIFY(indexed.dynamicDataSize() == 0);
        QVERIFY(indexed.numberOfDynamicEntries() == 0);
        QVERIFY(indexed.numberOfStaticEntries() != 0);
        QVERIFY(indexed.numberOfStaticEntries() == indexed.numberOfEntries());
        QVERIFY(indexed.prependField("custom-key", "custom-value"));
        QVERIFY(indexed.dynamicDataSize() == 54);
        QVERIFY(indexed.numberOfDynamicEntries() == 1);
        QVERIFY(indexed.numberOfEntries() == indexed.numberOfStaticEntries() + 1);
        QVERIFY(indexed.indexOf("custom-key") == indexed.numberOfStaticEntries() + 1);
        QVERIFY(indexed.indexOf("custom-key", "custom-value") == indexed.numberOfStaticEntries() + 1);
    }
}

void tst_Hpack::lookupTableStatic()
{
    const FieldLookupTable table(0, false /*all static, no need in 'search index'*/);
    const auto &staticTable = FieldLookupTable::staticPart();
    QByteArray name, value;
    quint32 currentIndex = 1; // HPACK is indexing starting from 1.
    for (const HeaderField &field : staticTable) {
        const quint32 index = table.indexOf(field.name, field.value);
        QVERIFY(index != 0);
        QCOMPARE(index, currentIndex);
        QVERIFY(table.field(index, &name, &value));
        QCOMPARE(name, field.name);
        QCOMPARE(value, field.value);
        ++currentIndex;
    }
}

void tst_Hpack::lookupTableDynamic()
{
    // HPACK's table size:
    // for every field -> size += field.name.length() + field.value.length() + 32.
    // Let's set some size limit and try to fill table with enough entries to have several
    // items evicted.
    const quint32 tableSize = 8192;
    const char stringData[] = "abcdefghijklmnopABCDEFGHIJKLMNOP0123456789()[]:";
    const quint32 dataSize = sizeof stringData - 1;

    FieldLookupTable table(tableSize, true);

    std::vector<QByteArray> fieldsToFind;
    quint32 evicted = 0;

    while (true) {
        // Strings are repeating way too often, I want to
        // have at least some items really evicted and not found,
        // therefore these weird dances with start/len.
        const quint32 start = QRandomGenerator::global()->bounded(dataSize - 10);
        quint32 len = QRandomGenerator::global()->bounded(dataSize - start);
        if (!len)
            len = 1;

        const QByteArray val(stringData + start, len);
        fieldsToFind.push_back(val);
        const quint32 entriesBefore = table.numberOfDynamicEntries();
        QVERIFY(table.prependField(val, val));
        QVERIFY(table.indexOf(val));
        QVERIFY(table.indexOf(val) == table.indexOf(val, val));
        QByteArray fieldName, fieldValue;
        table.field(table.indexOf(val), &fieldName, &fieldValue);

        QVERIFY(val == fieldName);
        QVERIFY(val == fieldValue);

        if (table.numberOfDynamicEntries() <= entriesBefore) {
            // We had to evict several items ...
            evicted += entriesBefore - table.numberOfDynamicEntries() + 1;
            if (evicted >= 200)
                break;
        }
    }

    QVERIFY(table.dynamicDataSize() <= tableSize);
    QVERIFY(table.numberOfDynamicEntries() > 0);
    QVERIFY(table.indexOf(fieldsToFind.back())); // We MUST have it in a table!

    using size_type = std::vector<QByteArray>::size_type;
    for (size_type i = 0, e = fieldsToFind.size(); i < e; ++i) {
        const auto &val = fieldsToFind[i];
        const quint32 index = table.indexOf(val);
        if (!index) {
            QVERIFY(i < size_type(evicted));
        } else {
            QVERIFY(index == table.indexOf(val, val));
            QByteArray fieldName, fieldValue;
            QVERIFY(table.field(index, &fieldName, &fieldValue));
            QVERIFY(val == fieldName);
            QVERIFY(val == fieldValue);
        }
    }

    table.clearDynamicTable();

    QVERIFY(table.numberOfDynamicEntries() == 0);
    QVERIFY(table.dynamicDataSize() == 0);
    QVERIFY(table.indexOf(fieldsToFind.back()) == 0);

    QVERIFY(table.prependField("name1", "value1"));
    QVERIFY(table.prependField("name2", "value2"));

    QVERIFY(table.indexOf("name1") == table.numberOfStaticEntries() + 2);
    QVERIFY(table.indexOf("name2", "value2") == table.numberOfStaticEntries() + 1);
    QVERIFY(table.indexOf("name1", "value2") == 0);
    QVERIFY(table.indexOf("name2", "value1") == 0);
    QVERIFY(table.indexOf("name3") == 0);

    QVERIFY(!table.indexIsValid(table.numberOfEntries() + 1));

    QVERIFY(table.prependField("name1", "value1"));
    QVERIFY(table.numberOfDynamicEntries() == 3);
    table.evictEntry();
    QVERIFY(table.indexOf("name1") != 0);
    table.evictEntry();
    QVERIFY(table.indexOf("name2") == 0);
    QVERIFY(table.indexOf("name1") != 0);
    table.evictEntry();
    QVERIFY(table.dynamicDataSize() == 0);
    QVERIFY(table.numberOfDynamicEntries() == 0);
    QVERIFY(table.indexOf("name1") == 0);
}

void  tst_Hpack::hpackEncodeRequest_data()
{
    QTest::addColumn<bool>("compression");
    QTest::newRow("no-string-compression") << false;
    QTest::newRow("with-string-compression") << true;
}

void tst_Hpack::hpackEncodeRequest(bool withHuffman)
{
    // This function uses examples from HPACK specs
    // (see appendix).

    Encoder encoder(4096, withHuffman);
    // HPACK, C.3.1 First Request
    /*
    :method: GET
    :scheme: http
    :path: /
    :authority: www.example.com

    Hex dump of encoded data (without Huffman):

    8286 8441 0f77 7777 2e65 7861 6d70 6c65 | ...A.www.example
    2e63 6f6d

    Hex dump of encoded data (with Huffman):

    8286 8441 8cf1 e3c2 e5f2 3a6b a0ab 90f4 ff
    */
    request1.clear();
    header1 = {{":method", "GET"},
               {":scheme", "http"},
               {":path", "/"},
               {":authority", "www.example.com"}};
    QVERIFY(encoder.encodeRequest(request1, header1));
    QVERIFY(encoder.dynamicTableSize() == 57);

    // HPACK, C.3.2 Second Request
    /*
    Header list to encode:

    :method: GET
    :scheme: http
    :path: /
    :authority: www.example.com
    cache-control: no-cache

    Hex dump of encoded data (without Huffman):

    8286 84be 5808 6e6f 2d63 6163 6865

    Hex dump of encoded data (with Huffman):

    8286 84be 5886 a8eb 1064 9cbf
    */

    request2.clear();
    header2 = {{":method", "GET"},
               {":scheme", "http"},
               {":path", "/"},
               {":authority", "www.example.com"},
               {"cache-control", "no-cache"}};
    encoder.encodeRequest(request2, header2);
    QVERIFY(encoder.dynamicTableSize() == 110);

    // HPACK, C.3.3 Third Request
    /*
    Header list to encode:

    :method: GET
    :scheme: https
    :path: /index.html
    :authority: www.example.com
    custom-key: custom-value

    Hex dump of encoded data (without Huffman):

    8287 85bf 400a 6375 7374 6f6d 2d6b 6579
    0c63 7573 746f 6d2d 7661 6c75 65

    Hex dump of encoded data (with Huffman):

    8287 85bf 4088 25a8 49e9 5ba9 7d7f 8925
    a849 e95b b8e8 b4bf
    */
    request3.clear();
    header3 = {{":method", "GET"},
               {":scheme", "https"},
               {":path", "/index.html"},
               {":authority", "www.example.com"},
               {"custom-key", "custom-value"}};
    encoder.encodeRequest(request3, header3);
    QVERIFY(encoder.dynamicTableSize() == 164);
}

void tst_Hpack::hpackEncodeRequest()
{
    QFETCH(bool, compression);

    hpackEncodeRequest(compression);

    // See comments above about these hex dumps ...
    const uchar bytes1NH[] = {0x82, 0x86, 0x84, 0x41,
                              0x0f, 0x77, 0x77, 0x77,
                              0x2e, 0x65, 0x78, 0x61,
                              0x6d, 0x70, 0x6c, 0x65,
                              0x2e, 0x63, 0x6f, 0x6d};

    const uchar bytes1WH[] = {0x82, 0x86, 0x84, 0x41,
                              0x8c, 0xf1, 0xe3, 0xc2,
                              0xe5, 0xf2, 0x3a, 0x6b,
                              0xa0, 0xab, 0x90, 0xf4,
                              0xff};

    const uchar *hexDump1 = compression ? bytes1WH : bytes1NH;
    const quint64 byteLength1 = compression ? sizeof bytes1WH : sizeof bytes1NH;

    QCOMPARE(request1.byteLength(), byteLength1);
    QCOMPARE(request1.bitLength(), byteLength1 * 8);

    for (quint32 i = 0, e = request1.byteLength(); i < e; ++i)
        QCOMPARE(hexDump1[i], request1.begin()[i]);

    const uchar bytes2NH[] = {0x82, 0x86, 0x84, 0xbe,
                              0x58, 0x08, 0x6e, 0x6f,
                              0x2d, 0x63, 0x61, 0x63,
                              0x68, 0x65};

    const uchar bytes2WH[] = {0x82, 0x86, 0x84, 0xbe,
                              0x58, 0x86, 0xa8, 0xeb,
                              0x10, 0x64, 0x9c, 0xbf};

    const uchar *hexDump2 = compression ? bytes2WH : bytes2NH;
    const auto byteLength2 = compression ? sizeof bytes2WH : sizeof bytes2NH;
    QVERIFY(request2.byteLength() == byteLength2);
    QVERIFY(request2.bitLength() == byteLength2 * 8);
    for (quint32 i = 0, e = request2.byteLength(); i < e; ++i)
        QCOMPARE(hexDump2[i], request2.begin()[i]);

    const uchar bytes3NH[] = {0x82, 0x87, 0x85, 0xbf,
                              0x40, 0x0a, 0x63, 0x75,
                              0x73, 0x74, 0x6f, 0x6d,
                              0x2d, 0x6b, 0x65, 0x79,
                              0x0c, 0x63, 0x75, 0x73,
                              0x74, 0x6f, 0x6d, 0x2d,
                              0x76, 0x61, 0x6c, 0x75,
                              0x65};
    const uchar bytes3WH[] = {0x82, 0x87, 0x85, 0xbf,
                              0x40, 0x88, 0x25, 0xa8,
                              0x49, 0xe9, 0x5b, 0xa9,
                              0x7d, 0x7f, 0x89, 0x25,
                              0xa8, 0x49, 0xe9, 0x5b,
                              0xb8, 0xe8, 0xb4, 0xbf};

    const uchar *hexDump3 = compression ? bytes3WH : bytes3NH;
    const quint64 byteLength3 = compression ? sizeof bytes3WH : sizeof bytes3NH;
    QCOMPARE(request3.byteLength(), byteLength3);
    QCOMPARE(request3.bitLength(), byteLength3 * 8);
    for (quint32 i = 0, e = request3.byteLength(); i < e; ++i)
        QCOMPARE(hexDump3[i], request3.begin()[i]);
}

void tst_Hpack::hpackDecodeRequest_data()
{
    QTest::addColumn<bool>("compression");
    QTest::newRow("no-string-compression") << false;
    QTest::newRow("with-string-compression") << true;
}

void tst_Hpack::hpackDecodeRequest()
{
    QFETCH(bool, compression);
    hpackEncodeRequest(compression);

    QVERIFY(request1.byteLength());
    QVERIFY(request2.byteLength());
    QVERIFY(request3.byteLength());

    Decoder decoder(4096);
    BitIStream inputStream1(request1.begin(), request1.end());
    QVERIFY(decoder.decodeHeaderFields(inputStream1));
    QCOMPARE(decoder.dynamicTableSize(), quint32(57));
    {
        const auto &decoded = decoder.decodedHeader();
        QVERIFY(decoded == header1);
    }

    BitIStream inputStream2{request2.begin(), request2.end()};
    QVERIFY(decoder.decodeHeaderFields(inputStream2));
    QCOMPARE(decoder.dynamicTableSize(), quint32(110));
    {
        const auto &decoded = decoder.decodedHeader();
        QVERIFY(decoded == header2);
    }

    BitIStream inputStream3(request3.begin(), request3.end());
    QVERIFY(decoder.decodeHeaderFields(inputStream3));
    QCOMPARE(decoder.dynamicTableSize(), quint32(164));
    {
        const auto &decoded = decoder.decodedHeader();
        QVERIFY(decoded == header3);
    }
}

void tst_Hpack::hpackEncodeResponse_data()
{
    hpackEncodeRequest_data();
}

void tst_Hpack::hpackEncodeResponse()
{
    QFETCH(bool, compression);

    hpackEncodeResponse(compression);

    // TODO: we can also test bytes - using hex dumps from HPACK's specs,
    // for now only test a table behavior/expected sizes.
}

void tst_Hpack::hpackEncodeResponse(bool withCompression)
{
    Encoder encoder(256, withCompression); // 256 - this will result in entries evicted.

    // HPACK, C.5.1 First Response
    /*
    Header list to encode:

    :status: 302
    cache-control: private
    date: Mon, 21 Oct 2013 20:13:21 GMT
    location: https://www.example.com
    */
    request1.clear();
    header1 = {{":status", "302"},
               {"cache-control", "private"},
               {"date", "Mon, 21 Oct 2013 20:13:21 GMT"},
               {"location", "https://www.example.com"}};

    QVERIFY(encoder.encodeResponse(request1, header1));
    QCOMPARE(encoder.dynamicTableSize(), quint32(222));

    // HPACK, C.5.2 Second Response
    /*


    The (":status", "302") header field is evicted from the dynamic
    table to free space to allow adding the (":status", "307") header field.

    Header list to encode:

    :status: 307
    cache-control: private
    date: Mon, 21 Oct 2013 20:13:21 GMT
    location: https://www.example.com
    */
    request2.clear();
    header2 = {{":status", "307"},
               {"cache-control", "private"},
               {"date", "Mon, 21 Oct 2013 20:13:21 GMT"},
               {"location", "https://www.example.com"}};
    QVERIFY(encoder.encodeResponse(request2, header2));
    QCOMPARE(encoder.dynamicTableSize(), quint32(222));

    // HPACK, C.5.3 Third Response
    /*
    Several header fields are evicted from the dynamic table
    during the processing of this header list.

    Header list to encode:

    :status: 200
    cache-control: private
    date: Mon, 21 Oct 2013 20:13:22 GMT
    location: https://www.example.com
    content-encoding: gzip
    set-cookie: foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1
    */
    request3.clear();
    header3 = {{":status", "200"},
               {"cache-control", "private"},
               {"date", "Mon, 21 Oct 2013 20:13:22 GMT"},
               {"location", "https://www.example.com"},
               {"content-encoding", "gzip"},
               {"set-cookie", "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1"}};
    QVERIFY(encoder.encodeResponse(request3, header3));
    QCOMPARE(encoder.dynamicTableSize(), quint32(215));
}

void tst_Hpack::hpackDecodeResponse_data()
{
    hpackEncodeRequest_data();
}

void tst_Hpack::hpackDecodeResponse()
{
    QFETCH(bool, compression);

    hpackEncodeResponse(compression);

    QVERIFY(request1.byteLength());
    Decoder decoder(256); // This size will result in entries evicted.
    BitIStream inputStream1(request1.begin(), request1.end());
    QVERIFY(decoder.decodeHeaderFields(inputStream1));
    QCOMPARE(decoder.dynamicTableSize(), quint32(222));

    {
        const auto &decoded = decoder.decodedHeader();
        QVERIFY(decoded == header1);
    }

    QVERIFY(request2.byteLength());
    BitIStream inputStream2(request2.begin(), request2.end());
    QVERIFY(decoder.decodeHeaderFields(inputStream2));
    QCOMPARE(decoder.dynamicTableSize(), quint32(222));

    {
        const auto &decoded = decoder.decodedHeader();
        QVERIFY(decoded == header2);
    }

    QVERIFY(request3.byteLength());
    BitIStream inputStream3(request3.begin(), request3.end());
    QVERIFY(decoder.decodeHeaderFields(inputStream3));
    QCOMPARE(decoder.dynamicTableSize(), quint32(215));

    {
        const auto &decoded = decoder.decodedHeader();
        QVERIFY(decoded == header3);
    }
}

QTEST_MAIN(tst_Hpack)

#include "tst_hpack.moc"
