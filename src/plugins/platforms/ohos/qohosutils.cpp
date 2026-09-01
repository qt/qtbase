// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosutils.h"
#include <cerrno>
#include <cinttypes>
#include <cstdarg>

QT_BEGIN_NAMESPACE

namespace QtOhos
{

namespace qohosutils_details {

std::optional<std::uintmax_t> tryParseStringAsUIntMax(const std::string &inputString)
{
    char *endPtr;
    auto value = std::strtoumax(inputString.c_str(), &endPtr, 10);
    auto validValue = !inputString.empty() && endPtr == inputString.c_str() + inputString.size();

    return validValue
        ? std::optional(value)
        : std::nullopt;
}

}

std::optional<double> tryParseStringAsFiniteDouble(const std::string &inputString)
{
    char *endPtr;
    errno = 0;
    auto value = std::strtod(inputString.c_str(), &endPtr);
    bool validValue = !inputString.empty()
        && endPtr == inputString.c_str() + inputString.size()
        && errno != ERANGE
        && std::isfinite(value);

    return validValue
        ? std::optional(value)
        : std::nullopt;
}

std::string printfToString(const char *format, ...)
{
    std::va_list ap;

    va_start(ap, format);
    auto dryVsnprintfResult = std::vsnprintf(nullptr, 0, format, ap);
    va_end(ap);

    if (dryVsnprintfResult < 0)
        qOhosReportFatalErrorAndAbort("String formatting with format '%s' failed: %d", format, dryVsnprintfResult);

    auto outputSize = static_cast<std::size_t>(dryVsnprintfResult);

    std::string output(outputSize + 1, '\0');

    va_start(ap, format);
    auto vsnprintfResult = std::vsnprintf(&output[0], output.size(), format, ap);
    va_end(ap);

    if (vsnprintfResult < 0)
        qOhosReportFatalErrorAndAbort("String formatting with format '%s' failed: %d", format, vsnprintfResult);

    output.resize(outputSize);

    return output;
}

const char *mapBoolToTrueFalseStr(bool value)
{
    return value ? "true" : "false";
}

std::shared_ptr<QtOhos::QAbilityPeer> tryMapOptMainWindowToAbilityPeer(
    QtOhos::JsState &jsState, std::optional<QtOhos::QObjectThreadSafeRef> optInstanceMainWindowRef)
{
    if (!optInstanceMainWindowRef.has_value()) {
        auto defaultQAbilityPeer = jsState.defaultQAbilityPeer();
        return !defaultQAbilityPeer->qAbility().IsEmpty() ? defaultQAbilityPeer : nullptr;
    }

    auto optAbilityPeer = jsState.tryGetQAbilityPeerByQWindow(optInstanceMainWindowRef.value());
    if (!optAbilityPeer) {
        qCWarning(
            QtForOhos, "%s: QAbilityPeer for window '%s' no longer exists",
            Q_FUNC_INFO, optInstanceMainWindowRef.value().refName().c_str());
    }

    return optAbilityPeer;
}

}

QT_END_NAMESPACE
