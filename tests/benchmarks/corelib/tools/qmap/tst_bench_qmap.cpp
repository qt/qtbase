// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QFile>
#include <QMap>
#include <QString>
#include <QTest>
#include <qdebug.h>


class tst_QMap : public QObject
{
    Q_OBJECT

private slots:
    void insertion_int_int();
    void insertion_int_string();
    void insertion_string_int();

    void lookup_int_int();
    void lookup_int_string();
    void lookup_string_int();

    void iteration();
    void toStdMap();
    void iterator_begin();

    void ctorStdMap();

    void insertion_int_intx();
    void insertion_int_int_with_hint1();
    void insertion_int_int2();
    void insertion_int_int_with_hint2();

    void insertion_string_int2();
    void insertion_string_int2_hint();

    void insertMap();

private:
    QStringList helloEachWorld(int count);
};

QStringList tst_QMap::helloEachWorld(int count)
{
    QStringList result;
    result.reserve(count);
    result << QStringLiteral("Hello World"); // at index 0, not used

    char16_t name[] = u"Hello    World";
    QStringView str(name);
    for (int i = 1; i < count; ++i) {
        auto p = name + 6; // In the gap between words.
        for (const auto ch : QChar::fromUcs4(i))
            p++[0] = ch;
        result << str.toString();
    }
    return result;
}

constexpr int huge = 100000; // one hundred thousand; simple integral data tests
// Sum of i with 0 <= i < huge; overflows, but that's OK as long as it's unsigned:
constexpr uint hugeSum = (uint(huge) / 2) * uint(huge - 1);
constexpr int bigish = 5000; // five thousand; tests using XString's expensive <

void tst_QMap::insertion_int_int()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 0; i < huge; ++i)
            map.insert(i, i);
    }
    QCOMPARE(map.size(), qsizetype(huge));
}

void tst_QMap::insertion_int_intx()
{
    // This is the same test - but executed later.
    // The results in the beginning of the test seems to be a somewhat inaccurate.
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 0; i < huge; ++i)
            map.insert(i, i);
    }
    QCOMPARE(map.size(), qsizetype(huge));
}

void tst_QMap::insertion_int_int_with_hint1()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 0; i < huge; ++i)
            map.insert(map.constEnd(), i, i);
    }
    QCOMPARE(map.size(), qsizetype(huge));
}

void tst_QMap::insertion_int_int2()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = huge; i >= 0; --i)
            map.insert(i, i);
    }
    QCOMPARE(map.size(), qsizetype(huge) + 1);
}

void tst_QMap::insertion_int_int_with_hint2()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = huge; i >= 0; --i)
            map.insert(map.constBegin(), i, i);
    }
    QCOMPARE(map.size(), qsizetype(huge) + 1);
}

void tst_QMap::insertion_int_string()
{
    QMap<int, QString> map;
    QString str("Hello World");
    QBENCHMARK {
        for (int i = 0; i < huge; ++i)
            map.insert(i, str);
    }
    QCOMPARE(map.size(), qsizetype(huge));
}

void tst_QMap::insertion_string_int()
{
    QMap<QString, int> map;
    const QStringList names = helloEachWorld(huge);
    QCOMPARE(names.size(), qsizetype(huge));
    QBENCHMARK {
        for (int i = 1; i < huge; ++i)
            map.insert(names.at(i), i);
    }
    QCOMPARE(map.size() + 1, qsizetype(huge));
}

void tst_QMap::lookup_int_int()
{
    QMap<int, int> map;
    for (int i = 0; i < huge; ++i)
        map.insert(i, i);
    QCOMPARE(map.size(), qsizetype(huge));

    uint sum = 0, count = 0;
    QBENCHMARK {
        for (int i = 0; i < huge; ++i)
             sum += map.value(i);
        ++count;
    }
    QCOMPARE(sum, hugeSum * count);
}

