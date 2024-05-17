// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 Keith Gardner <kreios4004@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>
#include <QtCore/qversionnumber.h>
#include <QtCore/qlibraryinfo.h>

class tst_QVersionNumber : public QObject
{
    Q_OBJECT

private:
    void singleInstanceData();
    void comparisonData();

private slots:
    void initTestCase();
    void compareCompiles();
    void constructorDefault();
    void constructorVersioned_data();
    void constructorVersioned();
    void constructorExplicit();
    void constructorCopy_data();
    void constructorCopy();
    void comparisonOperators_data();
    void comparisonOperators();
    void compare_data();
    void compare();
    void isPrefixOf_data();
    void isPrefixOf();
    void commonPrefix_data();
    void commonPrefix();
    void normalized_data();
    void normalized();
    void isNormalized_data();
    void isNormalized();
    void assignment_data();
    void assignment();
    void fromString_data();
    void fromString();
    void fromString_extra();
    void toString_data();
    void toString();
    void isNull_data();
    void isNull();
    void iterators_data();
    void iterators();
    void iteratorsAreDefaultConstructible();
    void valueInitializedIteratorsCompareEqual();
    void serialize_data();
    void serialize();
    void moveSemantics();
    void qtVersion();
};

void tst_QVersionNumber::singleInstanceData()
{
    QTest::addColumn<QList<int>>("segments");
    QTest::addColumn<QVersionNumber>("expectedVersion");
    QTest::addColumn<QString>("expectedString");
    QTest::addColumn<QString>("constructionString");
    QTest::addColumn<int>("suffixIndex");
    QTest::addColumn<bool>("isNull");

    //                                        segments                                    expectedVersion                                             expectedString                         constructionString                            suffixIndex null
    QTest::newRow("null")                     << QList<int>()                             << QVersionNumber()                                         << QString()                           << QString()                                  << 0        << true;
    QTest::newRow("text")                     << QList<int>()                             << QVersionNumber()                                         << QString()                           << QStringLiteral("text")                     << 0        << true;
    QTest::newRow(" text")                    << QList<int>()                             << QVersionNumber()                                         << QString()                           << QStringLiteral(" text")                    << 0        << true;
    QTest::newRow("Empty String")             << QList<int>()                             << QVersionNumber()                                         << QString()                           << QStringLiteral("Empty String")             << 0        << true;
    QTest::newRow("-1.-2")                    << QList<int>()                             << QVersionNumber()                                         << QStringLiteral("")                  << QStringLiteral("-1.-2")                    << 0        << true;
    QTest::newRow("1.-2-3")                   << QList<int> { 1 }                         << QVersionNumber(QList<int> { 1 })                         << QStringLiteral("1")                 << QStringLiteral("1.-2-3")                   << 1        << false;
    QTest::newRow("1.2-3")                    << QList<int> { 1, 2 }                      << QVersionNumber(QList<int> { 1, 2 })                      << QStringLiteral("1.2")               << QStringLiteral("1.2-3")                    << 3        << false;
    QTest::newRow("0")                        << QList<int> { 0 }                         << QVersionNumber(QList<int> { 0 })                         << QStringLiteral("0")                 << QStringLiteral("0")                        << 1        << false;
    QTest::newRow("0.1")                      << QList<int> { 0, 1 }                      << QVersionNumber(QList<int> { 0, 1 })                      << QStringLiteral("0.1")               << QStringLiteral("0.1")                      << 3        << false;
    QTest::newRow("0.1.2")                    << QList<int> { 0, 1, 2 }                   << QVersionNumber(QList<int> { 0, 1, 2 })                   << QStringLiteral("0.1.2")             << QStringLiteral("0.1.2")                    << 5        << false;
    QTest::newRow("0.1.2alpha")               << QList<int> { 0, 1, 2 }                   << QVersionNumber(QList<int> { 0, 1, 2 })                   << QStringLiteral("0.1.2")             << QStringLiteral("0.1.2alpha")               << 5        << false;
    QTest::newRow("0.1.2-alpha")              << QList<int> { 0, 1, 2 }                   << QVersionNumber(QList<int> { 0, 1, 2 })                   << QStringLiteral("0.1.2")             << QStringLiteral("0.1.2-alpha")              << 5        << false;
    QTest::newRow("0.1.2.alpha")              << QList<int> { 0, 1, 2 }                   << QVersionNumber(QList<int> { 0, 1, 2 })                   << QStringLiteral("0.1.2")             << QStringLiteral("0.1.2.alpha")              << 5        << false;
    QTest::newRow("0.1.2.3alpha")             << QList<int> { 0, 1, 2, 3 }                << QVersionNumber(QList<int> { 0, 1, 2, 3 })                << QStringLiteral("0.1.2.3")           << QStringLiteral("0.1.2.3alpha")             << 7        << false;
    QTest::newRow("0.1.2.3.alpha")            << QList<int> { 0, 1, 2, 3 }                << QVersionNumber(QList<int> { 0, 1, 2, 3 })                << QStringLiteral("0.1.2.3")           << QStringLiteral("0.1.2.3.alpha")            << 7        << false;
    QTest::newRow("0.1.2.3.4.alpha")          << QList<int> { 0, 1, 2, 3, 4 }             << QVersionNumber(QList<int> { 0, 1, 2, 3, 4 })             << QStringLiteral("0.1.2.3.4")         << QStringLiteral("0.1.2.3.4.alpha")          << 9        << false;
    QTest::newRow("0.1.2.3.4 alpha")          << QList<int> { 0, 1, 2, 3, 4 }             << QVersionNumber(QList<int> { 0, 1, 2, 3, 4 })             << QStringLiteral("0.1.2.3.4")         << QStringLiteral("0.1.2.3.4 alpha")          << 9        << false;
    QTest::newRow("0.1.2.3.4 alp ha")         << QList<int> { 0, 1, 2, 3, 4 }             << QVersionNumber(QList<int> { 0, 1, 2, 3, 4 })             << QStringLiteral("0.1.2.3.4")         << QStringLiteral("0.1.2.3.4 alp ha")         << 9        << false;
    QTest::newRow("0.1.2.3.4alp ha")          << QList<int> { 0, 1, 2, 3, 4 }             << QVersionNumber(QList<int> { 0, 1, 2, 3, 4 })             << QStringLiteral("0.1.2.3.4")         << QStringLiteral("0.1.2.3.4alp ha")          << 9        << false;
    QTest::newRow("0.1.2.3.4alpha ")          << QList<int> { 0, 1, 2, 3, 4 }             << QVersionNumber(QList<int> { 0, 1, 2, 3, 4 })             << QStringLiteral("0.1.2.3.4")         << QStringLiteral("0.1.2.3.4alpha ")          << 9        << false;
    QTest::newRow("0.1.2.3.4.5alpha ")        << QList<int> { 0, 1, 2, 3, 4, 5 }          << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5 })          << QStringLiteral("0.1.2.3.4.5")       << QStringLiteral("0.1.2.3.4.5alpha ")        << 11       << false;
    QTest::newRow("0.1.2.3.4.5.6alpha ")      << QList<int> { 0, 1, 2, 3, 4, 5, 6 }       << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5, 6 })       << QStringLiteral("0.1.2.3.4.5.6")     << QStringLiteral("0.1.2.3.4.5.6alpha ")      << 13       << false;
    QTest::newRow("0.1.2.3.4.5.6.7alpha ")    << QList<int> { 0, 1, 2, 3, 4, 5, 6, 7 }    << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5, 6, 7 })    << QStringLiteral("0.1.2.3.4.5.6.7")   << QStringLiteral("0.1.2.3.4.5.6.7alpha ")    << 15       << false;
    QTest::newRow("0.1.2.3.4.5.6.7.8alpha ")  << QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 } << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 }) << QStringLiteral("0.1.2.3.4.5.6.7.8") << QStringLiteral("0.1.2.3.4.5.6.7.8alpha ")  << 17       << false;
    QTest::newRow("0.1.2.3.4.5.6.7.8.alpha")  << QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 } << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 }) << QStringLiteral("0.1.2.3.4.5.6.7.8") << QStringLiteral("0.1.2.3.4.5.6.7.8.alpha")  << 17       << false;
    QTest::newRow("0.1.2.3.4.5.6.7.8 alpha")  << QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 } << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 }) << QStringLiteral("0.1.2.3.4.5.6.7.8") << QStringLiteral("0.1.2.3.4.5.6.7.8 alpha")  << 17       << false;
    QTest::newRow("0.1.2.3.4.5.6.7.8 alp ha") << QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 } << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 }) << QStringLiteral("0.1.2.3.4.5.6.7.8") << QStringLiteral("0.1.2.3.4.5.6.7.8 alp ha") << 17       << false;
    QTest::newRow("0.1.2.3.4.5.6.7.8alp ha")  << QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 } << QVersionNumber(QList<int> { 0, 1, 2, 3, 4, 5, 6, 7, 8 }) << QStringLiteral("0.1.2.3.4.5.6.7.8") << QStringLiteral("0.1.2.3.4.5.6.7.8alp ha")  << 17       << false;
    QTest::newRow("10.09")                    << QList<int> { 10, 9 }                     << QVersionNumber(QList<int> { 10, 9 })                     << QStringLiteral("10.9")              << QStringLiteral("10.09")                    << 5        << false;
    QTest::newRow("10.0x")                    << QList<int> { 10, 0 }                     << QVersionNumber(QList<int> { 10, 0 })                     << QStringLiteral("10.0")              << QStringLiteral("10.0x")                    << 4        << false;
    QTest::newRow("10.0xTest")                << QList<int> { 10, 0 }                     << QVersionNumber(QList<int> { 10, 0 })                     << QStringLiteral("10.0")              << QStringLiteral("10.0xTest")                << 4        << false;
    QTest::newRow("127.09")                   << QList<int> { 127, 9 }                    << QVersionNumber(QList<int> { 127, 9 })                    << QStringLiteral("127.9")             << QStringLiteral("127.09")                   << 6        << false;
    QTest::newRow("127.0x")                   << QList<int> { 127, 0 }                    << QVersionNumber(QList<int> { 127, 0 })                    << QStringLiteral("127.0")             << QStringLiteral("127.0x")                   << 5        << false;
    QTest::newRow("127.0xTest")               << QList<int> { 127, 0 }                    << QVersionNumber(QList<int> { 127, 0 })                    << QStringLiteral("127.0")             << QStringLiteral("127.0xTest")               << 5        << false;
    QTest::newRow("128.09")                   << QList<int> { 128, 9 }                    << QVersionNumber(QList<int> { 128, 9 })                    << QStringLiteral("128.9")             << QStringLiteral("128.09")                   << 6        << false;
    QTest::newRow("128.0x")                   << QList<int> { 128, 0 }                    << QVersionNumber(QList<int> { 128, 0 })                    << QStringLiteral("128.0")             << QStringLiteral("128.0x")                   << 5        << false;
    QTest::newRow("128.0xTest")               << QList<int> { 128, 0 }                    << QVersionNumber(QList<int> { 128, 0 })                    << QStringLiteral("128.0")             << QStringLiteral("128.0xTest")               << 5        << false;
}

