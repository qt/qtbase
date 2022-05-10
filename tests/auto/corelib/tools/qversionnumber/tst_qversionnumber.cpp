// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 Keith Gardner <kreios4004@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
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
    void constructorDefault();
    void constructorVersioned_data();
    void constructorVersioned();
    void constructorExplicit();
    void constructorCopy_data();
    void constructorCopy();
    void compareGreater_data();
    void compareGreater();
    void compareGreaterEqual_data();
    void compareGreaterEqual();
    void compareLess_data();
    void compareLess();
    void compareLessEqual_data();
    void compareLessEqual();
    void compareEqual_data();
    void compareEqual();
    void compareNotEqual_data();
    void compareNotEqual();
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
    void serialize_data();
    void serialize();
    void moveSemantics();
    void qtVersion();
    void qTypeRevision_data();
    void qTypeRevision();
    void qTypeRevisionTypes();
    void qTypeRevisionComparison();
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
    QTest::addColumn<bool>("equal");
    QTest::addColumn<bool>("notEqual");
    QTest::addColumn<bool>("lessThan");
    QTest::addColumn<bool>("lessThanOrEqual");
    QTest::addColumn<bool>("greaterThan");
    QTest::addColumn<bool>("greaterThanOrEqual");
    QTest::addColumn<int>("compareResult");
    QTest::addColumn<bool>("isPrefix");
    QTest::addColumn<QVersionNumber>("common");

    //                                LHS                          RHS                          ==       !=       <        <=       >        >=       compareResult     isPrefixOf        commonPrefix
    QTest::newRow("null, null")    << QVersionNumber()          << QVersionNumber()          << true  << false << false << true  << false << true  <<  0             << true           << QVersionNumber();
    QTest::newRow("null, 0")       << QVersionNumber()          << QVersionNumber(0)         << false << true  << true  << true  << false << false << -1             << true           << QVersionNumber();
    QTest::newRow("0, null")       << QVersionNumber(0)         << QVersionNumber()          << false << true  << false << false << true  << true  <<  1             << false          << QVersionNumber();
    QTest::newRow("0, 0")          << QVersionNumber(0)         << QVersionNumber(0)         << true  << false << false << true  << false << true  <<  0             << true           << QVersionNumber(0);
    QTest::newRow("1.0, 1.0")      << QVersionNumber(1, 0)      << QVersionNumber(1, 0)      << true  << false << false << true  << false << true  <<  0             << true           << QVersionNumber(1, 0);
    QTest::newRow("1, 1.0")        << QVersionNumber(1)         << QVersionNumber(1, 0)      << false << true  << true  << true  << false << false << -1             << true           << QVersionNumber(1);
    QTest::newRow("1.0, 1")        << QVersionNumber(1, 0)      << QVersionNumber(1)         << false << true  << false << false << true  << true  <<  1             << false          << QVersionNumber(1);

    QTest::newRow("0.1.2, 0.1")    << QVersionNumber(0, 1, 2)   << QVersionNumber(0, 1)      << false << true  << false << false << true  << true  <<  2             << false          << QVersionNumber(0, 1);
    QTest::newRow("0.1, 0.1.2")    << QVersionNumber(0, 1)      << QVersionNumber(0, 1, 2)   << false << true  << true  << true  << false << false << -2             << true           << QVersionNumber(0, 1);
    QTest::newRow("0.1.2, 0.1.2")  << QVersionNumber(0, 1, 2)   << QVersionNumber(0, 1, 2)   << true  << false << false << true  << false << true  <<  0             << true           << QVersionNumber(0, 1, 2);
    QTest::newRow("0.1.2, 1.1.2")  << QVersionNumber(0, 1, 2)   << QVersionNumber(1, 1, 2)   << false << true  << true  << true  << false << false << -1             << false          << QVersionNumber();
    QTest::newRow("1.1.2, 0.1.2")  << QVersionNumber(1, 1, 2)   << QVersionNumber(0, 1, 2)   << false << true  << false << false << true  << true  <<  1             << false          << QVersionNumber();
    QTest::newRow("1, -1")         << QVersionNumber(1)         << QVersionNumber(-1)        << false << true  << false << false << true  << true  <<  2             << false          << QVersionNumber();
    QTest::newRow("-1, 1")         << QVersionNumber(-1)        << QVersionNumber(1)         << false << true  << true  << true  << false << false << -2             << false          << QVersionNumber();
    QTest::newRow("0.1, 0.-1")     << QVersionNumber(0, 1)      << QVersionNumber(0, -1)     << false << true  << false << false << true  << true  <<  2             << false          << QVersionNumber(0);
    QTest::newRow("0.-1, 0.1")     << QVersionNumber(0, -1)     << QVersionNumber(0, 1)      << false << true  << true  << true  << false << false << -2             << false          << QVersionNumber(0);
    QTest::newRow("0.-1, 0")       << QVersionNumber(0, -1)     << QVersionNumber(0)         << false << true  << true  << true  << false << false << -1             << false          << QVersionNumber(0);
    QTest::newRow("0, 0.-1")       << QVersionNumber(0)         << QVersionNumber(0, -1)     << false << true  << false << false << true  << true  <<  1             << true           << QVersionNumber(0);

    QTest::newRow("0.127.2, 0.127")     << QVersionNumber(0, 127, 2)   << QVersionNumber(0, 127)      << false << true  << false << false << true  << true  <<  2    << false          << QVersionNumber(0, 127);
    QTest::newRow("0.127, 0.127.2")     << QVersionNumber(0, 127)      << QVersionNumber(0, 127, 2)   << false << true  << true  << true  << false << false << -2    << true           << QVersionNumber(0, 127);
    QTest::newRow("0.127.2, 0.127.2")   << QVersionNumber(0, 127, 2)   << QVersionNumber(0, 127, 2)   << true  << false << false << true  << false << true  <<  0    << true           << QVersionNumber(0, 127, 2);
    QTest::newRow("0.127.2, 127.127.2") << QVersionNumber(0, 127, 2)   << QVersionNumber(127, 127, 2) << false << true  << true  << true  << false << false << -127  << false          << QVersionNumber();
    QTest::newRow("127.127.2, 0.127.2") << QVersionNumber(127, 127, 2) << QVersionNumber(0, 127, 2)   << false << true  << false << false << true  << true  <<  127  << false          << QVersionNumber();
    QTest::newRow("127, -128")          << QVersionNumber(127)         << QVersionNumber(-128)        << false << true  << false << false << true  << true  <<  255  << false          << QVersionNumber();
    QTest::newRow("-128, 127")          << QVersionNumber(-128)        << QVersionNumber(127)         << false << true  << true  << true  << false << false << -255  << false          << QVersionNumber();
    QTest::newRow("0.127, 0.-128")      << QVersionNumber(0, 127)      << QVersionNumber(0, -128)     << false << true  << false << false << true  << true  <<  255  << false          << QVersionNumber(0);
    QTest::newRow("0.-128, 0.127")      << QVersionNumber(0, -128)     << QVersionNumber(0, 127)      << false << true  << true  << true  << false << false << -255  << false          << QVersionNumber(0);
    QTest::newRow("0.-128, 0")          << QVersionNumber(0, -128)     << QVersionNumber(0)           << false << true  << true  << true  << false << false << -128  << false          << QVersionNumber(0);
    QTest::newRow("0, 0.-128")          << QVersionNumber(0)           << QVersionNumber(0, -128)     << false << true  << false << false << true  << true  <<  128  << true           << QVersionNumber(0);

    QTest::newRow("0.128.2, 0.128")     << QVersionNumber(0, 128, 2)   << QVersionNumber(0, 128)      << false << true  << false << false << true  << true  <<  2    << false          << QVersionNumber(0, 128);
    QTest::newRow("0.128, 0.128.2")     << QVersionNumber(0, 128)      << QVersionNumber(0, 128, 2)   << false << true  << true  << true  << false << false << -2    << true           << QVersionNumber(0, 128);
    QTest::newRow("0.128.2, 0.128.2")   << QVersionNumber(0, 128, 2)   << QVersionNumber(0, 128, 2)   << true  << false << false << true  << false << true  <<  0    << true           << QVersionNumber(0, 128, 2);
    QTest::newRow("0.128.2, 128.128.2") << QVersionNumber(0, 128, 2)   << QVersionNumber(128, 128, 2) << false << true  << true  << true  << false << false << -128  << false          << QVersionNumber();
    QTest::newRow("128.128.2, 0.128.2") << QVersionNumber(128, 128, 2) << QVersionNumber(0, 128, 2)   << false << true  << false << false << true  << true  <<  128  << false          << QVersionNumber();
    QTest::newRow("128, -129")          << QVersionNumber(128)         << QVersionNumber(-129)        << false << true  << false << false << true  << true  <<  257  << false          << QVersionNumber();
    QTest::newRow("-129, 128")          << QVersionNumber(-129)        << QVersionNumber(128)         << false << true  << true  << true  << false << false << -257  << false          << QVersionNumber();
    QTest::newRow("0.128, 0.-129")      << QVersionNumber(0, 128)      << QVersionNumber(0, -129)     << false << true  << false << false << true  << true  <<  257  << false          << QVersionNumber(0);
    QTest::newRow("0.-129, 0.128")      << QVersionNumber(0, -129)     << QVersionNumber(0, 128)      << false << true  << true  << true  << false << false << -257  << false          << QVersionNumber(0);
    QTest::newRow("0.-129, 0")          << QVersionNumber(0, -129)     << QVersionNumber(0)           << false << true  << true  << true  << false << false << -129  << false          << QVersionNumber(0);
    QTest::newRow("0, 0.-129")          << QVersionNumber(0)           << QVersionNumber(0, -129)     << false << true  << false << false << true  << true  <<  129  << true           << QVersionNumber(0);

    const QList<int> common = QList<int>({ 0, 1, 2, 3, 4, 5, 6 });
    using namespace UglyOperator;
    QTest::newRow("0.1.2.3.4.5.6.0.1.2, 0.1.2.3.4.5.6.0.1")    << QVersionNumber(common + 0 + 1 + 2) << QVersionNumber(common + 0 + 1)     << false << true  << false << false << true  << true  <<  2             << false          << QVersionNumber(common + 0 + 1);
    QTest::newRow("0.1.2.3.4.5.6.0.1, 0.1.2.3.4.5.6.0.1.2")    << QVersionNumber(common + 0 + 1)     << QVersionNumber(common + 0 + 1 + 2) << false << true  << true  << true  << false << false << -2             << true           << QVersionNumber(common + 0 + 1);
    QTest::newRow("0.1.2.3.4.5.6.0.1.2, 0.1.2.3.4.5.6.0.1.2")  << QVersionNumber(common + 0 + 1 + 2) << QVersionNumber(common + 0 + 1 + 2) << true  << false << false << true  << false << true  <<  0             << true           << QVersionNumber(common + 0 + 1 + 2);
    QTest::newRow("0.1.2.3.4.5.6.0.1.2, 0.1.2.3.4.5.6.1.1.2")  << QVersionNumber(common + 0 + 1 + 2) << QVersionNumber(common + 1 + 1 + 2) << false << true  << true  << true  << false << false << -1             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.1.1.2, 0.1.2.3.4.5.6.0.1.2")  << QVersionNumber(common + 1 + 1 + 2) << QVersionNumber(common + 0 + 1 + 2) << false << true  << false << false << true  << true  <<  1             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.1, 0.1.2.3.4.5.6.-1")         << QVersionNumber(common + 1)         << QVersionNumber(common + -1)        << false << true  << false << false << true  << true  <<  2             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.-1, 0.1.2.3.4.5.6.1")         << QVersionNumber(common + -1)        << QVersionNumber(common + 1)         << false << true  << true  << true  << false << false << -2             << false          << QVersionNumber(common);
    QTest::newRow("0.1.2.3.4.5.6.0.1, 0.1.2.3.4.5.6.0.-1")     << QVersionNumber(common + 0 + 1)     << QVersionNumber(common + 0 + -1)    << false << true  << false << false << true  << true  <<  2             << false          << QVersionNumber(common + 0);
    QTest::newRow("0.1.2.3.4.5.6.0.-1, 0.1.2.3.4.5.6.0.1")     << QVersionNumber(common + 0 + -1)    << QVersionNumber(common + 0 + 1)     << false << true  << true  << true  << false << false << -2             << false          << QVersionNumber(common + 0);
    QTest::newRow("0.1.2.3.4.5.6.0.-1, 0.1.2.3.4.5.6.0")       << QVersionNumber(common + 0 + -1)    << QVersionNumber(common + 0)         << false << true  << true  << true  << false << false << -1             << false          << QVersionNumber(common + 0);
    QTest::newRow("0.1.2.3.4.5.6.0, 0.1.2.3.4.5.6.0.-1")       << QVersionNumber(common + 0)         << QVersionNumber(common + 0 + -1)    << false << true  << false << false << true  << true  <<  1             << true           << QVersionNumber(common + 0);
}

