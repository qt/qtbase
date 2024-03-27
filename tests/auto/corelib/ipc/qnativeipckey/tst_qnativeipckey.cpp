// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QNativeIpcKey>
#include <QtTest/QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>

#include "../ipctestcommon.h"

#if QT_CONFIG(sharedmemory)
#  include <qsharedmemory.h>
#endif
#if QT_CONFIG(systemsemaphore)
#  include <qsystemsemaphore.h>
#endif

#if QT_CONFIG(sharedmemory)
static const auto makeLegacyKey = QSharedMemory::legacyNativeKey;
#else
static const auto makeLegacyKey = QSystemSemaphore::legacyNativeKey;
#endif

using namespace Qt::StringLiterals;

class tst_QNativeIpcKey : public QObject
{
    Q_OBJECT
private slots:
    void compareCompiles();
    void defaultTypes();
    void construct();
    void getSetCheck();
    void equality();
    void hash();
    void swap();
    void toString_data();
    void toString();
    void fromString_data();
    void fromString();
    void legacyKeys_data();
    void legacyKeys();
};

void tst_QNativeIpcKey::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QNativeIpcKey>();
}

void tst_QNativeIpcKey::defaultTypes()
{
    auto isKnown = [](QNativeIpcKey::Type t) {
        switch (t) {
        case QNativeIpcKey::Type::SystemV:
        case QNativeIpcKey::Type::PosixRealtime:
        case QNativeIpcKey::Type::Windows:
            return true;
        }
        return false;
    };

    // because the letter Q looked nice in Håvard's Emacs font back in the 1990s
    static_assert(qToUnderlying(QNativeIpcKey::Type::SystemV) == 'Q',
            "QNativeIpcKey::Type::SystemV must be equal to the letter Q");

    auto type = QNativeIpcKey::DefaultTypeForOs;
    auto legacy = QNativeIpcKey::legacyDefaultTypeForOs();
    QVERIFY(isKnown(type));
    QVERIFY(isKnown(legacy));

#ifdef Q_OS_WIN
    QCOMPARE(type, QNativeIpcKey::Type::Windows);
#else
    QCOMPARE(type, QNativeIpcKey::Type::PosixRealtime);
#endif

#if defined(Q_OS_WIN)
    QCOMPARE(legacy, QNativeIpcKey::Type::Windows);
#elif defined(QT_POSIX_IPC)
    QCOMPARE(legacy, QNativeIpcKey::Type::PosixRealtime);
#elif !defined(Q_OS_DARWIN)
    QCOMPARE(legacy, QNativeIpcKey::Type::SystemV);
#endif
}

void tst_QNativeIpcKey::construct()
{
    {
        QNativeIpcKey invalid(QNativeIpcKey::Type{});
        QVERIFY(!invalid.isValid());
    }

    {
        QNativeIpcKey key;
        QVERIFY(key.nativeKey().isEmpty());
        QVERIFY(key.isEmpty());
        QVERIFY(key.isValid());
        QCOMPARE(key.type(), QNativeIpcKey::DefaultTypeForOs);

        QNativeIpcKey copy(key);
        QVERIFY(copy.nativeKey().isEmpty());
        QVERIFY(copy.isEmpty());
        QVERIFY(copy.isValid());
        QCOMPARE(copy.type(), QNativeIpcKey::DefaultTypeForOs);

        QNativeIpcKey moved(std::move(copy));
        QVERIFY(moved.nativeKey().isEmpty());
        QVERIFY(moved.isEmpty());
        QVERIFY(moved.isValid());
        QCOMPARE(moved.type(), QNativeIpcKey::DefaultTypeForOs);

        key.setType({});
        key.setNativeKey("something else");
        key = std::move(moved);
        QVERIFY(key.nativeKey().isEmpty());
        QVERIFY(key.isEmpty());
        QVERIFY(key.isValid());
        QCOMPARE(key.type(), QNativeIpcKey::DefaultTypeForOs);

        copy.setType({});
        copy.setNativeKey("something else");
        copy = key;
        QVERIFY(copy.nativeKey().isEmpty());
        QVERIFY(copy.isEmpty());
        QVERIFY(copy.isValid());
        QCOMPARE(copy.type(), QNativeIpcKey::DefaultTypeForOs);
    }

    {
        QNativeIpcKey key("dummy");
        QCOMPARE(key.nativeKey(), "dummy");
        QVERIFY(!key.isEmpty());
        QVERIFY(key.isValid());
        QCOMPARE(key.type(), QNativeIpcKey::DefaultTypeForOs);

        QNativeIpcKey copy(key);
        QCOMPARE(key.nativeKey(), "dummy");
        QCOMPARE(copy.nativeKey(), "dummy");
        QVERIFY(!copy.isEmpty());
        QVERIFY(copy.isValid());
        QCOMPARE(copy.type(), QNativeIpcKey::DefaultTypeForOs);

        QNativeIpcKey moved(std::move(copy));
        QCOMPARE(key.nativeKey(), "dummy");
        QCOMPARE(moved.nativeKey(), "dummy");
        QVERIFY(!moved.isEmpty());
        QVERIFY(moved.isValid());
        QCOMPARE(moved.type(), QNativeIpcKey::DefaultTypeForOs);

        key.setType({});
        key.setNativeKey("something else");
        key = std::move(moved);
        QCOMPARE(key.nativeKey(), "dummy");
        QVERIFY(!key.isEmpty());
        QVERIFY(key.isValid());
        QCOMPARE(key.type(), QNativeIpcKey::DefaultTypeForOs);

        copy.setType({});
        copy.setNativeKey("something else");
        copy = key;
        QCOMPARE(key.nativeKey(), "dummy");
        QCOMPARE(copy.nativeKey(), "dummy");
        QVERIFY(!copy.isEmpty());
        QVERIFY(copy.isValid());
        QCOMPARE(copy.type(), QNativeIpcKey::DefaultTypeForOs);
    }
}