namespace UglyOperator {
// ugh, but the alternative (operator <<) is even worse...
static inline QList<int> operator+(QList<int> v, int i) { v.push_back(i); return v; }
}

void tst_QVersionNumber::comparisonData()
{
    QTest::addColumn<QVersionNumber>("lhs");
    QTest::addColumn<QVersionNumber>("rhs");
    QTest::addColumn<Qt::strong_ordering>("ordering");
    QTest::addColumn<int>("compareResult");
    QTest::addColumn<bool>("isPrefix");
    QTest::addColumn<QVersionNumber>("common");

    //                                LHS                          RHS                          ordering                        compareResult     isPrefixOf        commonPrefix
    QTest::newRow("null, null")    << QVersionNumber()          << QVersionNumber()          << Qt::strong_ordering::equal   <<  0             << true           << QVersionNumber();
    QTest::newRow("null, 0")       << QVersionNumber()          << QVersionNumber(0)         << Qt::strong_ordering::less    << -1             << true           << QVersionNumber();
    QTest::newRow("0, null")       << QVersionNumber(0)         << QVersionNumber()          << Qt::strong_ordering::greater <<  1             << false          << QVersionNumber();
    QTest::newRow("0, 0")          << QVersionNumber(0)         << QVersionNumber(0)         << Qt::strong_ordering::equal   <<  0             << true           << QVersionNumber(0);
    QTest::newRow("1.0, 1.0")      << QVersionNumber(1, 0)      << QVersionNumber(1, 0)      << Qt::strong_ordering::equal   <<  0             << true           << QVersionNumber(1, 0);
    QTest::newRow("1, 1.0")        << QVersionNumber(1)         << QVersionNumber(1, 0)      << Qt::strong_ordering::less    << -1             << true           << QVersionNumber(1);
    QTest::newRow("1.0, 1")        << QVersionNumber(1, 0)      << QVersionNumber(1)         << Qt::strong_ordering::greater <<  1             << false          << QVersionNumber(1);

    QTest::newRow("0.1.2, 0.1")    << QVersionNumber(0, 1, 2)   << QVersionNumber(0, 1)      << Qt::strong_ordering::greater  <<  2             << false          << QVersionNumber(0, 1);
    QTest::newRow("0.1, 0.1.2")    << QVersionNumber(0, 1)      << QVersionNumber(0, 1, 2)   << Qt::strong_ordering::less     << -2             << true           << QVersionNumber(0, 1);
    QTest::newRow("0.1.2, 0.1.2")  << QVersionNumber(0, 1, 2)   << QVersionNumber(0, 1, 2)   << Qt::strong_ordering::equal    <<  0             << true           << QVersionNumber(0, 1, 2);
    QTest::newRow("0.1.2, 1.1.2")  << QVersionNumber(0, 1, 2)   << QVersionNumber(1, 1, 2)   << Qt::strong_ordering::less     << -1             << false          << QVersionNumber();
    QTest::newRow("1.1.2, 0.1.2")  << QVersionNumber(1, 1, 2)   << QVersionNumber(0, 1, 2)   << Qt::strong_ordering::greater  <<  1             << false          << QVersionNumber();
    QTest::newRow("1, -1")         << QVersionNumber(1)         << QVersionNumber(-1)        << Qt::strong_ordering::greater  <<  2             << false          << QVersionNumber();
    QTest::newRow("-1, 1")         << QVersionNumber(-1)        << QVersionNumber(1)         << Qt::strong_ordering::less     << -2             << false          << QVersionNumber();
    QTest::newRow("0.1, 0.-1")     << QVersionNumber(0, 1)      << QVersionNumber(0, -1)     << Qt::strong_ordering::greater  <<  2             << false          << QVersionNumber(0);
    QTest::newRow("0.-1, 0.1")     << QVersionNumber(0, -1)     << QVersionNumber(0, 1)      << Qt::strong_ordering::less     << -2             << false          << QVersionNumber(0);
    QTest::newRow("0.-1, 0")       << QVersionNumber(0, -1)     << QVersionNumber(0)         << Qt::strong_ordering::less     << -1             << false          << QVersionNumber(0);
    QTest::newRow("0, 0.-1")       << QVersionNumber(0)         << QVersionNumber(0, -1)     << Qt::strong_ordering::greater  <<  1             << true           << QVersionNumber(0);

    QTest::newRow("0.127.2, 0.127")     << QVersionNumber(0, 127, 2)   << QVersionNumber(0, 127)      << Qt::strong_ordering::greater  <<  2    << false          << QVersionNumber(0, 127);
    QTest::newRow("0.127, 0.127.2")     << QVersionNumber(0, 127)      << QVersionNumber(0, 127, 2)   << Qt::strong_ordering::less     << -2    << true           << QVersionNumber(0, 127);
    QTest::newRow("0.127.2, 0.127.2")   << QVersionNumber(0, 127, 2)   << QVersionNumber(0, 127, 2)   << Qt::strong_ordering::equal    <<  0    << true           << QVersionNumber(0, 127, 2);
    QTest::newRow("0.127.2, 127.127.2") << QVersionNumber(0, 127, 2)   << QVersionNumber(127, 127, 2) << Qt::strong_ordering::less     << -127  << false          << QVersionNumber();
    QTest::newRow("127.127.2, 0.127.2") << QVersionNumber(127, 127, 2) << QVersionNumber(0, 127, 2)   << Qt::strong_ordering::greater  <<  127  << false          << QVersionNumber();
    QTest::newRow("127, -128")          << QVersionNumber(127)         << QVersionNumber(-128)        << Qt::strong_ordering::greater  <<  255  << false          << QVersionNumber();
    QTest::newRow("-128, 127")          << QVersionNumber(-128)        << QVersionNumber(127)         << Qt::strong_ordering::less     << -255  << false          << QVersionNumber();
    QTest::newRow("0.127, 0.-128")      << QVersionNumber(0, 127)      << QVersionNumber(0, -128)     << Qt::strong_ordering::greater  <<  255  << false          << QVersionNumber(0);
    QTest::newRow("0.-128, 0.127")      << QVersionNumber(0, -128)     << QVersionNumber(0, 127)      << Qt::strong_ordering::less     << -255  << false          << QVersionNumber(0);
    QTest::newRow("0.-128, 0")          << QVersionNumber(0, -128)     << QVersionNumber(0)           << Qt::strong_ordering::less     << -128  << false          << QVersionNumber(0);
    QTest::newRow("0, 0.-128")          << QVersionNumber(0)           << QVersionNumber(0, -128)     << Qt::strong_ordering::greater  <<  128  << true           << QVersionNumber(0);

    QTest::newRow("0.128.2, 0.128")     << QVersionNumber(0, 128, 2)   << QVersionNumber(0, 128)      << Qt::strong_ordering::greater  <<  2    << false          << QVersionNumber(0, 128);
    QTest::newRow("0.128, 0.128.2")     << QVersionNumber(0, 128)      << QVersionNumber(0, 128, 2)   << Qt::strong_ordering::less     << -2    << true           << QVersionNumber(0, 128);
    QTest::newRow("0.128.2, 0.128.2")   << QVersionNumber(0, 128, 2)   << QVersionNumber(0, 128, 2)   << Qt::strong_ordering::equal    <<  0    << true           << QVersionNumber(0, 128, 2);
    QTest::newRow("0.128.2, 128.128.2") << QVersionNumber(0, 128, 2)   << QVersionNumber(128, 128, 2) << Qt::strong_ordering::less     << -128  << false          << QVersionNumber();
    QTest::newRow("128.128.2, 0.128.2") << QVersionNumber(128, 128, 2) << QVersionNumber(0, 128, 2)   << Qt::strong_ordering::greater  <<  128  << false          << QVersionNumber();
    QTest::newRow("128, -129")          << QVersionNumber(128)         << QVersionNumber(-129)        << Qt::strong_ordering::greater  <<  257  << false          << QVersionNumber();
    QTest::newRow("-129, 128")          << QVersionNumber(-129)        << QVersionNumber(128)         << Qt::strong_ordering::less     << -257  << false          << QVersionNumber();
    QTest::newRow("0.128, 0.-129")      << QVersionNumber(0, 128)      << QVersionNumber(0, -129)     << Qt::strong_ordering::greater  <<  257  << false          << QVersionNumber(0);
    QTest::newRow("0.-129, 0.128")      << QVersionNumber(0, -129)     << QVersionNumber(0, 128)      << Qt::strong_ordering::less     << -257  << false          << QVersionNumber(0);
    QTest::newRow("0.-129, 0")          << QVersionNumber(0, -129)     << QVersionNumber(0)           << Qt::strong_ordering::less     << -129  << false          << QVersionNumber(0);
    QTest::newRow("0, 0.-129")          << QVersionNumber(0)           << QVersionNumber(0, -129)     << Qt::strong_ordering::greater  <<  129  << true           << QVersionNumber(0);

    const QList<int> common = QList<int>({ 0, 1, 2, 3, 4, 5, 6 });
    using namespace UglyOperator;
    QTest::newRow("0.1.2.3.4.5.6.0.1.2, 0.1.2.3.4.5.6.0.1")    << QVersionNumber(common + 0 + 1 + 2) << QVersionNumber(common + 0 + 1)     << Qt::strong_ordering::greater  <<  2             << false          << QVersionNumber(common + 0 + 1);
    QTest::newRow("0.1.2.3.4.5.6.0.1, 0.1.2.3.4.5.6.0.1.2")    << QVersionNumber(common + 0 + 1)     << QVersionNumber(common + 0 + 1 + 2) << Qt::strong_ordering::less     << -2             << true           << QVersionNumber(common + 0 + 1);
    QTest::newRow("0.1.2.3.4.5.6.0.1.2, 0.1.2.3.4.5.6.0.1.2")  << QVersionNumber(common + 0 + 1 + 2) << QVersionNumber(common + 0 + 1 + 2) << Qt::strong_ordering::equal    <<  0             << true           << QVersionNumber(common + 0 + 1 + 2);
    QTest::newRow("0.1.2.3.4.5.6.0.1.2, 0.1.2.3.4.5.6.1.1.2")  << QVersionNumber(common + 0 + 1 + 2) << QVersionNumber(common + 1 + 1 + 2) << Qt::strong_ordering::less     << -1             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.1.1.2, 0.1.2.3.4.5.6.0.1.2")  << QVersionNumber(common + 1 + 1 + 2) << QVersionNumber(common + 0 + 1 + 2) << Qt::strong_ordering::greater  <<  1             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.1, 0.1.2.3.4.5.6.-1")         << QVersionNumber(common + 1)         << QVersionNumber(common + -1)        << Qt::strong_ordering::greater  <<  2             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.-1, 0.1.2.3.4.5.6.1")         << QVersionNumber(common + -1)        << QVersionNumber(common + 1)         << Qt::strong_ordering::less     << -2             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.0.1, 0.1.2.3.4.5.6.0.-1")     << QVersionNumber(common + 0 + 1)     << QVersionNumber(common + 0 + -1)    << Qt::strong_ordering::greater  <<  2             << false          << QVersionNumber(common + 0);
    QTest::newRow("0.1.2.3.4.5.6.0.-1, 0.1.2.3.4.5.6.0.1")     << QVersionNumber(common + 0 + -1)    << QVersionNumber(common + 0 + 1)     << Qt::strong_ordering::less     << -2             << false          << QVersionNumber(common + 0);
    QTest::newRow("0.1.2.3.4.5.6.0.-1, 0.1.2.3.4.5.6.0")       << QVersionNumber(common + 0 + -1)    << QVersionNumber(common + 0)         << Qt::strong_ordering::less     << -1             << false          << QVersionNumber(common + 0);
    QTest::newRow("0.1.2.3.4.5.6.0, 0.1.2.3.4.5.6.0.-1")       << QVersionNumber(common + 0)         << QVersionNumber(common + 0 + -1)    << Qt::strong_ordering::greater  <<  1             << true           << QVersionNumber(common + 0);
}