void tst_QVersionNumber::initTestCase()
{
    qRegisterMetaType<QList<int>>();
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

void tst_QVersionNumber::compareGreater_data()
{
    comparisonData();
}

void tst_QVersionNumber::compareGreater()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(bool, greaterThan);

    QCOMPARE(lhs > rhs, greaterThan);
}

void tst_QVersionNumber::compareGreaterEqual_data()
{
    comparisonData();
}

void tst_QVersionNumber::compareGreaterEqual()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(bool, greaterThanOrEqual);

    QCOMPARE(lhs >= rhs, greaterThanOrEqual);
}

void tst_QVersionNumber::compareLess_data()
{
    comparisonData();
}

void tst_QVersionNumber::compareLess()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(bool, lessThan);

    QCOMPARE(lhs < rhs, lessThan);
}

void tst_QVersionNumber::compareLessEqual_data()
{
    comparisonData();
}

void tst_QVersionNumber::compareLessEqual()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(bool, lessThanOrEqual);

    QCOMPARE(lhs <= rhs, lessThanOrEqual);
}

void tst_QVersionNumber::compareEqual_data()
{
    comparisonData();
}

void tst_QVersionNumber::compareEqual()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(bool, equal);

    QCOMPARE(lhs == rhs, equal);
}

void tst_QVersionNumber::compareNotEqual_data()
{
    comparisonData();
}

