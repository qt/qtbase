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


#include <QtTest/QtTest>
#include <QtCore/qendian.h>


class tst_QtEndian: public QObject
{
    Q_OBJECT

private slots:
    void fromBigEndian();
    void fromLittleEndian();

    void toBigEndian();
    void toLittleEndian();
};

struct TestData
{
    quint64 data64;
    quint32 data32;
    quint16 data16;
    quint8 data8;

    quint8 reserved;
};

union RawTestData
{
    uchar rawData[sizeof(TestData)];
    TestData data;
};

static const TestData inNativeEndian = { Q_UINT64_C(0x0123456789abcdef), 0x00c0ffee, 0xcafe, 0xcf, '\0' };
static const RawTestData inBigEndian = { "\x01\x23\x45\x67\x89\xab\xcd\xef" "\x00\xc0\xff\xee" "\xca\xfe" "\xcf" };
static const RawTestData inLittleEndian = { "\xef\xcd\xab\x89\x67\x45\x23\x01" "\xee\xff\xc0\x00" "\xfe\xca" "\xcf" };

#define EXPAND_ENDIAN_TEST(endian)          \
    do {                                    \
        /* Unsigned tests */                \
        ENDIAN_TEST(endian, quint, 64);     \
        ENDIAN_TEST(endian, quint, 32);     \
        ENDIAN_TEST(endian, quint, 16);     \
        ENDIAN_TEST(endian, quint, 8);      \
                                            \
        /* Signed tests */                  \
        ENDIAN_TEST(endian, qint, 64);      \
        ENDIAN_TEST(endian, qint, 32);      \
        ENDIAN_TEST(endian, qint, 16);      \
        ENDIAN_TEST(endian, qint, 8);       \
    } while (false)                         \
    /**/

#define ENDIAN_TEST(endian, type, size)                                                 \
    do {                                                                                \
        QCOMPARE(qFrom ## endian ## Endian(                                             \
                    (type ## size)(in ## endian ## Endian.data.data ## size)),          \
                (type ## size)(inNativeEndian.data ## size));                           \
        QCOMPARE(qFrom ## endian ## Endian<type ## size>(                               \
                    in ## endian ## Endian.rawData + offsetof(TestData, data ## size)), \
                (type ## size)(inNativeEndian.data ## size));                           \
    } while (false)                                                                     \
    /**/

void tst_QtEndian::fromBigEndian()
{
    EXPAND_ENDIAN_TEST(Big);
}

void tst_QtEndian::fromLittleEndian()
{
    EXPAND_ENDIAN_TEST(Little);
}

#undef ENDIAN_TEST


#define ENDIAN_TEST(endian, type, size)                                                 \
    do {                                                                                \
        QCOMPARE(qTo ## endian ## Endian(                                               \
                    (type ## size)(inNativeEndian.data ## size)),                       \
                (type ## size)(in ## endian ## Endian.data.data ## size));              \
                                                                                        \
        RawTestData test;                                                               \
        qTo ## endian ## Endian(                                                        \
                (type ## size)(inNativeEndian.data ## size),                            \
                test.rawData + offsetof(TestData, data ## size));                       \
        QCOMPARE(test.data.data ## size, in ## endian ## Endian.data.data ## size );    \
    } while (false)                                                                     \
    /**/

void tst_QtEndian::toBigEndian()
{
    EXPAND_ENDIAN_TEST(Big);
}

void tst_QtEndian::toLittleEndian()
{
    EXPAND_ENDIAN_TEST(Little);
}

#undef ENDIAN_TEST

QTEST_MAIN(tst_QtEndian)
#include "tst_qtendian.moc"
