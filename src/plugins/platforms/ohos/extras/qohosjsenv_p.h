// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSENV_H
#define QOHOSJSENV_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoslogger_p.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <napi.h>
#include <napi/native_api.h>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras::Private {

struct QOhosJsEnv
{
    template <typename T, typename Enable = void>
    struct QNapiValue {
        static QNapi::Value create(napi_env /*env*/, T /*value*/) {
            static_assert(sizeof(T) == 0, "Unsupported type - provide proper QOhosJsEnv::QNapiValue"
                                          " overload");
        }
        static Napi::Maybe<T> get(const QNapi::Value &/*value*/) {
            static_assert(sizeof(T) == 0, "Unsupported type - provide proper QOhosJsEnv::QNapiValue"
                                          " overload");
        }
    };

    template <typename T>
    static QNapi::Value toNapiValue(napi_env env, T &&value) {
        return QNapiValue<typename std::decay<T>::type>::create(env, std::forward<T>(value));
    }

    template<typename ReturnType>
    static ReturnType fromNapiValue(const QNapi::Value &value)
    {
        return QNapiValue<typename std::decay<ReturnType>::type>::get(value).UnwrapOr(ReturnType());
    }
};

//
// overloads of QNapiValue to support various types
//

template<>
struct QOhosJsEnv::QNapiValue<QList<QJsonValue>>
{
    static QNapi::Value create(napi_env env, const QList<QJsonValue> &inputValue);
    static Napi::Maybe<QList<QJsonValue>> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QJsonValue>
{
    static QNapi::Value create(napi_env env, const QJsonValue &inputValue);
    static Napi::Maybe<QJsonValue> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QJsonArray>
{
    static QNapi::Value create(napi_env env, const QJsonArray &inputValue);
    static Napi::Maybe<QJsonArray> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QJsonObject>
{
    static QNapi::Value create(napi_env env, const QJsonObject &inputValue);
    static Napi::Maybe<QJsonObject> get(const QNapi::Value &inputValue);
};

inline QNapi::Value QOhosJsEnv::QNapiValue<QList<QJsonValue>>::create(napi_env env, const QList<QJsonValue> &inputValue)
{
    return QNapi::runEscapingHandleScope<QNapi::Array>(
        env,
        [&]() {
            auto outputArray = QNapi::Array::New(env, inputValue.length());
            for (int i = 0; i < inputValue.length(); ++i)
                outputArray.Set(i, QNapiValue<QJsonValue>::create(env, inputValue[i]));
            return outputArray;
        });
}

inline Napi::Maybe<QList<QJsonValue>> QOhosJsEnv::QNapiValue<QList<QJsonValue>>::get(const QNapi::Value &inputValue)
{
    if (!inputValue.IsArray())
        return Napi::Nothing<QList<QJsonValue>>();

    auto inputArray = QNapi::checkedCast<QNapi::Array>(inputValue);
    std::uint32_t rawInputArrayLength = inputArray.Length();
    if (rawInputArrayLength > std::numeric_limits<int>::max())
        return Napi::Nothing<QList<QJsonValue>>();
    int inputArrayLength = static_cast<int>(rawInputArrayLength);

    QList<QJsonValue> outputList;
    outputList.reserve(inputArrayLength);

    for (int i = 0; i < inputArrayLength; ++i) {
        Napi::HandleScope inputElementScope{inputValue.Env()};
        auto optOutputElem = QNapiValue<QJsonValue>::get(inputArray.Get(i));
        if (optOutputElem.IsNothing())
            break;
        outputList.append(optOutputElem.Unwrap());
    }

    return outputList.length() == inputArrayLength
        ? Napi::Just(outputList)
        : Napi::Nothing<QList<QJsonValue>>();
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QJsonValue>::create(napi_env env, const QJsonValue &inputValue)
{
    switch (inputValue.type()) {
    case QJsonValue::Type::Array:
        return QNapiValue<QJsonArray>::create(env, inputValue.toArray());
    case QJsonValue::Type::Bool:
        return QNapi::Boolean::New(env, inputValue.toBool());
    case QJsonValue::Type::Double:
        return QNapi::Number::New(env, inputValue.toDouble());
    case QJsonValue::Type::Null:
    case QJsonValue::Type::Undefined:
        return Napi::Env(env).Null();
    case QJsonValue::Type::Object:
        return QNapiValue<QJsonObject>::create(env, inputValue.toObject());
    case QJsonValue::Type::String:
        return QNapi::String::New(env, inputValue.toString().toStdString());
    }

    throw Napi::Error::New(env, "Got unsupported (impossible) QJsonValue");
}

inline Napi::Maybe<QJsonValue> QOhosJsEnv::QNapiValue<QJsonValue>::get(const QNapi::Value &inputValue)
{
    if (inputValue.IsArray()) {
        const auto arrayValue = QNapiValue<QJsonArray>::get(inputValue);
        return arrayValue.IsJust() ? Napi::Just<QJsonValue>(arrayValue.Unwrap()) : Napi::Nothing<QJsonValue>();
    } else if (inputValue.IsBoolean()) {
        const auto boolValue = QNapi::checkedCast<QNapi::Boolean>(inputValue).Value();
        return Napi::Just(QJsonValue(boolValue));
    } else if (inputValue.IsNumber()) {
        const auto numberValue = QNapi::checkedCast<QNapi::Number>(inputValue).DoubleValue();
        return Napi::Just(QJsonValue(numberValue));
    } else if (inputValue.IsObject()) {
        const auto objectValue = QNapiValue<QJsonObject>::get(inputValue);
        return objectValue.IsJust() ? Napi::Just<QJsonValue>(objectValue.Unwrap()) : Napi::Nothing<QJsonValue>();
    } else if (inputValue.IsString()) {
        const auto stringValue = QString::fromStdString(QNapi::checkedCast<QNapi::String>(inputValue));
        return Napi::Just<QJsonValue>(stringValue);
    } else {
        return Napi::Nothing<QJsonValue>();
    }
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QJsonArray>::create(napi_env env, const QJsonArray &inputValue)
{
    QList<QJsonValue> listInputValue;
    listInputValue.reserve(inputValue.size());
    std::copy(inputValue.begin(), inputValue.end(), std::back_inserter(listInputValue));
    return QNapiValue<QList<QJsonValue>>::create(env, listInputValue);
}

inline Napi::Maybe<QJsonArray> QOhosJsEnv::QNapiValue<QJsonArray>::get(const QNapi::Value &inputValue)
{
    const auto transform = [](const QList<QJsonValue> &input) -> QJsonArray {
        QJsonArray result;
        std::copy(input.begin(), input.end(), std::back_inserter(result));
        return result;
    };

    const auto listValue = QNapiValue<QList<QJsonValue>>::get(inputValue);
    return listValue.IsJust()
        ? Napi::Just(transform(listValue.Unwrap()))
        : Napi::Nothing<QJsonArray>();
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QJsonObject>::create(napi_env env, const QJsonObject &inputValue)
{
    return QNapi::runEscapingHandleScope<QNapi::Object>(
        env,
        [&]() {
            auto outputObject = QNapi::Object::New(env);
            for (auto inputValueIter = inputValue.begin(); inputValueIter != inputValue.end(); ++inputValueIter) {
                outputObject.Set(
                    inputValueIter.key().toStdString(),
                    QNapiValue<QJsonValue>::create(env, inputValueIter.value()));
            }
            return outputObject;
        });
}

inline Napi::Maybe<QJsonObject> QOhosJsEnv::QNapiValue<QJsonObject>::get(const QNapi::Value &inputValue)
{
    if (!inputValue.IsObject())
        return Napi::Nothing<QJsonObject>();

    auto inputObject = QNapi::checkedCast<QNapi::Object>(inputValue);

    QJsonObject result;
    bool allElementsSet = true;

    for (const auto &inputElement : inputObject) {
        if (inputElement.first.IsString()) {
            const auto key = QString::fromStdString(QNapi::checkedCast<QNapi::String>(inputElement.first));

            auto optPropValue = QNapiValue<QJsonValue>::get(inputElement.second);
            if (optPropValue.IsNothing()) {
                allElementsSet = false;
                break;
            }

            result[key] = optPropValue.Unwrap();
        }
    }

    return allElementsSet
        ? Napi::Just(result)
        : Napi::Nothing<QJsonObject>();
}

}

QT_END_NAMESPACE

#endif // QOHOSJSENV_H
