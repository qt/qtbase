// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qexceptionhandling.h"

#include <exception>

QT_BEGIN_NAMESPACE

/*
   \internal
   Allows you to call std::terminate() without including <exception>.
   Called internally from QT_TERMINATE_ON_EXCEPTION
*/
Q_NORETURN void qTerminate() noexcept
{
    std::terminate();
}

QT_END_NAMESPACE
