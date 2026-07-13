// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohospathutils_p.h>

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qscopeguard.h>
#include <cstdlib>
#include <filemanagement/file_uri/oh_file_uri.h>

QT_BEGIN_NAMESPACE

namespace {

template<typename ConvFunc>
std::optional<std::string> tryCallOhFileUriConversionFunc(ConvFunc convFunc, const std::string &input)
{
    char *outputPtr = nullptr;
    auto outputPtrGuard = qScopeGuard([&outputPtr]() { ::free(outputPtr); });
    auto convFuncRetVal = convFunc(input.c_str(), input.size(), &outputPtr);

    if (convFuncRetVal == ::FileManagement_ErrCode::ERR_OK && outputPtr != nullptr)
        return std::string(outputPtr);

    qOhosPrintfWarning(
        "OH FileUri conversion function '%s' failed for input '%s', retval: %d",
        convFunc.name(), input.c_str(), static_cast<int>(convFuncRetVal));

    return {};
}

}

std::optional<std::string> tryMapPathToOhosFileUri(const std::string &path)
{
    return tryCallOhFileUriConversionFunc(Q_OHOS_NAMED_FUNC(::OH_FileUri_GetUriFromPath), path);
}

std::optional<std::string> tryMapOhosFileUriToPath(const std::string &ohosFileUri)
{
    return tryCallOhFileUriConversionFunc(Q_OHOS_NAMED_FUNC(::OH_FileUri_GetPathFromUri), ohosFileUri);
}

QT_END_NAMESPACE
