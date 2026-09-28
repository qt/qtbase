// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qharmonyjscontext.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoslogger_p.h>

QT_BEGIN_NAMESPACE

namespace {

QOhosJsState &jsState()
{
    return QOhosJsThreadOps::instance().jsState();
}

}

/*!
    \class QHarmonyJsContext
    \inmodule QtCore
    \brief The QHarmonyJsContext class gives code on the JS thread of a
    HarmonyOS application access to the JS runtime.

    On HarmonyOS, Qt runs the application on its own thread. The ArkTS
    runtime and the platform UI run on the main thread of the process,
    called the JS thread. Platform APIs are only available on the JS thread.
    Code that runs there receives a QHarmonyJsContext. JS callbacks created
    with \c napi_create_function() receive only a \c napi_env. They get the
    context with fromEnv().

    All member functions must be called on the JS thread. A \c napi_value
    that they return is valid until the code or callback that obtained it
    returns. It must not be passed to another thread.
*/

/*!
    \internal
*/
QHarmonyJsContext::QHarmonyJsContext() = default;

/*!
    \internal
*/
QHarmonyJsContext::~QHarmonyJsContext() = default;

/*!
    Returns the context of \a env. \a env must be the environment of the JS
    thread.

    Use this function in a JS callback created with
    \c napi_create_function(), which receives only the \c napi_env.
*/
QHarmonyJsContext &QHarmonyJsContext::fromEnv(napi_env env)
{
    Q_ASSERT(env == jsState().env());

    static QHarmonyJsContext jsContext;
    return jsContext;
}

/*!
    Returns the Node-API environment of the JS thread.
*/
napi_env QHarmonyJsContext::env()
{
    return jsState().env();
}

/*!
    Returns the JS module \a moduleName, for example
    \c {"@ohos.app.ability.AbilityConstant"}. If the module cannot be
    loaded, the function logs the error and returns \nullptr. No exception
    is left pending.

    The module is loaded on first use and then kept. Further calls with the
    same name return the same object, as with a JS \c import. Besides the
    platform's \c {@ohos.*} modules, the kit modules that the ArkTS
    application template bundled with Qt makes available to native code can
    be imported, such as \c {@kit.ShareKit.systemShare}.
*/
napi_value QHarmonyJsContext::tryImportModule(const std::string &moduleName)
{
    try {
        return jsState().tryGetModule(moduleName).value_or(QNapi::Object());
    } catch (const Napi::Error &error) {
        qOhosPrintfWarning("%s: importing '%s' failed: %s", Q_FUNC_INFO, moduleName.c_str(), error.what());
        return nullptr;
    }
}

QT_END_NAMESPACE
