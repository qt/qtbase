// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosjsonconversions_p.h"
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qstring.h>
#include <cstdint>
#include <napi.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

QJsonValue mapNapiValueToJsonValue(const QNapi::Value &napiValue);
QNapi::Value mapJsonValueToNapiValue(napi_env env, const QJsonValue &jsonValue);

QJsonArray mapNapiArrayToJsonArray(const QNapi::Array &napiArray)
{
    QJsonArray jsonArray;
    const std::uint32_t arrayLength = napiArray.Length();
    for (std::uint32_t i = 0; i < arrayLength; ++i) {
        Napi::HandleScope elementScope(napiArray.Env());
        auto jsonValue = mapNapiValueToJsonValue(napiArray.Get(i));
        jsonArray.append(jsonValue.isUndefined() ? QJsonValue(QJsonValue::Null) : jsonValue);
    }
    return jsonArray;
}

QNapi::Array mapJsonArrayToNapiArray(napi_env env, const QJsonArray &jsonArray)
{
    return QNapi::runEscapingHandleScope<QNapi::Array>(
        env,
        [&]() {
            const auto arrayLength = static_cast<std::uint32_t>(jsonArray.size());
            auto napiArray = QNapi::Array::New(env, arrayLength);
            for (std::uint32_t i = 0; i < arrayLength; ++i)
                napiArray.Set(i, mapJsonValueToNapiValue(env, jsonArray.at(i)));
            return napiArray;
        });
}

QJsonValue mapNapiValueToJsonValue(const QNapi::Value &napiValue)
{
    if (napiValue.IsNull())
        return QJsonValue(QJsonValue::Null);
    if (napiValue.IsBoolean())
        return QJsonValue(QNapi::checkedCast<QNapi::Boolean>(napiValue).Value());
    if (napiValue.IsNumber())
        return QJsonValue(QNapi::checkedCast<QNapi::Number>(napiValue).DoubleValue());
    if (napiValue.IsString())
        return QJsonValue(QString::fromStdString(QNapi::checkedCast<QNapi::String>(napiValue)));
    if (napiValue.IsArray())
        return mapNapiArrayToJsonArray(QNapi::checkedCast<QNapi::Array>(napiValue));
    if (napiValue.IsObject() && !napiValue.IsFunction())
        return mapNapiObjectToJsonObject(QNapi::checkedCast<QNapi::Object>(napiValue));
    return QJsonValue(QJsonValue::Undefined);
}

QNapi::Value mapJsonValueToNapiValue(napi_env env, const QJsonValue &jsonValue)
{
    switch (jsonValue.type()) {
    case QJsonValue::Null:
    case QJsonValue::Undefined:
        return Napi::Env(env).Null();
    case QJsonValue::Bool:
        return QNapi::Boolean::New(env, jsonValue.toBool());
    case QJsonValue::Double:
        return QNapi::Number::New(env, jsonValue.toDouble());
    case QJsonValue::String:
        return QNapi::String::New(env, jsonValue.toString().toStdString());
    case QJsonValue::Array:
        return mapJsonArrayToNapiArray(env, jsonValue.toArray());
    case QJsonValue::Object:
        return mapJsonObjectToNapiObject(env, jsonValue.toObject());
    }
    Q_UNREACHABLE_RETURN(QNapi::Value());
}

}

QJsonObject mapNapiObjectToJsonObject(const QNapi::Object &napiObject)
{
    QJsonObject jsonObject;
    for (const auto &property : napiObject) {
        if (!property.first.IsString())
            continue;
        auto jsonValue = mapNapiValueToJsonValue(property.second);
        if (!jsonValue.isUndefined())
            jsonObject.insert(QString::fromStdString(QNapi::checkedCast<QNapi::String>(property.first)), jsonValue);
    }
    return jsonObject;
}

QNapi::Object mapJsonObjectToNapiObject(napi_env env, const QJsonObject &jsonObject)
{
    return QNapi::runEscapingHandleScope<QNapi::Object>(
        env,
        [&]() {
            auto napiObject = QNapi::Object::New(env);
            for (auto propertyIter = jsonObject.begin(); propertyIter != jsonObject.end(); ++propertyIter)
                napiObject.Set(propertyIter.key().toStdString(), mapJsonValueToNapiValue(env, propertyIter.value()));
            return napiObject;
        });
}

}

QT_END_NAMESPACE
