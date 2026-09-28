// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qharmonyjsonconversions.h"
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohosjsonconversions_p.h>
#include <QtCore/private/qohoslogger_p.h>

QT_BEGIN_NAMESPACE

/*!
    Returns a new JS object in \a env with the properties of \a jsonObject,
    for use on the JS thread. Nested objects and arrays become JS objects and
    arrays; JSON null becomes JS \c null. Returns \nullptr, logging the error
    and leaving no exception pending, if a JS exception is raised during the
    mapping.

    This is how a request object, for example a \c Want, is built for a
    platform API call from Qt data.
*/
napi_value QtHarmony::mapQJsonObjectToJsObject(napi_env env, const QJsonObject &jsonObject)
{
    try {
        return QtOhos::mapJsonObjectToNapiObject(env, jsonObject);
    } catch (const Napi::Error &error) {
        qOhosPrintfWarning("%s: mapping failed: %s", Q_FUNC_INFO, error.what());
        return nullptr;
    }
}

/*!
    Returns the enumerable properties of the JS object \a jsObject in \a env
    as a QJsonObject. Booleans, numbers and strings become JSON values of the
    same kind, nested objects and arrays become JSON objects and arrays, and
    JS \c null becomes JSON null. Property values with no JSON counterpart
    (\c undefined, functions, symbols, \c BigInt) are skipped in objects and
    become null in arrays. Returns an empty QJsonObject if \a jsObject is not
    a JS object (\nullptr, \c null, \c undefined, a primitive) or is an array
    or a function. It also returns an empty QJsonObject, logging the error and
    leaving no exception pending, if \a jsObject is nested too deeply, for
    example because it contains a reference cycle, or if a JS exception is
    raised while reading it, for example by a property getter.

    This is how a platform object, for example a received \c Want, is taken
    to the Qt thread.
*/
QJsonObject QtHarmony::mapJsObjectToQJsonObject(napi_env env, napi_value jsObject)
{
    Napi::Value value(env, jsObject);
    if (value.IsEmpty() || !value.IsObject() || value.IsArray() || value.IsFunction())
        return {};

    try {
        return QtOhos::mapNapiObjectToJsonObject(QNapi::checkedCast<QNapi::Object>(value));
    } catch (const Napi::Error &error) {
        qOhosPrintfWarning("%s: mapping failed: %s", Q_FUNC_INFO, error.what());
        return {};
    }
}

QT_END_NAMESPACE
