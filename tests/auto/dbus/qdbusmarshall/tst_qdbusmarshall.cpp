/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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
#include <QtCore/QtCore>
#include <QtTest/QtTest>
#include <QtDBus/QtDBus>

#include "common.h"
#include <limits>

#include <QtDBus/private/qdbusutil_p.h>
#include <QtDBus/private/qdbusconnection_p.h>
#include <QtDBus/private/qdbus_symbols_p.h>

#ifndef DBUS_TYPE_UNIX_FD
#  define DBUS_TYPE_UNIX_FD int('h')
#  define DBUS_TYPE_UNIX_FD_AS_STRING "h"
#endif

static const char serviceName[] = "org.qtproject.autotests.qpong";
static const char objectPath[] = "/org/qtproject/qpong";
static const char *interfaceName = serviceName;

class tst_QDBusMarshall: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void sendBasic_data();
    void sendBasic();

    void sendVariant_data();
    void sendVariant();

    void sendArrays_data();
    void sendArrays();

    void sendArrayOfArrays_data();
    void sendArrayOfArrays();

    void sendMaps_data();
    void sendMaps();

    void sendStructs_data();
    void sendStructs();

    void sendComplex_data();
    void sendComplex();

    void sendArgument_data();
    void sendArgument();

    void sendSignalErrors();
    void sendCallErrors_data();
    void sendCallErrors();

    void receiveUnknownType_data();
    void receiveUnknownType();

    void demarshallPrimitives_data();
    void demarshallPrimitives();

    void demarshallStrings_data();
    void demarshallStrings();

    void demarshallInvalidStringList_data();
    void demarshallInvalidStringList();

    void demarshallInvalidByteArray_data();
    void demarshallInvalidByteArray();

private:
    int fileDescriptorForTest();

    QProcess proc;
    QTemporaryFile tempFile;
    bool fileDescriptorPassing;
};

class QDBusMessageSpy: public QObject
{
    Q_OBJECT
public slots:
    Q_SCRIPTABLE int theSlot(const QDBusMessage &msg)
    {
        list << msg;
        return 42;
    }
public:
    QList<QDBusMessage> list;
};

struct UnregisteredType { };
Q_DECLARE_METATYPE(UnregisteredType)

void tst_QDBusMarshall::initTestCase()
{
    commonInit();
    QDBusConnection con = QDBusConnection::sessionBus();
    fileDescriptorPassing = con.connectionCapabilities() & QDBusConnection::UnixFileDescriptorPassing;

#ifdef Q_OS_WIN
#  define EXE ".exe"
#else
#  define EXE ""
#endif
    proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    proc.start(QFINDTESTDATA("qpong/qpong" EXE));
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QVERIFY(proc.waitForReadyRead());
}

void tst_QDBusMarshall::cleanupTestCase()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath, interfaceName, "quit");
    QDBusConnection::sessionBus().call(msg);
    proc.waitForFinished(200);
    proc.close();
}

int tst_QDBusMarshall::fileDescriptorForTest()
{
    if (!tempFile.isOpen()) {
        tempFile.setFileTemplate(QDir::tempPath() + "/qdbusmarshalltestXXXXXX.tmp");
        if (!tempFile.open()) {
            qWarning("%s: Cannot create temporary file: %s", Q_FUNC_INFO,
                     qPrintable(tempFile.errorString()));
            return 0;
        }
    }
    return tempFile.handle();
}

void addBasicTypesColumns()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("sig");
    QTest::addColumn<QString>("stringResult");
}

void basicNumericTypes_data()
{
    QTest::newRow("bool") << QVariant(false) << "b" << "false";
    QTest::newRow("bool2") << QVariant(true) << "b" << "true";
    QTest::newRow("byte") << QVariant::fromValue(uchar(1)) << "y" << "1";
    QTest::newRow("int16") << QVariant::fromValue(short(2)) << "n" << "2";
    QTest::newRow("uint16") << QVariant::fromValue(ushort(3)) << "q" << "3";
    QTest::newRow("int") << QVariant(1) << "i" << "1";
    QTest::newRow("uint") << QVariant(2U) << "u" << "2";
    QTest::newRow("int64") << QVariant(Q_INT64_C(3)) << "x" << "3";
    QTest::newRow("uint64") << QVariant(Q_UINT64_C(4)) << "t" << "4";
    QTest::newRow("double") << QVariant(42.5) << "d" << "42.5";
}

void basicStringTypes_data()
{
    QTest::newRow("string") << QVariant("ping") << "s" << "\"ping\"";
    QTest::newRow("objectpath") << QVariant::fromValue(QDBusObjectPath("/org/kde")) << "o" << "[ObjectPath: /org/kde]";
    QTest::newRow("signature") << QVariant::fromValue(QDBusSignature("g")) << "g" << "[Signature: g]";
    QTest::newRow("emptystring") << QVariant("") << "s" << "\"\"";
    QTest::newRow("nullstring") << QVariant(QString()) << "s" << "\"\"";
}

void tst_QDBusMarshall::sendBasic_data()
{
    addBasicTypesColumns();

    // basic types:
    basicNumericTypes_data();
    basicStringTypes_data();

    if (fileDescriptorPassing)
        QTest::newRow("file-descriptor") << QVariant::fromValue(QDBusUnixFileDescriptor(fileDescriptorForTest())) << "h" << "[Unix FD: valid]";
}

void tst_QDBusMarshall::sendVariant_data()
{
    sendBasic_data();

    QTest::newRow("variant") << QVariant::fromValue(QDBusVariant(1)) << "v" << "[Variant(int): 1]";

    QDBusVariant nested(1);
    QTest::newRow("variant-variant") << QVariant::fromValue(QDBusVariant(QVariant::fromValue(nested))) << "v"
                                     << "[Variant(QDBusVariant): [Variant(int): 1]]";
}

void tst_QDBusMarshall::sendArrays_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("sig");
    QTest::addColumn<QString>("stringResult");

    // arrays
    QStringList strings;
    QTest::newRow("emptystringlist") << QVariant(strings) << "as" << "{}";
    strings << "hello" << "world";
    QTest::newRow("stringlist") << QVariant(strings) << "as" << "{\"hello\", \"world\"}";

    strings.clear();
    strings << "" << "" << "";
    QTest::newRow("list-of-emptystrings") << QVariant(strings) << "as" << "{\"\", \"\", \"\"}";

    strings.clear();
    strings << QString() << QString() << QString() << QString();
    QTest::newRow("list-of-nullstrings") << QVariant(strings) << "as" << "{\"\", \"\", \"\", \"\"}";

    QByteArray bytearray;
    QTest::newRow("nullbytearray") << QVariant(bytearray) << "ay" << "{}";
    bytearray = "";             // empty, not null
    QTest::newRow("emptybytearray") << QVariant(bytearray) << "ay" << "{}";
    bytearray = "foo";
    QTest::newRow("bytearray") << QVariant(bytearray) << "ay" << "{102, 111, 111}";

    QList<bool> bools;
    QTest::newRow("emptyboollist") << QVariant::fromValue(bools) << "ab" << "[Argument: ab {}]";
    bools << false << true << false;
    QTest::newRow("boollist") << QVariant::fromValue(bools) << "ab" << "[Argument: ab {false, true, false}]";

    QList<short> shorts;
    QTest::newRow("emptyshortlist") << QVariant::fromValue(shorts) << "an" << "[Argument: an {}]";
    shorts << 42 << -43 << 44 << 45 << -32768 << 32767;
    QTest::newRow("shortlist") << QVariant::fromValue(shorts) << "an"
                               << "[Argument: an {42, -43, 44, 45, -32768, 32767}]";

    QList<ushort> ushorts;
    QTest::newRow("emptyushortlist") << QVariant::fromValue(ushorts) << "aq" << "[Argument: aq {}]";
    ushorts << 12u << 13u << 14u << 15 << 65535;
    QTest::newRow("ushortlist") << QVariant::fromValue(ushorts) << "aq" << "[Argument: aq {12, 13, 14, 15, 65535}]";

    QList<int> ints;
    QTest::newRow("emptyintlist") << QVariant::fromValue(ints) << "ai" << "[Argument: ai {}]";
    ints << 42 << -43 << 44 << 45 << 2147483647 << -2147483647-1;
    QTest::newRow("intlist") << QVariant::fromValue(ints) << "ai" << "[Argument: ai {42, -43, 44, 45, 2147483647, -2147483648}]";

    QList<uint> uints;
    QTest::newRow("emptyuintlist") << QVariant::fromValue(uints) << "au" << "[Argument: au {}]";
    uints << uint(12) << uint(13) << uint(14) << 4294967295U;
    QTest::newRow("uintlist") << QVariant::fromValue(uints) << "au" << "[Argument: au {12, 13, 14, 4294967295}]";

    QList<qlonglong> llints;
    QTest::newRow("emptyllintlist") << QVariant::fromValue(llints) << "ax" << "[Argument: ax {}]";
    llints << Q_INT64_C(99) << Q_INT64_C(-100)
           << Q_INT64_C(-9223372036854775807)-1 << Q_INT64_C(9223372036854775807);
    QTest::newRow("llintlist") << QVariant::fromValue(llints) << "ax"
                               << "[Argument: ax {99, -100, -9223372036854775808, 9223372036854775807}]";

    QList<qulonglong> ullints;
    QTest::newRow("emptyullintlist") << QVariant::fromValue(ullints) << "at" << "[Argument: at {}]";
    ullints << Q_UINT64_C(66) << Q_UINT64_C(67)
            << Q_UINT64_C(18446744073709551615);
    QTest::newRow("ullintlist") << QVariant::fromValue(ullints) << "at" << "[Argument: at {66, 67, 18446744073709551615}]";

    QList<double> doubles;
    QTest::newRow("emptydoublelist") << QVariant::fromValue(doubles) << "ad" << "[Argument: ad {}]";
    doubles << 1.2 << 2.2 << 4.4
            << -std::numeric_limits<double>::infinity()
            << std::numeric_limits<double>::infinity()
            << std::numeric_limits<double>::quiet_NaN();
    QTest::newRow("doublelist") << QVariant::fromValue(doubles) << "ad" << "[Argument: ad {1.2, 2.2, 4.4, -inf, inf, nan}]";

    QList<QDBusObjectPath> objectPaths;
    QTest::newRow("emptyobjectpathlist") << QVariant::fromValue(objectPaths) << "ao" << "[Argument: ao {}]";
    objectPaths << QDBusObjectPath("/") << QDBusObjectPath("/foo");
    QTest::newRow("objectpathlist") << QVariant::fromValue(objectPaths) << "ao" << "[Argument: ao {[ObjectPath: /], [ObjectPath: /foo]}]";

    if (fileDescriptorPassing) {
        QList<QDBusUnixFileDescriptor> fileDescriptors;
        QTest::newRow("emptyfiledescriptorlist") << QVariant::fromValue(fileDescriptors) << "ah" << "[Argument: ah {}]";
        fileDescriptors << QDBusUnixFileDescriptor(fileDescriptorForTest()) << QDBusUnixFileDescriptor(1);
        QTest::newRow("filedescriptorlist") << QVariant::fromValue(fileDescriptors) << "ah" << "[Argument: ah {[Unix FD: valid], [Unix FD: valid]}]";
    }

    QVariantList variants;
    QTest::newRow("emptyvariantlist") << QVariant(variants) << "av" << "[Argument: av {}]";
    variants << QString("Hello") << QByteArray("World") << 42 << -43.0 << 44U << Q_INT64_C(-45)
             << Q_UINT64_C(46) << true << QVariant::fromValue(short(-47))
             << QVariant::fromValue(QDBusSignature("av"))
             << QVariant::fromValue(QDBusVariant(QVariant::fromValue(QDBusObjectPath("/"))));
    QTest::newRow("variantlist") << QVariant(variants) << "av"
        << "[Argument: av {[Variant(QString): \"Hello\"], [Variant(QByteArray): {87, 111, 114, 108, 100}], [Variant(int): 42], [Variant(double): -43], [Variant(uint): 44], [Variant(qlonglong): -45], [Variant(qulonglong): 46], [Variant(bool): true], [Variant(short): -47], [Variant: [Signature: av]], [Variant: [Variant: [ObjectPath: /]]]}]";
}

