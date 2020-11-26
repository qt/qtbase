/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