void tst_QMap::lookup_int_string()
{
    QMap<int, QString> map;
    QString str("Hello World");
    for (int i = 0; i < huge; ++i)
        map.insert(i, str);
    QCOMPARE(map.size(), qsizetype(huge));

    QBENCHMARK {
        for (int i = 0; i < huge; ++i)
             str = map.value(i);
    }
}

void tst_QMap::lookup_string_int()
{
    QMap<QString, int> map;
    const QStringList names = helloEachWorld(huge);
    for (int i = 1; i < huge; ++i)
        map.insert(names.at(i), i);
    QCOMPARE(map.size() + 1, qsizetype(huge));

    uint sum = 0, count = 0;
    QBENCHMARK {
        for (int i = 1; i < huge; ++i)
            sum += map.value(names.at(i));
        ++count;
    }
    QCOMPARE(sum, hugeSum * count);
}

// iteration speed doesn't depend on the type of the map.
void tst_QMap::iteration()
{
    QMap<int, int> map;
    for (int i = 0; i < huge; ++i)
        map.insert(i, i);
    QCOMPARE(map.size(), qsizetype(huge));

    uint sum = 0, count = 0;
    QBENCHMARK {
        for (int i = 0; i < 100; ++i) {
            QMap<int, int>::const_iterator it = map.constBegin();
            QMap<int, int>::const_iterator end = map.constEnd();
            while (it != end) {
                sum += *it;
                ++it;
            }
        }
        ++count;
    }
    QCOMPARE(sum, hugeSum * 100u * count);
}

void tst_QMap::toStdMap()
{
    QMap<int, int> map;
    for (int i = 0; i < huge; ++i)
        map.insert(i, i);
    QCOMPARE(map.size(), qsizetype(huge));

    QBENCHMARK {
        std::map<int, int> n = map.toStdMap();
        n.begin();
    }
}

void tst_QMap::iterator_begin()
{
    QMap<int, int> map;
    for (int i = 0; i < huge; ++i)
        map.insert(i, i);
    QCOMPARE(map.size(), qsizetype(huge));

    QBENCHMARK {
        for (int i = 0; i < huge; ++i) {
            QMap<int, int>::const_iterator it = map.constBegin();
            QMap<int, int>::const_iterator end = map.constEnd();
            if (it == end) // same as if (false)
                ++it;
        }
    }
}

void tst_QMap::ctorStdMap()
{
    std::map<int, int> map;
    for (int i = 0; i < huge; ++i)
        map.insert(std::pair<int, int>(i, i));
    QCOMPARE(map.size(), size_t(huge));

    QBENCHMARK {
        QMap<int, int> qmap(map);
        qmap.constBegin();
    }
}

class XString : public QString
{
public:
    bool operator < (const XString& x) const // an expensive operator <
    {
        return toInt() < x.toInt();
    }
};

void tst_QMap::insertion_string_int2()
{
    QMap<XString, int> map;
    QBENCHMARK {
        for (int i = 1; i < bigish; ++i) {
            XString str;
            str.setNum(i);
            map.insert(str, i);
        }
    }
    QCOMPARE(map.size() + 1, qsizetype(bigish));
}

void tst_QMap::insertion_string_int2_hint()
{
    QMap<XString, int> map;
    QBENCHMARK {
        for (int i = 1; i < bigish; ++i) {
            XString str;
            str.setNum(i);
            map.insert(map.end(), str, i);
        }
    }
    QCOMPARE(map.size() + 1, qsizetype(bigish));
}

void tst_QMap::insertMap()
{
    QMap<int, int> map;
    for (int i = 0; i < huge; ++i)
        map.insert(i * 4, 0);
    QMap<int, int> map2;
    for (int i = 0; i < huge / 2; ++i)
        map2.insert(i * 7, 0);
    QBENCHMARK_ONCE {
        map.insert(map2);
    }
}

QTEST_MAIN(tst_QMap)

#include "tst_bench_qmap.moc"
