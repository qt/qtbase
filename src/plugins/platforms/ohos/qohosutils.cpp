// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosutils.h"
#include <cinttypes>
#include <cstdarg>

QT_BEGIN_NAMESPACE

namespace QtOhos
{

namespace qohosutils_details {

QOhosOptional<std::uintmax_t> tryParseStringAsUIntMax(const std::string &inputString)
{
    char *endPtr;
    auto value = std::strtoumax(inputString.c_str(), &endPtr, 10);
    auto validValue = !inputString.empty() && endPtr == inputString.c_str() + inputString.size();

    return validValue
        ? makeQOhosOptional(value)
        : makeEmptyQOhosOptional();
}

}

QOhosOptional<double> tryParseStringAsFiniteDouble(const std::string &inputString)
{
    double parsedValue;
    std::size_t processedInputChars;
    try {
        parsedValue = std::stod(inputString, &processedInputChars);
    } catch (const std::logic_error &) {
        parsedValue = NAN;
        processedInputChars = 0;
    }

    return processedInputChars == inputString.size() && std::isfinite(parsedValue)
        ? makeQOhosOptional(parsedValue)
        : makeEmptyQOhosOptional();
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
    QtOhos::JsState &jsState, QOhosOptional<QtOhos::QObjectThreadSafeRef> optInstanceMainWindowRef)
{
    if (!optInstanceMainWindowRef.hasValue())
        return jsState.defaultQAbilityPeer();

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
