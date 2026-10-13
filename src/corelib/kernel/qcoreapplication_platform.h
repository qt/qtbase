// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCOREAPPLICATION_PLATFORM_H
#define QCOREAPPLICATION_PLATFORM_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the native interface APIs. Usage of
// this API may make your code source and binary incompatible
// with future versions of Qt.
//

#include <QtCore/qglobal.h>
#include <QtCore/qnativeinterface.h>
#include <QtCore/qcoreapplication.h>

#if defined(Q_OS_ANDROID) || defined(Q_QDOC)
#include <QtCore/qjnitypes.h>
#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
#include <QtCore/qfuture.h>
#include <QtCore/qvariant.h>
#endif
#endif // #if defined(Q_OS_ANDROID) || defined(Q_QDOC)

#if defined(Q_OS_ANDROID)
class _jobject;
typedef _jobject* jobject;
#endif

#if defined(Q_OS_HARMONY) || defined(Q_QDOC)
#include <QtCore/qharmonyjscontext.h>
#include <functional>
#include <optional>
#include <type_traits>
#endif // #if defined(Q_OS_HARMONY) || defined(Q_QDOC)

QT_BEGIN_NAMESPACE

namespace QNativeInterface
{
#if defined(Q_OS_ANDROID) || defined(Q_QDOC)
struct Q_CORE_EXPORT QAndroidApplication
{
    QT_DECLARE_NATIVE_INTERFACE(QAndroidApplication, 1, QCoreApplication)
#ifdef Q_QDOC
    static QJniObject context();
#else
    static QtJniTypes::Context context();
#endif
    static bool isActivityContext();
    static int sdkVersion();
    static void hideSplashScreen(int duration = 0);

#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
    static QFuture<QVariant> runOnAndroidMainThread(const std::function<QVariant()> &runnable,
                                            const QDeadlineTimer timeout = QDeadlineTimer::Forever);

    template <class T>
    std::enable_if_t<std::is_invocable_v<T> && std::is_same_v<std::invoke_result_t<T>, void>,
    QFuture<void>> static runOnAndroidMainThread(const T &runnable,
                                            const QDeadlineTimer timeout = QDeadlineTimer::Forever)
    {
        std::function<QVariant()> func = [runnable](){ runnable(); return QVariant(); };
        return static_cast<QFuture<void>>(runOnAndroidMainThread(func, timeout));
    }
#endif
};
#endif

#if defined(Q_OS_HARMONY) || defined(Q_QDOC)
struct Q_CORE_EXPORT QHarmonyApplication
{
    QT_DECLARE_NATIVE_INTERFACE(QHarmonyApplication, 1, QCoreApplication)

    static void runOnJsThread(std::function<void(QHarmonyJsContext &)> &&task);
    static void runOnJsThreadAndWait(const std::function<void(QHarmonyJsContext &)> &task);

#ifdef Q_QDOC
    template <typename Func, typename Result>
    static Result runOnJsThreadAndWait(Func &&task);
#else
    template <typename Func, typename Result = std::invoke_result_t<Func, QHarmonyJsContext &>>
    static std::enable_if_t<!std::is_void_v<Result>, Result> runOnJsThreadAndWait(Func &&task)
    {
        static_assert(!std::is_reference_v<Result>, "The task must return a value, not a reference");
        static_assert(std::is_default_constructible_v<Result>, "The task's result type must be default-constructible");
        static_assert(
            !std::is_convertible_v<Result, napi_value> && !std::is_convertible_v<Result, napi_ref>,
            "A napi_value or napi_ref result must not leave the JS thread");
        std::optional<Result> optResult;
        runOnJsThreadAndWait(
            [&](QHarmonyJsContext &jsContext) {
                auto result = std::invoke(std::forward<Func>(task), jsContext);
                bool exceptionPending = false;
                napi_is_exception_pending(jsContext.env(), &exceptionPending);
                if (!exceptionPending)
                    optResult.emplace(std::move(result));
            });
        return optResult ? std::move(*optResult) : Result();
    }
#endif

    static void runOnQtThread(std::function<void()> &&task);
};
#endif
}

QT_END_NAMESPACE

#endif // QCOREAPPLICATION_PLATFORM_H
