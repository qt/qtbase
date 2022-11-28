// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <qbytearray.h>

#include <q20iterator.h>
#include <stdexcept>
#include <string_view>

class tst_QByteArrayLarge : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
#ifndef QT_NO_COMPRESS
    void qCompress_data();
    void qCompress();
    void qUncompressCorruptedData_data();
    void qUncompressCorruptedData();
    void qUncompress4GiBPlus();
    void qCompressionZeroTermination();
#endif
    void base64_2GiB();
};

void tst_QByteArrayLarge::initTestCase()
{
#if defined(QT_ASAN_ENABLED)
    QSKIP("Skipping QByteArray tests under ASAN as they are too slow");
#endif
}

#ifndef QT_NO_COMPRESS
void tst_QByteArrayLarge::qCompress_data()
{
    QTest::addColumn<QByteArray>("ba");

    const int size1 = 1024*1024;
    QByteArray ba1( size1, 0 );

    QTest::newRow( "00" ) << QByteArray();

    int i;
    for ( i=0; i<size1; i++ )
        ba1[i] = (char)( i / 1024 );
    QTest::newRow( "01" ) << ba1;

    for ( i=0; i<size1; i++ )
        ba1[i] = (char)( i % 256 );
    QTest::newRow( "02" ) << ba1;

    ba1.fill( 'A' );
    QTest::newRow( "03" ) << ba1;

    QFile file( QFINDTESTDATA("rfc3252.txt") );
    QVERIFY( file.open(QIODevice::ReadOnly) );
    QTest::newRow( "04" ) << file.readAll();
}

void tst_QByteArrayLarge::qCompress()
{
    QFETCH( QByteArray, ba );
    QByteArray compressed = ::qCompress( ba );
    QTEST( ::qUncompress( compressed ), "ba" );
}

void tst_QByteArrayLarge::qUncompressCorruptedData_data()
{
    QTest::addColumn<QByteArray>("in");

    QTest::newRow("0x00000000") << QByteArray("\x00\x00\x00\x00", 4);
    QTest::newRow("0x000000ff") << QByteArray("\x00\x00\x00\xff", 4);
    QTest::newRow("0x3f000000") << QByteArray("\x3f\x00\x00\x00", 4);
    QTest::newRow("0x3fffffff") << QByteArray("\x3f\xff\xff\xff", 4);
    QTest::newRow("0x7fffff00") << QByteArray("\x7f\xff\xff\x00", 4);
    QTest::newRow("0x7fffffff") << QByteArray("\x7f\xff\xff\xff", 4);
    QTest::newRow("0x80000000") << QByteArray("\x80\x00\x00\x00", 4);
    QTest::newRow("0x800000ff") << QByteArray("\x80\x00\x00\xff", 4);
    QTest::newRow("0xcf000000") << QByteArray("\xcf\x00\x00\x00", 4);
    QTest::newRow("0xcfffffff") << QByteArray("\xcf\xff\xff\xff", 4);
    QTest::newRow("0xffffff00") << QByteArray("\xff\xff\xff\x00", 4);
    QTest::newRow("0xffffffff") << QByteArray("\xff\xff\xff\xff", 4);
}

// This test is expected to produce some warning messages in the test output.
void tst_QByteArrayLarge::qUncompressCorruptedData()
{
    QFETCH(QByteArray, in);

    QByteArray res;
    res = ::qUncompress(in);
    QCOMPARE(res, QByteArray());

    res = ::qUncompress(in + "blah");
    QCOMPARE(res, QByteArray());
}

