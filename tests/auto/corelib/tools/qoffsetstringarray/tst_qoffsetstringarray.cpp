/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <private/qoffsetstringarray_p.h>


class tst_QOffsetStringArray : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void access();
};


constexpr const auto messages = qOffsetStringArray(
    "level - 0",
    "level - 1",
    "level - 2",
    "level - 3",
    "level - 4",
    ""
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
    static_assert(messages.sizeString == 51, "message.sizeString");
    static_assert(messages.sizeOffsets == 6, "message.sizeOffsets");
    static_assert(std::is_same<decltype(messages)::Type, quint8>::value, "messages::Type != quint8");

    static_assert(messages257.sizeOffsets == 257, "messages257.sizeOffsets");
    static_assert(messages257.sizeString == 260, "messages257.sizeString");
    static_assert(std::is_same<decltype(messages257)::Type, quint16>::value,
                  "messages257::Type != quint16");

    static_assert(messagesBigOffsets.sizeOffsets == 4, "messagesBigOffsets.sizeOffsets");
    static_assert(messagesBigOffsets.sizeString == 364, "messagesBigOffsets.sizeString");
    static_assert(std::is_same<decltype(messagesBigOffsets)::Type, quint16>::value,
                  "messagesBigOffsets::Type != quint16");
}

void tst_QOffsetStringArray::access()
{
    QCOMPARE(messages[0], "level - 0");
    QCOMPARE(messages[1], "level - 1");
    QCOMPARE(messages[2], "level - 2");
    QCOMPARE(messages[3], "level - 3");
    QCOMPARE(messages[4], "level - 4");
    QCOMPARE(messages[5], "");
    QCOMPARE(messages[6], "");
}


QTEST_APPLESS_MAIN(tst_QOffsetStringArray)
#include "tst_qoffsetstringarray.moc"
