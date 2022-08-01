// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

#include "qassert.h"

#include <QtCore/qlogging.h>

QT_BEGIN_NAMESPACE

/*!
    \macro void Q_ASSERT(bool test)
    \relates <QtAssert>

    Prints a warning message containing the source code file name and
    line number if \a test is \c false.

    Q_ASSERT() is useful for testing pre- and post-conditions
    during development. It does nothing if \c QT_NO_DEBUG was defined
    during compilation.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 17

    If \c b is zero, the Q_ASSERT statement will output the following
    message using the qFatal() function:

    \snippet code/src_corelib_global_qglobal.cpp 18

    \sa Q_ASSERT_X(), qFatal(), {Debugging Techniques}
*/

/*!
    \macro void Q_ASSERT_X(bool test, const char *where, const char *what)
    \relates <QtAssert>

    Prints the message \a what together with the location \a where,
    the source file name and line number if \a test is \c false.

    Q_ASSERT_X is useful for testing pre- and post-conditions during
    development. It does nothing if \c QT_NO_DEBUG was defined during
    compilation.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 19

    If \c b is zero, the Q_ASSERT_X statement will output the following
    message using the qFatal() function:

    \snippet code/src_corelib_global_qglobal.cpp 20

    \sa Q_ASSERT(), qFatal(), {Debugging Techniques}
*/

/*
    The Q_ASSERT macro calls this function when the test fails.
*/
void qt_assert(const char *assertion, const char *file, int line) noexcept
{
    QMessageLogger(file, line, nullptr)
            .fatal("ASSERT: \"%s\" in file %s, line %d", assertion, file, line);
}

/*
    The Q_ASSERT_X macro calls this function when the test fails.
*/
void qt_assert_x(const char *where, const char *what, const char *file, int line) noexcept
{
    QMessageLogger(file, line, nullptr)
            .fatal("ASSERT failure in %s: \"%s\", file %s, line %d", where, what, file, line);
}

QT_END_NAMESPACE