void tst_QByteArrayLarge::qUncompress4GiBPlus()
{
    // after three rounds, this decompresses to 4GiB + 1 'X' bytes:
    constexpr uchar compressed_3x[] = {
       0x00, 0x00, 0x1a, 0x76, 0x78, 0x9c, 0x63, 0xb0, 0xdf, 0xb4, 0xad, 0x62,
       0xce, 0xdb, 0x3b, 0x0b, 0xf3, 0x26, 0x27, 0x4a, 0xb4, 0x3d, 0x34, 0x5b,
       0xed, 0xb4, 0x41, 0xf1, 0xc0, 0x99, 0x2f, 0x02, 0x05, 0x67, 0x26, 0x88,
       0x6c, 0x66, 0x71, 0x34, 0x62, 0x9c, 0x75, 0x26, 0xb1, 0xa0, 0xe5, 0xcc,
       0xda, 0x94, 0x83, 0xc9, 0x05, 0x73, 0x0e, 0x3c, 0x39, 0xc2, 0xc7, 0xd0,
       0xae, 0x38, 0x53, 0x7b, 0x87, 0xdc, 0x01, 0x91, 0x45, 0x59, 0x4f, 0xda,
       0xbf, 0xca, 0xcc, 0x52, 0xdb, 0xbb, 0xde, 0xbb, 0xf6, 0xd3, 0x55, 0xff,
       0x7d, 0x77, 0x0e, 0x1b, 0xf0, 0xa4, 0xdf, 0xcf, 0xdb, 0x5f, 0x2f, 0xf5,
       0xd7, 0x7c, 0xfe, 0xbf, 0x3f, 0xbf, 0x3f, 0x9d, 0x7c, 0xda, 0x2c, 0xc8,
       0xc0, 0xc0, 0xb0, 0xe1, 0xf1, 0xb3, 0xfd, 0xfa, 0xdf, 0x8e, 0x7d, 0xef,
       0x7f, 0xb9, 0xc1, 0xc2, 0xae, 0x92, 0x19, 0x28, 0xf2, 0x66, 0xd7, 0xe5,
       0xbf, 0xed, 0x93, 0xbf, 0x6a, 0x14, 0x7c, 0xff, 0xf6, 0xe1, 0xe8, 0xb6,
       0x7e, 0x46, 0xa0, 0x90, 0xd9, 0xbb, 0xcf, 0x9f, 0x17, 0x37, 0x7f, 0xe5,
       0x6f, 0xb4, 0x7f, 0xfe, 0x5e, 0xfd, 0xb6, 0x1d, 0x1b, 0x50, 0xe8, 0xc6,
       0x8e, 0xe3, 0xab, 0x9f, 0xe6, 0xec, 0x65, 0xfd, 0x23, 0xb1, 0x4e, 0x7e,
       0xef, 0xbd, 0x6f, 0xa6, 0x40, 0xa1, 0x03, 0xc7, 0xfe, 0x0a, 0xf1, 0x00,
       0xe9, 0x06, 0x91, 0x83, 0x40, 0x92, 0x21, 0x43, 0x10, 0xcc, 0x11, 0x03,
       0x73, 0x3a, 0x90, 0x39, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32,
       0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32,
       0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32,
       0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32,
       0xa3, 0x32, 0xa3, 0x32, 0xa3, 0x32, 0x34, 0x90, 0x99, 0xb6, 0x7e, 0xf5,
       0xd3, 0xe9, 0xbf, 0x35, 0x13, 0xca, 0x8c, 0x75, 0xec, 0xec, 0xa4, 0x2f,
       0x7e, 0x2d, 0xf9, 0xf3, 0xf0, 0xee, 0xea, 0xd5, 0xf5, 0xd3, 0x14, 0x57,
       0x06, 0x00, 0x00, 0xb9, 0x1e, 0x35, 0xce
    };

    constexpr qint64 GiB = 1024LL * 1024 * 1024;

    if constexpr (sizeof(qsizetype) == sizeof(int)) {
        QSKIP("This is a 64-bit-only test.");
    } else {

        // 1st
        auto c = ::qUncompress(std::data(compressed_3x), q20::ssize(compressed_3x));
        QVERIFY(!c.isNull()); // check for decompression error

        // 2nd
        c = ::qUncompress(c);
        QVERIFY(!c.isNull());

        // 3rd
        try {
            c = ::qUncompress(c);
            if (c.isNull())  // this step (~18MiB -> 4GiB) might have run out of memory
                QSKIP("Failed to allocate enough memory.");
        } catch (const std::bad_alloc &) {
            QSKIP("Failed to allocate enough memory.");
        }

        QCOMPARE(c.size(), 4 * GiB + 1);
        QCOMPARE(std::string_view{c}.find_first_not_of('X'),
                 std::string_view::npos);

        // re-compress once
        // (produces 18MiB, we shouldn't use much more than that in allocated capacity)
        c = ::qCompress(c);
        QVERIFY(!c.isNull());

        // and un-compress again, to make sure compression worked (we
        // can't compare with compressed_3x, because zlib may change):
        c = ::qUncompress(c);

        QCOMPARE(c.size(), 4 * GiB + 1);
        QCOMPARE(std::string_view{c}.find_first_not_of('X'),
                 std::string_view::npos);
    }
}

void tst_QByteArrayLarge::qCompressionZeroTermination()
{
    QByteArray s = "Hello, I'm a string.";
    QByteArray ba = ::qUncompress(::qCompress(s));
    QCOMPARE(ba.data()[ba.size()], '\0');
    QCOMPARE(ba, s);
}
#endif

void tst_QByteArrayLarge::base64_2GiB()
{
#ifdef Q_OS_ANDROID
    QSKIP("Android kills the test when using too much memory");
#endif
    if constexpr (sizeof(qsizetype) > sizeof(int)) {
        try {
            constexpr qint64 GiB = 1024 * 1024 * 1024;
            static_assert((2 * GiB + 1) % 3 == 0);
            const char inputChar = '\0';    // all-NULs encode as
            const char outputChar = 'A';    // all-'A's
            const qint64 inputSize = 2 * GiB + 1;
            const qint64 outputSize = inputSize / 3 * 4;
            const auto sv = [](const QByteArray &ba) {
                    return std::string_view{ba.data(), size_t(ba.size())};
                };
            QByteArray output;
            {
                const QByteArray input(inputSize, inputChar);
                output = input.toBase64();
                QCOMPARE(output.size(), outputSize);
                QCOMPARE(sv(output).find_first_not_of(outputChar),
                         std::string_view::npos);
            }
            {
                auto r = QByteArray::fromBase64Encoding(output);
                QCOMPARE_EQ(r.decodingStatus, QByteArray::Base64DecodingStatus::Ok);
                QCOMPARE(r.decoded.size(), inputSize);
                QCOMPARE(sv(r.decoded).find_first_not_of(inputChar),
                         std::string_view::npos);
            }
        } catch (const std::bad_alloc &) {
            QSKIP("Could not allocate enough RAM.");
        }
    } else {
        QSKIP("This is a 64-bit only test");
    }
}

QTEST_MAIN(tst_QByteArrayLarge)
#include "tst_qbytearray_large.moc"