void tst_QVersionNumber::initTestCase()
{
    qRegisterMetaType<QList<int>>();
}

void tst_QVersionNumber::compareCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QVersionNumber>();
}

void tst_QVersionNumber::constructorDefault()
{
    QVersionNumber version;

    QCOMPARE(version.majorVersion(), 0);
    QCOMPARE(version.minorVersion(), 0);
    QCOMPARE(version.microVersion(), 0);
    QVERIFY(version.segments().isEmpty());
}

void tst_QVersionNumber::constructorVersioned_data()
{
    singleInstanceData();
}

void tst_QVersionNumber::constructorVersioned()
{
    QFETCH(QList<int>, segments);
    QFETCH(QVersionNumber, expectedVersion);

    QVersionNumber version(segments);
    QCOMPARE(version.majorVersion(), expectedVersion.majorVersion());
    QCOMPARE(version.minorVersion(), expectedVersion.minorVersion());
    QCOMPARE(version.microVersion(), expectedVersion.microVersion());
    QCOMPARE(version.segments(), expectedVersion.segments());
}

void tst_QVersionNumber::constructorExplicit()
{
    QVersionNumber v1(1);
    QVersionNumber v2(QList<int>({ 1 }));

    QCOMPARE(v1.segments(), v2.segments());

    QVersionNumber v3(1, 2);
    QVersionNumber v4(QList<int>({ 1, 2 }));

    QCOMPARE(v3.segments(), v4.segments());

    QVersionNumber v5(1, 2, 3);
    QVersionNumber v6(QList<int>({ 1, 2, 3 }));

    QCOMPARE(v5.segments(), v6.segments());

    QVersionNumber v7(4, 5, 6);
    QVersionNumber v8 = {4, 5, 6};

    QCOMPARE(v7.segments(), v8.segments());

    QVersionNumber v9(4, 5, 6);
    QVersionNumber vA({4, 5, 6});

    QCOMPARE(v9.segments(), vA.segments());
}