void tst_QVersionNumber::compareNotEqual()
{
    QFETCH(QVersionNumber, lhs);
    QFETCH(QVersionNumber, rhs);
    QFETCH(bool, notEqual);

    QCOMPARE(lhs != rhs, notEqual);
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
    QCOMPARE(calculatedPrefix, common);
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
        QCOMPARE(v, QVersionNumber({1, 2, 3}));
    }
    {
        auto v = QVersionNumber::fromString("1.2.3-rc1", 0);
        QCOMPARE(v, QVersionNumber({1, 2, 3}));
    }

    // check the UTF16->L1 conversion isn't doing something weird
    {
        qsizetype i = -1;
        auto v = QVersionNumber::fromString(u"1.0ı", &i); // LATIN SMALL LETTER DOTLESS I
        QCOMPARE(v, QVersionNumber(1, 0));
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
        QCOMPARE(v2, QVersionNumber(1, 2, 3));
    }
    // QVersionNumber &operator=(QVersionNumber &&)
    {
        QVersionNumber v1(1, 2, 3);
        QVersionNumber v2;
        v2 = std::move(v1);
        QCOMPARE(v2, QVersionNumber(1, 2, 3));
    }
    // QVersionNumber(QList<int> &&)
    {
        QList<int> segments = QList<int>({ 1, 2, 3});
        QVersionNumber v1(segments);
        QVersionNumber v2(std::move(segments));
        QVERIFY(!v1.isNull());
        QVERIFY(!v2.isNull());
        QCOMPARE(v1, v2);
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

template<typename Integer>
void compileTestRevisionMajorMinor()
{
    const Integer major = 8;
    const Integer minor = 4;

    const QTypeRevision r2 = QTypeRevision::fromVersion(major, minor);
    QCOMPARE(r2.majorVersion(), 8);
    QCOMPARE(r2.minorVersion(), 4);

    const QTypeRevision r3 = QTypeRevision::fromMajorVersion(major);
    QCOMPARE(r3.majorVersion(), 8);
    QVERIFY(!r3.hasMinorVersion());

    const QTypeRevision r4 = QTypeRevision::fromMinorVersion(minor);
    QVERIFY(!r4.hasMajorVersion());
    QCOMPARE(r4.minorVersion(), 4);
}


template<typename Integer>
void compileTestRevision()
{
    if (std::is_signed<Integer>::value)
        compileTestRevision<typename QIntegerForSize<sizeof(Integer) / 2>::Signed>();
    else
        compileTestRevision<typename QIntegerForSize<sizeof(Integer) / 2>::Unsigned>();

    const Integer value = 0x0510;
    const QTypeRevision r = QTypeRevision::fromEncodedVersion(value);

    QCOMPARE(r.majorVersion(), 5);
    QCOMPARE(r.minorVersion(), 16);
    QCOMPARE(r.toEncodedVersion<Integer>(), value);

    compileTestRevisionMajorMinor<Integer>();
}

template<>
void compileTestRevision<qint16>()
{
    compileTestRevisionMajorMinor<quint8>();
}

template<>
void compileTestRevision<quint8>()
{
    compileTestRevisionMajorMinor<quint8>();
}

template<>
void compileTestRevision<qint8>()
{
    compileTestRevisionMajorMinor<qint8>();
}

void tst_QVersionNumber::qTypeRevision_data()
{
    QTest::addColumn<QTypeRevision>("revision");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("major");
    QTest::addColumn<int>("minor");

    QTest::addRow("Qt revision") << QTypeRevision::fromVersion(QT_VERSION_MAJOR, QT_VERSION_MINOR)
                                 << true << QT_VERSION_MAJOR << QT_VERSION_MINOR;
    QTest::addRow("invalid")     << QTypeRevision() << false << 0xff << 0xff;
    QTest::addRow("major")       << QTypeRevision::fromMajorVersion(6) << true << 6 << 0xff;
    QTest::addRow("minor")       << QTypeRevision::fromMinorVersion(15) << true << 0xff << 15;
    QTest::addRow("zero")        << QTypeRevision::fromVersion(0, 0) << true << 0 << 0;

    // We're intentionally not testing negative numbers.
    // There are asserts against negative numbers in QTypeRevision.
    // You must not pass them as major or minor versions, or values.
}

void tst_QVersionNumber::qTypeRevision()
{
    const QTypeRevision other = QTypeRevision::fromVersion(127, 128);

    QFETCH(QTypeRevision, revision);

    QFETCH(bool, valid);
    QFETCH(int, major);
    QFETCH(int, minor);

    QCOMPARE(revision.isValid(), valid);
    QCOMPARE(revision.majorVersion(), major);
    QCOMPARE(revision.minorVersion(), minor);

    QCOMPARE(revision.hasMajorVersion(), QTypeRevision::isValidSegment(major));
    QCOMPARE(revision.hasMinorVersion(), QTypeRevision::isValidSegment(minor));

    const QTypeRevision copy = QTypeRevision::fromEncodedVersion(revision.toEncodedVersion<int>());
    QCOMPARE(copy, revision);

    QVERIFY(revision != other);
    QVERIFY(copy != other);
}

void tst_QVersionNumber::qTypeRevisionTypes()
{
    compileTestRevision<quint64>();
    compileTestRevision<qint64>();

    QVERIFY(!QTypeRevision::isValidSegment(0xff));
    QVERIFY(!QTypeRevision::isValidSegment(-1));

    const QTypeRevision maxRevision = QTypeRevision::fromVersion(254, 254);
    QVERIFY(maxRevision.hasMajorVersion());
    QVERIFY(maxRevision.hasMinorVersion());
}

void tst_QVersionNumber::qTypeRevisionComparison()
{
    const QTypeRevision revisions[] = {
        QTypeRevision::zero(),
        QTypeRevision::fromMajorVersion(0),
        QTypeRevision::fromVersion(0, 1),
        QTypeRevision::fromVersion(0, 20),
        QTypeRevision::fromMinorVersion(0),
        QTypeRevision(),
        QTypeRevision::fromMinorVersion(1),
        QTypeRevision::fromMinorVersion(20),
        QTypeRevision::fromVersion(1, 0),
        QTypeRevision::fromMajorVersion(1),
        QTypeRevision::fromVersion(1, 1),
        QTypeRevision::fromVersion(1, 20),
        QTypeRevision::fromVersion(20, 0),
        QTypeRevision::fromMajorVersion(20),
        QTypeRevision::fromVersion(20, 1),
        QTypeRevision::fromVersion(20, 20),
    };

    const int length = sizeof(revisions) / sizeof(QTypeRevision);

    for (int i = 0; i < length; ++i) {
        for (int j = 0; j < length; ++j) {
            QCOMPARE(revisions[i] == revisions[j], i == j);
            QCOMPARE(revisions[i] != revisions[j], i != j);
            QCOMPARE(revisions[i] < revisions[j], i < j);
            QCOMPARE(revisions[i] > revisions[j], i > j);
            QCOMPARE(revisions[i] <= revisions[j], i <= j);
            QCOMPARE(revisions[i] >= revisions[j], i >= j);
        }
    }
}

QTEST_APPLESS_MAIN(tst_QVersionNumber)

#include "tst_qversionnumber.moc"