void tst_QDBusMarshall::sendArrayOfArrays_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("sig");
    QTest::addColumn<QString>("stringResult");

    // arrays:
    QList<QStringList> strings;
    QTest::newRow("empty-list-of-stringlist") << QVariant::fromValue(strings) << "aas"
            << "[Argument: aas {}]";
    strings << QStringList();
    QTest::newRow("list-of-emptystringlist") << QVariant::fromValue(strings) << "aas"
            << "[Argument: aas {{}}]";
    strings << (QStringList() << "hello" << "world")
            << (QStringList() << "hi" << "there")
            << (QStringList() << QString());
    QTest::newRow("stringlist") << QVariant::fromValue(strings) << "aas"
            << "[Argument: aas {{}, {\"hello\", \"world\"}, {\"hi\", \"there\"}, {\"\"}}]";

    QList<QByteArray> bytearray;
    QTest::newRow("empty-list-of-bytearray") << QVariant::fromValue(bytearray) << "aay"
            << "[Argument: aay {}]";
    bytearray << QByteArray();
    QTest::newRow("list-of-emptybytearray") << QVariant::fromValue(bytearray) << "aay"
            << "[Argument: aay {{}}]";
    bytearray << "foo" << "bar" << "baz" << "" << QByteArray();
    QTest::newRow("bytearray") << QVariant::fromValue(bytearray) << "aay"
            << "[Argument: aay {{}, {102, 111, 111}, {98, 97, 114}, {98, 97, 122}, {}, {}}]";

    QList<QList<bool> > bools;
    QTest::newRow("empty-list-of-boollist") << QVariant::fromValue(bools) << "aab"
            << "[Argument: aab {}]";
    bools << QList<bool>();
    QTest::newRow("list-of-emptyboollist") << QVariant::fromValue(bools) << "aab"
            << "[Argument: aab {[Argument: ab {}]}]";
    bools << (QList<bool>() << false << true) << (QList<bool>() << false) << (QList<bool>());
    QTest::newRow("boollist") << QVariant::fromValue(bools) << "aab"
            << "[Argument: aab {[Argument: ab {}], [Argument: ab {false, true}], [Argument: ab {false}], [Argument: ab {}]}]";
    QList<QList<short> > shorts;
    QTest::newRow("empty-list-of-shortlist") << QVariant::fromValue(shorts) << "aan"
            << "[Argument: aan {}]";
    shorts << QList<short>();
    QTest::newRow("list-of-emptyshortlist") << QVariant::fromValue(shorts) << "aan"
            << "[Argument: aan {[Argument: an {}]}]";
    shorts << (QList<short>() << 42 << -43 << 44 << 45)
           << (QList<short>() << -32768 << 32767)
           << (QList<short>());
    QTest::newRow("shortlist") << QVariant::fromValue(shorts) << "aan"
            << "[Argument: aan {[Argument: an {}], [Argument: an {42, -43, 44, 45}], [Argument: an {-32768, 32767}], [Argument: an {}]}]";

    QList<QList<ushort> > ushorts;
    QTest::newRow("empty-list-of-ushortlist") << QVariant::fromValue(ushorts) << "aaq"
            << "[Argument: aaq {}]";
    ushorts << QList<ushort>();
    QTest::newRow("list-of-emptyushortlist") << QVariant::fromValue(ushorts) << "aaq"
            << "[Argument: aaq {[Argument: aq {}]}]";
    ushorts << (QList<ushort>() << 12u << 13u << 14u << 15)
            << (QList<ushort>() << 65535)
            << (QList<ushort>());
    QTest::newRow("ushortlist") << QVariant::fromValue(ushorts) << "aaq"
            << "[Argument: aaq {[Argument: aq {}], [Argument: aq {12, 13, 14, 15}], [Argument: aq {65535}], [Argument: aq {}]}]";

    QList<QList<int> > ints;
    QTest::newRow("empty-list-of-intlist") << QVariant::fromValue(ints) << "aai"
            << "[Argument: aai {}]";
    ints << QList<int>();
    QTest::newRow("list-of-emptyintlist") << QVariant::fromValue(ints) << "aai"
            << "[Argument: aai {[Argument: ai {}]}]";
    ints << (QList<int>() << 42 << -43 << 44 << 45)
         << (QList<int>() << 2147483647 << -2147483647-1)
         << (QList<int>());
    QTest::newRow("intlist") << QVariant::fromValue(ints) << "aai"
            << "[Argument: aai {[Argument: ai {}], [Argument: ai {42, -43, 44, 45}], [Argument: ai {2147483647, -2147483648}], [Argument: ai {}]}]";

    QList<QList<uint> > uints;
    QTest::newRow("empty-list-of-uintlist") << QVariant::fromValue(uints) << "aau"
            << "[Argument: aau {}]";
    uints << QList<uint>();
    QTest::newRow("list-of-emptyuintlist") << QVariant::fromValue(uints) << "aau"
            << "[Argument: aau {[Argument: au {}]}]";
    uints << (QList<uint>() << uint(12) << uint(13) << uint(14))
          << (QList<uint>() << 4294967295U)
          << (QList<uint>());
    QTest::newRow("uintlist") << QVariant::fromValue(uints) << "aau"
            << "[Argument: aau {[Argument: au {}], [Argument: au {12, 13, 14}], [Argument: au {4294967295}], [Argument: au {}]}]";

    QList<QList<qlonglong> > llints;
    QTest::newRow("empty-list-of-llintlist") << QVariant::fromValue(llints) << "aax"
            << "[Argument: aax {}]";
    llints << QList<qlonglong>();
    QTest::newRow("list-of-emptyllintlist") << QVariant::fromValue(llints) << "aax"
            << "[Argument: aax {[Argument: ax {}]}]";
    llints << (QList<qlonglong>() << Q_INT64_C(99) << Q_INT64_C(-100))
           << (QList<qlonglong>() << Q_INT64_C(-9223372036854775807)-1 << Q_INT64_C(9223372036854775807))
           << (QList<qlonglong>());
    QTest::newRow("llintlist") << QVariant::fromValue(llints) << "aax"
            << "[Argument: aax {[Argument: ax {}], [Argument: ax {99, -100}], [Argument: ax {-9223372036854775808, 9223372036854775807}], [Argument: ax {}]}]";

    QList<QList<qulonglong> > ullints;
    QTest::newRow("empty-list-of-ullintlist") << QVariant::fromValue(ullints) << "aat"
            << "[Argument: aat {}]";
    ullints << QList<qulonglong>();
    QTest::newRow("list-of-emptyullintlist") << QVariant::fromValue(ullints) << "aat"
            << "[Argument: aat {[Argument: at {}]}]";
    ullints << (QList<qulonglong>() << Q_UINT64_C(66) << Q_UINT64_C(67))
            << (QList<qulonglong>() << Q_UINT64_C(18446744073709551615))
            << (QList<qulonglong>());
    QTest::newRow("ullintlist") << QVariant::fromValue(ullints) << "aat"
            << "[Argument: aat {[Argument: at {}], [Argument: at {66, 67}], [Argument: at {18446744073709551615}], [Argument: at {}]}]";

    QList<QList<double> > doubles;
    QTest::newRow("empty-list-ofdoublelist") << QVariant::fromValue(doubles) << "aad"
            << "[Argument: aad {}]";
    doubles << QList<double>();
    QTest::newRow("list-of-emptydoublelist") << QVariant::fromValue(doubles) << "aad"
            << "[Argument: aad {[Argument: ad {}]}]";
    doubles << (QList<double>() << 1.2 << 2.2 << 4.4)
            << (QList<double>() << -std::numeric_limits<double>::infinity()
                << std::numeric_limits<double>::infinity()
                << std::numeric_limits<double>::quiet_NaN())
            << (QList<double>());
    QTest::newRow("doublelist") << QVariant::fromValue(doubles) << "aad"
            << "[Argument: aad {[Argument: ad {}], [Argument: ad {1.2, 2.2, 4.4}], [Argument: ad {-inf, inf, nan}], [Argument: ad {}]}]";

    QList<QVariantList> variants;
    QTest::newRow("emptyvariantlist") << QVariant::fromValue(variants) << "aav"
            << "[Argument: aav {}]";
    variants << QVariantList();
    QTest::newRow("emptyvariantlist") << QVariant::fromValue(variants) << "aav"
            << "[Argument: aav {[Argument: av {}]}]";
    variants << (QVariantList() << QString("Hello") << QByteArray("World"))
             << (QVariantList() << 42 << -43.0 << 44U << Q_INT64_C(-45))
             << (QVariantList() << Q_UINT64_C(46) << true << QVariant::fromValue(short(-47)));
    QTest::newRow("variantlist") << QVariant::fromValue(variants) << "aav"
            << "[Argument: aav {[Argument: av {}], [Argument: av {[Variant(QString): \"Hello\"], [Variant(QByteArray): {87, 111, 114, 108, 100}]}], [Argument: av {[Variant(int): 42], [Variant(double): -43], [Variant(uint): 44], [Variant(qlonglong): -45]}], [Argument: av {[Variant(qulonglong): 46], [Variant(bool): true], [Variant(short): -47]}]}]";
}

