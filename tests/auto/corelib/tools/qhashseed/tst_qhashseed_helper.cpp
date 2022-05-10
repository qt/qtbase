// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qhashfunctions.h>
#include <stdio.h>

int main()
{
    // appless:
    QHashSeed seed1 = QHashSeed::globalSeed();
    QHashSeed seed2 = QHashSeed::globalSeed();
    printf("%zu\n%zu\n", size_t(seed1), size_t(seed2));
    return 0;
}
