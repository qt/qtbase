// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QStringList>
#include <QTest>

#include <sstream>
#include <string>
#include <vector>

using namespace Qt::StringLiterals;

class tst_QStringList: public QObject
{
    Q_OBJECT

private slots:
    void join() const;
    void join_data() const;

    void removeDuplicates() const;
    void removeDuplicates_data() const;

    void filter_data() const;
    void filter() const;
    void filter_stringMatcher_data() const { filter_data(); }
    void filter_stringMatcher() const;

    void split_qlist_qbytearray() const;
    void split_qlist_qbytearray_data() const { return split_data(); }

    void split_data() const;
    void split_qlist_qstring() const;
    void split_qlist_qstring_data() const { return split_data(); }

    void split_stdvector_stdstring() const;
    void split_stdvector_stdstring_data() const { return split_data(); }

    void split_stdvector_stdwstring() const;
    void split_stdvector_stdwstring_data() const { return split_data(); }

    void split_stdlist_stdstring() const;
    void split_stdlist_stdstring_data() const { return split_data(); }

private:
    static QStringList populateList(const int count, const QString &unit);
    static QString populateString(const int count, const QString &unit);
};

QStringList tst_QStringList::populateList(const int count, const QString &unit)
{
    QStringList retval;
    retval.reserve(count);
    for (int i = 0; i < count; ++i)
        retval.append(unit + QString::number(i));

    return retval;
}

QString tst_QStringList::populateString(const int count, const QString &unit)
{
    QString retval;

    for (int i = 0; i < count; ++i) {
        retval.append(unit);
        retval.append(QLatin1Char(':'));
    }

    return retval;
}

void tst_QStringList::join() const
{
    QFETCH(QStringList, input);
    QFETCH(QString, separator);

    QBENCHMARK {
        [[maybe_unused]] auto r = input.join(separator);
    }
}

void tst_QStringList::join_data() const
{
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QString>("separator");

    QTest::newRow("100")
        << populateList(100, QLatin1String("unit"))
        << QString();

    QTest::newRow("1000")
        << populateList(1000, QLatin1String("unit"))
        << QString();

    QTest::newRow("10000")
        << populateList(10'000, QLatin1String("unit"))
        << QString();

    QTest::newRow("100000")
        << populateList(100'000, QLatin1String("unit"))
        << QString();
}

void tst_QStringList::removeDuplicates() const
{
    QFETCH(const QStringList, input);

    QBENCHMARK {
        auto copy = input;
        copy.removeDuplicates();
    }
}

void tst_QStringList::removeDuplicates_data() const
{
    QTest::addColumn<QStringList>("input");

    const QStringList s = {"one", "two", "three"};

    QTest::addRow("empty") << QStringList();
    QTest::addRow("short-dup-0.00") << s;
    QTest::addRow("short-dup-0.50") << (s + s);
    QTest::addRow("short-dup-0.66") << (s + s + s);
    QTest::addRow("short-dup-0.75") << (s + s + s + s);

    const QStringList l = []() {
        QStringList result;
        const int n = 1000;
        result.reserve(n);
        for (int i = 0; i < n; ++i)
            result.push_back(QString::number(i));
        return result;
    }();
    QTest::addRow("long-dup-0.00") << l;
    QTest::addRow("long-dup-0.50") << (l + l);
    QTest::addRow("long-dup-0.66") << (l + l + l);
    QTest::addRow("long-dup-0.75") << (l + l + l + l);
}

void tst_QStringList::filter_data() const
{
    QTest::addColumn<QStringList>("list");
    QTest::addColumn<QStringList>("expected");

    for (int i : {10, 20, 30, 40, 50, 70, 80, 100, 300, 500, 700, 900, 10'000}) {
        QStringList list = populateList(i, u"A rather long string to test QStringMatcher"_s);
        list.append(u"Horse and cart from old"_s);
        QTest::addRow("list%d", i) << list << QStringList(u"Horse and cart from old"_s);
    }
}

void tst_QStringList::filter() const
{
    QFETCH(QStringList, list);
    QFETCH(QStringList, expected);

    QBENCHMARK {
        QCOMPARE(list.filter(u"Horse and cart from old", Qt::CaseSensitive), expected);
    }
}

void tst_QStringList::filter_stringMatcher() const
{
    QFETCH(QStringList, list);
    QFETCH(QStringList, expected);

    const QStringMatcher matcher(u"Horse and cart from old", Qt::CaseSensitive);
    QBENCHMARK {
        QCOMPARE(list.filter(matcher), expected);
    }
}

void tst_QStringList::split_data() const
{
    QTest::addColumn<QString>("input");
    QString unit = QLatin1String("unit") + QString(100, QLatin1Char('s'));
    //QTest::newRow("") << populateString(10, unit);
    QTest::newRow("") << populateString(100, unit);
    //QTest::newRow("") << populateString(100, unit);
    //QTest::newRow("") << populateString(1000, unit);
    //QTest::newRow("") << populateString(10000, unit);
}

void tst_QStringList::split_qlist_qbytearray() const
{
    QFETCH(QString, input);
    const char splitChar = ':';
    QByteArray ba = input.toLatin1();

    QBENCHMARK {
        [[maybe_unused]] auto r = ba.split(splitChar);
    }
}

void tst_QStringList::split_qlist_qstring() const
{
    QFETCH(QString, input);
    const QChar splitChar = ':';

    QBENCHMARK {
        [[maybe_unused]] auto r = input.split(splitChar);
    }
}

void tst_QStringList::split_stdvector_stdstring() const
{
    QFETCH(QString, input);
    const char split_char = ':';
    std::string stdinput = input.toStdString();

    QBENCHMARK {
        std::istringstream split(stdinput);
        std::vector<std::string> token;
        for (std::string each;
             std::getline(split, each, split_char);
             token.push_back(each))
            ;
    }
}

void tst_QStringList::split_stdvector_stdwstring() const
{
    QFETCH(QString, input);
    const wchar_t split_char = ':';
    std::wstring stdinput = input.toStdWString();

    QBENCHMARK {
        std::wistringstream split(stdinput);
        std::vector<std::wstring> token;
        for (std::wstring each;
             std::getline(split, each, split_char);
             token.push_back(each))
            ;
    }
}

void tst_QStringList::split_stdlist_stdstring() const
{
    QFETCH(QString, input);
    const char split_char = ':';
    std::string stdinput = input.toStdString();

    QBENCHMARK {
        std::istringstream split(stdinput);
        std::list<std::string> token;
        for (std::string each;
             std::getline(split, each, split_char);
             token.push_back(each))
            ;
    }
}

QTEST_MAIN(tst_QStringList)

#include "tst_bench_qstringlist.moc"
