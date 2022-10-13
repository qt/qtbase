// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QNativeIpcKey>
#include <QtTest/QTest>

#include "../ipctestcommon.h"

using namespace Qt::StringLiterals;

class tst_QNativeIpcKey : public QObject
{
    Q_OBJECT
private slots:
    void defaultTypes();
    void construct();
    void getSetCheck();
    void equality();
    void swap();
    void toString_data();
    void toString();
    void fromString_data();
    void fromString();
};

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
#elif !defined(QT_POSIX_IPC)
    QCOMPARE(type, QNativeIpcKey::Type::SystemV);
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

    key1.setNativeKey("key1");
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));

    key2.setType({});
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));

    key2.setNativeKey(key1.nativeKey());
    QCOMPARE_NE(key1, key2);
    QVERIFY(!(key1 == key2));

    key2.setType(QNativeIpcKey::DefaultTypeForOs);
    QCOMPARE(key1, key2);
    QVERIFY(!(key1 != key2));
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
        add("non-ascii", "\xa0\xff"_L1);
        add("percent", "%"_L1, "%25"_L1);
        add("question-hash", "?#"_L1, "%3F%23"_L1);
        add("hash-question", "#?"_L1, "%23%3F"_L1);
        add("double-slash", "//"_L1, "/%2F"_L1);
        add("triple-slash", "///"_L1, "/%2F/"_L1);
        add("non-ascii", "/\xe9"_L1);
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

    // query and fragment are ignored
    QTest::addRow("with-query") << "posix:/foo?bar" << valid;
    QTest::addRow("with-fragment") << "posix:/foo#bar" << valid;
    QTest::addRow("with-queryfragment") << "posix:/foo?bar#baz" << valid;
    QTest::addRow("with-fragmentquery") << "posix:/foo#bar?baz" << valid;

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

QTEST_MAIN(tst_QNativeIpcKey)
#include "tst_qnativeipckey.moc"
