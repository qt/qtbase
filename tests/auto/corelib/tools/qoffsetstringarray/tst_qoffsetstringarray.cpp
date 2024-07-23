// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <private/qoffsetstringarray_p.h>


class tst_QOffsetStringArray : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void access();
    void contains();
};


constexpr const auto messages = qOffsetStringArray(
    "level - 0",
    "level - 1",
    "level - 2",
    "level - 3",
    "level - 4"
);

// When compiled with C++20, this is using the native char8_t
constexpr auto utf8Messages = qOffsetStringArray(
    u8"level - 0",
    u8"level - 1",
    u8"level - 2",
    u8"level - 3",
    u8"level - 4"
);

constexpr auto utf16Messages = qOffsetStringArray(
    u"level - 0",
    u"level - 1",
    u"level - 2",
    u"level - 3",
    u"level - 4"
);

constexpr const auto messages257 = qOffsetStringArray(
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",

    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",

    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "end"
);

constexpr const auto messagesBigOffsets = qOffsetStringArray(
    "        10        20        30        40        50        60        70        80        90",
    "        10        20        30        40        50        60        70        80        90",
    "        10        20        30        40        50        60        70        80        90",
    "        10        20        30        40        50        60        70        80        90"
);

void tst_QOffsetStringArray::init()
{
    static_assert(messages.m_string.size() == 51);
    static_assert(messages.m_offsets.size() == 6);
    static_assert(std::is_same_v<decltype(messages.m_offsets)::value_type, quint8>);

    static_assert(utf8Messages.m_string.size() == 51);
    static_assert(utf8Messages.m_offsets.size() == 6);
    static_assert(std::is_same_v<decltype(utf8Messages.m_offsets)::value_type, quint8>);

    static_assert(utf16Messages.m_string.size() == 51);
    static_assert(utf16Messages.m_offsets.size() == 6);
    static_assert(std::is_same_v<decltype(utf16Messages.m_offsets)::value_type, quint8>);

    static_assert(messages257.m_offsets.size() == 258);
    static_assert(messages257.m_string.size() == 261);
    static_assert(std::is_same_v<decltype(messages257.m_offsets)::value_type, quint16>);

    static_assert(messagesBigOffsets.m_offsets.size() == 5);
    static_assert(messagesBigOffsets.m_string.size() == 365);
    static_assert(std::is_same_v<decltype(messagesBigOffsets.m_offsets)::value_type, quint16>);
}

void tst_QOffsetStringArray::access()
{
    QCOMPARE(messages[0], "level - 0");
    QCOMPARE(messages[1], "level - 1");
    QCOMPARE(messages[2], "level - 2");
    QCOMPARE(messages[3], "level - 3");
    QCOMPARE(messages[4], "level - 4");
    // out of bounds returns empty strings:
    QCOMPARE(messages[5], "");
    QCOMPARE(messages[6], "");

    auto view0 = messages.viewAt(0);
    static_assert(std::is_same_v<decltype(view0), QByteArrayView>);
    QCOMPARE(view0, "level - 0");

    QCOMPARE(utf8Messages[0], QUtf8StringView(u8"level - 0"));
    QCOMPARE(utf8Messages[1], QUtf8StringView(u8"level - 1"));
    QCOMPARE(utf8Messages[2], QUtf8StringView(u8"level - 2"));
    QCOMPARE(utf8Messages[3], QUtf8StringView(u8"level - 3"));
    QCOMPARE(utf8Messages[4], QUtf8StringView(u8"level - 4"));
    QCOMPARE(utf8Messages[5], QUtf8StringView(u8""));
    QCOMPARE(utf8Messages[6], QUtf8StringView(u8""));

    auto u8view0 = utf8Messages.viewAt(0);
#ifdef __cpp_char8_t
    static_assert(std::is_same_v<decltype(u8view0), QUtf8StringView>);
#endif
    QCOMPARE(u8view0, u8"level - 0");
    QCOMPARE(utf8Messages.viewAt(1), u8"level - 1");
    QCOMPARE(utf8Messages.viewAt(2), u8"level - 2");
    QCOMPARE(utf8Messages.viewAt(3), u8"level - 3");
    QCOMPARE(utf8Messages.viewAt(4), u8"level - 4");
    // viewAt has no size checking!

    QCOMPARE(utf16Messages[0], QStringView(u"level - 0"));
    QCOMPARE(utf16Messages[1], QStringView(u"level - 1"));
    QCOMPARE(utf16Messages[2], QStringView(u"level - 2"));
    QCOMPARE(utf16Messages[3], QStringView(u"level - 3"));
    QCOMPARE(utf16Messages[4], QStringView(u"level - 4"));
    QCOMPARE(utf16Messages[5], QStringView(u""));
    QCOMPARE(utf16Messages[6], QStringView(u""));

    auto uview0 = utf16Messages.viewAt(0);
    static_assert(std::is_same_v<decltype(uview0), QStringView>);
    QCOMPARE(uview0, u"level - 0");
    QCOMPARE(utf16Messages.viewAt(1), u"level - 1");
    QCOMPARE(utf16Messages.viewAt(2), u"level - 2");
    QCOMPARE(utf16Messages.viewAt(3), u"level - 3");
    QCOMPARE(utf16Messages.viewAt(4), u"level - 4");
}

void tst_QOffsetStringArray::contains()
{
    QVERIFY(!messages.contains(""));
    QVERIFY( messages.contains("level - 0"));
    std::string l2 = "level - 2"; // make sure we don't compare pointer values
    QVERIFY( messages.contains(l2));
    QByteArray L4 = "Level - 4";
    QVERIFY( messages.contains(L4, Qt::CaseInsensitive));
    QVERIFY(!messages.contains(L4, Qt::CaseSensitive));

    QVERIFY(!utf8Messages.contains(u8""));
    QVERIFY( utf8Messages.contains(u8"level - 0"));
#ifdef __cpp_lib_char8_t
    std::u8string u8l2 = u8"level - 2"; // make sure we don't compare pointer values
    QVERIFY( utf8Messages.contains(u8l2));
#endif
    QUtf8StringView u8l4 = u8"Level - 4";
    QVERIFY( utf8Messages.contains(u8l4, Qt::CaseInsensitive));
    QVERIFY(!utf8Messages.contains(u8l4, Qt::CaseSensitive));

    QVERIFY(!utf16Messages.contains(u""));
    QVERIFY( utf16Messages.contains(u"level - 0"));
    std::u16string ul2 = u"level - 2"; // make sure we don't compare pointer values
    QVERIFY( utf16Messages.contains(ul2));
    QString ul4 = "Level - 4";
    QVERIFY( utf16Messages.contains(ul4, Qt::CaseInsensitive));
    QVERIFY(!utf16Messages.contains(ul4, Qt::CaseSensitive));
}

QTEST_APPLESS_MAIN(tst_QOffsetStringArray)
#include "tst_qoffsetstringarray.moc"