void tst_QDBusMarshall::sendMaps_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("sig");
    QTest::addColumn<QString>("stringResult");

    QMap<int, QString> ismap;
    QTest::newRow("empty-is-map") << QVariant::fromValue(ismap) << "a{is}"
            << "[Argument: a{is} {}]";
    ismap[1] = "a";
    ismap[2000] = "b";
    ismap[-47] = "c";
    QTest::newRow("is-map") << QVariant::fromValue(ismap) << "a{is}"
            << "[Argument: a{is} {-47 = \"c\", 1 = \"a\", 2000 = \"b\"}]";

    QMap<QString, QString> ssmap;
    QTest::newRow("empty-ss-map") << QVariant::fromValue(ssmap) << "a{ss}"
            << "[Argument: a{ss} {}]";
    ssmap["a"] = "a";
    ssmap["c"] = "b";
    ssmap["b"] = "c";
    QTest::newRow("ss-map") << QVariant::fromValue(ssmap) << "a{ss}"
            << "[Argument: a{ss} {\"a\" = \"a\", \"b\" = \"c\", \"c\" = \"b\"}]";

    QVariantMap svmap;
    QTest::newRow("empty-sv-map") << QVariant::fromValue(svmap) << "a{sv}"
            << "[Argument: a{sv} {}]";
    svmap["a"] = 1;
    svmap["c"] = "b";
    svmap["b"] = QByteArray("c");
    svmap["d"] = 42U;
    svmap["e"] = QVariant::fromValue(short(-47));
    svmap["f"] = QVariant::fromValue(QDBusVariant(0));
    QTest::newRow("sv-map1") << QVariant::fromValue(svmap) << "a{sv}"
            << "[Argument: a{sv} {\"a\" = [Variant(int): 1], \"b\" = [Variant(QByteArray): {99}], \"c\" = [Variant(QString): \"b\"], \"d\" = [Variant(uint): 42], \"e\" = [Variant(short): -47], \"f\" = [Variant: [Variant(int): 0]]}]";

    QMap<QDBusObjectPath, QString> osmap;
    QTest::newRow("empty-os-map") << QVariant::fromValue(osmap) << "a{os}"
            << "[Argument: a{os} {}]";
    osmap[QDBusObjectPath("/")] = "root";
    osmap[QDBusObjectPath("/foo")] = "foo";
    osmap[QDBusObjectPath("/bar/baz")] = "bar and baz";
    QTest::newRow("os-map") << QVariant::fromValue(osmap) << "a{os}"
            << "[Argument: a{os} {[ObjectPath: /] = \"root\", [ObjectPath: /bar/baz] = \"bar and baz\", [ObjectPath: /foo] = \"foo\"}]";

    QMap<QDBusSignature, QString> gsmap;
    QTest::newRow("empty-gs-map") << QVariant::fromValue(gsmap) << "a{gs}"
            << "[Argument: a{gs} {}]";
    gsmap[QDBusSignature("i")] = "int32";
    gsmap[QDBusSignature("s")] = "string";
    gsmap[QDBusSignature("a{gs}")] = "array of dict_entry of (signature, string)";
    QTest::newRow("gs-map") << QVariant::fromValue(gsmap) << "a{gs}"
            << "[Argument: a{gs} {[Signature: a{gs}] = \"array of dict_entry of (signature, string)\", [Signature: i] = \"int32\", [Signature: s] = \"string\"}]";

    if (fileDescriptorPassing) {
        svmap["zzfiledescriptor"] = QVariant::fromValue(QDBusUnixFileDescriptor(fileDescriptorForTest()));
        QTest::newRow("sv-map1-fd") << QVariant::fromValue(svmap) << "a{sv}"
                                    << "[Argument: a{sv} {\"a\" = [Variant(int): 1], \"b\" = [Variant(QByteArray): {99}], \"c\" = [Variant(QString): \"b\"], \"d\" = [Variant(uint): 42], \"e\" = [Variant(short): -47], \"f\" = [Variant: [Variant(int): 0]], \"zzfiledescriptor\" = [Variant(QDBusUnixFileDescriptor): [Unix FD: valid]]}]";
    }

    svmap.clear();
    svmap["ismap"] = QVariant::fromValue(ismap);
    svmap["ssmap"] = QVariant::fromValue(ssmap);
    svmap["osmap"] = QVariant::fromValue(osmap);
    svmap["gsmap"] = QVariant::fromValue(gsmap);
    QTest::newRow("sv-map2") << QVariant::fromValue(svmap) << "a{sv}"
            << "[Argument: a{sv} {\"gsmap\" = [Variant: [Argument: a{gs} {[Signature: a{gs}] = \"array of dict_entry of (signature, string)\", [Signature: i] = \"int32\", [Signature: s] = \"string\"}]], \"ismap\" = [Variant: [Argument: a{is} {-47 = \"c\", 1 = \"a\", 2000 = \"b\"}]], \"osmap\" = [Variant: [Argument: a{os} {[ObjectPath: /] = \"root\", [ObjectPath: /bar/baz] = \"bar and baz\", [ObjectPath: /foo] = \"foo\"}]], \"ssmap\" = [Variant: [Argument: a{ss} {\"a\" = \"a\", \"b\" = \"c\", \"c\" = \"b\"}]]}]";
}

