// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QRegularExpression>
#include <QTest>

/*!
    \internal
    The main idea of the benchmark is to compare performance of QRE classes
    before and after any changes in the implementation.
    It does not try to compare performance between different patterns or
    matching options.
*/

static const QString textToMatch { "The quick brown fox jumped over the lazy dogs" };
static const QString nonEmptyPattern { "(?<article>\\w+) (?<noun>\\w+)" };
static const QRegularExpression::PatternOptions nonEmptyPatternOptions {
    QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
};

class tst_QRegularExpressionBenchmark : public QObject
{
    Q_OBJECT

private slots:
    void createDefault();
    void createAndMoveDefault();

    void createCustom();
    void createAndMoveCustom();

    void matchDefault();
    void matchDefaultOptimized();

    void matchCustom();
    void matchCustomOptimized();

    void globalMatchDefault();
    void globalMatchDefaultOptimized();

    void globalMatchCustom();
    void globalMatchCustomOptimized();

    void queryMatchResults();
    void queryMatchResultsByGroupIndex();
    void queryMatchResultsByGroupName();
    void iterateThroughGlobalMatchResults();
};

void tst_QRegularExpressionBenchmark::createDefault()
{
    QBENCHMARK {
        QRegularExpression re;
        Q_UNUSED(re);
    }
}

void tst_QRegularExpressionBenchmark::createAndMoveDefault()
{
    QBENCHMARK {
        QRegularExpression re;
        // We can compare to results of previous test to roughly
        // estimate the cost for the move() call.
        QRegularExpression re2(std::move(re));
        Q_UNUSED(re2);
    }
}

void tst_QRegularExpressionBenchmark::createCustom()
{
    QBENCHMARK {
        QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
        Q_UNUSED(re);
    }
}

void tst_QRegularExpressionBenchmark::createAndMoveCustom()
{
    QBENCHMARK {
        QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
        // We can compare to results of previous test to roughly
        // estimate the cost for the move() call.
        QRegularExpression re2(std::move(re));
        Q_UNUSED(re2);
    }
}

/*!
    \internal This benchmark measures the performance of the match() together
    with pattern compilation for a default-constructed object.
    We need to create the object every time, so that the compiled pattern
    does not get cached.
*/
void tst_QRegularExpressionBenchmark::matchDefault()
{
    QBENCHMARK {
        QRegularExpression re;
        auto matchResult = re.match(textToMatch);
        Q_UNUSED(matchResult);
    }
}

/*!
    \internal This benchmark measures the performance of the match() without
    pattern compilation for a default-constructed object.
    The pattern is precompiled before the actual benchmark starts.

    \note In case we make the default constructor non-allocating, the results
    of the benchmark will be very close to the unoptimized one, as it will have
    to compile the pattern inside the call to match().
*/
void tst_QRegularExpressionBenchmark::matchDefaultOptimized()
{
    QRegularExpression re;
    re.optimize();
    QBENCHMARK {
        auto matchResult = re.match(textToMatch);
        Q_UNUSED(matchResult);
    }
}

/*!
    \internal This benchmark measures the performance of the match() together
    with pattern compilation for an object with custom pattern and pattern
    options.
    We need to create the object every time, so that the compiled pattern
    does not get cached.
*/
void tst_QRegularExpressionBenchmark::matchCustom()
{
    QBENCHMARK {
        QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
        auto matchResult = re.match(textToMatch);
        Q_UNUSED(matchResult);
    }
}

/*!
    \internal This benchmark measures the performance of the match() without
    pattern compilation for an object with custom pattern and pattern
    options.
    The pattern is precompiled before the actual benchmark starts.
*/
void tst_QRegularExpressionBenchmark::matchCustomOptimized()
{
    QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
    re.optimize();
    QBENCHMARK {
        auto matchResult = re.match(textToMatch);
        Q_UNUSED(matchResult);
    }
}

/*!
    \internal This benchmark measures the performance of the globalMatch()
    together with the pattern compilation for a default-constructed object.
    We need to create the object every time, so that the compiled pattern
    does not get cached.
*/
void tst_QRegularExpressionBenchmark::globalMatchDefault()
{
    QBENCHMARK {
        QRegularExpression re;
        auto matchResultIterator = re.globalMatch(textToMatch);
        Q_UNUSED(matchResultIterator);
    }
}

/*!
    \internal This benchmark measures the performance of the globalMatch()
    without the pattern compilation for a default-constructed object.
    The pattern is precompiled before the actual benchmark starts.

    \note In case we make the default constructor non-allocating, the results
    of the benchmark will be very close to the unoptimized one, as it will have
    to compile the pattern inside the call to globalMatch().
*/
void tst_QRegularExpressionBenchmark::globalMatchDefaultOptimized()
{
    QRegularExpression re;
    re.optimize();
    QBENCHMARK {
        auto matchResultIterator = re.globalMatch(textToMatch);
        Q_UNUSED(matchResultIterator);
    }
}

/*!
    \internal This benchmark measures the performance of the globalMatch()
    together with the pattern compilation for an object with custom pattern
    and pattern options.
    We need to create the object every time, so that the compiled pattern
    does not get cached.
*/
void tst_QRegularExpressionBenchmark::globalMatchCustom()
{
    QBENCHMARK {
        QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
        auto matchResultIterator = re.globalMatch(textToMatch);
        Q_UNUSED(matchResultIterator);
    }
}

/*!
    \internal This benchmark measures the performance of the globalMatch()
    without the pattern compilation for an object with custom pattern and
    pattern options.
    The pattern is precompiled before the actual benchmark starts.
*/
void tst_QRegularExpressionBenchmark::globalMatchCustomOptimized()
{
    QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
    re.optimize();
    QBENCHMARK {
        auto matchResultIterator = re.globalMatch(textToMatch);
        Q_UNUSED(matchResultIterator);
    }
}

void tst_QRegularExpressionBenchmark::queryMatchResults()
{
    QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
    auto matchResult = re.match(textToMatch);
    QBENCHMARK {
        auto texts = matchResult.capturedTexts();
        Q_UNUSED(texts);
    }
}

void tst_QRegularExpressionBenchmark::queryMatchResultsByGroupIndex()
{
    QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
    auto matchResult = re.match(textToMatch);
    const int capturedCount = matchResult.lastCapturedIndex();
    QBENCHMARK {
        for (int i = 0, imax = capturedCount; i < imax; ++i) {
            auto result = matchResult.capturedView(i);
            Q_UNUSED(result);
        }
    }
}

void tst_QRegularExpressionBenchmark::queryMatchResultsByGroupName()
{
    QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
    auto matchResult = re.match(textToMatch);
    const auto groups = matchResult.regularExpression().namedCaptureGroups();
    QBENCHMARK {
        for (const QString &groupName : groups) {
            // introduce this checks to get rid of tons of warnings.
            // The result for empty string is always the same.
            if (!groupName.isEmpty()) {
                auto result = matchResult.capturedView(groupName);
                Q_UNUSED(result);
            }
        }
    }
}

void tst_QRegularExpressionBenchmark::iterateThroughGlobalMatchResults()
{
    QRegularExpression re(nonEmptyPattern, nonEmptyPatternOptions);
    auto matchResultIterator = re.globalMatch(textToMatch);
    QBENCHMARK {
        if (matchResultIterator.isValid()) {
            while (matchResultIterator.hasNext()) {
                matchResultIterator.next();
            }
        }
    }
}

QTEST_MAIN(tst_QRegularExpressionBenchmark)

#include "tst_bench_qregularexpression.moc"
