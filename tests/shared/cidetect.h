// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QT_TESTS_SHARED_CI_DETECT_H
#define QT_TESTS_SHARED_CI_DETECT_H

#include <QtCore/qlatin1stringview.h>
#include <QtCore/qtenvironmentvariables.h> // for qgetenv()

namespace QTestCiDetect {
    /* We set environment variable QTEST_ENVIRONMENT to a sequence of words with
       spaces between them; if one of these is "ci", we're either running on
       Coin or faking it to test how it would behave. See
       src/testlib/qtestblacklist.cpp for other values it may contain.

       Because it may be a sequence of tokens, it's not sufficient to test for
       equality, so tokenize the environment on spaces and test each of the
       resulting words. (Awkwardly, tokens.begin() is an iterator but
       tokens.end() is a sentinel, so std::any_of() can't handle this for us.
       Once we can rely on C++20, std::ranges::any_of() can handle it.)
    */
    inline bool ciInTestEnvironment()
    {
        using namespace Qt::StringLiterals;
        const auto env = qgetenv("QTEST_ENVIRONMENT");
        auto tokens = QLatin1StringView(env).tokenize(' '_L1);
        for (QLatin1StringView tok : tokens) {
            if (tok == "ci"_L1)
                return true;
        }
        return false;
    }
}

#endif // QT_TESTS_SHARED_CI_DETECT_H
