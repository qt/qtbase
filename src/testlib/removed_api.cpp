// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_TESTLIB_BUILD_REMOVED_API

#include "qtest.h"

#if QT_TESTLIB_REMOVED_SINCE(6, 8)

QT_BEGIN_NAMESPACE

namespace QTest {

Q_TESTLIB_EXPORT char *toString(const void *p)
{
    const volatile void *ptr = p;
    return toString(ptr);
}

} // namespace QTest

QT_END_NAMESPACE

// #include "qotherheader.h"
// implement removed functions from qotherheader.h
// order sections alphabetically to reduce chances of merge conflicts

#endif // QT_TESTLIB_REMOVED_SINCE(6, 8)