void tst_QNativeIpcKey::getSetCheck()
{
    QNativeIpcKey key("key1", QNativeIpcKey::Type::Windows);
    QVERIFY(key.isValid());
    QVERIFY(!key.isEmpty());
    QCOMPARE(key.nativeKey(), "key1");
    QCOMPARE(key.type(), QNativeIpcKey::Type::Windows);

    key.setType(QNativeIpcKey::Type::SystemV);
    QVERIFY(key.isValid());
    QVERIFY(!key.isEmpty());
    QCOMPARE(key.type(), QNativeIpcKey::Type::SystemV);

    key.setNativeKey("key2");
    QCOMPARE(key.nativeKey(), "key2");
}

void tst_QNativeIpcKey::equality()
{
    QNativeIpcKey key1, key2;
    QCOMPARE(key1, key2);
    QVERIFY(!(key1 != key2));
    QT_TEST_EQUALITY_OPS(key1, key2, true);

    key1.setNativeKey("key1");
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));
    QT_TEST_EQUALITY_OPS(key1, key2, false);

    key2.setType({});
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));
    QT_TEST_EQUALITY_OPS(key1, key2, false);

    key2.setNativeKey(key1.nativeKey());
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));
    QT_TEST_EQUALITY_OPS(key1, key2, false);

    key2.setType(QNativeIpcKey::DefaultTypeForOs);
    QCOMPARE(key1, key2);
    QVERIFY(!(key1 != key2));
    QT_TEST_EQUALITY_OPS(key1, key2, true);

    key1 = makeLegacyKey("key1", QNativeIpcKey::DefaultTypeForOs);
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));
    QT_TEST_EQUALITY_OPS(key1, key2, false);

    key2 = key1;
    QCOMPARE(key1, key2);
    QVERIFY(!(key1 != key2));
    QT_TEST_EQUALITY_OPS(key1, key2, true);

    // just setting the native key won't make them equal again!
    key2.setNativeKey(key1.nativeKey());
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));
    QT_TEST_EQUALITY_OPS(key1, key2, false);
}

void tst_QNativeIpcKey::hash()
{
    QNativeIpcKey key1("key1", QNativeIpcKey::DefaultTypeForOs);
    QNativeIpcKey key2(key1);
    QCOMPARE_EQ(qHash(key1), qHash(key2));
    QCOMPARE_EQ(qHash(key1, 123), qHash(key2, 123));
}

