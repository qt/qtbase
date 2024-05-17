// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qhashfunctions.h>
#if QT_CONFIG(process)
#include <qprocess.h>
#endif

class tst_QHashSeed : public QObject
{
    Q_OBJECT
public:
    static void initMain();

private Q_SLOTS:
    void initTestCase();
    void environmentVariable_data();
    void environmentVariable();
    void deterministicSeed();
    void reseeding();
    void quality();
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && QT_DEPRECATED_SINCE(6,6)
    void compatibilityApi();
    void deterministicSeed_compat();
#endif
};

void tst_QHashSeed::initMain()
{
    qunsetenv("QT_HASH_SEED");
}

void tst_QHashSeed::initTestCase()
{
    // in case the qunsetenv above didn't work
    if (qEnvironmentVariableIsSet("QT_HASH_SEED"))
        QSKIP("QT_HASH_SEED environment variable is set, please don't do that");
}

void tst_QHashSeed::environmentVariable_data()
{
#ifdef Q_OS_ANDROID
    QSKIP("This test needs a helper binary, so is excluded from this platform.");
#endif

    QTest::addColumn<QByteArray>("envVar");
    QTest::addColumn<bool>("isZero");
    QTest::newRow("unset-environment") << QByteArray() << false;
    QTest::newRow("empty-environment") << QByteArray("") << false;
    QTest::newRow("zero-seed") << QByteArray("0") << true;
}

void tst_QHashSeed::environmentVariable()
{
 #if QT_CONFIG(process)
    QFETCH(QByteArray, envVar);
    QFETCH(bool, isZero);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (envVar.isNull())
        env.remove("QT_HASH_SEED");
    else
        env.insert("QT_HASH_SEED", envVar);

    QProcess helper;
    helper.setProcessEnvironment(env);
    helper.setProgram("./tst_qhashseed_helper");
    helper.start();
    QVERIFY2(helper.waitForStarted(5000), qPrintable(helper.errorString()));
    QVERIFY2(helper.waitForFinished(5000), qPrintable(helper.errorString()));
    QCOMPARE(helper.exitStatus(), 0);

    QByteArray line1 = helper.readLine().trimmed();
    QByteArray line2 = helper.readLine().trimmed();
    QCOMPARE(line2, line1);
    QCOMPARE(line1 == "0", isZero);
#endif
}

void tst_QHashSeed::deterministicSeed()
{
    QHashSeed::setDeterministicGlobalSeed();
    QCOMPARE(size_t(QHashSeed::globalSeed()), size_t(0));

    // now reset
    QHashSeed::resetRandomGlobalSeed();
    QVERIFY(QHashSeed::globalSeed() != 0);
}

void tst_QHashSeed::reseeding()
{
    constexpr int Iterations = 4;
    size_t seeds[Iterations];
    for (int i = 0; i < Iterations; ++i) {
        seeds[i] = QHashSeed::globalSeed();
        QHashSeed::resetRandomGlobalSeed();
    }

    // verify that they are all different
    QString fmt = QStringLiteral("seeds[%1] = 0x%3, seeds[%2] = 0x%4");
    for (int i = 0; i < Iterations; ++i) {
        for (int j = i + 1; j < Iterations; ++j) {
            QVERIFY2(seeds[i] != seeds[j],
                     qPrintable(fmt.arg(i).arg(j).arg(seeds[i], 16).arg(seeds[j], 16)));
        }
    }
}

void tst_QHashSeed::quality()
{
    // this "bad seed" is used internally in qhash.cpp and should never leak!
    constexpr size_t BadSeed = size_t(Q_UINT64_C(0x5555'5555'5555'5555));

    constexpr int Iterations = 24;  // nicely divisible by 3
    int oneThird = 0;
    int badSeeds = 0;
    int seedsToMinus1 = 0;
    size_t ored = 0;

    for (int i = 0; i < Iterations; ++i) {
        size_t seed = QHashSeed::globalSeed();
        ored |= seed;
        int bits = qPopulationCount(quintptr(seed));
        QVERIFY2(bits > 0, QByteArray::number(bits));   // mandatory

        if (bits >= std::numeric_limits<size_t>::digits / 3)
            ++oneThird;
        if (seed == BadSeed)
            ++badSeeds;
        if (ored != size_t(-1))
            ++seedsToMinus1;

        QHashSeed::resetRandomGlobalSeed();
    }

    // report out
    qInfo() << "Number of seeds until all bits became set:" << seedsToMinus1 << '/' << Iterations;
    qInfo() << "Number of seeds with at least one third of the bits set:"
            << oneThird << '/' << Iterations;

    // we must have set all bits after all the iterations
    QCOMPARE(ored, size_t(-1));

    // at least one third of the seeds must have one third of all the bits set
    QVERIFY(oneThird > (Iterations/3));

    // at most one seed can be the bad seed, if 32-bit, none on 64-bit
    if (std::numeric_limits<size_t>::digits > 32)
        QCOMPARE(badSeeds, 0);
    else
        QVERIFY2(badSeeds <= 1, "badSeeds = " + QByteArray::number(badSeeds));

    // we must have taken at most two thirds of the iterations to have set each
    // bit at least once
    QVERIFY2(seedsToMinus1 < 2*Iterations/3,
             "seedsToMinus1 = " + QByteArray::number(seedsToMinus1));
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && QT_DEPRECATED_SINCE(6,6)
QT_WARNING_DISABLE_DEPRECATED
void tst_QHashSeed::compatibilityApi()
{
    int oldSeed = qGlobalQHashSeed();
    size_t newSeed = QHashSeed::globalSeed();

    QCOMPARE(size_t(oldSeed), newSeed & size_t(INT_MAX));
}

void tst_QHashSeed::deterministicSeed_compat()
{
    // same as above, but using the compat API
    qSetGlobalQHashSeed(0);
    QCOMPARE(size_t(QHashSeed::globalSeed()), size_t(0));
    QCOMPARE(qGlobalQHashSeed(), 0);

    // now reset
    qSetGlobalQHashSeed(-1);
    QVERIFY(QHashSeed::globalSeed() != 0);
    QVERIFY(qGlobalQHashSeed() != 0);
    QVERIFY(qGlobalQHashSeed() != -1);  // possible, but extremely unlikely
}
#endif // Qt 7

QTEST_MAIN(tst_QHashSeed)
#include "tst_qhashseed.moc"
