// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qharmonyappjscontext.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <thread>

QT_BEGIN_NAMESPACE

namespace {

QOhosJsState &jsState()
{
    return QOhosJsThreadOps::instance().jsState();
}

std::shared_ptr<void> tryRegisterUiAbilityDelegate(QNapi::Object delegate)
{
    try {
        return jsState().registerUiAbilityDelegate(delegate);
    } catch (const Napi::Error &error) {
        qOhosPrintfError("%s: registering the delegate failed: %s", Q_FUNC_INFO, error.what());
        return nullptr;
    }
}

}

/*!
    \class QHarmonyAppJsContext
    \inmodule QtCore
    \brief The QHarmonyAppJsContext class gives code on the JS thread of a
    HarmonyOS application access to the application's state.

    QHarmonyJsContext describes the JS runtime. QHarmonyAppJsContext holds
    the state of the application in that runtime, such as its default
    \c UIAbility instance. Keeping the two apart makes function signatures
    meaningful. A function that takes a QHarmonyAppJsContext depends on the
    application's state. A function that takes a QHarmonyJsContext needs
    only the runtime. A task gets the application context once, with
    fromJsContext(), and passes it on.

    The class derives from QHarmonyJsContext, and there is only one object.
    QHarmonyJsContext::fromEnv() returns it as the base class. All member
    functions must be called on the JS thread. References to it and its
    \c napi_value results follow the lifetime rules of QHarmonyJsContext.

    \sa {Qt for HarmonyOS Platform API}
*/

QHarmonyAppJsContext::QHarmonyAppJsContext() = default;

QHarmonyAppJsContext::~QHarmonyAppJsContext() = default;

/*!
    Returns the application context of \a env. \a env must be the
    environment of the JS thread. The returned context is valid as long as
    \a env.

    Use this function in a JS callback created with
    \c napi_create_function(), which receives only the \c napi_env.
*/
QHarmonyAppJsContext &QHarmonyAppJsContext::fromEnv(napi_env env)
{
    Q_ASSERT(env == jsState().env());

    static QHarmonyAppJsContext appJsContext;
    return appJsContext;
}

/*!
    Returns the application context of \a jsContext. The returned context is
    valid as long as \a jsContext.
*/
QHarmonyAppJsContext &QHarmonyAppJsContext::fromJsContext(QHarmonyJsContext &jsContext)
{
    return fromEnv(jsContext.env());
}

/*!
    Returns the default ArkTS \c UIAbility instance of the application. This
    is the first instance created for the application or, once that instance
    is destroyed, another live instance. The result can therefore change
    over the lifetime of the process. Returns \nullptr when no ability is
    left, which happens at some stages of application shutdown. Also returns
    \nullptr in a child process started without a UI, where no ability
    exists.

    The ability is the entry point to the ability-bound platform APIs. Its
    \c context property is the \c UIAbilityContext. APIs such as
    \c {notificationManager.requestEnableNotification()},
    \c {systemShare.ShareController.show()}, and \c {iap.createPurchase()}
    take this context. \c context.getApplicationContext() returns the
    \c ApplicationContext, which provides \c restartApp() and the
    application directories.
*/
napi_value QHarmonyAppJsContext::defaultAbility()
{
    return jsState().defaultQAbility().value_or(QNapi::Object());
}

/*!
    Registers \a delegateValue as a delegate for the methods of the
    application's \c UIAbility class and returns a handle that keeps the
    registration. \a delegateValue must be a JS object.

    Qt forwards a \c UIAbility method by calling the delegate method of the
    same name with the \c UIAbility instance as the first argument, followed
    by the arguments the \c UIAbility method received. It looks the method
    up at each call and calls it as a method of the delegate. A delegate
    defines a method when the property of that name is a function. It may
    define any subset of the forwarded methods. Delegates are called in
    registration order. A method with a result has a default, which Qt
    returns when no delegate defines the method or when the delegate method
    throws. If a delegate method throws, the exception is logged and
    cleared. The throw does not change which other delegates are called.

    The forwarded methods are:

    \list
    \li \c onNewWant(): called on every delegate. The return value is
        ignored. The delegate must not modify the arguments. Qt does not
        forward the Wants that it sends to its own abilities.
    \li \c onContinue(): called on the first delegate that defines it. Its
        result is returned to the system. The default is
        \c {AbilityConstant.OnContinueResult.REJECT}.
    \endlist

    Methods not listed are not forwarded. Later versions may forward more
    methods.

    Qt holds a strong reference to the delegate until the handle is
    released. Releasing it unregisters the delegate, and no delegate method
    is called afterwards. A delegate method may release the handle.

    The handle can be released on any thread. Releasing it on another
    thread blocks that thread until the JS thread has unregistered the
    delegate.

    \sa {Qt for HarmonyOS Platform API}
*/
std::shared_ptr<void> QHarmonyAppJsContext::addUiAbilityDelegate(napi_value delegateValue)
{
    const char *funcInfo = Q_FUNC_INFO;

    if (!delegateValue || !Napi::Value(env(), delegateValue).IsObject()) {
        qOhosPrintfError("%s: the delegate is not a JS object, not registering it", funcInfo);
        return nullptr;
    }

    auto registrationHandle = tryRegisterUiAbilityDelegate(QNapi::Object(env(), delegateValue));
    if (!registrationHandle)
        return nullptr;

    return QtOhos::makeDestroyNotifier(
        [registrationHandle, jsThreadId = std::this_thread::get_id(), funcInfo]() mutable {
            if (std::this_thread::get_id() != jsThreadId) {
                qCDebug(
                    QtForOhos,
                    "%s: the delegate handle is released off the JS thread, waiting for the JS thread",
                    funcInfo);
            }
            QOhosJsThreadGateway::runAndWait(
                [&](QOhosJsState &) {
                    registrationHandle.reset();
                },
                funcInfo);
        });
}

QT_END_NAMESPACE
