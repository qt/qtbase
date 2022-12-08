// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    static_assert(messages.m_string.size() == 50);
    static_assert(messages.m_offsets.size() == 6);
    static_assert(std::is_same_v<decltype(messages.m_offsets)::value_type, quint8>);

    static_assert(messages257.m_offsets.size() == 258);
    static_assert(messages257.m_string.size() == 260);
    static_assert(std::is_same_v<decltype(messages257.m_offsets)::value_type, quint16>);

    static_assert(messagesBigOffsets.m_offsets.size() == 5);
    static_assert(messagesBigOffsets.m_string.size() == 364);
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
}

QTEST_APPLESS_MAIN(tst_QOffsetStringArray)
#include "tst_qoffsetstringarray.moc"
