// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../crasher.h"
using namespace tst_QProcessCrash;

int main()
{
    [[maybe_unused]] // NoCoreDumps may be an empty struct, not a RAII class
    NoCoreDumps disableCoreDumps;
    crash();
    return 0;
}
