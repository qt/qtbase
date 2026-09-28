// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qharmonyappjscontext.h"
#include <QtCore/private/qcore_ohos_p.h>

QT_BEGIN_NAMESPACE

namespace {

QOhosJsState &jsState()
{
    return QOhosJsThreadOps::instance().jsState();
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
    only the runtime. Code gets the application context once, with
    fromJsContext(), and passes it on.

    The class derives from QHarmonyJsContext, and there is only one object.
    QHarmonyJsContext::fromEnv() returns it as the base class. All member
    functions must be called on the JS thread. References to it and its
    \c napi_value results follow the lifetime rules of QHarmonyJsContext.
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

QT_END_NAMESPACE
