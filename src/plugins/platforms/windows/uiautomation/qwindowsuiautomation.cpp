// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiautomation.h"

#ifndef Q_CC_MSVC

template<typename T, typename... TArg>
struct winapi_func
{
    using func_t = T(WINAPI*)(TArg...);
    const func_t func;
    const T error_value;
#ifdef __GNUC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    winapi_func(const char *lib_name, const char *func_name, func_t func_proto,
        T error_value = T(__HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND))) :
        func(reinterpret_cast<func_t>(GetProcAddress(LoadLibraryA(lib_name), func_name))),
        error_value(error_value)
    {
        std::ignore = func_proto;
    }
#ifdef __GNUC__
#   pragma GCC diagnostic pop
#endif
    T invoke(TArg... arg)
    {
        if (!func)
            return error_value;
        return func(arg...);
    }
};

#define FN(fn) #fn,fn

BOOL WINAPI UiaClientsAreListening()
{
    static auto func = winapi_func("uiautomationcore", FN(UiaClientsAreListening), BOOL(false));
    return func.invoke();
}

LRESULT WINAPI UiaReturnRawElementProvider(
    HWND hwnd, WPARAM wParam, LPARAM lParam, IRawElementProviderSimple *el)
{
    static auto func = winapi_func("uiautomationcore", FN(UiaReturnRawElementProvider));
    return func.invoke(hwnd, wParam, lParam, el);
}

HRESULT WINAPI UiaHostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple **ppProvider)
{
    static auto func = winapi_func("uiautomationcore", FN(UiaHostProviderFromHwnd));
    return func.invoke(hwnd, ppProvider);
}

HRESULT WINAPI UiaRaiseAutomationPropertyChangedEvent(
    IRawElementProviderSimple *pProvider, PROPERTYID id, VARIANT oldValue, VARIANT newValue)
{
    static auto func = winapi_func("uiautomationcore", FN(UiaRaiseAutomationPropertyChangedEvent));
    return func.invoke(pProvider, id, oldValue, newValue);
}

HRESULT WINAPI UiaRaiseAutomationEvent(IRawElementProviderSimple *pProvider, EVENTID id)
{
    static auto func = winapi_func("uiautomationcore", FN(UiaRaiseAutomationEvent));
    return func.invoke(pProvider, id);
}

HRESULT WINAPI UiaRaiseNotificationEvent(
    IRawElementProviderSimple *pProvider, NotificationKind notificationKind,
    NotificationProcessing notificationProcessing, BSTR displayString, BSTR activityId)
{
    static auto func = winapi_func("uiautomationcore", FN(UiaRaiseNotificationEvent));
    return func.invoke(pProvider, notificationKind, notificationProcessing, displayString, activityId);
}

#endif // !Q_CC_MSVC

#endif // QT_CONFIG(accessibility)
