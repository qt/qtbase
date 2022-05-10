// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define CATCH_CONFIG_RUNNER
#define CATCH_CLARA_CONFIG_CONSOLE_WIDTH 1000

#if defined(QT_NO_EXCEPTIONS)
#define CATCH_CONFIG_DISABLE_EXCEPTIONS
#endif

#include "catch_p.h"

QT_BEGIN_NAMESPACE

namespace QTestPrivate {

int catchMain(int argc, char **argv)
{
    Catch::Session session;

    if (int returnCode = session.applyCommandLine(argc, argv))
        return returnCode; // Command line error

    return session.run();
}

} // namespace QTestPrivate

QT_END_NAMESPACE