void tst_QVersionNumber::constructorCopy_data()
{
    singleInstanceData();
}

void tst_QVersionNumber::constructorCopy()
{
    QFETCH(QList<int>, segments);
    QFETCH(QVersionNumber, expectedVersion);

    QVersionNumber original(segments);
    QVersionNumber version(original);

    QCOMPARE(version.majorVersion(), expectedVersion.majorVersion());
    QCOMPARE(version.minorVersion(), expectedVersion.minorVersion());
    QCOMPARE(version.microVersion(), expectedVersion.microVersion());
    QCOMPARE(version.segments(), expectedVersion.segments());
}

void tst_QVersionNumber::comparisonOperators_data()
{
    comparisonData();
}

void tst_QVersionNumber::comparisonOperators()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(Qt::strong_ordering, ordering);

    QT_TEST_ALL_COMPARISON_OPS(lhs, rhs, ordering);
}

void tst_QVersionNumber::compare_data()
{
    comparisonData();
}

void tst_QVersionNumber::compare()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(int, compareResult);

    QCOMPARE(QVersionNumber::compare(lhs, rhs), compareResult);
}

void tst_QVersionNumber::isPrefixOf_data()
{
    comparisonData();
}

void tst_QVersionNumber::isPrefixOf()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(bool, isPrefix);

    QCOMPARE(lhs.isPrefixOf(rhs), isPrefix);
}

