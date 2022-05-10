// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtconcurrentfilter.h>
#include <qtconcurrentmap.h>
#include <QTest>

#include "generation_helpers.h"

using namespace QtConcurrent;

class tst_QtConcurrentFilterMapGenerated : public QObject
{
    Q_OBJECT

private slots:
    void mapReduceThroughDifferentTypes();
    void moveOnlyFilterObject();
    void moveOnlyMapObject();
    void moveOnlyReduceObject();
    void functorAsReduction();
    void moveOnlyReductionItem();
    void noDefaultConstructorItemMapped();
    void noDefaultConstructorItemFiltered();
    // START_GENERATED_SLOTS (see generate_tests.py)
    void test1();
    // END_GENERATED_SLOTS (see generate_tests.py)
};
