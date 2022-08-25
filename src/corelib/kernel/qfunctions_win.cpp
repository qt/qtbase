// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfunctions_win_p.h"

#include <combaseapi.h>
#include <objbase.h>

QT_BEGIN_NAMESPACE

QComHelper::QComHelper(COINIT concurrencyModel)
{
    // Avoid overhead of initializing and using obsolete technology
    concurrencyModel = COINIT(concurrencyModel | COINIT_DISABLE_OLE1DDE);

    m_initResult = CoInitializeEx(nullptr, concurrencyModel);

    if (FAILED(m_initResult))
        qErrnoWarning(m_initResult, "Failed to initialize COM library");
}

QComHelper::~QComHelper()
{
    if (SUCCEEDED(m_initResult))
        CoUninitialize();
}

QT_END_NAMESPACE
