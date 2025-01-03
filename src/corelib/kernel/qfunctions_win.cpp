// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfunctions_win_p.h"

#include <QtCore/qdebug.h>

#include <combaseapi.h>
#include <objbase.h>

#if __has_include(<appmodel.h>)
#  include <appmodel.h>
#  define HAS_APPMODEL
#endif

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

/*!
    \internal
    Checks if the application has a \e{package identity}

    Having a \e{package identity} is required to use many modern
    Windows APIs.

    https://docs.microsoft.com/en-us/windows/apps/desktop/modernize/modernize-packaged-apps
*/
bool qt_win_hasPackageIdentity()
{
#if defined(HAS_APPMODEL)
    static const bool hasPackageIdentity = []() {
        UINT32 length = 0;
        switch (const auto result = GetCurrentPackageFullName(&length, nullptr)) {
        case ERROR_INSUFFICIENT_BUFFER:
            return true;
        case APPMODEL_ERROR_NO_PACKAGE:
            return false;
        default:
            qWarning("Failed to resolve package identity (error code %ld)", result);
            return false;
        }
    }();
    return hasPackageIdentity;
#else
    return false;
#endif
}

QT_END_NAMESPACE
