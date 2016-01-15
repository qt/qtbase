/****************************************************************************
**
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

#include <QtCore/QUrlQuery>
#include <QtTest/QtTest>

typedef QList<QPair<QString, QString> > QueryItems;
Q_DECLARE_METATYPE(QueryItems)
Q_DECLARE_METATYPE(QUrl::ComponentFormattingOptions)

class tst_QUrlQuery : public QObject
{
    Q_OBJECT

public:
    tst_QUrlQuery()
    {
        qRegisterMetaType<QueryItems>();
    }

private Q_SLOTS:
    void constructing();
    void addRemove();
    void multiAddRemove();
    void multiplyAddSamePair();
    void setQueryItems_data();
    void setQueryItems();
    void basicParsing_data();
    void basicParsing();
    void reconstructQuery_data();
    void reconstructQuery();
    void encodedSetQueryItems_data();
    void encodedSetQueryItems();
    void encodedParsing_data();
    void encodedParsing();
    void differentDelimiters();

    // old tests from tst_qurl.cpp
    // add new tests above
    void old_queryItems();
    void old_hasQueryItem_data();
    void old_hasQueryItem();
};

static QString prettyElement(const QString &key, const QString &value)
{
    QString result;
    if (key.isNull())
        result += "null -> ";
    else
        result += '"' % key % "\" -> ";
    if (value.isNull())
        result += "null";
    else
        result += '"' % value % '"';
    return result;
}

static QString prettyPair(QList<QPair<QString, QString> >::const_iterator it)
{
    return prettyElement(it->first, it->second);
}

template <typename T>
static QByteArray prettyList(const T &items)
{
    QString result = "(";
    bool first = true;
    typename T::const_iterator it = items.constBegin();
    for ( ; it != items.constEnd(); ++it) {
        if (!first)
            result += ", ";
        first = false;
        result += prettyPair(it);
    }
    result += QLatin1Char(')');
    return result.toLocal8Bit();
}

static bool compare(const QList<QPair<QString, QString> > &actual, const QueryItems &expected,
                    const char *actualStr, const char *expectedStr, const char *file, int line)
{
    return QTest::compare_helper(actual == expected, "Compared values are not the same",
                                 qstrdup(prettyList(actual)), qstrdup(prettyList(expected).data()),
                                 actualStr, expectedStr, file, line);
}

#define COMPARE_ITEMS(actual, expected) \
    do { \
        if (!compare(actual, expected, #actual, #expected, __FILE__, __LINE__)) \
            return; \
    } while (0)

inline QueryItems operator+(QueryItems items, const QPair<QString, QString> &pair)
{
    // items is already a copy
    items.append(pair);
    return items;
}

inline QueryItems operator+(const QPair<QString, QString> &pair, QueryItems items)
{
    // items is already a copy
    items.prepend(pair);
    return items;
}

inline QPair<QString, QString> qItem(const QString &first, const QString &second)
{
    return qMakePair(first, second);
}

inline QPair<QString, QString> qItem(const char *first, const QString &second)
{
    return qMakePair(QString::fromUtf8(first), second);
}

inline QPair<QString, QString> qItem(const char *first, const char *second)
{
    return qMakePair(QString::fromUtf8(first), QString::fromUtf8(second));
}

inline QPair<QString, QString> qItem(const QString &first, const char *second)
{
    return qMakePair(first, QString::fromUtf8(second));
}

static QUrlQuery emptyQuery()
{
    return QUrlQuery();
}

void tst_QUrlQuery::constructing()
{
    QUrlQuery empty;
    QVERIFY(empty.isEmpty());
    QCOMPARE(empty.queryPairDelimiter(), QUrlQuery::defaultQueryPairDelimiter());
    QCOMPARE(empty.queryValueDelimiter(), QUrlQuery::defaultQueryValueDelimiter());
    // undefined whether it is detached, but don't crash
    QVERIFY(empty.isDetached() || !empty.isDetached());

    empty.clear();
    QVERIFY(empty.isEmpty());

    {
        QUrlQuery copy(empty);
        QVERIFY(copy.isEmpty());
        QVERIFY(!copy.isDetached());
        QCOMPARE(copy, empty);
        QCOMPARE(qHash(copy), qHash(empty));
        QVERIFY(!(copy != empty));

        copy = empty;
        QCOMPARE(copy, empty);

        copy = QUrlQuery();
        QCOMPARE(copy, empty);
        QCOMPARE(qHash(copy), qHash(empty));
    }
    {
        QUrlQuery copy(emptyQuery());
        QCOMPARE(copy, empty);
    }

    QVERIFY(!empty.hasQueryItem("a"));
    QVERIFY(empty.queryItemValue("a").isEmpty());
    QVERIFY(empty.allQueryItemValues("a").isEmpty());

    QVERIFY(!empty.hasQueryItem(""));
    QVERIFY(empty.queryItemValue("").isEmpty());
    QVERIFY(empty.allQueryItemValues("").isEmpty());

    QVERIFY(!empty.hasQueryItem(QString()));
    QVERIFY(empty.queryItemValue(QString()).isEmpty());
    QVERIFY(empty.allQueryItemValues(QString()).isEmpty());

    QVERIFY(empty.queryItems().isEmpty());

    QUrlQuery other;
    other.addQueryItem("a", "b");
    QVERIFY(!other.isEmpty());
    QVERIFY(other.isDetached());
    QVERIFY(other != empty);
    QVERIFY(!(other == empty));

    QUrlQuery copy(other);
    QCOMPARE(copy, other);

    copy.clear();
    QVERIFY(copy.isEmpty());
    QVERIFY(copy != other);

    copy = other;
    QVERIFY(!copy.isEmpty());
    QCOMPARE(copy, other);

    copy = QUrlQuery();
    QVERIFY(copy.isEmpty());

    empty.setQueryDelimiters('(', ')');
    QCOMPARE(empty.queryValueDelimiter(), QChar(QLatin1Char('(')));
    QCOMPARE(empty.queryPairDelimiter(), QChar(QLatin1Char(')')));

    QList<QPair<QString, QString> > query;
    query += qMakePair(QString("type"), QString("login"));
    query += qMakePair(QString("name"), QString::fromUtf8("åge nissemannsen"));
    query += qMakePair(QString("ole&du"), QString::fromUtf8("anne+jørgen=sant"));
    query += qMakePair(QString("prosent"), QString("%"));
    copy.setQueryItems(query);
    QVERIFY(!copy.isEmpty());
}

void tst_QUrlQuery::addRemove()
{
    QUrlQuery query;

    {
        // one item
        query.addQueryItem("a", "b");
        QVERIFY(!query.isEmpty());
        QVERIFY(query.hasQueryItem("a"));
        QCOMPARE(query.queryItemValue("a"), QString("b"));
        QCOMPARE(query.allQueryItemValues("a"), QStringList() << "b");

        QList<QPair<QString, QString> > allItems = query.queryItems();
        QCOMPARE(allItems.count(), 1);
        QCOMPARE(allItems.at(0).first, QString("a"));
        QCOMPARE(allItems.at(0).second, QString("b"));
    }

    QUrlQuery original = query;

    {
        // two items
        query.addQueryItem("c", "d");
        QVERIFY(query.hasQueryItem("a"));
        QCOMPARE(query.queryItemValue("a"), QString("b"));
        QCOMPARE(query.allQueryItemValues("a"), QStringList() << "b");
        QVERIFY(query.hasQueryItem("c"));
        QCOMPARE(query.queryItemValue("c"), QString("d"));
        QCOMPARE(query.allQueryItemValues("c"), QStringList() << "d");

        QList<QPair<QString, QString> > allItems = query.queryItems();
        QCOMPARE(allItems.count(), 2);
        QVERIFY(allItems.contains(qItem("a", "b")));
        QVERIFY(allItems.contains(qItem("c", "d")));

        QVERIFY(query != original);
        QVERIFY(!(query == original));
    }

    {
        // remove an item that isn't there
        QUrlQuery copy = query;
        query.removeQueryItem("e");
        QCOMPARE(query, copy);
    }

    {
        // remove an item
        query.removeQueryItem("c");
        QVERIFY(query.hasQueryItem("a"));
        QCOMPARE(query.queryItemValue("a"), QString("b"));
        QCOMPARE(query.allQueryItemValues("a"), QStringList() << "b");

        QList<QPair<QString, QString> > allItems = query.queryItems();
        QCOMPARE(allItems.count(), 1);
        QCOMPARE(allItems.at(0).first, QString("a"));
        QCOMPARE(allItems.at(0).second, QString("b"));

        QCOMPARE(query, original);
        QVERIFY(!(query != original));
        QCOMPARE(qHash(query), qHash(original));
    }

    {
        // add an item with en empty value
        QString emptyButNotNull(0, Qt::Uninitialized);
        QVERIFY(emptyButNotNull.isEmpty());
        QVERIFY(!emptyButNotNull.isNull());

        query.addQueryItem("e", "");
        QVERIFY(query.hasQueryItem("a"));
        QCOMPARE(query.queryItemValue("a"), QString("b"));
        QCOMPARE(query.allQueryItemValues("a"), QStringList() << "b");
        QVERIFY(query.hasQueryItem("e"));
        QCOMPARE(query.queryItemValue("e"), emptyButNotNull);
        QCOMPARE(query.allQueryItemValues("e"), QStringList() << emptyButNotNull);

        QList<QPair<QString, QString> > allItems = query.queryItems();
        QCOMPARE(allItems.count(), 2);
        QVERIFY(allItems.contains(qItem("a", "b")));
        QVERIFY(allItems.contains(qItem("e", emptyButNotNull)));

        QVERIFY(query != original);
        QVERIFY(!(query == original));
    }

    {
        // remove the items
        query.removeQueryItem("a");
        query.removeQueryItem("e");
        QVERIFY(query.isEmpty());
    }
}

void tst_QUrlQuery::multiAddRemove()
{
    QUrlQuery query;

    {
        // one item, two values
        query.addQueryItem("a", "b");
        query.addQueryItem("a", "c");
        QVERIFY(!query.isEmpty());
        QVERIFY(query.hasQueryItem("a"));

        // returns the first one
        QCOMPARE(query.queryItemValue("a"), QLatin1String("b"));

        // order is the order we set them in
        QVERIFY(query.allQueryItemValues("a") == QStringList() << "b" << "c");
    }

    {
        // add another item, two values
        query.addQueryItem("A", "B");
        query.addQueryItem("A", "C");
        QVERIFY(query.hasQueryItem("A"));
        QVERIFY(query.hasQueryItem("a"));

        QCOMPARE(query.queryItemValue("a"), QLatin1String("b"));
        QVERIFY(query.allQueryItemValues("a") == QStringList() << "b" << "c");
        QCOMPARE(query.queryItemValue("A"), QLatin1String("B"));
        QVERIFY(query.allQueryItemValues("A") == QStringList() << "B" << "C");
    }

    {
        // remove one of the original items
        query.removeQueryItem("a");
        QVERIFY(query.hasQueryItem("a"));

        // it must have removed the first one
        QCOMPARE(query.queryItemValue("a"), QLatin1String("c"));
    }

    {
        // remove the items we added later
        query.removeAllQueryItems("A");
        QVERIFY(!query.isEmpty());
        QVERIFY(!query.hasQueryItem("A"));
    }

    {
        // add one element to the current, then remove them
        query.addQueryItem("a", "d");
        query.removeAllQueryItems("a");
        QVERIFY(!query.hasQueryItem("a"));
        QVERIFY(query.isEmpty());
    }
}

void tst_QUrlQuery::multiplyAddSamePair()
{
    QUrlQuery query;
    query.addQueryItem("a", "a");
    query.addQueryItem("a", "a");
    QCOMPARE(query.allQueryItemValues("a"), QStringList() << "a" << "a");

    query.addQueryItem("a", "a");
    QCOMPARE(query.allQueryItemValues("a"), QStringList() << "a" << "a" << "a");

    query.removeQueryItem("a");
    QCOMPARE(query.allQueryItemValues("a"), QStringList() << "a" << "a");
}

void tst_QUrlQuery::setQueryItems_data()
{
    QTest::addColumn<QueryItems>("items");
    QString emptyButNotNull(0, Qt::Uninitialized);

    QTest::newRow("empty") << QueryItems();
    QTest::newRow("1-novalue") << (QueryItems() << qItem("a", QString()));
    QTest::newRow("1-emptyvalue") << (QueryItems() << qItem("a", emptyButNotNull));

    QueryItems list;
    list << qItem("a", "b");
    QTest::newRow("1-value") << list;
    QTest::newRow("1-multi") << (list + qItem("a", "c"));
    QTest::newRow("1-duplicated") << (list + qItem("a", "b"));

    list << qItem("c", "d");
    QTest::newRow("2") << list;

    list << qItem("c", "e");
    QTest::newRow("2-multi") << list;
}

void tst_QUrlQuery::setQueryItems()
{
    QFETCH(QueryItems, items);
    QUrlQuery query;

    QueryItems::const_iterator it = items.constBegin();
    for ( ; it != items.constEnd(); ++it)
        query.addQueryItem(it->first, it->second);
    COMPARE_ITEMS(query.queryItems(), items);

    query.clear();

    query.setQueryItems(items);
    COMPARE_ITEMS(query.queryItems(), items);
}

void tst_QUrlQuery::basicParsing_data()
{
    QTest::addColumn<QString>("queryString");
    QTest::addColumn<QueryItems>("items");
    QString emptyButNotNull(0, Qt::Uninitialized);

    QTest::newRow("null") << QString() << QueryItems();
    QTest::newRow("empty") << "" << QueryItems();

    QTest::newRow("1-novalue") << "a" << (QueryItems() << qItem("a", QString()));
    QTest::newRow("1-emptyvalue") << "a=" << (QueryItems() << qItem("a", emptyButNotNull));
    QTest::newRow("1-value") << "a=b" << (QueryItems() << qItem("a", "b"));

    // some longer keys
    QTest::newRow("1-longkey-novalue") << "thisisalongkey" << (QueryItems() << qItem("thisisalongkey", QString()));
    QTest::newRow("1-longkey-emptyvalue") << "thisisalongkey=" << (QueryItems() << qItem("thisisalongkey", emptyButNotNull));
    QTest::newRow("1-longkey-value") << "thisisalongkey=b" << (QueryItems() << qItem("thisisalongkey", "b"));

    // longer values
    QTest::newRow("1-longvalue-value") << "a=thisisalongreasonablyvalue"
                                       << (QueryItems() << qItem("a", "thisisalongreasonablyvalue"));
    QTest::newRow("1-longboth-value") << "thisisalongkey=thisisalongreasonablyvalue"
                                      << (QueryItems() << qItem("thisisalongkey", "thisisalongreasonablyvalue"));

    // two or more entries
    QueryItems baselist;
    baselist << qItem("a", "b") << qItem("c", "d");
    QTest::newRow("2-ab-cd") << "a=b&c=d" << baselist;
    QTest::newRow("2-cd-ab") << "c=d&a=b" << (QueryItems() << qItem("c", "d") << qItem("a", "b"));

    // the same entry multiply defined
    QTest::newRow("2-a-a") << "a&a" << (QueryItems() << qItem("a", QString()) << qItem("a", QString()));
    QTest::newRow("2-ab-a") << "a=b&a" << (QueryItems() << qItem("a", "b") << qItem("a", QString()));
    QTest::newRow("2-ab-ab") << "a=b&a=b" << (QueryItems() << qItem("a", "b") << qItem("a", "b"));
    QTest::newRow("2-ab-ac") << "a=b&a=c" << (QueryItems() << qItem("a", "b") << qItem("a", "c"));

    QPair<QString, QString> novalue = qItem("somekey", QString());
    QueryItems list2 = baselist + novalue;
    QTest::newRow("3-novalue-ab-cd") << "somekey&a=b&c=d" << (novalue + baselist);
    QTest::newRow("3-ab-novalue-cd") << "a=b&somekey&c=d" << (QueryItems() << qItem("a", "b") << novalue << qItem("c", "d"));
    QTest::newRow("3-ab-cd-novalue") << "a=b&c=d&somekey" << list2;

    list2 << qItem("otherkeynovalue", QString());
    QTest::newRow("4-ab-cd-novalue-novalue") << "a=b&c=d&somekey&otherkeynovalue" << list2;

    QPair<QString, QString> emptyvalue = qItem("somekey", emptyButNotNull);
    list2 = baselist + emptyvalue;
    QTest::newRow("3-emptyvalue-ab-cd") << "somekey=&a=b&c=d" << (emptyvalue + baselist);
    QTest::newRow("3-ab-emptyvalue-cd") << "a=b&somekey=&c=d" << (QueryItems() << qItem("a", "b") << emptyvalue << qItem("c", "d"));
    QTest::newRow("3-ab-cd-emptyvalue") << "a=b&c=d&somekey=" << list2;
}

void tst_QUrlQuery::basicParsing()
{
    QFETCH(QString, queryString);
    QFETCH(QueryItems, items);

    QUrlQuery query(queryString);
    QCOMPARE(query.isEmpty(), items.isEmpty());
    COMPARE_ITEMS(query.queryItems(), items);
}

void tst_QUrlQuery::reconstructQuery_data()
{
    QTest::addColumn<QString>("queryString");
    QTest::addColumn<QueryItems>("items");
    QString emptyButNotNull(0, Qt::Uninitialized);

    QTest::newRow("null") << QString() << QueryItems();
    QTest::newRow("empty") << "" << QueryItems();

    QTest::newRow("1-novalue") << "a" << (QueryItems() << qItem("a", QString()));
    QTest::newRow("1-emptyvalue") << "a=" << (QueryItems() << qItem("a", emptyButNotNull));
    QTest::newRow("1-value") << "a=b" << (QueryItems() << qItem("a", "b"));

    // some longer keys
    QTest::newRow("1-longkey-novalue") << "thisisalongkey" << (QueryItems() << qItem("thisisalongkey", QString()));
    QTest::newRow("1-longkey-emptyvalue") << "thisisalongkey=" << (QueryItems() << qItem("thisisalongkey", emptyButNotNull));
    QTest::newRow("1-longkey-value") << "thisisalongkey=b" << (QueryItems() << qItem("thisisalongkey", "b"));

    // longer values
    QTest::newRow("1-longvalue-value") << "a=thisisalongreasonablyvalue"
                                       << (QueryItems() << qItem("a", "thisisalongreasonablyvalue"));
    QTest::newRow("1-longboth-value") << "thisisalongkey=thisisalongreasonablyvalue"
                                      << (QueryItems() << qItem("thisisalongkey", "thisisalongreasonablyvalue"));

    // two or more entries
    QueryItems baselist;
    baselist << qItem("a", "b") << qItem("c", "d");
    QTest::newRow("2-ab-cd") << "a=b&c=d" << baselist;

    // the same entry multiply defined
    QTest::newRow("2-a-a") << "a&a" << (QueryItems() << qItem("a", QString()) << qItem("a", QString()));
    QTest::newRow("2-ab-ab") << "a=b&a=b" << (QueryItems() << qItem("a", "b") << qItem("a", "b"));
    QTest::newRow("2-ab-ac") << "a=b&a=c" << (QueryItems() << qItem("a", "b") << qItem("a", "c"));
    QTest::newRow("2-ac-ab") << "a=c&a=b" << (QueryItems() << qItem("a", "c") << qItem("a", "b"));
    QTest::newRow("2-ab-cd") << "a=b&c=d" << (QueryItems() << qItem("a", "b") << qItem("c", "d"));
    QTest::newRow("2-cd-ab") << "c=d&a=b" << (QueryItems() << qItem("c", "d") << qItem("a", "b"));

    QueryItems list2 = baselist + qItem("somekey", QString());
    QTest::newRow("3-ab-cd-novalue") << "a=b&c=d&somekey" << list2;

    list2 << qItem("otherkeynovalue", QString());
    QTest::newRow("4-ab-cd-novalue-novalue") << "a=b&c=d&somekey&otherkeynovalue" << list2;

    list2 = baselist + qItem("somekey", emptyButNotNull);
    QTest::newRow("3-ab-cd-emptyvalue") << "a=b&c=d&somekey=" << list2;
}

void tst_QUrlQuery::reconstructQuery()
{
    QFETCH(QString, queryString);
    QFETCH(QueryItems, items);

    QUrlQuery query;

    // add the items
    for (QueryItems::ConstIterator it = items.constBegin(); it != items.constEnd(); ++it) {
        query.addQueryItem(it->first, it->second);
    }
    QCOMPARE(query.query(), queryString);
}

void tst_QUrlQuery::encodedSetQueryItems_data()
{
    QTest::addColumn<QString>("queryString");
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("value");
    QTest::addColumn<QUrl::ComponentFormattingOptions>("encoding");
    QTest::addColumn<QString>("expectedQuery");
    QTest::addColumn<QString>("expectedKey");
    QTest::addColumn<QString>("expectedValue");
    typedef QUrl::ComponentFormattingOptions F;

    QTest::newRow("nul") << "f%00=bar%00" << "f%00" << "bar%00" << F(QUrl::PrettyDecoded)
                         << "f%00=bar%00" << "f%00" << "bar%00";
    QTest::newRow("non-decodable-1") << "foo%01%7f=b%1ar" << "foo%01%7f" << "b%1ar" << F(QUrl::PrettyDecoded)
                                     << "foo%01%7F=b%1Ar" << "foo%01%7F" << "b%1Ar";
    QTest::newRow("non-decodable-2") << "foo\x01\x7f=b\x1ar" << "foo\x01\x7f" << "b\x1Ar" << F(QUrl::PrettyDecoded)
                                     << "foo%01%7F=b%1Ar" << "foo%01%7F" << "b%1Ar";

    QTest::newRow("space") << "%20=%20" << "%20" << "%20" << F(QUrl::PrettyDecoded)
                           << " = " << " " << " ";
    QTest::newRow("encode-space") << " = " << " " << " " << F(QUrl::FullyEncoded)
                                  << "%20=%20" << "%20" << "%20";

    // tri-state
    QTest::newRow("decode-non-delimiters") << "%3C%5C%3E=%7B%7C%7D%5E%60" << "%3C%5C%3E" << "%7B%7C%7D%5E%60" << F(QUrl::DecodeReserved)
                                    << "<\\>={|}^`" << "<\\>" << "{|}^`";
    QTest::newRow("encode-non-delimiters") << "<\\>={|}^`" << "<\\>" << "{|}^`" << F(QUrl::EncodeReserved)
                                           << "%3C%5C%3E=%7B%7C%7D%5E%60" << "%3C%5C%3E" << "%7B%7C%7D%5E%60";
    QTest::newRow("pretty-non-delimiters") << "<\\>={|}^`" << "<\\>" << "{|}^`" << F(QUrl::PrettyDecoded)
                                           << "%3C%5C%3E=%7B%7C%7D%5E%60" << "<\\>" << "{|}^`";

    QTest::newRow("equals") << "%3D=%3D" << "%3D" << "%3D" << F(QUrl::PrettyDecoded)
                            << "%3D=%3D" << "=" << "=";
    QTest::newRow("equals-2") << "%3D==" << "=" << "=" << F(QUrl::PrettyDecoded)
                              << "%3D=%3D" << "=" << "=";
    QTest::newRow("ampersand") << "%26=%26" << "%26" << "%26" << F(QUrl::PrettyDecoded)
                               << "%26=%26" << "&" << "&";
    QTest::newRow("hash") << "#=#" << "%23" << "%23" << F(QUrl::PrettyDecoded)
                            << "#=#" << "#" << "#";
    QTest::newRow("decode-hash") << "%23=%23" << "%23" << "%23" << F(QUrl::PrettyDecoded)
                                 << "#=#" << "#" << "#";

    QTest::newRow("percent") << "%25=%25" << "%25" << "%25" << F(QUrl::PrettyDecoded)
                             << "%25=%25" << "%25" << "%25";
    QTest::newRow("bad-percent-1") << "%=%" << "%" << "%" << F(QUrl::PrettyDecoded)
                                   << "%25=%25" << "%25" << "%25";
    QTest::newRow("bad-percent-2") << "%2=%2" << "%2" << "%2" << F(QUrl::PrettyDecoded)
                                   << "%252=%252" << "%252" << "%252";

    QTest::newRow("plus") << "+=+" << "+" << "+" << F(QUrl::PrettyDecoded)
                            << "+=+" << "+" << "+";
    QTest::newRow("2b") << "%2b=%2b" << "%2b" << "%2b" << F(QUrl::PrettyDecoded)
                            << "%2B=%2B" << "%2B" << "%2B";
    // plus signs must not be touched
    QTest::newRow("encode-plus") << "+=+" << "+" << "+" << F(QUrl::FullyEncoded)
                            << "+=+" << "+" << "+";
    QTest::newRow("decode-2b") << "%2b=%2b" << "%2b" << "%2b" << F(QUrl::PrettyDecoded)
                            << "%2B=%2B" << "%2B" << "%2B";


    QTest::newRow("unicode") << "q=R%C3%a9sum%c3%A9" << "q" << "R%C3%a9sum%c3%A9" << F(QUrl::PrettyDecoded)
                             << QString::fromUtf8("q=R\xc3\xa9sum\xc3\xa9") << "q" << QString::fromUtf8("R\xc3\xa9sum\xc3\xa9");
    QTest::newRow("encode-unicode") << QString::fromUtf8("q=R\xc3\xa9sum\xc3\xa9") << "q" << QString::fromUtf8("R\xc3\xa9sum\xc3\xa9")
                                    << F(QUrl::FullyEncoded)
                                    << "q=R%C3%A9sum%C3%A9" << "q" << "R%C3%A9sum%C3%A9";
}

void tst_QUrlQuery::encodedSetQueryItems()
{
    QFETCH(QString, key);
    QFETCH(QString, value);
    QFETCH(QString, expectedQuery);
    QFETCH(QString, expectedKey);
    QFETCH(QString, expectedValue);
    QFETCH(QUrl::ComponentFormattingOptions, encoding);
    QUrlQuery query;

    query.addQueryItem(key, value);
    COMPARE_ITEMS(query.queryItems(encoding), QueryItems() << qItem(expectedKey, expectedValue));
    QCOMPARE(query.query(encoding), expectedQuery);
}

void tst_QUrlQuery::encodedParsing_data()
{
    encodedSetQueryItems_data();
}

void tst_QUrlQuery::encodedParsing()
{
    QFETCH(QString, queryString);
    QFETCH(QString, expectedQuery);
    QFETCH(QString, expectedKey);
    QFETCH(QString, expectedValue);
    QFETCH(QUrl::ComponentFormattingOptions, encoding);

    QUrlQuery query(queryString);
    COMPARE_ITEMS(query.queryItems(encoding), QueryItems() << qItem(expectedKey, expectedValue));
    QCOMPARE(query.query(encoding), expectedQuery);
}

void tst_QUrlQuery::differentDelimiters()
{
    QUrlQuery query;
    query.setQueryDelimiters('(', ')');

    {
        // parse:
        query.setQuery("foo(bar)hello(world)");

        QueryItems expected;
        expected << qItem("foo", "bar") << qItem("hello", "world");
        COMPARE_ITEMS(query.queryItems(), expected);
        COMPARE_ITEMS(query.queryItems(QUrl::FullyEncoded), expected);
        COMPARE_ITEMS(query.queryItems(QUrl::PrettyDecoded), expected);
    }

    {
        // reconstruct:
        // note the final ')' is missing because there are no further items
        QCOMPARE(query.query(), QString("foo(bar)hello(world"));
    }

    {
        // set items containing the new delimiters and the old ones
        query.clear();
        query.addQueryItem("z(=)", "y(&)");
        QCOMPARE(query.query(), QString("z%28=%29(y%28&%29"));

        QUrlQuery copy = query;
        QCOMPARE(query.query(), QString("z%28=%29(y%28&%29"));

        copy.setQueryDelimiters(QUrlQuery::defaultQueryValueDelimiter(),
                                QUrlQuery::defaultQueryPairDelimiter());
        QCOMPARE(copy.query(), QString("z(%3D)=y(%26)"));
    }
}

void tst_QUrlQuery::old_queryItems()
{
    // test imported from old tst_qurl.cpp
    QUrlQuery url;

    QList<QPair<QString, QString> > newItems;
    newItems += qMakePair(QString("1"), QString("a"));
    newItems += qMakePair(QString("2"), QString("b"));
    newItems += qMakePair(QString("3"), QString("c"));
    newItems += qMakePair(QString("4"), QString("a b"));
    newItems += qMakePair(QString("5"), QString("&"));
    newItems += qMakePair(QString("foo bar"), QString("hello world"));
    newItems += qMakePair(QString("foo+bar"), QString("hello+world"));
    newItems += qMakePair(QString("tex"), QString("a + b = c"));
    url.setQueryItems(newItems);
    QVERIFY(!url.isEmpty());

    QList<QPair<QString, QString> > setItems = url.queryItems();
    QCOMPARE(newItems, setItems);

    url.addQueryItem("1", "z");

#if 0
    // undefined behaviour in the new QUrlQuery

    QVERIFY(url.hasQueryItem("1"));
    QCOMPARE(url.queryItemValue("1").toLatin1().constData(), "a");

    url.addQueryItem("1", "zz");

    QStringList expected;
    expected += "a";
    expected += "z";
    expected += "zz";
    QCOMPARE(url.allQueryItemValues("1"), expected);

    url.removeQueryItem("1");
    QCOMPARE(url.allQueryItemValues("1").size(), 2);
    QCOMPARE(url.queryItemValue("1").toLatin1().constData(), "z");
#endif

    url.removeAllQueryItems("1");
    QVERIFY(!url.hasQueryItem("1"));

    QCOMPARE(url.queryItemValue("4"), QLatin1String("a b"));
    QCOMPARE(url.queryItemValue("5"), QLatin1String("&"));
    QCOMPARE(url.queryItemValue("tex"), QLatin1String("a + b = c"));
    QCOMPARE(url.queryItemValue("foo bar"), QLatin1String("hello world"));

    //url.setUrl("http://www.google.com/search?q=a+b");
    url.setQuery("q=a+b");
    QCOMPARE(url.queryItemValue("q"), QLatin1String("a+b"));

    //url.setUrl("http://www.google.com/search?q=a=b"); // invalid, but should be tolerated
    url.setQuery("q=a=b");
    QCOMPARE(url.queryItemValue("q"), QLatin1String("a=b"));
}

void tst_QUrlQuery::old_hasQueryItem_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("item");
    QTest::addColumn<bool>("trueFalse");

    // the old tests started with "http://www.foo.bar"
    QTest::newRow("no query items") << "" << "baz" << false;
    QTest::newRow("query item: hello") << "hello=world" << "hello" << true;
    QTest::newRow("no query item: world") << "hello=world" << "world" << false;
    QTest::newRow("query item: qt") << "hello=world&qt=rocks" << "qt" << true;
}

void tst_QUrlQuery::old_hasQueryItem()
{
    QFETCH(QString, url);
    QFETCH(QString, item);
    QFETCH(bool, trueFalse);

    QCOMPARE(QUrlQuery(url).hasQueryItem(item), trueFalse);
}

#if 0
// this test doesn't make sense anymore
void tst_QUrl::removeAllEncodedQueryItems_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QUrl>("result");

    QTest::newRow("test1") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&bbb=b&ccc=c") << QByteArray("bbb") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&ccc=c");
    QTest::newRow("test2") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&bbb=b&ccc=c") << QByteArray("aaa") << QUrl::fromEncoded("http://qt-project.org/foo?bbb=b&ccc=c");
//    QTest::newRow("test3") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&bbb=b&ccc=c") << QByteArray("ccc") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&bbb=b");
    QTest::newRow("test4") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&bbb=b&ccc=c") << QByteArray("b%62b") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&bbb=b&ccc=c");
    QTest::newRow("test5") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&b%62b=b&ccc=c") << QByteArray("b%62b") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&ccc=c");
    QTest::newRow("test6") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&b%62b=b&ccc=c") << QByteArray("bbb") << QUrl::fromEncoded("http://qt-project.org/foo?aaa=a&b%62b=b&ccc=c");
}

void tst_QUrl::removeAllEncodedQueryItems()
{
    QFETCH(QUrl, url);
    QFETCH(QByteArray, key);
    QFETCH(QUrl, result);
    url.removeAllEncodedQueryItems(key);
    QCOMPARE(url, result);
}
#endif

QTEST_APPLESS_MAIN(tst_QUrlQuery)

#include "tst_qurlquery.moc"