void tst_QVersionNumber::commonPrefix_data()
{
    comparisonData();
}

void tst_QVersionNumber::commonPrefix()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(QVersionNumber, common);

    QVersionNumber calculatedPrefix = QVersionNumber::commonPrefix(lhs, rhs);
    QT_TEST_EQUALITY_OPS(calculatedPrefix, common, true);
    QCOMPARE(calculatedPrefix.segments(), common.segments());
}

void tst_QVersionNumber::normalized_data()
{
    QTest::addColumn<QVersionNumber>("version");
    QTest::addColumn<QVersionNumber>("expected");

    QTest::newRow("0")       << QVersionNumber(0)       << QVersionNumber();
    QTest::newRow("1")       << QVersionNumber(1)       << QVersionNumber(1);
    QTest::newRow("1.2")     << QVersionNumber(1, 2)    << QVersionNumber(1, 2);
    QTest::newRow("1.0")     << QVersionNumber(1, 0)    << QVersionNumber(1);
    QTest::newRow("1.0.0")   << QVersionNumber(1, 0, 0) << QVersionNumber(1);
    QTest::newRow("1.0.1")   << QVersionNumber(1, 0, 1) << QVersionNumber(1, 0, 1);
    QTest::newRow("1.0.1.0") << QVersionNumber(QList<int>({ 1, 0, 1, 0 })) << QVersionNumber(1, 0, 1);
    QTest::newRow("0.0.1.0") << QVersionNumber(QList<int>({ 0, 0, 1, 0 })) << QVersionNumber(0, 0, 1);
}