void tst_QNativeIpcKey::swap()
{
    QNativeIpcKey key1("key1", QNativeIpcKey::Type::PosixRealtime);
    QNativeIpcKey key2("key2", QNativeIpcKey::Type::Windows);

    // self-swaps
    key1.swap(key1);
    key2.swap(key2);
    QCOMPARE(key1.nativeKey(), "key1");
    QCOMPARE(key1.type(), QNativeIpcKey::Type::PosixRealtime);
    QCOMPARE(key2.nativeKey(), "key2");
    QCOMPARE(key2.type(), QNativeIpcKey::Type::Windows);

    key1.swap(key2);
    QCOMPARE(key2.nativeKey(), "key1");
    QCOMPARE(key2.type(), QNativeIpcKey::Type::PosixRealtime);
    QCOMPARE(key1.nativeKey(), "key2");
    QCOMPARE(key1.type(), QNativeIpcKey::Type::Windows);

    key1.swap(key2);
    QCOMPARE(key1.nativeKey(), "key1");
    QCOMPARE(key1.type(), QNativeIpcKey::Type::PosixRealtime);
    QCOMPARE(key2.nativeKey(), "key2");
    QCOMPARE(key2.type(), QNativeIpcKey::Type::Windows);

    key1 = makeLegacyKey("key1", QNativeIpcKey::DefaultTypeForOs);
    QCOMPARE(key1.type(), QNativeIpcKey::DefaultTypeForOs);
    key1.swap(key2);
    QCOMPARE(key1.type(), QNativeIpcKey::Type::Windows);
    QCOMPARE(key2.type(), QNativeIpcKey::DefaultTypeForOs);
}

void tst_QNativeIpcKey::toString_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QNativeIpcKey>("key");

    QTest::newRow("invalid") << QString() << QNativeIpcKey(QNativeIpcKey::Type(0));

    auto addRow = [](const char *prefix, QNativeIpcKey::Type type) {
        auto add = [=](const char *name, QLatin1StringView key, QLatin1StringView encoded = {}) {
            if (encoded.isNull())
                encoded = key;
            QTest::addRow("%s-%s", prefix, name)
                    << prefix + u":"_s + encoded << QNativeIpcKey(key, type);
        };
        add("empty", {});
        add("text", "foobar"_L1);
        add("pathlike", "/sometext"_L1);
        add("objectlike", "Global\\sometext"_L1);
        add("colon-slash", ":/"_L1);
        add("slash-colon", "/:"_L1);
        add("percent", "%"_L1, "%25"_L1);
        add("question-hash", "?#"_L1, "%3F%23"_L1);
        add("hash-question", "#?"_L1, "%23%3F"_L1);
        add("double-slash", "//"_L1, "/%2F"_L1);
        add("triple-slash", "///"_L1, "/%2F/"_L1);
        add("non-ascii", "\xe9"_L1);
        add("non-utf8", "\xa0\xff"_L1);
        QTest::addRow("%s-%s", prefix, "non-latin1")
                << prefix + u":\u0100.\u2000.\U00010000"_s
                << QNativeIpcKey(u"\u0100.\u2000.\U00010000"_s, type);
    };
    addRow("systemv", QNativeIpcKey::Type::SystemV);
    addRow("posix", QNativeIpcKey::Type::PosixRealtime);
    addRow("windows", QNativeIpcKey::Type::Windows);

    addRow("systemv-1", QNativeIpcKey::Type(1));
    addRow("systemv-84", QNativeIpcKey::Type('T'));
    addRow("systemv-255", QNativeIpcKey::Type(0xff));
}

void tst_QNativeIpcKey::toString()
{
    QFETCH(QString, string);
    QFETCH(QNativeIpcKey, key);

    QCOMPARE(key.toString(), string);
}