void tst_QDBusMarshall::sendStructs_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("sig");
    QTest::addColumn<QString>("stringResult");

    QTest::newRow("point") << QVariant(QPoint(1, 2)) << "(ii)" << "[Argument: (ii) 1, 2]";
    QTest::newRow("pointf") << QVariant(QPointF(1.5, -1.5)) << "(dd)" << "[Argument: (dd) 1.5, -1.5]";

    QTest::newRow("size") << QVariant(QSize(1, 2)) << "(ii)" << "[Argument: (ii) 1, 2]";
    QTest::newRow("sizef") << QVariant(QSizeF(1.5, 1.5)) << "(dd)" << "[Argument: (dd) 1.5, 1.5]";

    QTest::newRow("rect") << QVariant(QRect(1, 2, 3, 4)) << "(iiii)" << "[Argument: (iiii) 1, 2, 3, 4]";
    QTest::newRow("rectf") << QVariant(QRectF(0.5, 0.5, 1.5, 1.5)) << "(dddd)" << "[Argument: (dddd) 0.5, 0.5, 1.5, 1.5]";

    QTest::newRow("line") << QVariant(QLine(1, 2, 3, 4)) << "((ii)(ii))"
                          << "[Argument: ((ii)(ii)) [Argument: (ii) 1, 2], [Argument: (ii) 3, 4]]";
    QTest::newRow("linef") << QVariant(QLineF(0.5, 0.5, 1.5, 1.5)) << "((dd)(dd))"
                           << "[Argument: ((dd)(dd)) [Argument: (dd) 0.5, 0.5], [Argument: (dd) 1.5, 1.5]]";

    QDate date(2006, 6, 18);
    QTime time(12, 25, 00);     // the date I wrote this test on :-)
    QTest::newRow("date") << QVariant(date) << "(iii)" << "[Argument: (iii) 2006, 6, 18]";
    QTest::newRow("time") << QVariant(time) << "(iiii)" << "[Argument: (iiii) 12, 25, 0, 0]";
    QTest::newRow("datetime") << QVariant(QDateTime(date, time)) << "((iii)(iiii)i)"
        << "[Argument: ((iii)(iiii)i) [Argument: (iii) 2006, 6, 18], [Argument: (iiii) 12, 25, 0, 0], 0]";

    MyStruct ms = { 1, "Hello, World" };
    QTest::newRow("int-string") << QVariant::fromValue(ms) << "(is)" << "[Argument: (is) 1, \"Hello, World\"]";

    MyVariantMapStruct mvms = { "Hello, World", QVariantMap() };
    QTest::newRow("string-variantmap") << QVariant::fromValue(mvms) << "(sa{sv})" << "[Argument: (sa{sv}) \"Hello, World\", [Argument: a{sv} {}]]";

    // use only basic types, otherwise comparison will fail
    mvms.map["int"] = 42;
    mvms.map["uint"] = 42u;
    mvms.map["short"] = QVariant::fromValue<short>(-47);
    mvms.map["bytearray"] = QByteArray("Hello, world");
    QTest::newRow("string-variantmap2") << QVariant::fromValue(mvms) << "(sa{sv})" << "[Argument: (sa{sv}) \"Hello, World\", [Argument: a{sv} {\"bytearray\" = [Variant(QByteArray): {72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100}], \"int\" = [Variant(int): 42], \"short\" = [Variant(short): -47], \"uint\" = [Variant(uint): 42]}]]";

    QList<MyVariantMapStruct> list;
    QTest::newRow("empty-list-of-string-variantmap") << QVariant::fromValue(list) << "a(sa{sv})" << "[Argument: a(sa{sv}) {}]";
    list << mvms;
    QTest::newRow("list-of-string-variantmap") << QVariant::fromValue(list) << "a(sa{sv})" << "[Argument: a(sa{sv}) {[Argument: (sa{sv}) \"Hello, World\", [Argument: a{sv} {\"bytearray\" = [Variant(QByteArray): {72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100}], \"int\" = [Variant(int): 42], \"short\" = [Variant(short): -47], \"uint\" = [Variant(uint): 42]}]]}]";

    if (fileDescriptorPassing) {
        MyFileDescriptorStruct fds;
        fds.fd = QDBusUnixFileDescriptor(fileDescriptorForTest());
        QTest::newRow("fdstruct") << QVariant::fromValue(fds) << "(h)" << "[Argument: (h) [Unix FD: valid]]";

        QList<MyFileDescriptorStruct> fdlist;
        QTest::newRow("empty-list-of-fdstruct") << QVariant::fromValue(fdlist) << "a(h)" << "[Argument: a(h) {}]";

        fdlist << fds;
        QTest::newRow("list-of-fdstruct") << QVariant::fromValue(fdlist) << "a(h)" << "[Argument: a(h) {[Argument: (h) [Unix FD: valid]]}]";
    }
}

void tst_QDBusMarshall::sendComplex_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("sig");
    QTest::addColumn<QString>("stringResult");

    QList<QDateTime> dtlist;
    QTest::newRow("empty-datetimelist") << QVariant::fromValue(dtlist) << "a((iii)(iiii)i)"
            << "[Argument: a((iii)(iiii)i) {}]";
    dtlist << QDateTime();
    QTest::newRow("list-of-emptydatetime") << QVariant::fromValue(dtlist) << "a((iii)(iiii)i)"
            << "[Argument: a((iii)(iiii)i) {[Argument: ((iii)(iiii)i) [Argument: (iii) 0, 0, 0], [Argument: (iiii) -1, -1, -1, -1], 0]}]";
    dtlist << QDateTime(QDate(1977, 9, 13), QTime(0, 0, 0))
           << QDateTime(QDate(2006, 6, 18), QTime(13, 14, 0));
    QTest::newRow("datetimelist") << QVariant::fromValue(dtlist) << "a((iii)(iiii)i)"
            << "[Argument: a((iii)(iiii)i) {[Argument: ((iii)(iiii)i) [Argument: (iii) 0, 0, 0], [Argument: (iiii) -1, -1, -1, -1], 0], [Argument: ((iii)(iiii)i) [Argument: (iii) 1977, 9, 13], [Argument: (iiii) 0, 0, 0, 0], 0], [Argument: ((iii)(iiii)i) [Argument: (iii) 2006, 6, 18], [Argument: (iiii) 13, 14, 0, 0], 0]}]";

    QMap<qlonglong, QDateTime> lldtmap;
    QTest::newRow("empty-lldtmap") << QVariant::fromValue(lldtmap) << "a{x((iii)(iiii)i)}"
            << "[Argument: a{x((iii)(iiii)i)} {}]";
    lldtmap[0] = QDateTime();
    lldtmap[1] = QDateTime(QDate(1970, 1, 1), QTime(0, 0, 1), Qt::UTC);
    lldtmap[1150629776] = QDateTime(QDate(2006, 6, 18), QTime(11, 22, 56), Qt::UTC);
    QTest::newRow("lldtmap") << QVariant::fromValue(lldtmap) << "a{x((iii)(iiii)i)}"
            << "[Argument: a{x((iii)(iiii)i)} {0 = [Argument: ((iii)(iiii)i) [Argument: (iii) 0, 0, 0], [Argument: (iiii) -1, -1, -1, -1], 0], 1 = [Argument: ((iii)(iiii)i) [Argument: (iii) 1970, 1, 1], [Argument: (iiii) 0, 0, 1, 0], 1], 1150629776 = [Argument: ((iii)(iiii)i) [Argument: (iii) 2006, 6, 18], [Argument: (iiii) 11, 22, 56, 0], 1]}]";


    QMap<int, QString> ismap;
    ismap[1] = "a";
    ismap[2000] = "b";
    ismap[-47] = "c";

    QMap<QString, QString> ssmap;
    ssmap["a"] = "a";
    ssmap["c"] = "b";
    ssmap["b"] = "c";

    QMap<QDBusSignature, QString> gsmap;
    gsmap[QDBusSignature("i")] = "int32";
    gsmap[QDBusSignature("s")] = "string";
    gsmap[QDBusSignature("a{gs}")] = "array of dict_entry of (signature, string)";

    QVariantMap svmap;
    svmap["a"] = 1;
    svmap["c"] = "b";
    svmap["b"] = QByteArray("c");
    svmap["d"] = 42U;
    svmap["e"] = QVariant::fromValue(short(-47));
    svmap["f"] = QVariant::fromValue(QDBusVariant(0));
    svmap["date"] = QDate(1977, 1, 1);
    svmap["time"] = QTime(8, 58, 0);
    svmap["datetime"] = QDateTime(QDate(13, 9, 2008), QTime(8, 59, 31));
    svmap["pointf"] = QPointF(0.5, -0.5);
    svmap["ismap"] = QVariant::fromValue(ismap);
    svmap["ssmap"] = QVariant::fromValue(ssmap);
    svmap["gsmap"] = QVariant::fromValue(gsmap);
    svmap["dtlist"] = QVariant::fromValue(dtlist);
    svmap["lldtmap"] = QVariant::fromValue(lldtmap);
    QTest::newRow("sv-map") << QVariant::fromValue(svmap) << "a{sv}"
            << "[Argument: a{sv} {\"a\" = [Variant(int): 1], \"b\" = [Variant(QByteArray): {99}], \"c\" = [Variant(QString): \"b\"], \"d\" = [Variant(uint): 42], \"date\" = [Variant: [Argument: (iii) 1977, 1, 1]], \"datetime\" = [Variant: [Argument: ((iii)(iiii)i) [Argument: (iii) 0, 0, 0], [Argument: (iiii) 8, 59, 31, 0], 0]], \"dtlist\" = [Variant: [Argument: a((iii)(iiii)i) {[Argument: ((iii)(iiii)i) [Argument: (iii) 0, 0, 0], [Argument: (iiii) -1, -1, -1, -1], 0], [Argument: ((iii)(iiii)i) [Argument: (iii) 1977, 9, 13], [Argument: (iiii) 0, 0, 0, 0], 0], [Argument: ((iii)(iiii)i) [Argument: (iii) 2006, 6, 18], [Argument: (iiii) 13, 14, 0, 0], 0]}]], \"e\" = [Variant(short): -47], \"f\" = [Variant: [Variant(int): 0]], \"gsmap\" = [Variant: [Argument: a{gs} {[Signature: a{gs}] = \"array of dict_entry of (signature, string)\", [Signature: i] = \"int32\", [Signature: s] = \"string\"}]], \"ismap\" = [Variant: [Argument: a{is} {-47 = \"c\", 1 = \"a\", 2000 = \"b\"}]], \"lldtmap\" = [Variant: [Argument: a{x((iii)(iiii)i)} {0 = [Argument: ((iii)(iiii)i) [Argument: (iii) 0, 0, 0], [Argument: (iiii) -1, -1, -1, -1], 0], 1 = [Argument: ((iii)(iiii)i) [Argument: (iii) 1970, 1, 1], [Argument: (iiii) 0, 0, 1, 0], 1], 1150629776 = [Argument: ((iii)(iiii)i) [Argument: (iii) 2006, 6, 18], [Argument: (iiii) 11, 22, 56, 0], 1]}]], \"pointf\" = [Variant: [Argument: (dd) 0.5, -0.5]], \"ssmap\" = [Variant: [Argument: a{ss} {\"a\" = \"a\", \"b\" = \"c\", \"c\" = \"b\"}]], \"time\" = [Variant: [Argument: (iiii) 8, 58, 0, 0]]}]";
}

