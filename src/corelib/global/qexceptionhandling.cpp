// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