void tst_QNativeIpcKey::fromString_data()
{
    toString_data();
    QTest::addRow("systemv-alias") << "systemv-81:" << QNativeIpcKey(QNativeIpcKey::Type::SystemV);
    QTest::addRow("systemv-zeropadded") << "systemv-009:" << QNativeIpcKey(QNativeIpcKey::Type(9));

    QNativeIpcKey valid("/foo", QNativeIpcKey::Type::PosixRealtime);
    QNativeIpcKey invalid(QNativeIpcKey::Type(0));

    // percent-decoding
    QTest::addRow("percent-encoded") << "posix:%2f%66o%6f" << valid;
    QTest::addRow("percent-utf8")
            << "posix:%C4%80.%E2%80%80.%F0%90%80%80"
            << QNativeIpcKey(u"\u0100.\u2000.\U00010000"_s, QNativeIpcKey::Type::PosixRealtime);

    // fragments are ignored
    QTest::addRow("with-fragment") << "posix:/foo#bar" << valid;
    QTest::addRow("with-fragmentquery") << "posix:/foo#bar?baz" << valid;

    // but unknown query items are not
    QTest::addRow("with-query") << "posix:/foo?bar" << invalid;
    QTest::addRow("with-queryfragment") << "posix:/foo?bar#baz" << invalid;

    // add some ones that won't parse well
    QTest::addRow("positive-number") << "81" << invalid;
    QTest::addRow("negative-number") << "-81" << invalid;
    QTest::addRow("invalidprefix") << "invalidprefix:" << invalid;
    QTest::addRow("systemv-nodash") << "systemv255" << invalid;
    QTest::addRow("systemv-doubledash") << "systemv--255:" << invalid;
    QTest::addRow("systemv-plus") << "systemv+255" << invalid;
    QTest::addRow("systemv-hex") << "systemv-0x01:" << invalid;
    QTest::addRow("systemv-too-low") << "systemv-0:" << invalid;
    QTest::addRow("systemv-too-high") << "systemv-256" << invalid;
    QTest::addRow("systemv-overflow-15bit") << "systemv-32769:" << invalid;
    QTest::addRow("systemv-overflow-16bit") << "systemv-65537:" << invalid;
    QTest::addRow("systemv-overflow-31bit") << "systemv-2147483649:" << invalid;
    QTest::addRow("systemv-overflow-32bit") << "systemv-4294967297:" << invalid;

    auto addRows = [=](const char *name) {
        QTest::addRow("%s-nocolon", name) << name << invalid;
        QTest::addRow("%s-junk", name) << name + u"junk"_s << invalid;
        QTest::addRow("junk-%s-colon", name) << u"junk:"_s + name + u':' << invalid;
    };
    addRows("systemv");
    addRows("posix");
    addRows("windows");
    addRows("systemv-1");
    addRows("systemv-255");
}

void tst_QNativeIpcKey::fromString()
{
    QFETCH(QString, string);
    QFETCH(QNativeIpcKey, key);

    QCOMPARE(QNativeIpcKey::fromString(string), key);
}

void tst_QNativeIpcKey::legacyKeys_data()
{
    QTest::addColumn<QNativeIpcKey::Type>("type");
    QTest::addColumn<QString>("legacyKey");
    auto addRows = [](QNativeIpcKey::Type type) {
        const char *label = "<unknown-type>";
        switch (type) {
        case QNativeIpcKey::Type::SystemV:
            label = "systemv";
            break;
        case QNativeIpcKey::Type::PosixRealtime:
            label = "posix";
            break;
        case QNativeIpcKey::Type::Windows:
            label = "windows";
            break;
        }
        auto add = [=](const char *name, const QString &legacyKey) {
            QTest::addRow("%s-%s", label, name) << type << legacyKey;
        };
        add("empty", {});
        add("text", "foobar"_L1);
        add("pathlike", "/sometext"_L1);
        add("objectlike", "Global\\sometext"_L1);
        add("colon-slash", ":/"_L1);
        add("slash-colon", "/:"_L1);
        add("percent", "%"_L1);
        add("question-hash", "?#"_L1);
        add("hash-question", "#?"_L1);
        add("double-slash", "//"_L1);
        add("triple-slash", "///"_L1);
        add("non-ascii", "\xe9"_L1);
        add("non-utf8", "\xa0\xff"_L1);
        add("non-latin1", u":\u0100.\u2000.\U00010000"_s);
    };

    addRows(QNativeIpcKey::DefaultTypeForOs);
    if (auto type = QNativeIpcKey::legacyDefaultTypeForOs();
            type != QNativeIpcKey::DefaultTypeForOs)
        addRows(type);
}

void tst_QNativeIpcKey::legacyKeys()
{
    QFETCH(QNativeIpcKey::Type, type);
    QFETCH(QString, legacyKey);

    QNativeIpcKey key = makeLegacyKey(legacyKey, type);
    QCOMPARE(key.type(), type);

    QString string = key.toString();
    QNativeIpcKey key2 = QNativeIpcKey::fromString(string);
    QCOMPARE(key2, key);
    QT_TEST_EQUALITY_OPS(key, key2, true);

    if (!legacyKey.isEmpty()) {
        // confirm it shows up in the encoded form
        Q_ASSERT(!legacyKey.contains(u'&'));    // needs extra encoding
        QUrl u;
        u.setQuery("legacyKey="_L1 + legacyKey, QUrl::DecodedMode);
        QString encodedLegacyKey = u.toString(QUrl::RemoveScheme | QUrl::RemoveAuthority
                                              | QUrl::DecodeReserved);
        QVERIFY2(string.contains(encodedLegacyKey), qPrintable(string));
    }
}

QTEST_MAIN(tst_QNativeIpcKey)
#include "tst_qnativeipckey.moc"