void tst_QVersionNumber::normalized()
{
    QFETCH(QVersionNumber, version);
    QFETCH(QVersionNumber, expected);

    QCOMPARE(version.normalized(), expected);
    QCOMPARE(std::move(version).normalized(), expected);
}

void tst_QVersionNumber::isNormalized_data()
{
    QTest::addColumn<QVersionNumber>("version");
    QTest::addColumn<bool>("expected");

    QTest::newRow("null")  << QVersionNumber() << true;
    QTest::newRow("0")     << QVersionNumber(0) << false;
    QTest::newRow("1")     << QVersionNumber(1) << true;
    QTest::newRow("1.2")   << QVersionNumber(1, 2) << true;
    QTest::newRow("1.0")   << QVersionNumber(1, 0) << false;
    QTest::newRow("1.0.0") << QVersionNumber(1, 0, 0) << false;
    QTest::newRow("1.0.1") << QVersionNumber(1, 0, 1) << true;
}

void tst_QVersionNumber::isNormalized()
{
    QFETCH(QVersionNumber, version);
    QFETCH(bool, expected);

    QCOMPARE(version.isNormalized(), expected);
}

void tst_QVersionNumber::assignment_data()
{
    singleInstanceData();
}

void tst_QVersionNumber::assignment()
{
    QFETCH(QList<int>, segments);
    QFETCH(QVersionNumber, expectedVersion);

    QVersionNumber original(segments);
    QVersionNumber version;
    version = original;

    QCOMPARE(version.majorVersion(), expectedVersion.majorVersion());
    QCOMPARE(version.minorVersion(), expectedVersion.minorVersion());
    QCOMPARE(version.microVersion(), expectedVersion.microVersion());
    QCOMPARE(version.segments(), expectedVersion.segments());
}