void tst_QDBusMarshall::sendArgument_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("sig");
    QTest::addColumn<int>("classification");

    QDBusArgument();
    QDBusArgument arg;

    arg = QDBusArgument();
    arg << true;
    QTest::newRow("bool") << QVariant::fromValue(arg) << "b" << int(QDBusArgument::BasicType);;

    arg = QDBusArgument();
    arg << false;
    QTest::newRow("bool2") << QVariant::fromValue(arg) << "b" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << uchar(1);
    QTest::newRow("byte") << QVariant::fromValue(arg) << "y" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << short(2);
    QTest::newRow("int16") << QVariant::fromValue(arg) << "n" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << ushort(3);
    QTest::newRow("uint16") << QVariant::fromValue(arg) << "q" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << 1;
    QTest::newRow("int32") << QVariant::fromValue(arg) << "i" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << 2U;
    QTest::newRow("uint32") << QVariant::fromValue(arg) << "u" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << Q_INT64_C(3);
    QTest::newRow("int64") << QVariant::fromValue(arg) << "x" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << Q_UINT64_C(4);
    QTest::newRow("uint64") << QVariant::fromValue(arg) << "t" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << 42.5;
    QTest::newRow("double") << QVariant::fromValue(arg) << "d" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << QLatin1String("ping");
    QTest::newRow("string") << QVariant::fromValue(arg) << "s" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << QDBusObjectPath("/org/kde");
    QTest::newRow("objectpath") << QVariant::fromValue(arg) << "o" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << QDBusSignature("g");
    QTest::newRow("signature") << QVariant::fromValue(arg) << "g" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << QLatin1String("");
    QTest::newRow("emptystring") << QVariant::fromValue(arg) << "s" << int(QDBusArgument::BasicType);

    arg = QDBusArgument();
    arg << QString();
    QTest::newRow("nullstring") << QVariant::fromValue(arg) << "s" << int(QDBusArgument::BasicType);

    if (fileDescriptorPassing) {
        arg = QDBusArgument();
        arg << QDBusUnixFileDescriptor(fileDescriptorForTest());
        QTest::newRow("filedescriptor") << QVariant::fromValue(arg) << "h" << int(QDBusArgument::BasicType);
    }

    arg = QDBusArgument();
    arg << QDBusVariant(1);
    QTest::newRow("variant") << QVariant::fromValue(arg) << "v" << int(QDBusArgument::VariantType);

    arg = QDBusArgument();
    arg << QDBusVariant(QVariant::fromValue(QDBusVariant(1)));
    QTest::newRow("variant-variant") << QVariant::fromValue(arg) << "v" << int(QDBusArgument::VariantType);

    arg = QDBusArgument();
    arg.beginArray(QVariant::Int);
    arg << 1 << 2 << 3 << -4;
    arg.endArray();
    QTest::newRow("array-of-int") << QVariant::fromValue(arg) << "ai" << int(QDBusArgument::ArrayType);

    arg = QDBusArgument();
    arg.beginMap(QVariant::Int, QVariant::UInt);
    arg.beginMapEntry();
    arg << 1 << 2U;
    arg.endMapEntry();
    arg.beginMapEntry();
    arg << 3 << 4U;
    arg.endMapEntry();
    arg.endMap();
    QTest::newRow("map") << QVariant::fromValue(arg) << "a{iu}" << int(QDBusArgument::MapType);

    arg = QDBusArgument();
    arg.beginStructure();
    arg << 1 << 2U << short(-3) << ushort(4) << 5.0 << false;
    arg.endStructure();
    QTest::newRow("structure") << QVariant::fromValue(arg) << "(iunqdb)" << int(QDBusArgument::StructureType);
}

void tst_QDBusMarshall::sendBasic()
{
    QFETCH(QVariant, value);
    QFETCH(QString, stringResult);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName,
                                                      objectPath, interfaceName, "ping");
    msg << value;

    QDBusMessage reply = con.call(msg);
    QVERIFY2(reply.type() == QDBusMessage::ReplyMessage,
             qPrintable(reply.errorName() + ": " + reply.errorMessage()));
    //qDebug() << reply;

    QCOMPARE(reply.arguments().count(), msg.arguments().count());
    QTEST(reply.signature(), "sig");
    for (int i = 0; i < reply.arguments().count(); ++i) {
        QVERIFY(compare(reply.arguments().at(i), msg.arguments().at(i)));
        //printf("\n! %s\n* %s\n", qPrintable(qDBusArgumentToString(reply.arguments().at(i))), qPrintable(stringResult));
        QCOMPARE(QDBusUtil::argumentToString(reply.arguments().at(i)), stringResult);
    }
}

void tst_QDBusMarshall::sendVariant()
{
    QFETCH(QVariant, value);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName,
                                                      objectPath, interfaceName, "ping");
    msg << QVariant::fromValue(QDBusVariant(value));

    QDBusMessage reply = con.call(msg);
 //   qDebug() << reply;

    QCOMPARE(reply.arguments().count(), msg.arguments().count());
    QCOMPARE(reply.signature(), QString("v"));
    for (int i = 0; i < reply.arguments().count(); ++i)
        QVERIFY(compare(reply.arguments().at(i), msg.arguments().at(i)));
}

void tst_QDBusMarshall::sendArrays()
{
    sendBasic();
}

void tst_QDBusMarshall::sendArrayOfArrays()
{
    sendBasic();
}

void tst_QDBusMarshall::sendMaps()
{
    sendBasic();
}

void tst_QDBusMarshall::sendStructs()
{
    sendBasic();
}

void tst_QDBusMarshall::sendComplex()
{
    sendBasic();
}

void tst_QDBusMarshall::sendArgument()
{
    QFETCH(QVariant, value);
    QFETCH(QString, sig);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath,
                                                      interfaceName, "ping");
    msg << value;

    QDBusMessage reply = con.call(msg);

//    QCOMPARE(reply.arguments().count(), msg.arguments().count());
    QCOMPARE(reply.signature(), sig);
//    for (int i = 0; i < reply.arguments().count(); ++i)
//        QVERIFY(compare(reply.arguments().at(i), msg.arguments().at(i)));

    // do it again inside a STRUCT now
    QDBusArgument sendArg;
    sendArg.beginStructure();
    sendArg.appendVariant(value);
    sendArg.endStructure();
    msg.setArguments(QVariantList() << QVariant::fromValue(sendArg));
    reply = con.call(msg);

    QCOMPARE(reply.signature(), QString("(%1)").arg(sig));
    QCOMPARE(reply.arguments().at(0).userType(), qMetaTypeId<QDBusArgument>());

    const QDBusArgument arg = qvariant_cast<QDBusArgument>(reply.arguments().at(0));
    QCOMPARE(int(arg.currentType()), int(QDBusArgument::StructureType));

    arg.beginStructure();
    QVERIFY(!arg.atEnd());
    QCOMPARE(arg.currentSignature(), sig);
    QTEST(int(arg.currentType()), "classification");

    QVariant extracted = arg.asVariant();
    QVERIFY(arg.atEnd());

    arg.endStructure();
    QVERIFY(arg.atEnd());
    QCOMPARE(arg.currentType(), QDBusArgument::UnknownType);

    if (value.type() != QVariant::UserType)
        QCOMPARE(extracted, value);
}

