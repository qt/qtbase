// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Testing qtmochelpers.h is probably pointless... if there's a problem with it
// then you most likely can't compile this test in the first place.
#include <QtCore/qtmochelpers.h>

#include <QTest>

#include <QtCore/qobject.h>

#include <initializer_list>

class tst_MocHelpers : public QObject
{
    Q_OBJECT
private slots:
    void stringData();
};

template <int Count, size_t StringSize>
void verifyStringData(const QtMocHelpers::StringData<Count, StringSize> &data,
                      std::initializer_list<const char *> strings)
{
    QCOMPARE(std::size(strings), size_t(Count) / 2);
    ptrdiff_t i = 0;
    for (const char *str : strings) {
        uint offset = data.offsetsAndSizes[i++] - sizeof(data.offsetsAndSizes);
        uint len = data.offsetsAndSizes[i++];
        QByteArrayView result(data.stringdata0 + offset, len);

        QCOMPARE(len, strlen(str));
        QCOMPARE(result, str);
    }
}

void tst_MocHelpers::stringData()
{
#define CHECK(...)  \
    verifyStringData(QtMocHelpers::stringData(__VA_ARGS__), { __VA_ARGS__ })

    QTest::setThrowOnFail(true);
    CHECK("Hello");
    CHECK("Hello", "World");
    CHECK("Hello", "", "World");
#undef CHECK
}

QTEST_MAIN(tst_MocHelpers)
#include "tst_mochelpers.moc"