void tst_QVersionNumber::fromString_data()
{
    singleInstanceData();

    const quint64 largerThanIntCanHold = quint64(std::numeric_limits<int>::max()) + 1;
    const QString largerThanIntCanHoldString0 = QString::number(largerThanIntCanHold) + ".0";
    const QString largerThanIntCanHoldString1 = "0." + QString::number(largerThanIntCanHold);

    QTest::newRow(qPrintable(largerThanIntCanHoldString0))
            << QList<int>()  << QVersionNumber()  << QString()           << largerThanIntCanHoldString0 << 0 << true;
    QTest::newRow(qPrintable(largerThanIntCanHoldString1))
            << QList<int>(0) << QVersionNumber(0) << QStringLiteral("0") << largerThanIntCanHoldString1 << 1 << true;

    const QString largerThanULongLongCanHoldString0 = QString::number(std::numeric_limits<qulonglong>::max()) + "0.0";      // 10x ULLONG_MAX
    const QString largerThanULongLongCanHoldString1 = "0." + QString::number(std::numeric_limits<qulonglong>::max()) + '0'; // 10x ULLONG_MAX

    QTest::newRow(qPrintable(largerThanULongLongCanHoldString0))
            << QList<int>()  << QVersionNumber()  << QString()           << largerThanULongLongCanHoldString0 << 0 << true;
    QTest::newRow(qPrintable(largerThanULongLongCanHoldString1))
            << QList<int>(0) << QVersionNumber(0) << QStringLiteral("0") << largerThanULongLongCanHoldString1 << 1 << true;
}

void tst_QVersionNumber::fromString()
{
    QFETCH(QString, constructionString);
    QFETCH(QVersionNumber, expectedVersion);
    QFETCH(int, suffixIndex);

    qsizetype index;
    QCOMPARE(QVersionNumber::fromString(constructionString), expectedVersion);
    QCOMPARE(QVersionNumber::fromString(constructionString, &index), expectedVersion);
    QCOMPARE(index, suffixIndex);

    QCOMPARE(QVersionNumber::fromString(QStringView(constructionString)), expectedVersion);
    QCOMPARE(QVersionNumber::fromString(QStringView(constructionString), &index), expectedVersion);
    QCOMPARE(index, suffixIndex);

    QCOMPARE(QVersionNumber::fromString(QLatin1String(constructionString.toLatin1())), expectedVersion);
    QCOMPARE(QVersionNumber::fromString(QLatin1String(constructionString.toLatin1()), &index), expectedVersion);
    QCOMPARE(index, suffixIndex);

#if QT_DEPRECATED_SINCE(6, 4)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    // check deprecated `int *suffixIndex` overload, too
    {
        int i;
        QCOMPARE(QVersionNumber::fromString(constructionString, &i), expectedVersion);
        QCOMPARE(i, suffixIndex);

        QCOMPARE(QVersionNumber::fromString(QStringView(constructionString), &i), expectedVersion);
        QCOMPARE(i, suffixIndex);

        QCOMPARE(QVersionNumber::fromString(QLatin1String(constructionString.toLatin1()), &i), expectedVersion);
        QCOMPARE(i, suffixIndex);
    }
    QT_WARNING_POP
#endif
}

void tst_QVersionNumber::fromString_extra()
{
    // check the overloaded fromString() functions aren't ambiguous
    // when passing explicit nullptr:
    {
        auto v = QVersionNumber::fromString("1.2.3-rc1", nullptr);
        QT_TEST_EQUALITY_OPS(v, QVersionNumber({1, 2, 3}), true);
    }
    {
        auto v = QVersionNumber::fromString("1.2.3-rc1", 0);
        QT_TEST_EQUALITY_OPS(v, QVersionNumber({1, 2, 3}), true);
    }

    // check the UTF16->L1 conversion isn't doing something weird
    {
        qsizetype i = -1;
        auto v = QVersionNumber::fromString(u"1.0ı", &i); // LATIN SMALL LETTER DOTLESS I
        QT_TEST_EQUALITY_OPS(v, QVersionNumber(1, 0), true);
        QCOMPARE(i, 3);
    }
}

void tst_QVersionNumber::toString_data()
{
    singleInstanceData();

    //                    segments                   expectedVersion          expectedString      constructionString suffixIndex null
    QTest::newRow("-1")   << (QList<int>({ -1 }))    << QVersionNumber(-1)    << QString("-1")    << QString()       << 0        << true;
    QTest::newRow("-1.0") << (QList<int>({ -1, 0 })) << QVersionNumber(-1, 0) << QString("-1.0")  << QString()       << 0        << true;
    QTest::newRow("1.-2") << (QList<int>({ 1, -2 })) << QVersionNumber(1, -2) << QString("1.-2")  << QString()       << 0        << true;
}

void tst_QVersionNumber::toString()
{
    QFETCH(QVersionNumber, expectedVersion);
    QFETCH(QString, expectedString);

    QCOMPARE(expectedVersion.toString(), expectedString);
}

void tst_QVersionNumber::isNull_data()
{
    singleInstanceData();
}