void tst_QDBusMarshall::sendSignalErrors()
{
    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());
    QDBusMessage msg = QDBusMessage::createSignal("/foo", "local.interfaceName",
                                                  "signalName");
    msg << QVariant::fromValue(QDBusObjectPath());

    QTest::ignoreMessage(QtWarningMsg, "QDBusConnection: error: could not send signal to service \"\" path \"/foo\" interface \"local.interfaceName\" member \"signalName\": Marshalling failed: Invalid object path passed in arguments");
    QVERIFY(!con.send(msg));

    msg.setArguments(QVariantList());
    QDBusObjectPath path;

    QTest::ignoreMessage(QtWarningMsg, "QDBusObjectPath: invalid path \"abc\"");
    path.setPath("abc");
    msg << QVariant::fromValue(path);

    QTest::ignoreMessage(QtWarningMsg, "QDBusConnection: error: could not send signal to service \"\" path \"/foo\" interface \"local.interfaceName\" member \"signalName\": Marshalling failed: Invalid object path passed in arguments");
    QVERIFY(!con.send(msg));

    QDBusSignature sig;
    msg.setArguments(QVariantList() << QVariant::fromValue(sig));
    QTest::ignoreMessage(QtWarningMsg, "QDBusConnection: error: could not send signal to service \"\" path \"/foo\" interface \"local.interfaceName\" member \"signalName\": Marshalling failed: Invalid signature passed in arguments");
    QVERIFY(!con.send(msg));

    QTest::ignoreMessage(QtWarningMsg, "QDBusSignature: invalid signature \"a\"");
    sig.setSignature("a");
    msg.setArguments(QVariantList());
    msg << QVariant::fromValue(sig);
    QTest::ignoreMessage(QtWarningMsg, "QDBusConnection: error: could not send signal to service \"\" path \"/foo\" interface \"local.interfaceName\" member \"signalName\": Marshalling failed: Invalid signature passed in arguments");
    QVERIFY(!con.send(msg));
}

void tst_QDBusMarshall::sendCallErrors_data()
{
    QTest::addColumn<QString>("service");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("interface");
    QTest::addColumn<QString>("method");
    QTest::addColumn<QVariantList>("arguments");
    QTest::addColumn<QString>("errorName");
    QTest::addColumn<QString>("errorMsg");
    QTest::addColumn<QString>("ignoreMsg");

    // this error comes from the bus server
    QTest::newRow("empty-service") << "" << objectPath << interfaceName << "ping" << QVariantList()
            << "org.freedesktop.DBus.Error.UnknownMethod"
            << "Method \"ping\" with signature \"\" on interface \"org.qtproject.autotests.qpong\" doesn't exist\n" << (const char*)0;

    QTest::newRow("invalid-service") << "this isn't valid" << objectPath << interfaceName << "ping" << QVariantList()
            << "org.qtproject.QtDBus.Error.InvalidService"
            << "Invalid service name: this isn't valid" << "";

    QTest::newRow("empty-path") << serviceName << "" << interfaceName << "ping" << QVariantList()
            << "org.qtproject.QtDBus.Error.InvalidObjectPath"
            << "Object path cannot be empty" << "";
    QTest::newRow("invalid-path") << serviceName << "//" << interfaceName << "ping" << QVariantList()
            << "org.qtproject.QtDBus.Error.InvalidObjectPath"
            << "Invalid object path: //" << "";

    // empty interfaces are valid
    QTest::newRow("invalid-interface") << serviceName << objectPath << "this isn't valid" << "ping" << QVariantList()
            << "org.qtproject.QtDBus.Error.InvalidInterface"
            << "Invalid interface class: this isn't valid" << "";

    QTest::newRow("empty-method") << serviceName << objectPath << interfaceName << "" << QVariantList()
            << "org.qtproject.QtDBus.Error.InvalidMember"
            << "method name cannot be empty" << "";
    QTest::newRow("invalid-method") << serviceName << objectPath << interfaceName << "this isn't valid" << QVariantList()
            << "org.qtproject.QtDBus.Error.InvalidMember"
            << "Invalid method name: this isn't valid" << "";

    QTest::newRow("invalid-variant1") << serviceName << objectPath << interfaceName << "ping"
            << (QVariantList() << QVariant())
            << "org.freedesktop.DBus.Error.Failed"
            << "Marshalling failed: Variant containing QVariant::Invalid passed in arguments"
            << "QDBusMarshaller: cannot add an invalid QVariant";
    QTest::newRow("invalid-variant1") << serviceName << objectPath << interfaceName << "ping"
            << (QVariantList() << QVariant::fromValue(QDBusVariant()))
            << "org.freedesktop.DBus.Error.Failed"
            << "Marshalling failed: Variant containing QVariant::Invalid passed in arguments"
            << "QDBusMarshaller: cannot add a null QDBusVariant";

    QTest::newRow("builtin-unregistered") << serviceName << objectPath << interfaceName << "ping"
            << (QVariantList() << QLocale::c())
            << "org.freedesktop.DBus.Error.Failed"
            << "Marshalling failed: Unregistered type QLocale passed in arguments"
            << "QDBusMarshaller: type `QLocale' (18) is not registered with D-BUS. Use qDBusRegisterMetaType to register it";

    // this type is known to the meta type system, but not registered with D-Bus
    qRegisterMetaType<UnregisteredType>();
    QTest::newRow("extra-unregistered") << serviceName << objectPath << interfaceName << "ping"
            << (QVariantList() << QVariant::fromValue(UnregisteredType()))
            << "org.freedesktop.DBus.Error.Failed"
            << "Marshalling failed: Unregistered type UnregisteredType passed in arguments"
            << QString("QDBusMarshaller: type `UnregisteredType' (%1) is not registered with D-BUS. Use qDBusRegisterMetaType to register it")
            .arg(qMetaTypeId<UnregisteredType>());

    QTest::newRow("invalid-object-path-arg") << serviceName << objectPath << interfaceName << "ping"
            << (QVariantList() << QVariant::fromValue(QDBusObjectPath()))
            << "org.freedesktop.DBus.Error.Failed"
            << "Marshalling failed: Invalid object path passed in arguments"
            << "";

    QTest::newRow("invalid-signature-arg") << serviceName << objectPath << interfaceName << "ping"
            << (QVariantList() << QVariant::fromValue(QDBusSignature()))
            << "org.freedesktop.DBus.Error.Failed"
            << "Marshalling failed: Invalid signature passed in arguments"
            << "";

    // invalid file descriptor
    if (fileDescriptorPassing) {
        QTest::newRow("invalid-file-descriptor") << serviceName << objectPath << interfaceName << "ping"
                << (QVariantList() << QVariant::fromValue(QDBusUnixFileDescriptor(-1)))
                << "org.freedesktop.DBus.Error.Failed"
                << "Marshalling failed: Invalid file descriptor passed in arguments"
                << "";
    }
}

void tst_QDBusMarshall::sendCallErrors()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    QFETCH(QString, service);
    QFETCH(QString, path);
    QFETCH(QString, interface);
    QFETCH(QString, method);
    QFETCH(QVariantList, arguments);
    QFETCH(QString, errorMsg);

    QFETCH(QString, ignoreMsg);
    if (!ignoreMsg.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, ignoreMsg.toLatin1());
    if (!ignoreMsg.isNull())
        QTest::ignoreMessage(QtWarningMsg,
                             QString("QDBusConnection: error: could not send message to service \"%1\" path \"%2\" interface \"%3\" member \"%4\": %5")
                             .arg(service, path, interface, method, errorMsg)
                             .toLatin1());

    QDBusMessage msg = QDBusMessage::createMethodCall(service, path, interface, method);
    msg.setArguments(arguments);

    QDBusMessage reply = con.call(msg, QDBus::Block);
    QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);
    QTEST(reply.errorName(), "errorName");
    QCOMPARE(reply.errorMessage(), errorMsg);
}

// If DBUS_TYPE_UNIX_FD is not defined, it means the current system's D-Bus library is too old for this test
void tst_QDBusMarshall::receiveUnknownType_data()
{
    QTest::addColumn<int>("receivedTypeId");
    QTest::newRow("in-call") << qMetaTypeId<void*>();
    QTest::newRow("type-variant") << qMetaTypeId<QDBusVariant>();
    QTest::newRow("type-array") << qMetaTypeId<QDBusArgument>();
    QTest::newRow("type-struct") << qMetaTypeId<QDBusArgument>();
    QTest::newRow("type-naked") << qMetaTypeId<void *>();
}

struct DisconnectRawDBus {
    static void cleanup(DBusConnection *connection)
    {
        if (!connection)
            return;
        q_dbus_connection_close(connection);
        q_dbus_connection_unref(connection);
    }
};
struct UnrefDBusMessage
{
    static void cleanup(DBusMessage *type)
    {
        if (!type) return;
        q_dbus_message_unref(type);
    }
};
struct UnrefDBusPendingCall
{
    static void cleanup(DBusPendingCall *type)
    {
        if (!type) return;
        q_dbus_pending_call_unref(type);
    }
};

