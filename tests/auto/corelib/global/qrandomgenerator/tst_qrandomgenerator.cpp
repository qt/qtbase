/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
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

#include <QtTest>
#include <qlinkedlist.h>
#include <qobject.h>
#include <qrandom.h>
#include <qvector.h>
#include <private/qrandom_p.h>

#include <algorithm>
#include <random>

#if !QT_CONFIG(getentropy) && (defined(Q_OS_BSD4) || defined(Q_OS_WIN))
#  define HAVE_FALLBACK_ENGINE
#endif

#define COMMA   ,
#define QVERIFY_3TIMES(statement)    \
    do {\
        if (!QTest::qVerify(static_cast<bool>(statement), #statement, "1st try", __FILE__, __LINE__))\
            if (!QTest::qVerify(static_cast<bool>(statement), #statement, "2nd try", __FILE__, __LINE__))\
                if (!QTest::qVerify(static_cast<bool>(statement), #statement, "3rd try", __FILE__, __LINE__))\
                    return;\
    } while (0)

// values chosen at random
static const quint32 RandomValue32 = 0x4d1169f1U;
static const quint64 RandomValue64 = Q_UINT64_C(0x3ce63161b998aa91);
static const double RandomValueFP = double(0.3010463714599609f);

static void setRNGControl(uint v)
{
#ifdef QT_BUILD_INTERNAL
    qt_randomdevice_control.store(v);
#else
    Q_UNUSED(v);
#endif
}

class tst_QRandomGenerator : public QObject
{
    Q_OBJECT

public slots:
    void cleanup() { setRNGControl(0); }

private slots:
    void generate32_data();
    void generate32();
    void generate64_data() { generate32_data(); }
    void generate64();
    void quality_data() { generate32_data(); }
    void quality();
    void fillRangeUInt_data() { generate32_data(); }
    void fillRangeUInt();
    void fillRangeULong_data() { generate32_data(); }
    void fillRangeULong();
    void fillRangeULLong_data() { generate32_data(); }
    void fillRangeULLong();
    void generateUInt_data() { generate32_data(); }
    void generateUInt();
    void generateULLong_data() { generate32_data(); }
    void generateULLong();
    void generateNonContiguous_data() { generate32_data(); }
    void generateNonContiguous();

    void bounded_data();
    void bounded();
    void boundedQuality_data() { generate32_data(); }
    void boundedQuality();

    void generateReal_data() { generate32_data(); }
    void generateReal();

    void seedStdRandomEngines();
    void stdUniformIntDistribution_data();
    void stdUniformIntDistribution();
    void stdGenerateCanonical_data() { generateReal_data(); }
    void stdGenerateCanonical();
    void stdUniformRealDistribution_data();
    void stdUniformRealDistribution();
    void stdRandomDistributions();
};

using namespace std;
QT_WARNING_DISABLE_GCC("-Wfloat-equal")
QT_WARNING_DISABLE_CLANG("-Wfloat-equal")

void tst_QRandomGenerator::generate32_data()
{
    QTest::addColumn<uint>("control");
    QTest::newRow("default") << 0U;
#ifdef QT_BUILD_INTERNAL
    QTest::newRow("direct") << uint(SkipMemfill);
    QTest::newRow("system") << uint(SkipHWRNG);
#  ifdef HAVE_FALLBACK_ENGINE
    QTest::newRow("fallback") << uint(SkipHWRNG | SkipSystemRNG);
#  endif
#endif
}

void tst_QRandomGenerator::generate32()
{
    QFETCH(uint, control);
    setRNGControl(control);

    for (int i = 0; i < 4; ++i) {
        QVERIFY_3TIMES([] {
            quint32 value = QRandomGenerator::generate();
            return value != 0 && value != RandomValue32;
        }());
    }

    // and should hopefully be different from repeated calls
    for (int i = 0; i < 4; ++i)
        QVERIFY_3TIMES(QRandomGenerator::generate() != QRandomGenerator::generate());
}

void tst_QRandomGenerator::generate64()
{
    QFETCH(uint, control);
    setRNGControl(control);

    for (int i = 0; i < 4; ++i) {
        QVERIFY_3TIMES([] {
            quint64 value = QRandomGenerator::generate();
            return value != 0 && value != RandomValue32 && value != RandomValue64;
        }());
    }

    // and should hopefully be different from repeated calls
    for (int i = 0; i < 4; ++i)
        QVERIFY_3TIMES(QRandomGenerator::generate64() != QRandomGenerator::generate64());
    for (int i = 0; i < 4; ++i)
        QVERIFY_3TIMES(QRandomGenerator::generate() != quint32(QRandomGenerator::generate64()));
    for (int i = 0; i < 4; ++i)
        QVERIFY_3TIMES(QRandomGenerator::generate() != (QRandomGenerator::generate64() >> 32));
}

void tst_QRandomGenerator::quality()
{
    enum {
        BufferSize = 2048,
        BufferCount = BufferSize / sizeof(quint32),

        // if the distribution were perfect, each byte in the buffer would
        // appear exactly:
        PerfectDistribution = BufferSize / (UCHAR_MAX + 1),

        // The chance of a value appearing N times above its perfect
        // distribution is the same as it appearing N times in a row:
        //   N      Probability
        //   1       100%
        //   2       0.390625%
        //   3       15.25 in a million
        //   4       59.60 in a billion
        //   8       5.421e-20
        //   16      2.938e-39

        AcceptableThreshold = 4 * PerfectDistribution,
        FailureThreshold = 16 * PerfectDistribution
    };
    Q_STATIC_ASSERT(FailureThreshold > AcceptableThreshold);

    QFETCH(uint, control);
    setRNGControl(control);

    int histogram[UCHAR_MAX + 1];
    memset(histogram, 0, sizeof(histogram));

    {
        // test the quality of the generator
        quint32 buffer[BufferCount];
        memset(buffer, 0xcc, sizeof(buffer));
        generate_n(buffer, +BufferCount, [] { return QRandomGenerator::generate(); });

        quint8 *ptr = reinterpret_cast<quint8 *>(buffer);
        quint8 *end = ptr + sizeof(buffer);
        for ( ; ptr != end; ++ptr)
            histogram[*ptr]++;
    }

    for (uint i = 0; i < sizeof(histogram)/sizeof(histogram[0]); ++i) {
        int v = histogram[i];
        if (v > AcceptableThreshold)
            qDebug() << i << "above threshold:" << v;
        QVERIFY2(v < FailureThreshold, QByteArray::number(i));
    }
    qDebug() << "Average:" << (std::accumulate(begin(histogram), end(histogram), 0) / (1. * (UCHAR_MAX + 1)))
             << "(expected" << int(PerfectDistribution) << "ideally)"
             << "Max:" << *std::max_element(begin(histogram), end(histogram))
             << "at" << std::max_element(begin(histogram), end(histogram)) - histogram
             << "Min:" << *std::min_element(begin(histogram), end(histogram))
             << "at" << std::min_element(begin(histogram), end(histogram)) - histogram;
}

template <typename T> void fillRange_template()
{
    QFETCH(uint, control);
    setRNGControl(control);

    for (int i = 0; i < 4; ++i) {
        QVERIFY_3TIMES([] {
            T value[1] = { RandomValue32 };
            QRandomGenerator::fillRange(value);
            return value[0] != 0 && value[0] != RandomValue32;
        }());
    }

    for (int i = 0; i < 4; ++i) {
        QVERIFY_3TIMES([] {
            T array[2] = {};
            QRandomGenerator::fillRange(array);
            return array[0] != array[1];
        }());
    }

    if (sizeof(T) > sizeof(quint32)) {
        // just to shut up a warning about shifting uint more than the width
        enum { Shift = sizeof(T) / 2 * CHAR_BIT };
        QVERIFY_3TIMES([] {
            T value[1] = { };
            QRandomGenerator::fillRange(value);
            return quint32(value[0] >> Shift) != quint32(value[0]);
        }());
    }

    // fill in a longer range
    auto longerArrayCheck = [] {
        T array[32];
        memset(array, 0, sizeof(array));
        QRandomGenerator::fillRange(array);
        if (sizeof(T) == sizeof(RandomValue64)
                && find(begin(array), end(array), RandomValue64) != end(array))
            return false;
        return find(begin(array), end(array), 0) == end(array) &&
                find(begin(array), end(array), RandomValue32) == end(array);
    };
    QVERIFY_3TIMES(longerArrayCheck());
}

void tst_QRandomGenerator::fillRangeUInt() { fillRange_template<uint>(); }
void tst_QRandomGenerator::fillRangeULong() { fillRange_template<ulong>(); }
void tst_QRandomGenerator::fillRangeULLong() { fillRange_template<qulonglong>(); }

template <typename T> void generate_template()
{
    QFETCH(uint, control);
    setRNGControl(control);

    // almost the same as fillRange, but limited to 32 bits
    for (int i = 0; i < 4; ++i) {
        QVERIFY_3TIMES([] {
            T value[1] = { RandomValue32 };
            QRandomGenerator().generate(begin(value), end(value));
            return value[0] != 0 && value[0] != RandomValue32
                    && value[0] <= numeric_limits<quint32>::max();
        }());
    }

    // fill in a longer range
    auto longerArrayCheck = [] {
        T array[72] = {};   // at least 256 bytes
        QRandomGenerator().generate(begin(array), end(array));
        return find_if(begin(array), end(array), [](T cur) {
                return cur == 0 || cur == RandomValue32 ||
                        cur == RandomValue64 || cur > numeric_limits<quint32>::max();
            }) == end(array);
    };
    QVERIFY_3TIMES(longerArrayCheck());
}

void tst_QRandomGenerator::generateUInt() { generate_template<uint>(); }
void tst_QRandomGenerator::generateULLong() { generate_template<qulonglong>(); }

void tst_QRandomGenerator::generateNonContiguous()
{
    QFETCH(uint, control);
    setRNGControl(control);

    QLinkedList<quint64> list = { 0, 0, 0, 0,  0, 0, 0, 0 };
    auto longerArrayCheck = [&] {
        QRandomGenerator().generate(list.begin(), list.end());
        return find_if(list.begin(), list.end(), [](quint64 cur) {
                return cur == 0 || cur == RandomValue32 ||
                        cur == RandomValue64 || cur > numeric_limits<quint32>::max();
            }) == list.end();
    };
    QVERIFY_3TIMES(longerArrayCheck());
}

void tst_QRandomGenerator::bounded_data()
{
#ifndef QT_BUILD_INTERNAL
    QSKIP("Test only possible in developer builds");
#endif

    QTest::addColumn<uint>("control");
    QTest::addColumn<quint32>("sup");
    QTest::addColumn<quint32>("expected");

    auto newRow = [](quint32 val, quint32 sup) {
        // calculate the scaled value
        quint64 scaled = val;
        scaled <<= 32;
        scaled /= sup;
        unsigned shifted = unsigned(scaled);
        Q_ASSERT(val < sup);
        Q_ASSERT((shifted & RandomDataMask) == shifted);

        unsigned control = SetRandomData | shifted;
        QTest::addRow("%u,%u", val, sup) << control << sup << val;
    };

    // useless: we can only generate zeroes:
    newRow(0, 1);

    newRow(25, 200);
    newRow(50, 200);
    newRow(75, 200);
}

void tst_QRandomGenerator::bounded()
{
    QFETCH(uint, control);
    QFETCH(quint32, sup);
    QFETCH(quint32, expected);
    setRNGControl(control);

    quint32 value = QRandomGenerator::bounded(sup);
    QVERIFY(value < sup);
    QCOMPARE(value, expected);

    int ivalue = QRandomGenerator::bounded(sup);
    QVERIFY(ivalue < int(sup));
    QCOMPARE(ivalue, int(expected));

    // confirm only the bound now
    setRNGControl(control & (SkipHWRNG|SkipSystemRNG|SkipMemfill));
    value = QRandomGenerator::bounded(sup);
    QVERIFY(value < sup);

    value = QRandomGenerator::bounded(sup / 2, 3 * sup / 2);
    QVERIFY(value >= sup / 2);
    QVERIFY(value < 3 * sup / 2);

    ivalue = QRandomGenerator::bounded(-int(sup), int(sup));
    QVERIFY(ivalue >= -int(sup));
    QVERIFY(ivalue < int(sup));

    // wholly negative range
    ivalue = QRandomGenerator::bounded(-int(sup), 0);
    QVERIFY(ivalue >= -int(sup));
    QVERIFY(ivalue < 0);
}

void tst_QRandomGenerator::boundedQuality()
{
    enum { Bound = 283 };       // a prime number
    enum {
        BufferCount = Bound * 32,

        // if the distribution were perfect, each byte in the buffer would
        // appear exactly:
        PerfectDistribution = BufferCount / Bound,

        // The chance of a value appearing N times above its perfect
        // distribution is the same as it appearing N times in a row:
        //   N      Probability
        //   1       100%
        //   2       0.390625%
        //   3       15.25 in a million
        //   4       59.60 in a billion
        //   8       5.421e-20
        //   16      2.938e-39

        AcceptableThreshold = 4 * PerfectDistribution,
        FailureThreshold = 16 * PerfectDistribution
    };
    Q_STATIC_ASSERT(FailureThreshold > AcceptableThreshold);

    QFETCH(uint, control);
    setRNGControl(control);

    int histogram[Bound];
    memset(histogram, 0, sizeof(histogram));

    {
        // test the quality of the generator
        QVector<quint32> buffer(BufferCount, 0xcdcdcdcd);
        generate(buffer.begin(), buffer.end(), [] { return QRandomGenerator::bounded(Bound); });

        for (quint32 value : qAsConst(buffer)) {
            QVERIFY(value < Bound);
            histogram[value]++;
        }
    }

    for (unsigned i = 0; i < sizeof(histogram)/sizeof(histogram[0]); ++i) {
        int v = histogram[i];
        if (v > AcceptableThreshold)
            qDebug() << i << "above threshold:" << v;
        QVERIFY2(v < FailureThreshold, QByteArray::number(i));
    }

    qDebug() << "Average:" << (std::accumulate(begin(histogram), end(histogram), 0) / qreal(Bound))
             << "(expected" << int(PerfectDistribution) << "ideally)"
             << "Max:" << *std::max_element(begin(histogram), end(histogram))
             << "at" << std::max_element(begin(histogram), end(histogram)) - histogram
             << "Min:" << *std::min_element(begin(histogram), end(histogram))
             << "at" << std::min_element(begin(histogram), end(histogram)) - histogram;
}

void tst_QRandomGenerator::generateReal()
{
    QFETCH(uint, control);
    setRNGControl(control);

    for (int i = 0; i < 4; ++i) {
        QVERIFY_3TIMES([] {
            qreal value = QRandomGenerator::generateDouble();
            return value > 0 && value < 1 && value != RandomValueFP;
        }());
    }

    // and should hopefully be different from repeated calls
    for (int i = 0; i < 4; ++i)
        QVERIFY_3TIMES(QRandomGenerator::generateDouble() != QRandomGenerator::generateDouble());
}

template <typename Engine> void seedStdRandomEngine()
{
    QRandomGenerator rd;
    Engine e(rd);
    QVERIFY_3TIMES(e() != 0);

    e.seed(rd);
    QVERIFY_3TIMES(e() != 0);
}

void tst_QRandomGenerator::seedStdRandomEngines()
{
    seedStdRandomEngine<std::default_random_engine>();
    seedStdRandomEngine<std::minstd_rand0>();
    seedStdRandomEngine<std::minstd_rand>();
    seedStdRandomEngine<std::mt19937>();
    seedStdRandomEngine<std::mt19937_64>();
    seedStdRandomEngine<std::ranlux24_base>();
    seedStdRandomEngine<std::ranlux48_base>();
    seedStdRandomEngine<std::ranlux24>();
    seedStdRandomEngine<std::ranlux48>();
}

void tst_QRandomGenerator::stdUniformIntDistribution_data()
{
#ifndef QT_BUILD_INTERNAL
    QSKIP("Test only possible in developer builds");
#endif

    QTest::addColumn<uint>("control");
    QTest::addColumn<quint32>("max");

    auto newRow = [](quint32 max) {
        QTest::addRow("default:%u", max) << 0U << max;
        QTest::addRow("direct:%u", max) << uint(SkipMemfill) << max;
        QTest::addRow("system:%u", max) << uint(SkipHWRNG) << max;
    #ifdef HAVE_FALLBACK_ENGINE
        QTest::addRow("fallback:%u", max) << uint(SkipHWRNG | SkipSystemRNG) << max;
    #endif
    };

    // useless: we can only generate zeroes:
    newRow(0);

    newRow(1);
    newRow(199);
    newRow(numeric_limits<quint32>::max());
}

void tst_QRandomGenerator::stdUniformIntDistribution()
{
    QFETCH(uint, control);
    QFETCH(quint32, max);
    setRNGControl(control & (SkipHWRNG|SkipSystemRNG|SkipMemfill));

    {
        QRandomGenerator rd;
        {
            std::uniform_int_distribution<quint32> dist(0, max);
            quint32 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }
        if ((3 * max / 2) > max) {
            std::uniform_int_distribution<quint32> dist(max / 2, 3 * max / 2);
            quint32 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }

        {
            std::uniform_int_distribution<quint64> dist(0, quint64(max) << 32);
            quint64 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }
        {
            std::uniform_int_distribution<quint64> dist(max / 2, 3 * quint64(max) / 2);
            quint64 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }
    }

    {
        QRandomGenerator64 rd;
        {
            std::uniform_int_distribution<quint32> dist(0, max);
            quint32 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }
        if ((3 * max / 2) > max) {
            std::uniform_int_distribution<quint32> dist(max / 2, 3 * max / 2);
            quint32 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }

        {
            std::uniform_int_distribution<quint64> dist(0, quint64(max) << 32);
            quint64 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }
        {
            std::uniform_int_distribution<quint64> dist(max / 2, 3 * quint64(max) / 2);
            quint64 value = dist(rd);
            QVERIFY(value >= dist.min());
            QVERIFY(value <= dist.max());
        }
    }
}

void tst_QRandomGenerator::stdGenerateCanonical()
{
#if defined(Q_CC_MSVC) && Q_CC_MSVC < 1900
    // see https://connect.microsoft.com/VisualStudio/feedback/details/811611
    QSKIP("MSVC 2013's std::generate_canonical is broken");
#else
    QFETCH(uint, control);
    setRNGControl(control);

    for (int i = 0; i < 4; ++i) {
        QVERIFY_3TIMES([] {
            QRandomGenerator rd;
            qreal value = std::generate_canonical<qreal COMMA 32>(rd);
            return value > 0 && value < 1 && value != RandomValueFP;
        }());
    }

    // and should hopefully be different from repeated calls
    QRandomGenerator rd;
    for (int i = 0; i < 4; ++i)
        QVERIFY_3TIMES(std::generate_canonical<qreal COMMA 32>(rd) !=
                std::generate_canonical<qreal COMMA 32>(rd));
#endif
}

void tst_QRandomGenerator::stdUniformRealDistribution_data()
{
#ifndef QT_BUILD_INTERNAL
    QSKIP("Test only possible in developer builds");
#endif

    QTest::addColumn<uint>("control");
    QTest::addColumn<double>("min");
    QTest::addColumn<double>("sup");

    auto newRow = [](double min, double sup) {
        QTest::addRow("default:%g-%g", min, sup) << 0U << min << sup;
        QTest::addRow("direct:%g-%g", min, sup) << uint(SkipMemfill) << min << sup;
        QTest::addRow("system:%g-%g", min, sup) << uint(SkipHWRNG) << min << sup;
    #ifdef HAVE_FALLBACK_ENGINE
        QTest::addRow("fallback:%g-%g", min, sup) << uint(SkipHWRNG | SkipSystemRNG) << min << sup;
    #endif
    };

    newRow(0, 0);   // useless: we can only generate zeroes
    newRow(0, 1);   // canonical
    newRow(0, 200);
    newRow(0, numeric_limits<quint32>::max() + 1.);
    newRow(0, numeric_limits<quint64>::max() + 1.);
    newRow(-1, 1.6);
}

void tst_QRandomGenerator::stdUniformRealDistribution()
{
    QFETCH(uint, control);
    QFETCH(double, min);
    QFETCH(double, sup);
    setRNGControl(control & (SkipHWRNG|SkipSystemRNG|SkipMemfill));

    {
        QRandomGenerator rd;
        {
            std::uniform_real_distribution<double> dist(min, sup);
            double value = dist(rd);
            QVERIFY(value >= dist.min());
            if (min != sup)
                QVERIFY(value < dist.max());
        }
    }

    {
        QRandomGenerator64 rd;
        {
            std::uniform_real_distribution<double> dist(min, sup);
            double value = dist(rd);
            QVERIFY(value >= dist.min());
            if (min != sup)
                QVERIFY(value < dist.max());
        }
    }
}

void tst_QRandomGenerator::stdRandomDistributions()
{
    // just a compile check for some of the distributions, besides
    // std::uniform_int_distribution and std::uniform_real_distribution (tested
    // above)

    QRandomGenerator rd;

    std::bernoulli_distribution()(rd);

    std::binomial_distribution<quint32>()(rd);
    std::binomial_distribution<quint64>()(rd);

    std::negative_binomial_distribution<quint32>()(rd);
    std::negative_binomial_distribution<quint64>()(rd);

    std::poisson_distribution<int>()(rd);
    std::poisson_distribution<qint64>()(rd);

    std::normal_distribution<qreal>()(rd);

    {
        std::discrete_distribution<int> discrete{0, 1, 1, 10000, 2};
        QVERIFY(discrete(rd) != 0);
        QVERIFY_3TIMES(discrete(rd) == 3);
    }
}

QTEST_APPLESS_MAIN(tst_QRandomGenerator)

#include "tst_qrandomgenerator.moc"