void tst_QVersionNumber::isNull()
{
    QFETCH(QList<int>, segments);
    QFETCH(bool, isNull);

    QVersionNumber version(segments);

    QCOMPARE(version.isNull(), isNull);
}

void tst_QVersionNumber::iterators_data()
{
    singleInstanceData();
}

void tst_QVersionNumber::iterators()
{
    QFETCH(const QList<int>, segments);
    QFETCH(QVersionNumber, expectedVersion);

    QVERIFY(std::equal(expectedVersion.begin(), expectedVersion.end(),
                       segments.begin(), segments.end()));
    QVERIFY(std::equal(std::as_const(expectedVersion).begin(), std::as_const(expectedVersion).end(),
                       segments.begin(), segments.end()));
    QVERIFY(std::equal(expectedVersion.cbegin(), expectedVersion.cend(),
                       segments.cbegin(), segments.cend()));
    QVERIFY(std::equal(expectedVersion.rbegin(), expectedVersion.rend(),
                       segments.rbegin(), segments.rend()));
    QVERIFY(std::equal(std::as_const(expectedVersion).rbegin(), std::as_const(expectedVersion).rend(),
                       segments.rbegin(), segments.rend()));
    QVERIFY(std::equal(expectedVersion.crbegin(), expectedVersion.crend(),
                       segments.crbegin(), segments.crend()));
}

void tst_QVersionNumber::iteratorsAreDefaultConstructible()
{
    static_assert(std::is_default_constructible_v<QVersionNumber::const_iterator>);
    [[maybe_unused]] QVersionNumber::const_iterator ci;
    [[maybe_unused]] QVersionNumber::const_reverse_iterator cri;
}

void tst_QVersionNumber::valueInitializedIteratorsCompareEqual()
{
    QVersionNumber::const_iterator it = {}, jt = {};
    QCOMPARE_EQ(it, jt);
    QVersionNumber::const_reverse_iterator rit = {}, rjt = {};
    QCOMPARE_EQ(rit, rjt);
}

void tst_QVersionNumber::serialize_data()
{
    singleInstanceData();
}

void tst_QVersionNumber::serialize()
{
    QFETCH(QList<int>, segments);

    QVersionNumber original(segments);
    QVersionNumber version;

    QByteArray buffer;
    {
        QDataStream ostream(&buffer, QIODevice::WriteOnly);
        ostream << original;
    }
    {
        QDataStream istream(buffer);
        istream >> version;
    }

    QCOMPARE(version.majorVersion(), original.majorVersion());
    QCOMPARE(version.minorVersion(), original.minorVersion());
    QCOMPARE(version.microVersion(), original.microVersion());
    QCOMPARE(version.segments(), original.segments());
}

void tst_QVersionNumber::moveSemantics()
{
    // QVersionNumber(QVersionNumber &&)
    {
        QVersionNumber v1(1, 2, 3);
        QVersionNumber v2 = std::move(v1);
        QT_TEST_EQUALITY_OPS(v2, QVersionNumber(1, 2, 3), true);
    }
    // QVersionNumber &operator=(QVersionNumber &&)
    {
        QVersionNumber v1(1, 2, 3);
        QVersionNumber v2;
        v2 = std::move(v1);
        QT_TEST_EQUALITY_OPS(v2, QVersionNumber(1, 2, 3), true);
    }
    // QVersionNumber(QList<int> &&)
    {
        QList<int> segments = QList<int>({ 1, 2, 3});
        QVersionNumber v1(segments);
        QVersionNumber v2(std::move(segments));
        QVERIFY(!v1.isNull());
        QVERIFY(!v2.isNull());
        QT_TEST_EQUALITY_OPS(v1, v2, true);
    }
#ifdef Q_COMPILER_REF_QUALIFIERS
    // normalized()
    {
        QVersionNumber v(1, 0, 0);
        QVERIFY(!v.isNull());
        QVersionNumber nv;
        nv = v.normalized();
        QVERIFY(!v.isNull());
        QVERIFY(!nv.isNull());
        QVERIFY(nv.isNormalized());
        nv = std::move(v).normalized();
        QVERIFY(!nv.isNull());
        QVERIFY(nv.isNormalized());
    }
    // segments()
    {
        QVersionNumber v(1, 2, 3);
        QVERIFY(!v.isNull());
        QList<int> segments;
        segments = v.segments();
        QVERIFY(!v.isNull());
        QVERIFY(!segments.empty());
        segments = std::move(v).segments();
        QVERIFY(!segments.empty());
    }
#endif
}

void tst_QVersionNumber::qtVersion()
{
    QVersionNumber v = QLibraryInfo::version();
    QVERIFY(!v.isNull());
    QCOMPARE(v.majorVersion(), QT_VERSION_MAJOR);
    // we can't compare the minor and micro version:
    // the library may change without the test being recompiled

    QCOMPARE(v.toString(), QString(qVersion()));
}

QTEST_APPLESS_MAIN(tst_QVersionNumber)

#include "tst_qversionnumber.moc"