// use these scoped types to avoid memory leaks if QVERIFY or QCOMPARE fails
typedef QScopedPointer<DBusConnection, DisconnectRawDBus> ScopedDBusConnection;
typedef QScopedPointer<DBusMessage, UnrefDBusMessage> ScopedDBusMessage;
typedef QScopedPointer<DBusPendingCall, UnrefDBusPendingCall> ScopedDBusPendingCall;

template <typename T> struct SetResetValue
{
    const T oldValue;
    T &value;
public:
    SetResetValue(T &v, T newValue) : oldValue(v), value(v)
    {
        value = newValue;
    }
    ~SetResetValue()
    {
        value = oldValue;
    }
};

// mostly the same as qdbusintegrator.cpp:connectionCapabilies
static bool canSendUnixFd(DBusConnection *connection)
{
    typedef dbus_bool_t (*can_send_type_t)(DBusConnection *, int);
    static can_send_type_t can_send_type = 0;

#if defined(QT_LINKED_LIBDBUS)
# if DBUS_VERSION-0 >= 0x010400
    can_send_type = dbus_connection_can_send_type;
# endif
#elif !defined(QT_NO_LIBRARY)
    // run-time check if the next functions are available
    can_send_type = (can_send_type_t)qdbus_resolve_conditionally("dbus_connection_can_send_type");
#endif

    return can_send_type && can_send_type(connection, DBUS_TYPE_UNIX_FD);
}

void tst_QDBusMarshall::receiveUnknownType()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QVERIFY(con.isConnected());

    // this needs to be implemented in raw
    // open a new connection to the bus daemon
    DBusError error;
    q_dbus_error_init(&error);
    ScopedDBusConnection rawcon(q_dbus_bus_get_private(DBUS_BUS_SESSION, &error));
    QVERIFY2(rawcon.data(), error.name);

    // check if this bus supports passing file descriptors

    if (!canSendUnixFd(rawcon.data()))
        QSKIP("Your session bus does not allow sending Unix file descriptors");

    // make sure this QDBusConnection won't handle Unix file descriptors
    QDBusConnection::ConnectionCapabilities &capabRef = QDBusConnectionPrivate::d(con)->capabilities;
    SetResetValue<QDBusConnection::ConnectionCapabilities> resetter(capabRef, capabRef & ~QDBusConnection::UnixFileDescriptorPassing);

    if (qstrcmp(QTest::currentDataTag(), "in-call") == 0) {
        // create a call back to us containing a file descriptor
        QDBusMessageSpy spy;
        con.registerObject("/spyObject", &spy, QDBusConnection::ExportAllSlots);
        ScopedDBusMessage msg(q_dbus_message_new_method_call(con.baseService().toLatin1(), "/spyObject", NULL, "theSlot"));

        int fd = fileno(stdout);
        DBusMessageIter iter;
        q_dbus_message_iter_init_append(msg.data(), &iter);
        q_dbus_message_iter_append_basic(&iter, DBUS_TYPE_UNIX_FD, &fd);

        // try to send to us
        DBusPendingCall *pending_ptr;
        q_dbus_connection_send_with_reply(rawcon.data(), msg.data(), &pending_ptr, 1000);
        ScopedDBusPendingCall pending(pending_ptr);

        // check that it got sent
        while (q_dbus_connection_dispatch(rawcon.data()) == DBUS_DISPATCH_DATA_REMAINS)
            ;

        // now spin our event loop. We don't catch this call, so let's get the reply
        QEventLoop loop;
        QTimer::singleShot(200, &loop, SLOT(quit()));
        loop.exec();

        // now try to receive the reply
        q_dbus_pending_call_block(pending.data());

        // check that the spy received what it was supposed to receive
        QCOMPARE(spy.list.size(), 1);
        QCOMPARE(spy.list.at(0).arguments().size(), 1);
        QFETCH(int, receivedTypeId);
        QCOMPARE(spy.list.at(0).arguments().at(0).userType(), receivedTypeId);

        msg.reset(q_dbus_pending_call_steal_reply(pending.data()));
        QVERIFY(msg);
        QCOMPARE(q_dbus_message_get_type(msg.data()), DBUS_MESSAGE_TYPE_METHOD_RETURN);
        QCOMPARE(q_dbus_message_get_signature(msg.data()), DBUS_TYPE_INT32_AS_STRING);

        int retval;
        QVERIFY(q_dbus_message_iter_init(msg.data(), &iter));
        q_dbus_message_iter_get_basic(&iter, &retval);
        QCOMPARE(retval, 42);
    } else {
        // create a signal that we'll emit
        static const char signalName[] = "signalName";
        static const char interfaceName[] = "local.interface.name";
        ScopedDBusMessage msg(q_dbus_message_new_signal("/", interfaceName, signalName));
        con.connect(q_dbus_bus_get_unique_name(rawcon.data()), QString(), interfaceName, signalName, &QTestEventLoop::instance(), SLOT(exitLoop()));

        QDBusMessageSpy spy;
        con.connect(q_dbus_bus_get_unique_name(rawcon.data()), QString(), interfaceName, signalName, &spy, SLOT(theSlot(QDBusMessage)));

        DBusMessageIter iter;
        q_dbus_message_iter_init_append(msg.data(), &iter);
        int fd = fileno(stdout);

        if (qstrcmp(QTest::currentDataTag(), "type-naked") == 0) {
            // send naked
            q_dbus_message_iter_append_basic(&iter, DBUS_TYPE_UNIX_FD, &fd);
        } else {
            DBusMessageIter subiter;
            if (qstrcmp(QTest::currentDataTag(), "type-variant") == 0)
                q_dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, DBUS_TYPE_UNIX_FD_AS_STRING, &subiter);
            else if (qstrcmp(QTest::currentDataTag(), "type-array") == 0)
                q_dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, DBUS_TYPE_UNIX_FD_AS_STRING, &subiter);
            else if (qstrcmp(QTest::currentDataTag(), "type-struct") == 0)
                q_dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, 0, &subiter);
            q_dbus_message_iter_append_basic(&subiter, DBUS_TYPE_UNIX_FD, &fd);
            q_dbus_message_iter_close_container(&iter, &subiter);
        }

        // send it
        q_dbus_connection_send(rawcon.data(), msg.data(), 0);

        // check that it got sent
        while (q_dbus_connection_dispatch(rawcon.data()) == DBUS_DISPATCH_DATA_REMAINS)
            ;

        // now let's see what happens
        QTestEventLoop::instance().enterLoop(1);
        QVERIFY(!QTestEventLoop::instance().timeout());
        QCOMPARE(spy.list.size(), 1);
        QCOMPARE(spy.list.at(0).arguments().count(), 1);
        QFETCH(int, receivedTypeId);
        //qDebug() << spy.list.at(0).arguments().at(0).typeName();
        QCOMPARE(spy.list.at(0).arguments().at(0).userType(), receivedTypeId);
    }
}

void tst_QDBusMarshall::demarshallPrimitives_data()
{
    addBasicTypesColumns();

    // Primitive types, excluding strings and FD
    basicNumericTypes_data();
}

template<class T>
QVariant demarshallPrimitiveAs(const QDBusArgument& dbusArg)
{
    T val;
    dbusArg >> val;
    return QVariant::fromValue(val);
}

QVariant demarshallPrimitiveAs(int typeIndex, const QDBusArgument& dbusArg)
{
    switch (typeIndex) {
    case 0:
        return demarshallPrimitiveAs<uchar>(dbusArg);
    case 1:
        return demarshallPrimitiveAs<bool>(dbusArg);
    case 2:
        return demarshallPrimitiveAs<short>(dbusArg);
    case 3:
        return demarshallPrimitiveAs<ushort>(dbusArg);
    case 4:
        return demarshallPrimitiveAs<int>(dbusArg);
    case 5:
        return demarshallPrimitiveAs<uint>(dbusArg);
    case 6:
        return demarshallPrimitiveAs<qlonglong>(dbusArg);
    case 7:
        return demarshallPrimitiveAs<qulonglong>(dbusArg);
    case 8:
        return demarshallPrimitiveAs<double>(dbusArg);
    default:
        return QVariant();
    }
}

void tst_QDBusMarshall::demarshallPrimitives()
{
    QFETCH(QVariant, value);
    QFETCH(QString, sig);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    // Demarshall each test data value to all primitive types to test
    // demarshalling to the wrong type does not cause a crash
    for (int typeIndex = 0; true; ++typeIndex) {
        QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath,
                                                          interfaceName, "ping");
        QDBusArgument sendArg;
        sendArg.beginStructure();
        sendArg.appendVariant(value);
        sendArg.endStructure();
        msg.setArguments(QVariantList() << QVariant::fromValue(sendArg));
        QDBusMessage reply = con.call(msg);

        const QDBusArgument receiveArg = qvariant_cast<QDBusArgument>(reply.arguments().at(0));
        receiveArg.beginStructure();
        QCOMPARE(receiveArg.currentSignature(), sig);

        const QVariant receiveValue = demarshallPrimitiveAs(typeIndex, receiveArg);
        if (receiveValue.type() == value.type()) {
            // Value type is the same, compare the values
            QCOMPARE(receiveValue, value);
            QVERIFY(receiveArg.atEnd());
        }

        receiveArg.endStructure();
        QVERIFY(receiveArg.atEnd());

        if (!receiveValue.isValid())
            break;
    }
}

