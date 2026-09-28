// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qharmonyjsobjectref.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoslogger_p.h>

QT_BEGIN_NAMESPACE

namespace {

struct JsObjectIdentityRegistry
{
    QNapi::Reference<QNapi::Object> weakMap;
    std::int64_t nextId = 1;
};

std::int64_t jsObjectIdentity(QOhosJsState &jsState, QNapi::Object object)
{
    auto &registry = jsState.getAttachedObjectWithLazyCreate<JsObjectIdentityRegistry>();
    if (registry.weakMap.IsEmpty())
        registry.weakMap = QNapi::Reference<>::makePersistentFrom(
            QNapi::makeNewInstance(Napi::Env(jsState.env()).Global(), "WeakMap"));

    auto weakMap = registry.weakMap.Value();
    auto existingId = weakMap.eval("get(*)", {object});
    if (existingId.IsNumber())
        return QNapi::checkedCast<QNapi::Number>(existingId);

    const auto id = registry.nextId++;
    weakMap.eval("set(*)", {object, id});
    return id;
}

}

class QHarmonyJsObjectRef::Private
{
public:
    Private(QNapi::Reference<QNapi::Object> reference, std::int64_t identity)
        : reference(std::move(reference))
        , identity(identity)
    {
    }

    QNapi::Reference<QNapi::Object> reference;
    std::int64_t identity;
};

/*!
    \class QHarmonyJsObjectRef
    \inmodule QtCore
    \brief The QHarmonyJsObjectRef class keeps a JS object of a HarmonyOS
    application alive and lets other threads hold a reference to it.

    \reentrant

    A \c napi_value is valid only on the JS thread, and only until the task
    or callback that obtained it returns. A QHarmonyJsObjectRef holds a
    strong reference to the object instead. You can copy, store, and compare
    it on any thread. On the JS thread, value() returns the object again.

    The reference is released when the last copy is destroyed, on whichever
    thread that happens. The release itself runs asynchronously on the JS
    thread, so the JS object may outlive the last copy for a short time.

    Two references compare equal when they refer to the same JS object, even
    if they were created independently.

    \sa {Qt for HarmonyOS Platform API}
*/

/*!
    Constructs a null reference.
*/
QHarmonyJsObjectRef::QHarmonyJsObjectRef() = default;

/*!
    Constructs a copy of \a other, which refers to the same JS object.
*/
QHarmonyJsObjectRef::QHarmonyJsObjectRef(const QHarmonyJsObjectRef &other) = default;

/*!
    Move-constructs a reference from \a other. \a other becomes a null
    reference.
*/
QHarmonyJsObjectRef::QHarmonyJsObjectRef(QHarmonyJsObjectRef &&other) noexcept = default;

/*!
    Assigns \a other to this reference. This reference then refers to the
    same JS object as \a other.
*/
QHarmonyJsObjectRef &QHarmonyJsObjectRef::operator=(const QHarmonyJsObjectRef &other) = default;

/*!
    Move-assigns \a other to this reference. \a other becomes a null
    reference.
*/
QHarmonyJsObjectRef &QHarmonyJsObjectRef::operator=(QHarmonyJsObjectRef &&other) noexcept = default;

/*!
    Destroys the reference. If this is the last copy, the JS object is
    released asynchronously on the JS thread.
*/
QHarmonyJsObjectRef::~QHarmonyJsObjectRef() = default;

/*!
    Returns a reference to the JS object \a value, obtained in \a jsContext
    on the JS thread. Returns a null reference if \a value is not a JS
    object, that is, if it is \nullptr, JS \c null, \c undefined, or a
    primitive. Also returns a null reference if creating the reference
    fails. In that case, the error is logged and no exception is left
    pending.
*/
QHarmonyJsObjectRef QHarmonyJsObjectRef::fromValue(QHarmonyJsContext &jsContext, napi_value value)
{
    QHarmonyJsObjectRef ref;
    if (!value)
        return ref;

    Napi::Value napiValue(jsContext.env(), value);
    if (!napiValue.IsObject())
        return ref;

    auto object = QNapi::checkedCast<QNapi::Object>(napiValue);
    try {
        ref.d = QtOhos::makeProxyWithAsyncJsThreadDeleter(
            std::make_shared<Private>(
                QNapi::Reference<>::makePersistentFrom(object),
                jsObjectIdentity(QOhosJsThreadOps::instance().jsState(), object)));
    } catch (const Napi::Error &error) {
        qOhosPrintfWarning("%s: creating the reference failed: %s", Q_FUNC_INFO, error.what());
    }

    return ref;
}

/*!
    Returns the referenced JS object for use in \a jsContext on the JS
    thread, or \nullptr for a null reference. The returned value is valid
    until the current task or callback returns.
*/
napi_value QHarmonyJsObjectRef::value(QHarmonyJsContext &jsContext) const
{
    Q_UNUSED(jsContext);
    return d ? d->reference.Value() : QNapi::Object();
}

/*!
    Returns \c true if this reference does not refer to any JS object. You
    can call this function on any thread.
*/
bool QHarmonyJsObjectRef::isNull() const
{
    return !d;
}

std::int64_t QHarmonyJsObjectRef::identity() const noexcept
{
    return d ? d->identity : 0;
}

/*!
    \fn bool QHarmonyJsObjectRef::operator==(const QHarmonyJsObjectRef &lhs, const QHarmonyJsObjectRef &rhs)
    \fn bool QHarmonyJsObjectRef::operator!=(const QHarmonyJsObjectRef &lhs, const QHarmonyJsObjectRef &rhs)

    Returns whether \a lhs and \a rhs refer to the same JS object. Two null
    references are equal. You can call these operators on any thread.
*/
bool comparesEqual(const QHarmonyJsObjectRef &lhs, const QHarmonyJsObjectRef &rhs) noexcept
{
    return lhs.identity() == rhs.identity();
}

/*!
    \relates QHarmonyJsObjectRef

    Returns the hash value for \a key, using \a seed to seed the
    calculation. References that compare equal have the same hash value. You
    can call this function on any thread.
*/
size_t qHash(const QHarmonyJsObjectRef &key, size_t seed) noexcept
{
    return qHash(key.identity(), seed);
}

QT_END_NAMESPACE
