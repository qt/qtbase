// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUTILS_H
#define QARKUTILS_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qpoint.h>
#include <arkui/native_type.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <qohosutils.h>
#include <string>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QArkUi {

class CZString
{
public:
    CZString(const char *value);

    operator const char *() const;
    operator std::string() const;

    const char *value() const;

private:
    const char *m_value;
};

std::array<float, 2> toFloatArray(const QPointF &point);

template<typename Func, Func f, typename... FuncArgs>
std::enable_if_t<
    std::is_same<QOhosInvokeResult<Func, FuncArgs...>, std::int32_t>::value
        || std::is_enum<QOhosInvokeResult<Func, FuncArgs...>>::value,
    void>
callArkUiOrFailOnErrorResult(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs);

template<typename Func, Func f, typename... FuncArgs>
Q_REQUIRED_RESULT std::enable_if_t<
    std::is_pointer<QOhosInvokeResult<Func, FuncArgs...>>::value,
    QOhosInvokeResult<Func, FuncArgs...>>
callArkUiOrFailOnNullResult(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs);

template<typename Func, Func f, typename... FuncArgs>
QOhosInvokeResult<Func, FuncArgs...> callArkUi(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs);

namespace details_qarkuiutils_h {

constexpr bool arkUiCallsLogging = false;

template<typename T>
std::enable_if_t<std::is_arithmetic<T>::value, std::string>
buildFuncArgString(const T &funcArg)
{
    return std::to_string(funcArg);
}

template<typename T>
std::enable_if_t<std::is_enum<T>::value, std::string>
buildFuncArgString(const T &funcArg)
{
    return std::to_string(funcArg) + ":enum(" + typeid(T).name() + "):";
}

template<typename T>
std::enable_if_t<std::is_pointer<T>::value, std::string>
buildFuncArgString(const T &funcArg)
{
    char buffer[] = "0xFFFFFFFFFFFFFFFF";
    std::snprintf(buffer, sizeof(buffer), "%p", funcArg);
    return std::string(buffer);
}

template<typename T>
std::enable_if_t<!std::is_arithmetic<T>::value && !std::is_enum<T>::value && !std::is_pointer<T>::value, std::string>
buildFuncArgString(const T &)
{
    return "...";
}

inline std::string buildFuncArgString(CZString funcArg)
{
    return '"' + std::string(funcArg) + '"';
}

template<typename... FuncArgs>
std::string buildFuncCallString(const char *funcName, FuncArgs &&...funcArgs)
{
    std::string allArgsString;
    std::string argsStrings[] = {buildFuncArgString(funcArgs)...};
    for (const auto &argString : argsStrings) {
        if (!allArgsString.empty())
            allArgsString += ", ";
        allArgsString += argString;
    }

    return std::string(funcName) + "(" + allArgsString + ")";
}

template<typename Func, typename... FuncArgs>
std::enable_if_t<
    !std::is_void<QOhosInvokeResult<Func, FuncArgs...>>::value,
    QOhosInvokeResult<Func, FuncArgs...>>
callArkUiFunc(const char *funcName, Func &&func, FuncArgs &&...funcArgs)
{
    auto funcResult = func(funcArgs...);

    if (arkUiCallsLogging) {
        qOhosPrintfDebug(
            "callArkUiFunc: %s => %s",
            buildFuncCallString(funcName, funcArgs...).c_str(),
            buildFuncArgString(funcResult).c_str());
    }

    return funcResult;
}

template<typename Func, typename... FuncArgs>
std::enable_if_t<
    std::is_void<QOhosInvokeResult<Func, FuncArgs...>>::value,
    void>
callArkUiFunc(const char *funcName, Func &&func, FuncArgs &&...funcArgs)
{
    func(funcArgs...);

    if (arkUiCallsLogging)
        qOhosPrintfDebug("callArkUiFunc: %s", buildFuncCallString(funcName, funcArgs...).c_str());
}

}

inline std::array<float, 2> toFloatArray(const QPointF &point)
{
    return {static_cast<float>(point.x()), static_cast<float>(point.y())};
}

inline CZString::CZString(const char *value)
    : m_value(value)
{
}

inline CZString::operator const char *() const
{
    return m_value;
}

inline CZString::operator std::string() const
{
    return m_value;
}

inline const char *CZString::value() const
{
    return m_value;
}

template<typename Func, Func f, typename... FuncArgs>
std::enable_if_t<
    std::is_same<QOhosInvokeResult<Func, FuncArgs...>, std::int32_t>::value
        || std::is_enum<QOhosInvokeResult<Func, FuncArgs...>>::value,
    void>
callArkUiOrFailOnErrorResult(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs)
{
    using namespace details_qarkuiutils_h;

    std::int32_t funcResult = details_qarkuiutils_h::callArkUiFunc(func.name(), func.ptr(), funcArgs...);
    if (funcResult != ::ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "ArkUi function call %s failed with error: %d",
            buildFuncCallString(func.name(), funcArgs...).c_str(), funcResult);
    }
}

template<typename Func, Func f, typename... FuncArgs>
Q_REQUIRED_RESULT std::enable_if_t<
    std::is_pointer<QOhosInvokeResult<Func, FuncArgs...>>::value,
    QOhosInvokeResult<Func, FuncArgs...>>
callArkUiOrFailOnNullResult(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs)
{
    using namespace details_qarkuiutils_h;

    auto *funcResult = details_qarkuiutils_h::callArkUiFunc(func.name(), func.ptr(), funcArgs...);
    if (funcResult == nullptr) {
        qOhosReportFatalErrorAndAbort(
            "ArkUi function call %s failed (returned null)",
            buildFuncCallString(func.name(), funcArgs...).c_str());
    }
    return funcResult;
}

template<typename Func, Func f, typename... FuncArgs>
QOhosInvokeResult<Func, FuncArgs...> callArkUi(QOhosNamedFunc<Func, f> func, FuncArgs &&...funcArgs)
{
    return details_qarkuiutils_h::callArkUiFunc(func.name(), func.ptr(), funcArgs...);
}

}

QT_END_NAMESPACE

#endif
