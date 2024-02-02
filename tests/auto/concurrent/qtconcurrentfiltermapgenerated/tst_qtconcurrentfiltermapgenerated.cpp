// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <qtconcurrentfilter.h>
#include <qtconcurrentmap.h>
#include <QCoreApplication>
#include <QList>
#include <QTest>

#include "../testhelper_functions.h"
#include "generation_helpers.h"

#include "tst_qtconcurrentfiltermapgenerated.h"

using namespace QtConcurrent;

// START_GENERATED_IMPLEMENTATIONS (see generate_tests.py)

void tst_QtConcurrentFilterMapGenerated::test1()
{
    /* test for
    template<typename Sequence, typename KeepFunctor>
    void blockingFilter(QThreadPool* pool, Sequence & sequence, KeepFunctor filterFunction);

    with
      inputsequence=standard
      inputitemtype=standard
      filterfunction=functor
      filterfunctionpassing=lvalue
    */

    QThreadPool pool;
    pool.setMaxThreadCount(1);
    auto input_sequence = []() {
        std::vector<SequenceItem<tag_input>> result;
        result.push_back(SequenceItem<tag_input>(1, true));
        result.push_back(SequenceItem<tag_input>(2, true));
        result.push_back(SequenceItem<tag_input>(3, true));
        result.push_back(SequenceItem<tag_input>(4, true));
        result.push_back(SequenceItem<tag_input>(5, true));
        result.push_back(SequenceItem<tag_input>(6, true));
        return result;
    }();
    auto filter = MyFilter<SequenceItem<tag_input>> {};

    QtConcurrent::blockingFilter(&pool, input_sequence, filter);

    auto expected_result = []() {
        std::vector<SequenceItem<tag_input>> result;
        result.push_back(SequenceItem<tag_input>(1, true));
        result.push_back(SequenceItem<tag_input>(3, true));
        result.push_back(SequenceItem<tag_input>(5, true));
        return result;
    }();

    QCOMPARE(input_sequence, expected_result);
}
// END_GENERATED_IMPLEMENTATION (see generate_tests.py)

QTEST_MAIN(tst_QtConcurrentFilterMapGenerated)