void tst_QDBusMarshall::demarshallStrings_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<char>("targetSig");
    QTest::addColumn<QVariant>("expectedValue");

    // All primitive types demarshall to null string types
    typedef QPair<QVariant, char> ValSigPair;
    const QList<ValSigPair> nullStringTypes
        = QList<ValSigPair>()
            << ValSigPair(QVariant::fromValue(QString()), 's')
            << ValSigPair(QVariant::fromValue(QDBusObjectPath()), 'o')
            << ValSigPair(QVariant::fromValue(QDBusSignature()), 'g');
    foreach (ValSigPair valSigPair, nullStringTypes) {
        QTest::newRow("bool(false)") << QVariant(false) << valSigPair.second << valSigPair.first;
        QTest::newRow("bool(true)") << QVariant(true) << valSigPair.second << valSigPair.first;
        QTest::newRow("byte") << QVariant::fromValue(uchar(1)) << valSigPair.second << valSigPair.first;
        QTest::newRow("int16") << QVariant::fromValue(short(2)) << valSigPair.second << valSigPair.first;
        QTest::newRow("uint16") << QVariant::fromValue(ushort(3)) << valSigPair.second << valSigPair.first;
        QTest::newRow("int") << QVariant(1) << valSigPair.second << valSigPair.first;
        QTest::newRow("uint") << QVariant(2U) << valSigPair.second << valSigPair.first;
        QTest::newRow("int64") << QVariant(Q_INT64_C(3)) << valSigPair.second << valSigPair.first;
        QTest::newRow("uint64") << QVariant(Q_UINT64_C(4)) << valSigPair.second << valSigPair.first;
        QTest::newRow("double") << QVariant(42.5) << valSigPair.second << valSigPair.first;
    }

    // String types should demarshall to each other. This is a regression test
    // to check released functionality is maintained even after checks have
    // been added to string demarshalling
    QTest::newRow("empty string->invalid objectpath") << QVariant("")
                                                      << 'o' << QVariant::fromValue(QDBusObjectPath());
    QTest::newRow("null string->invalid objectpath") << QVariant(QString())
                                                     << 'o' << QVariant::fromValue(QDBusObjectPath());
    QTest::newRow("string->invalid objectpath") << QVariant("invalid objectpath")
                                                << 'o' << QVariant::fromValue(QDBusObjectPath());
    QTest::newRow("string->valid objectpath") << QVariant("/org/kde")
                                              << 'o' << QVariant::fromValue(QDBusObjectPath("/org/kde"));

    QTest::newRow("empty string->invalid signature") << QVariant("")
                                                     << 'g' << QVariant::fromValue(QDBusSignature());
    QTest::newRow("null string->invalid signature") << QVariant(QString())
                                                    << 'g' << QVariant::fromValue(QDBusSignature());
    QTest::newRow("string->invalid signature") << QVariant("_invalid signature")
                                               << 'g' << QVariant::fromValue(QDBusSignature());
    QTest::newRow("string->valid signature") << QVariant("s")
                                             << 'g' << QVariant::fromValue(QDBusSignature("s"));

    QTest::newRow("objectpath->string") << QVariant::fromValue(QDBusObjectPath("/org/kde"))
                                        << 's' << QVariant::fromValue(QString("/org/kde"));
    QTest::newRow("objectpath->invalid signature") << QVariant::fromValue(QDBusObjectPath("/org/kde"))
                                                   << 'g' << QVariant::fromValue(QDBusSignature());

    QTest::newRow("signature->string") << QVariant::fromValue(QDBusSignature("s"))
                                       << 's' << QVariant::fromValue(QString("s"));
    QTest::newRow("signature->invalid objectpath") << QVariant::fromValue(QDBusSignature("s"))
                                                   << 'o' << QVariant::fromValue(QDBusObjectPath());
}

QVariant demarshallAsString(const QDBusArgument& dbusArg, char targetSig)
{
    switch (targetSig) {
        case 's': {
            QString s;
            dbusArg >> s;
            return s;
        }
        case 'o': {
            QDBusObjectPath op;
            dbusArg >> op;
            return QVariant::fromValue(op);
        }
        case 'g' : {
            QDBusSignature sig;
            dbusArg >> sig;
            return QVariant::fromValue(sig);
        }
        default: {
            return QVariant();
        }
    }
}

void tst_QDBusMarshall::demarshallStrings()
{
    QFETCH(QVariant, value);
    QFETCH(char, targetSig);
    QFETCH(QVariant, expectedValue);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath,
                                                      interfaceName, "ping");
    QDBusArgument sendArg;
    sendArg.beginStructure();
    sendArg.appendVariant(value);
    sendArg.endStructure();
    msg.setArguments(QVariantList() << QVariant::fromValue(sendArg));
    QDBusMessage reply = con.call(msg);

    const QDBusArgument receiveArg = qvariant_cast<QDBusArgument>(reply.arguments().at(0));
    receiveArg.beginStructure();

    QVariant receiveValue = demarshallAsString(receiveArg, targetSig);
    QVERIFY2(receiveValue.isValid(), "Invalid targetSig in demarshallStrings_data()");
    QVERIFY(compare(receiveValue, expectedValue));

    receiveArg.endStructure();
    QVERIFY(receiveArg.atEnd());
}

void tst_QDBusMarshall::demarshallInvalidStringList_data()
{
    addBasicTypesColumns();

    // None of the basic types should demarshall to a string list
    basicNumericTypes_data();
    basicStringTypes_data();

    // Arrays of non-string type should not demarshall to a string list
    QList<bool> bools;
    QTest::newRow("emptyboollist") << QVariant::fromValue(bools);
    bools << false << true << false;
    QTest::newRow("boollist") << QVariant::fromValue(bools);

    // Structures should not demarshall to a QByteArray
    QTest::newRow("struct of strings")
            << QVariant::fromValue(QVariantList() << QString("foo") << QString("bar"));
    QTest::newRow("struct of mixed types")
            << QVariant::fromValue(QVariantList() << QString("foo") << int(42) << double(3.14));
}

void tst_QDBusMarshall::demarshallInvalidStringList()
{
    QFETCH(QVariant, value);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath,
                                                      interfaceName, "ping");
    QDBusArgument sendArg;
    sendArg.beginStructure();
    sendArg.appendVariant(value);
    sendArg.endStructure();
    msg.setArguments(QVariantList() << QVariant::fromValue(sendArg));
    QDBusMessage reply = con.call(msg);

    const QDBusArgument receiveArg = qvariant_cast<QDBusArgument>(reply.arguments().at(0));
    receiveArg.beginStructure();

    QStringList receiveValue;
    receiveArg >> receiveValue;
    QCOMPARE(receiveValue, QStringList());

    receiveArg.endStructure();
    QVERIFY(receiveArg.atEnd());
}

void tst_QDBusMarshall::demarshallInvalidByteArray_data()
{
    addBasicTypesColumns();

    // None of the basic types should demarshall to a QByteArray
    basicNumericTypes_data();
    basicStringTypes_data();

    // Arrays of other types than byte should not demarshall to a QByteArray
    QList<bool> bools;
    QTest::newRow("empty array of bool") << QVariant::fromValue(bools);
    bools << true << false << true;
    QTest::newRow("non-empty array of bool") << QVariant::fromValue(bools);

    // Structures should not demarshall to a QByteArray
    QTest::newRow("struct of bytes")
            << QVariant::fromValue(QVariantList() << uchar(1) << uchar(2));

    QTest::newRow("struct of mixed types")
            << QVariant::fromValue(QVariantList() << int(42) << QString("foo") << double(3.14));
}

void tst_QDBusMarshall::demarshallInvalidByteArray()
{
    QFETCH(QVariant, value);

    QDBusConnection con = QDBusConnection::sessionBus();

    QVERIFY(con.isConnected());

    QDBusMessage msg = QDBusMessage::createMethodCall(serviceName, objectPath,
                                                      interfaceName, "ping");
    QDBusArgument sendArg;
    sendArg.beginStructure();
    sendArg.appendVariant(value);
    sendArg.endStructure();
    msg.setArguments(QVariantList() << QVariant::fromValue(sendArg));
    QDBusMessage reply = con.call(msg);

    const QDBusArgument receiveArg = qvariant_cast<QDBusArgument>(reply.arguments().at(0));
    receiveArg.beginStructure();

    QByteArray receiveValue;
    receiveArg >> receiveValue;
    QCOMPARE(receiveValue, QByteArray());

    receiveArg.endStructure();
    QVERIFY(receiveArg.atEnd());
}

QTEST_MAIN(tst_QDBusMarshall)
#include "tst_qdbusmarshall.moc"
