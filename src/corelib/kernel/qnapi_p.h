// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNAPI_P_H
#define QNAPI_P_H

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

#include <QtCore/private/qohoslogger_p.h>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <napi.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QNapi {

template<typename... ExtraArgs>
class ExtendedCallbackFuncWrapper
{
public:
    template<typename Func>
    ExtendedCallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func(const Napi::CallbackInfo &, ExtraArgs...)>>::value, char *> = nullptr);

    template<typename Func>
    ExtendedCallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func(const Napi::CallbackInfo &, ExtraArgs...)>>::value, short *> = nullptr);

    template<typename Func>
    ExtendedCallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func(ExtraArgs...)>>::value, int *> = nullptr);

    template<typename Func>
    ExtendedCallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func(ExtraArgs...)>>::value, long *> = nullptr);

    std::function<Napi::Value(const Napi::CallbackInfo &, ExtraArgs...)> &callbackFunc();

private:
    std::function<Napi::Value(const Napi::CallbackInfo &, ExtraArgs...)> m_callbackFunc;
};

using CallbackFuncWrapper = ExtendedCallbackFuncWrapper<>;

using Value = Napi::Value;

using Boolean = Napi::Boolean;

using Number = Napi::Number;

using BigInt = Napi::BigInt;

using Date = Napi::Date;

using String = Napi::String;

using Symbol = Napi::Symbol;

using ArrayBuffer = Napi::ArrayBuffer;

using TypedArray = Napi::TypedArray;

using Function = Napi::Function;

using DataView = Napi::DataView;

template<typename BufferT>
using Buffer = Napi::Buffer<BufferT>;

template<typename ExternalT>
using External = Napi::External<ExternalT>;

template<typename T>
using TypedArrayOf = Napi::TypedArrayOf<T>;

class Promise;

class ValueWrapper
{
public:
    template<typename T, std::enable_if_t<!std::is_same<std::decay_t<T>, ValueWrapper>::value, int> = 0>
    ValueWrapper(T &&inputValue);

    Napi::Value mapToValue(napi_env env) const;

private:
    std::function<Napi::Value(napi_env)> m_valueFactory;
};

class Array : public Napi::Array
{
public:
    using Napi::Array::Array;

    static Array New(napi_env env, std::size_t length = 0);

    Array(const Napi::Array &other);
    Array &operator=(const Napi::Array &other);

    void fill(const ValueWrapper &value);
};

class Object : public Napi::Object
{
public:
    using Napi::Object::Object;

    static Object New(napi_env env);

    Object(const Napi::Object &other);
    Object &operator=(const Napi::Object &other);

    template<typename T = Value>
    T get(const std::string &expr) const;

    template<typename T = Value>
    T get(const Napi::Name &name) const;

    template<typename T = Value>
    T eval(const std::string &expr, const std::vector<ValueWrapper> &exprArgs = {}) const;

    QNapi::Promise evalToPromiseOrRejectOnThrow(const std::string &expr, const std::vector<ValueWrapper> &exprArgs = {}) const;

    void set(const std::string &name, const ValueWrapper &value);

    void set(const Napi::Name &name, const ValueWrapper &value);

    void set(const std::vector<std::pair<std::string, ValueWrapper>> &namedValues);

    template<typename Result = Value>
    Result call(const std::string &methodName, const std::vector<ValueWrapper> &args = {}) const;
};

template<typename Context>
class PromiseWithContext;

class Promise : public Napi::Promise
{
public:
    using Napi::Promise::Promise;

    Promise();
    Promise(const Napi::Promise &other);
    Promise &operator=(const Napi::Promise &other);

    Promise onThen(CallbackFuncWrapper &&onFulfilledFunc);
    Promise onThen(CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc);
    Promise onCatch(CallbackFuncWrapper &&onRejectedFunc);
    Promise onFinally(CallbackFuncWrapper &&onFinallyFunc);
    Promise onThenAndFinally(
        CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc,
        CallbackFuncWrapper &&onFinallyFunc);

    template<typename Context>
    PromiseWithContext<Context> withContext(Context &&context);
};

template<typename Context>
class PromiseWithContext : public Promise
{
public:
    PromiseWithContext(const Promise &promise, std::shared_ptr<Context> context);

    PromiseWithContext onThenWithContext(ExtendedCallbackFuncWrapper<Context &> &&onFulfilledFunc);
    PromiseWithContext onThenWithContext(
        ExtendedCallbackFuncWrapper<Context &> &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc);
    PromiseWithContext onCatchWithContext(ExtendedCallbackFuncWrapper<Context &> &&onRejectedFunc);
    PromiseWithContext onFinallyWithContext(ExtendedCallbackFuncWrapper<Context &> &&onFinallyFunc);

private:
    std::shared_ptr<Context> m_context;
};

template<typename T = void>
class Reference : public Napi::Reference<T>
{
public:
    using Napi::Reference<T>::Reference;

    Reference(Reference<T> &&other);
    Reference<T> &operator=(Reference<T> &&other);

    Reference(const Reference<T> &other) = delete;
    Reference<T> &operator=(const Reference<T> &other) = delete;

    Reference(Napi::Reference<T> &&other);
    Reference<T> &operator=(Napi::Reference<T> &&other);

    static Reference<T> makePersistentFrom(const T &value);

    static Reference<T> makeEmpty();
};

template<>
class Reference<Object> : public Napi::Reference<Object>
{
public:
    using Napi::Reference<Object>::Reference;

    Reference(Reference<Object> &&other);
    Reference<Object> &operator=(Reference<Object> &&other);

    Reference(const Reference<Object> &other) = delete;
    Reference<Object> &operator=(const Reference<Object> &other) = delete;

    Reference(Napi::Reference<Object> &&other);
    Reference<Object> &operator=(Napi::Reference<Object> &&other);

    template<typename T>
    T eval(const std::string &expr, const std::vector<ValueWrapper> &exprArgs = {}) const;

    QNapi::Promise evalToPromiseOrRejectOnThrow(const std::string &expr, const std::vector<ValueWrapper> &exprArgs = {}) const;

    void set(const std::string &name, const ValueWrapper &value);

    void set(const std::vector<std::pair<std::string, ValueWrapper>> &namedValues);

    template<typename Result = ::QNapi::Value>
    Result call(const std::string &methodName, const std::vector<ValueWrapper> &args = {}) const;

    static Reference<Object> makePersistentFrom(const Object &value);

    static Reference<Object> makeEmpty();
};

template<>
class Reference<void>
{
public:
    Reference() = delete;

    template<typename T>
    static std::enable_if_t<std::is_base_of<Napi::Value, T>::value, Reference<T>> makePersistentFrom(const T &value);

    template<typename T = Value>
    static Reference<T> makeEmpty();
};

class CallbackInfo
{
public:
    CallbackInfo(const Napi::CallbackInfo &cbInfo);

    CallbackInfo(const CallbackInfo &other) = delete;
    CallbackInfo &operator=(const CallbackInfo &other) = delete;

    Napi::Env Env() const;
    std::size_t Length() const;

    template<typename... Args>
    void getLeadingArgs(const std::string &funcName, Args &...args) const;

    template<typename Arg>
    Arg getFirstArg(const std::string &funcName) const;

    operator const Napi::CallbackInfo &() const;

private:
    const Napi::CallbackInfo &m_cbInfo;
};

std::vector<napi_value> unwrapValues(napi_env env, const std::vector<ValueWrapper> &wrappedValues);

Napi::Error makeLoggedException(napi_env env, const std::string &msg);

template<typename T>
bool valueTypeMatches(const Napi::Value &value);

template<typename Element>
bool arrayElementTypesMatch(const Napi::Value &value);

std::string getValueTypeString(const Napi::Value &value);

template<typename T, typename ValueDescriptionSupplier>
T checkedCast(const Napi::Value &value, ValueDescriptionSupplier &&valueDescSupplier);

template<typename T>
T checkedCast(const Napi::Value &value);

template<typename OutputContainer, typename Element, typename TransFunc>
OutputContainer getArrayElements(const Napi::Array &inputArray, TransFunc &&transFunc);

template<typename OutputContainer, typename Element>
OutputContainer getArrayElements(const Napi::Array &inputArray);

Object makeNewInstance(const Napi::Function &type, const std::vector<ValueWrapper> &args = {});

Object makeNewInstance(const Napi::Object &baseObj, const std::string &typePath, const std::vector<ValueWrapper> &args = {});

Napi::Value getPropOrUndefined(const Napi::Value &obj, const std::string &propName);

template<typename T>
Napi::Value getPropOrUndefined(const Napi::Reference<T> &objRef, const std::string &propName);

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, T>
getOptionalPropOrEmpty(const Napi::Object &optObj, const std::string &propName, const std::string &objDesc = {});

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, T>
getOptionalPropOrEmpty(const Napi::Object &optObj, const Napi::Name &propName, const std::string &objDesc = {});

Object makeObject(napi_env env, const std::vector<std::pair<std::string, ValueWrapper>> &namedValues = {});

Array makeArray(napi_env env, std::initializer_list<ValueWrapper> values = {});

template<typename InputContainer, typename TransFunc>
Array makeArray(napi_env env, InputContainer &&inputContainer, TransFunc &&transFunc);

template<typename InputContainer>
Array makeArray(napi_env env, InputContainer &&inputContainer);

template<typename Result = Napi::Value, typename F>
Result runEscapingHandleScope(napi_env env, F &&func);

std::string toJsonString(const Napi::Value &value);

namespace details_qnapi_p_h {

template<typename T>
struct ValueTypeTraits
{
};

template<>
struct ValueTypeTraits<Napi::Boolean>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsBoolean;
    static constexpr const char *typeName = "Boolean";
};

template<>
struct ValueTypeTraits<Napi::Number>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsNumber;
    static constexpr const char *typeName = "Number";
};

template<>
struct ValueTypeTraits<Napi::BigInt>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsBigInt;
    static constexpr const char *typeName = "BigInt";
};

template<>
struct ValueTypeTraits<Napi::Date>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsDate;
    static constexpr const char *typeName = "Date";
};

template<>
struct ValueTypeTraits<Napi::String>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsString;
    static constexpr const char *typeName = "String";
};

template<>
struct ValueTypeTraits<Napi::Symbol>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsSymbol;
    static constexpr const char *typeName = "Symbol";
};

template<>
struct ValueTypeTraits<Napi::Array>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsArray;
    static constexpr const char *typeName = "Array";
};

template<>
struct ValueTypeTraits<Array> : public ValueTypeTraits<Napi::Array>
{
};

template<>
struct ValueTypeTraits<Napi::ArrayBuffer>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsArrayBuffer;
    static constexpr const char *typeName = "ArrayBuffer";
};

template<>
struct ValueTypeTraits<Napi::TypedArray>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsTypedArray;
    static constexpr const char *typeName = "TypedArray";
};

template<>
struct ValueTypeTraits<Napi::Object>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsObject;
    static constexpr const char *typeName = "Object";
};

template<>
struct ValueTypeTraits<Object> : public ValueTypeTraits<Napi::Object>
{
};

template<>
struct ValueTypeTraits<Napi::Function>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsFunction;
    static constexpr const char *typeName = "Function";
};

template<>
struct ValueTypeTraits<Napi::Promise>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsPromise;
    static constexpr const char *typeName = "Promise";
};

template<>
struct ValueTypeTraits<Promise> : public ValueTypeTraits<Napi::Promise>
{
};

template<>
struct ValueTypeTraits<Napi::DataView>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsDataView;
    static constexpr const char *typeName = "DataView";
};

template<typename BufferT>
struct ValueTypeTraits<Napi::Buffer<BufferT>>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsBuffer;
    static constexpr const char *typeName = "Buffer";
};

template<typename ExternalT>
struct ValueTypeTraits<Napi::External<ExternalT>>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsExternal;
    static constexpr const char *typeName = "External";
};

template<typename T>
struct ValueTypeTraits<Napi::TypedArrayOf<T>>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsTypedArray;
    static constexpr const char *typeName = "TypedArray";
};

template<typename T>
constexpr bool isCallbackFuncType()
{
    return std::is_constructible<CallbackFuncWrapper, T>::value;
}

template<typename T>
inline std::enable_if_t<isCallbackFuncType<T>(), Napi::Value> makeValue(napi_env env, T &&inputValue)
{
    return Napi::Function::New(env, std::move(CallbackFuncWrapper(std::forward<T>(inputValue)).callbackFunc()));
}

template<typename T>
inline std::enable_if_t<!isCallbackFuncType<T>(), Napi::Value> makeValue(napi_env env, T &&inputValue)
{
    return Napi::Value::From(env, std::forward<T>(inputValue));
}

inline Napi::Error makeLoggedExceptionImpl(napi_env env, const std::string &msg)
{
    qOhosPrintfError("QNapi: exception: %s", msg.c_str());
    return Napi::Error::New(env, msg);
}

template<typename T>
std::enable_if_t<std::is_same<T, Napi::Value>::value, bool> valueTypeMatchesImpl(const Napi::Value &)
{
    return true;
}

template<typename T>
std::enable_if_t<!std::is_same<T, Napi::Value>::value, bool> valueTypeMatchesImpl(const Napi::Value &value)
{
    return (value.*ValueTypeTraits<T>::typeCheckMemFun)();
}

std::string getArrayElementValueTypeString(const Napi::Array &arrayValue);

inline std::string getValueTypeStringImpl(const Napi::Value &value)
{
    using namespace std::string_literals;

    if (value.IsArray()) {
        auto arrayElemTypeString = getArrayElementValueTypeString(value.As<Napi::Array>());
        return "Array<"s + arrayElemTypeString + ">"s;
    }

    // put subtypes of the Napi::Object at the beginning, so that we find most specific type name
    const std::pair<bool (Napi::Value::*)() const, const char *> typesChecksAndNames[] = {
        {&Napi::Value::IsEmpty, "<empty>"},
        {&Napi::Value::IsNull, "Null"},
        {&Napi::Value::IsUndefined, "Undefined"},
        {&Napi::Value::IsArray, "Array"},
        {&Napi::Value::IsArrayBuffer, "ArrayBuffer"},
        {&Napi::Value::IsTypedArray, "TypedArray"},
        {&Napi::Value::IsDataView, "DataView"},
        {&Napi::Value::IsFunction, "Function"},
        {&Napi::Value::IsPromise, "Promise"},
        {&Napi::Value::IsBigInt, "BigInt"},
        {&Napi::Value::IsBuffer, "Buffer"},
        {&Napi::Value::IsBoolean, "Boolean"},
        {&Napi::Value::IsDate, "Date"},
        {&Napi::Value::IsExternal, "External"},
        {&Napi::Value::IsNumber, "Number"},
        {&Napi::Value::IsObject, "Object"},
        {&Napi::Value::IsString, "String"},
        {&Napi::Value::IsSymbol, "Symbol"},
    };

    const char *foundTypeName = nullptr;
    for (const auto &typeCheckEntry : typesChecksAndNames) {
        if ((value.*(typeCheckEntry.first))()) {
            foundTypeName = typeCheckEntry.second;
            break;
        }
    }

    return foundTypeName != nullptr ? foundTypeName : "?";
}

inline std::string getArrayElementValueTypeString(const Napi::Array &arrayValue)
{
    constexpr std::size_t maxArrayElementsForTypeCheck = 10;

    std::size_t arrayLength = arrayValue.Length();
    auto checkRangeSize = std::min(arrayLength, maxArrayElementsForTypeCheck);

    std::string commonElemTypeString;
    for (std::size_t i = 0; i < checkRangeSize; ++i) {
        auto elemTypeString = getValueTypeStringImpl(arrayValue.Get(i));
        if (commonElemTypeString.empty()) {
            commonElemTypeString = elemTypeString;
        } else if (commonElemTypeString != elemTypeString) {
            commonElemTypeString = "";
            break;
        }
    }

    return !commonElemTypeString.empty()
        ? checkRangeSize == arrayLength
            ? commonElemTypeString
            : commonElemTypeString + "?"
        : "?";
}

template<typename T, typename ValueDescriptionSupplier>
std::enable_if_t<std::is_same<T, Napi::Value>::value, T> checkedCastImpl(
    const Napi::Value &value, ValueDescriptionSupplier &&)
{
    return value;
}

template<typename T, typename ValueDescriptionSupplier>
std::enable_if_t<!std::is_same<T, Napi::Value>::value, T> checkedCastImpl(
    const Napi::Value &value, ValueDescriptionSupplier &&valueDescSupplier)
{
    using namespace std::string_literals;

    if (!valueTypeMatchesImpl<T>(value)) {
        constexpr const char *expectedTypeName = ValueTypeTraits<T>::typeName;
        auto valueTypeStr = getValueTypeStringImpl(value);
        std::string valueDesc = valueDescSupplier();
        auto baseEerrorMsg =
            "wrong type (expected '"s + expectedTypeName + "', got '"s + valueTypeStr + "') of Napi value"s;
        throw makeLoggedExceptionImpl(
            value.Env(), valueDesc.empty() ? baseEerrorMsg : baseEerrorMsg + ": "s + valueDesc);
    }

    return T(value.Env(), value);
}

template<typename Result, typename F>
Result runEscapingHandleScopeImpl(napi_env env, F &&func)
{
    Napi::EscapableHandleScope scope(env);
    Result result = std::forward<F>(func)();
    return checkedCastImpl<Result>(
        scope.Escape(result),
        [&]() {
            return "value escaping from HandleScope";
        });
}

inline std::vector<napi_value> expandEvalCallArgs(
    napi_env env, const std::string &callArgsSubExpr, const std::vector<napi_value> &exprArgs)
{
    using namespace std::string_literals;

    if (callArgsSubExpr.empty()) {
        return {};
    } else if (callArgsSubExpr == "*") {
        return exprArgs;
    } else {
        throw makeLoggedException(
            env, "illegal call arguments in expression: '"s + callArgsSubExpr + "'"s);
    }
}

template<typename T>
std::pair<Napi::Object, T> evalWithContextImpl(
    Napi::Object obj, const std::string &expr, const std::vector<napi_value> &exprArgs = {})
{
    using namespace details_qnapi_p_h;
    using namespace std::string_literals;

    static const std::string newCallTag = "<new>";

    std::pair<Napi::Object, Napi::Value> result;

    auto lastDotPos = expr.rfind('.');

    if (!expr.empty() && expr.back() == ')') {
        auto openingBracketPos = expr.rfind('(');
        if (openingBracketPos == std::string::npos)
            throw makeLoggedExceptionImpl(obj.Env(), "missing opening bracket in '"s + expr + "'"s);
        bool newInstanceCall =
            openingBracketPos > newCallTag.size()
            && expr.compare(openingBracketPos - newCallTag.size(), newCallTag.size(), newCallTag) == 0;
        auto subFuncArgsExpr = expr.substr(
            openingBracketPos + 1, expr.size() - 1 - (openingBracketPos + 1));
        auto subFuncArgs = expandEvalCallArgs(obj.Env(), subFuncArgsExpr, exprArgs);
        auto subFuncExpr = expr.substr(
            0, newInstanceCall ? openingBracketPos - newCallTag.size() : openingBracketPos);
        auto subFuncWithCtx = evalWithContextImpl<Napi::Function>(obj, subFuncExpr, exprArgs);
        Napi::Value funcCallResult;
        try {
            funcCallResult =
                newInstanceCall
                    ? subFuncWithCtx.second.New(subFuncArgs)
                    : !subFuncWithCtx.first.IsEmpty()
                        ? subFuncWithCtx.second.Call(subFuncWithCtx.first, subFuncArgs)
                        : subFuncWithCtx.second.Call(subFuncArgs);
        } catch (const Napi::Error &error) {
            auto message = "QNapi: got exception from call '"s + expr + "': "s + error.what();
            qOhosPrintfError("%s", message.c_str());
            throw;
        }

        result = std::make_pair(Napi::Object(), funcCallResult);
    } else if (lastDotPos != std::string::npos) {
        auto subObjExpr = expr.substr(0, lastDotPos);
        auto propName = expr.substr(lastDotPos + 1);

        auto subObj = evalWithContextImpl<Napi::Object>(obj, subObjExpr, exprArgs).second;
        if (!subObj.Has(propName)) {
            throw makeLoggedExceptionImpl(
                obj.Env(), "object '"s + subObjExpr + "' has no property named '"s + propName + "'"s);
        }

        result = std::make_pair(subObj, subObj.Get(propName));
    } else {
        if (!obj.Has(expr)) {
            throw makeLoggedExceptionImpl(
                obj.Env(), "object has no property named '"s + expr + "'"s);
        }

        result = std::make_pair(obj, obj.Get(expr));
    }

    return std::make_pair(result.first, checkedCastImpl<T>(result.second, [&]() { return expr; }));
}

inline Napi::Value callMethodWithValueResultImpl(
    const Napi::Object &obj, const std::string &methodName, const std::vector<napi_value> &args)
{
    using namespace std::string_literals;
    return runEscapingHandleScopeImpl<Napi::Value>(
        obj.Env(),
        [&]() {
            auto funcWithCtx = evalWithContextImpl<Napi::Function>(obj, methodName);
            Napi::Value funcResult;
            try {
                funcResult = funcWithCtx.second.Call(funcWithCtx.first, args);
            } catch (const Napi::Error &error) {
                auto message = "QNapi: got exception from method call '"s + methodName + "': "s + error.what();
                qOhosPrintfError("%s", message.c_str());
                throw;
            }

            return funcResult;
        });
}

template<typename Result>
Result callMethodImpl(const Napi::Object &obj, const std::string &methodName, const std::vector<napi_value> &args)
{
    using namespace std::string_literals;

    return checkedCastImpl<Result>(
        callMethodWithValueResultImpl(obj, methodName, args),
        [&]() {
            return "result of '"s + methodName + "' method call"s;
        });
}

template<typename Result = Value>
Result callMethod(const Napi::Object &obj, const std::string &methodName, const std::vector<ValueWrapper> &args)
{
    return runEscapingHandleScopeImpl<Result>(
        obj.Env(),
        [&]() {
            return callMethodImpl<Result>(obj, methodName, unwrapValues(obj.Env(), args));
        });
}

template<typename Arg>
void getArgImpl(const std::string &funcName, const Napi::CallbackInfo &cbInfo, Arg &arg, std::size_t argIndex)
{
    using namespace std::string_literals;

    arg = checkedCastImpl<Arg>(
        cbInfo[argIndex],
        [&]() {
            return "arg #"s + std::to_string(argIndex) + " of '"s + funcName + "' func call"s;
        });
}

template<typename... Args, std::size_t... Is>
void getLeadingArgsImpl(
    const std::string &funcName, const Napi::CallbackInfo &cbInfo, std::tuple<Args...> args, std::index_sequence<Is...>)
{
    using namespace std::string_literals;

    if (cbInfo.Length() < sizeof...(Args)) {
        throw makeLoggedExceptionImpl(
            cbInfo.Env(),
            "getArgs: func '"s + funcName + "' received less args than expected minimum: "s
            + std::to_string(cbInfo.Length()) + " vs "s + std::to_string(sizeof...(Args)));
    }

    auto unused = {(getArgImpl(funcName, cbInfo, std::get<Is>(args), Is), 0)..., 0};
    (void) unused;
}

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, T>
getOptionalPropOrEmptyImpl(const Napi::Object &optObj, const Napi::Name &propName, const std::string &objDesc)
{
    using namespace std::string_literals;

    if (optObj.IsEmpty() || !optObj.Has(propName))
        return T();

    auto propValue = optObj.Get(propName);

    return !propValue.IsUndefined()
        ? QNapi::checkedCast<T>(
            propValue,
            [&]() {
                auto baseDesc = "property '"s + propName.ToString().Utf8Value() + "' of object"s;
                return !objDesc.empty()
                    ? baseDesc + ": "s + objDesc
                    : baseDesc;
            })
        : T();
}

template<typename Container, typename Element>
constexpr decltype(auto) forwardContainerElement(Element &element) noexcept
{
    using Result = std::conditional_t<std::is_lvalue_reference<Container>::value, Element &, Element &&>;
    return static_cast<Result>(element);
}

}

template<typename T, std::enable_if_t<!std::is_same<std::decay_t<T>, ValueWrapper>::value, int>>
ValueWrapper::ValueWrapper(T &&inputValue)
    : m_valueFactory(
        [inputValue = std::forward<T>(inputValue)](napi_env env) {
            return details_qnapi_p_h::makeValue(env, inputValue);
        })
{
}

inline Napi::Value ValueWrapper::mapToValue(napi_env env) const
{
    return m_valueFactory(env);
}

template<typename... ExtraArgs>
template<typename Func>
ExtendedCallbackFuncWrapper<ExtraArgs...>::ExtendedCallbackFuncWrapper(
    Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func(const Napi::CallbackInfo &, ExtraArgs...)>>::value, char *>)
    : m_callbackFunc(
        [callbackFunc = std::make_shared<std::decay_t<Func>>(std::move(callbackFunc))](const Napi::CallbackInfo &cbInfo, ExtraArgs... extraArgs) {
            (*callbackFunc)(cbInfo, extraArgs...);
            return Napi::Value();
        })
{
}

template<typename... ExtraArgs>
template<typename Func>
ExtendedCallbackFuncWrapper<ExtraArgs...>::ExtendedCallbackFuncWrapper(
    Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func(const Napi::CallbackInfo &, ExtraArgs...)>>::value, short *>)
    : m_callbackFunc(
        [callbackFunc = std::make_shared<std::decay_t<Func>>(std::move(callbackFunc))](const Napi::CallbackInfo &cbInfo, ExtraArgs... extraArgs) {
            return (*callbackFunc)(cbInfo, extraArgs...);
        })
{
}

template<typename... ExtraArgs>
template<typename Func>
ExtendedCallbackFuncWrapper<ExtraArgs...>::ExtendedCallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func(ExtraArgs...)>>::value, int *>)
    : m_callbackFunc(
        [callbackFunc = std::make_shared<std::decay_t<Func>>(std::move(callbackFunc))](const Napi::CallbackInfo &, ExtraArgs... extraArgs) {
            (*callbackFunc)(extraArgs...);
            return Napi::Value();
        })
{
}

template<typename... ExtraArgs>
template<typename Func>
ExtendedCallbackFuncWrapper<ExtraArgs...>::ExtendedCallbackFuncWrapper(
    Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func(ExtraArgs...)>>::value, long *>)
    : m_callbackFunc(
        [callbackFunc = std::make_shared<std::decay_t<Func>>(std::move(callbackFunc))](const Napi::CallbackInfo &, ExtraArgs... extraArgs) {
            return (*callbackFunc)(extraArgs...);
        })
{
}

template<typename... ExtraArgs>
std::function<Napi::Value(const Napi::CallbackInfo &, ExtraArgs... extraArgs)> &ExtendedCallbackFuncWrapper<ExtraArgs...>::callbackFunc()
{
    return m_callbackFunc;
}

inline Object Object::New(napi_env env)
{
    return Napi::Object::New(env);
}

inline Array Array::New(napi_env env, std::size_t length)
{
    return length != 0
        ? Napi::Array::New(env, length)
        : Napi::Array::New(env);
}

inline Array::Array(const Napi::Array &other)
    : Napi::Array(other)
{
}

inline Array &Array::operator=(const Napi::Array &other)
{
    Napi::Array::operator=(other);
    return *this;
}

inline void Array::fill(const ValueWrapper &value)
{
    details_qnapi_p_h::callMethod(*this, "fill", {value});
}

inline Object::Object(const Napi::Object &other)
    : Napi::Object(other)
{
}

inline Object &Object::operator=(const Napi::Object &other)
{
    Napi::Object::operator=(other);
    return *this;
}

template<typename T>
T Object::get(const std::string &expr) const
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<T>(
        Env(),
        [&]() {
            return details_qnapi_p_h::evalWithContextImpl<T>(*this, expr).second;
        });
}

template<typename T>
T Object::get(const Napi::Name &name) const
{
    using namespace std::string_literals;

    if (!Has(name)) {
        throw makeLoggedException(
            Env(), "object has no property '"s + name.ToString().Utf8Value() + "'"s);
    }

    return checkedCast<T>(
        Get(name),
        [&]() {
            return "property '"s + name.ToString().Utf8Value() + "'"s;
        });
}

template<typename T>
T Object::eval(const std::string &expr, const std::vector<ValueWrapper> &exprArgs) const
{
    using namespace details_qnapi_p_h;

    return runEscapingHandleScope<T>(
        Env(),
        [&]() {
            return evalWithContextImpl<T>(*this, expr, unwrapValues(Env(), exprArgs)).second;
        });
}

inline QNapi::Promise Object::evalToPromiseOrRejectOnThrow(
    const std::string &expr, const std::vector<ValueWrapper> &exprArgs) const
{
    return runEscapingHandleScope<QNapi::Promise>(
        Env(),
        [&]() {
            try {
                return eval<QNapi::Promise>(expr, exprArgs);
            } catch (const Napi::Error &error) {
                auto deferred = Napi::Promise::Deferred::New(Env());
                deferred.Reject(error.Value());
                return QNapi::Promise(deferred.Promise());
            }
        });
}

inline void Object::set(const std::string &name, const ValueWrapper &value)
{
    Napi::HandleScope setPropScope(Env());
    Set(name, value.mapToValue(Env()));
}

inline void Object::set(const Napi::Name &name, const ValueWrapper &value)
{
    Napi::HandleScope setPropScope(Env());
    Set(name, value.mapToValue(Env()));
}

inline void Object::set(const std::vector<std::pair<std::string, ValueWrapper>> &namedValues)
{
    Napi::HandleScope setPropsScope(Env());
    for (const auto &namedValue : namedValues)
        Set(namedValue.first, namedValue.second.mapToValue(Env()));
}

template<typename Result>
Result Object::call(const std::string &methodName, const std::vector<ValueWrapper> &args) const
{
    return details_qnapi_p_h::callMethod<Result>(*this, methodName, args);
}

inline Promise::Promise()
    : Napi::Promise(nullptr, nullptr)
{
}

inline Promise::Promise(const Napi::Promise &other)
    : Napi::Promise(other)
{
}

inline Promise &Promise::operator=(const Napi::Promise &other)
{
    Napi::Promise::operator=(other);
    return *this;
}

inline Promise Promise::onThen(CallbackFuncWrapper &&onFulfilledFunc)
{
    return details_qnapi_p_h::callMethod<Promise>(
        *this, "then", {std::move(onFulfilledFunc.callbackFunc())});
}

inline Promise Promise::onThen(CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc)
{
    return details_qnapi_p_h::callMethod<Promise>(
        *this, "then", {std::move(onFulfilledFunc.callbackFunc()), std::move(onRejectedFunc.callbackFunc())});
}

inline Promise Promise::onCatch(CallbackFuncWrapper &&onRejectedFunc)
{
    return details_qnapi_p_h::callMethod<Promise>(
        *this, "catch", {std::move(onRejectedFunc.callbackFunc())});
}

inline Promise Promise::onFinally(CallbackFuncWrapper &&onFinallyFunc)
{
    return details_qnapi_p_h::callMethod<Promise>(
        *this, "finally", {std::move(onFinallyFunc.callbackFunc())});
}

inline Promise Promise::onThenAndFinally(
    CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc,
    CallbackFuncWrapper &&onFinallyFunc)
{
    return onThen(std::move(onFulfilledFunc), std::move(onRejectedFunc)).onFinally(std::move(onFinallyFunc));
}

template<typename Context>
PromiseWithContext<Context> Promise::withContext(Context &&context)
{
    return PromiseWithContext<Context>(*this, std::make_shared<Context>(std::forward<Context>(context)));
}

template<typename Context>
PromiseWithContext<Context>::PromiseWithContext(const Promise &promise, std::shared_ptr<Context> context)
    : Promise(promise)
    , m_context(context)
{
}

template<typename Context>
PromiseWithContext<Context> PromiseWithContext<Context>::onThenWithContext(
    ExtendedCallbackFuncWrapper<Context &> &&onFulfilledFunc)
{
    return PromiseWithContext(
        Promise::onThen(
            [context = m_context, onFulfilledFunc = std::move(onFulfilledFunc.callbackFunc())](const Napi::CallbackInfo &cbInfo) {
                return onFulfilledFunc(cbInfo, *context);
            }),
        m_context);
}

template<typename Context>
PromiseWithContext<Context> PromiseWithContext<Context>::onThenWithContext(
    ExtendedCallbackFuncWrapper<Context &> &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc)
{
    return PromiseWithContext(
        Promise::onThen(
            [context = m_context, onFulfilledFunc = std::move(onFulfilledFunc.callbackFunc())](const Napi::CallbackInfo &cbInfo) {
                return onFulfilledFunc(cbInfo, *context);
            },
            [context = m_context, onRejectedFunc = std::move(onRejectedFunc.callbackFunc())](const Napi::CallbackInfo &cbInfo) {
                return onRejectedFunc(cbInfo, *context);
            }),
        m_context);
}

template<typename Context>
PromiseWithContext<Context> PromiseWithContext<Context>::onCatchWithContext(
    ExtendedCallbackFuncWrapper<Context &> &&onRejectedFunc)
{
    return PromiseWithContext(
        Promise::onCatch(
            [context = m_context, onRejectedFunc = std::move(onRejectedFunc.callbackFunc())](const Napi::CallbackInfo &cbInfo) {
                return onRejectedFunc(cbInfo, *context);
            }),
        m_context);
}

template<typename Context>
PromiseWithContext<Context> PromiseWithContext<Context>::onFinallyWithContext(
    ExtendedCallbackFuncWrapper<Context &> &&onFinallyFunc)
{
    return PromiseWithContext(
        Promise::onFinally(
            [context = m_context, onFinallyFunc = std::move(onFinallyFunc.callbackFunc())](const Napi::CallbackInfo &cbInfo) {
                return onFinallyFunc(cbInfo, *context);
            }),
        m_context);
}

template<typename T>
Reference<T>::Reference(Reference<T> &&other)
    : Napi::Reference<T>(std::move(other))
{
}

template<typename T>
Reference<T> &Reference<T>::operator=(Reference<T> &&other)
{
    Napi::Reference<T>::operator=(std::move(other));
    return *this;
}

template<typename T>
Reference<T>::Reference(Napi::Reference<T> &&other)
    : Napi::Reference<T>(std::move(other))
{
}

template<typename T>
Reference<T> &Reference<T>::operator=(Napi::Reference<T> &&other)
{
    Napi::Reference<T>::operator=(std::move(other));
    return *this;
}

template<typename T>
Reference<T> Reference<T>::makePersistentFrom(const T &value)
{
    return Napi::Persistent(value);
}

template<typename T>
Reference<T> Reference<T>::makeEmpty()
{
    return Napi::Reference<T>();
}

inline Reference<Object>::Reference(Reference<Object> &&other)
    : Napi::Reference<Object>(std::move(other))
{
}

inline Reference<Object> &Reference<Object>::operator=(Reference<Object> &&other)
{
    Napi::Reference<Object>::operator=(std::move(other));
    return *this;
}

inline Reference<Object>::Reference(Napi::Reference<Object> &&other)
    : Napi::Reference<Object>(std::move(other))
{
}

inline Reference<Object> &Reference<Object>::operator=(Napi::Reference<Object> &&other)
{
    Napi::Reference<Object>::operator=(std::move(other));
    return *this;
}

template<typename T>
T Reference<Object>::eval(const std::string &expr, const std::vector<ValueWrapper> &exprArgs) const
{
    return runEscapingHandleScope<T>(
        Env(),
        [&]() {
            return Value().eval<T>(expr, exprArgs);
        });
}

inline QNapi::Promise Reference<Object>::evalToPromiseOrRejectOnThrow(
    const std::string &expr, const std::vector<ValueWrapper> &exprArgs) const
{
    return runEscapingHandleScope<QNapi::Promise>(
        Env(),
        [&]() {
            return Value().evalToPromiseOrRejectOnThrow(expr, exprArgs);
        });
}

inline void Reference<Object>::set(const std::string &name, const ValueWrapper &value)
{
    Napi::HandleScope scope(Env());
    Value().set(name, value);
}

inline void Reference<Object>::set(const std::vector<std::pair<std::string, ValueWrapper>> &namedValues)
{
    Napi::HandleScope scope(Env());
    Value().set(namedValues);
}

template<typename Result>
Result Reference<Object>::call(const std::string &methodName, const std::vector<ValueWrapper> &args) const
{
    return runEscapingHandleScope<Result>(
        Env(),
        [&]() {
            return Value().call<Result>(methodName, args);
        });
}

inline Reference<Object> Reference<Object>::makePersistentFrom(const Object &value)
{
    return Napi::Persistent(value);
}

inline Reference<Object> Reference<Object>::makeEmpty()
{
    return Napi::Reference<Object>();
}

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, Reference<T>>
Reference<void>::makePersistentFrom(const T &value)
{
    return Napi::Persistent(value);
}

template<typename T>
Reference<T> Reference<void>::makeEmpty()
{
    return Napi::Reference<T>();
}

inline CallbackInfo::CallbackInfo(const Napi::CallbackInfo &cbInfo)
    : m_cbInfo(cbInfo)
{
}

inline Napi::Env CallbackInfo::Env() const
{
    return m_cbInfo.Env();
}

inline std::size_t CallbackInfo::Length() const
{
    return m_cbInfo.Length();
}

template<typename... Args>
void CallbackInfo::getLeadingArgs(const std::string &funcName, Args &...args) const
{
    details_qnapi_p_h::getLeadingArgsImpl(
        funcName, m_cbInfo, std::tie(args...), std::make_index_sequence<sizeof...(Args)>());
}

template<typename Arg>
Arg CallbackInfo::getFirstArg(const std::string &funcName) const
{
    Arg arg;
    getLeadingArgs(funcName, arg);
    return arg;
}

inline CallbackInfo::operator const Napi::CallbackInfo &() const
{
    return m_cbInfo;
}

inline std::vector<napi_value> unwrapValues(napi_env env, const std::vector<ValueWrapper> &wrappedValues)
{
    std::vector<napi_value> unwrappedValues;
    std::transform(
        wrappedValues.begin(), wrappedValues.end(),
        std::back_inserter(unwrappedValues),
        [&](const ValueWrapper &arg) {
            return arg.mapToValue(env);
        });
    return unwrappedValues;
}

inline Napi::Error makeLoggedException(napi_env env, const std::string &msg)
{
    return details_qnapi_p_h::makeLoggedExceptionImpl(env, msg);
}

template<typename T>
bool valueTypeMatches(const Napi::Value &value)
{
    return details_qnapi_p_h::valueTypeMatchesImpl<T>(value);
}

template<typename Element>
bool arrayElementTypesMatch(const Napi::Value &value)
{
    if (!valueTypeMatches<Napi::Array>(value))
        return false;

    auto arrayValue = checkedCast<Napi::Array>(value);
    auto arrayLength = arrayValue.Length();

    bool allElementsMatch = true;
    for (std::size_t i = 0; i < arrayLength; ++i) {
        if (!valueTypeMatches<Element>(arrayValue.Get(i))) {
            allElementsMatch = false;
            break;
        }
    }

    return allElementsMatch;
}

inline std::string getValueTypeString(const Napi::Value &value)
{
    return details_qnapi_p_h::getValueTypeStringImpl(value);
}

template<typename T, typename ValueDescriptionSupplier>
T checkedCast(const Napi::Value &value, ValueDescriptionSupplier &&valueDescSupplier)
{
    return details_qnapi_p_h::checkedCastImpl<T>(
        value, std::forward<ValueDescriptionSupplier>(valueDescSupplier));
}

template<typename T>
T checkedCast(const Napi::Value &value)
{
    return details_qnapi_p_h::checkedCastImpl<T>(
        value,
        []() {
            return std::string();
        });
}

template<typename OutputContainer, typename Element, typename TransFunc>
OutputContainer getArrayElements(const Napi::Array &inputArray, TransFunc &&transFunc)
{
    using namespace std::string_literals;

    OutputContainer result;
    auto arrayLength = inputArray.Length();
    for (std::size_t i = 0; i < arrayLength; ++i) {
        auto arg = inputArray.Get(i);
        if (!valueTypeMatches<Element>(arg)) {
            constexpr const char *expectedTypeName = details_qnapi_p_h::ValueTypeTraits<Element>::typeName;
            auto argTypeStr = getValueTypeString(arg);
            throw makeLoggedException(
                inputArray.Env(),
                "wrong type of Napi array element #"s + std::to_string(i)
                + ", expected '"s + expectedTypeName + "', got '"s + argTypeStr + "'"s);
        }
        result.insert(result.end(), transFunc(checkedCast<Element>(arg)));
    }

    return result;
}

template<typename OutputContainer, typename Element>
OutputContainer getArrayElements(const Napi::Array &inputArray)
{
    return getArrayElements<OutputContainer, Element>(
        inputArray,
        [](Element &&elem) {
            return std::forward<Element>(elem);
        });
}

inline Object makeNewInstance(const Napi::Function &type, const std::vector<ValueWrapper> &args)
{
    return type.New(unwrapValues(type.Env(), args));
}

inline Object makeNewInstance(const Napi::Object &baseObj, const std::string &typePath, const std::vector<ValueWrapper> &args)
{
    return makeNewInstance(Object(baseObj).get<Napi::Function>(typePath), args);
}

inline Napi::Value getPropOrUndefined(const Napi::Value &obj, const std::string &propName)
{
    if (!obj.IsObject())
        return obj.Env().Undefined();

    auto typedObj = checkedCast<Napi::Object>(obj);

    return typedObj.Has(propName)
        ? typedObj.Get(propName)
        : typedObj.Env().Undefined();
}

template<typename T>
Napi::Value getPropOrUndefined(const Napi::Reference<T> &objRef, const std::string &propName)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<Napi::Value>(
        objRef.Env(),
        [&]() {
            return getPropOrUndefined(objRef.Value(), propName);
        });
}

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, T>
getOptionalPropOrEmpty(const Napi::Object &optObj, const std::string &propName, const std::string &objDesc)
{
    return !optObj.IsEmpty()
        ? details_qnapi_p_h::getOptionalPropOrEmptyImpl<T>(
            optObj, Napi::String::New(optObj.Env(), propName), objDesc)
        : T();
}

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, T>
getOptionalPropOrEmpty(const Napi::Object &optObj, const Napi::Name &propName, const std::string &objDesc)
{
    return details_qnapi_p_h::getOptionalPropOrEmptyImpl<T>(optObj, propName, objDesc);
}

inline Object makeObject(napi_env env, const std::vector<std::pair<std::string, ValueWrapper>> &namedValues)
{
    auto obj = QNapi::Object::New(env);
    obj.set(namedValues);
    return obj;
}

inline Array makeArray(napi_env env, std::initializer_list<ValueWrapper> values)
{
    return makeArray<std::initializer_list<ValueWrapper> &>(env, values);
}

template<typename InputContainer, typename TransFunc>
Array makeArray(napi_env env, InputContainer &&inputContainer, TransFunc &&transFunc)
{
    using namespace details_qnapi_p_h;

    return runEscapingHandleScopeImpl<Napi::Array>(
        env,
        [&]() {
            auto containerSize = static_cast<std::size_t>(
                std::distance(std::begin(inputContainer), std::end(inputContainer)));
            auto array = Napi::Array::New(env, containerSize);
            std::size_t i = 0;
            for (auto &element : inputContainer) {
                array.Set(i, ValueWrapper(transFunc(forwardContainerElement<InputContainer>(element))).mapToValue(env));
                ++i;
            }
            return array;
        });
}

template<typename InputContainer>
Array makeArray(napi_env env, InputContainer &&inputContainer)
{
    return makeArray(
        env, std::forward<InputContainer>(inputContainer),
        [](auto &&element) -> decltype(auto) {
            return std::forward<decltype(element)>(element);
        });
}

template<typename Result, typename F>
Result runEscapingHandleScope(napi_env env, F &&func)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<Result>(env, std::forward<F>(func));
}

inline std::string toJsonString(const Napi::Value &value)
{
    std::string jsonString;
    try {
        Napi::HandleScope stringifyScope(value.Env());
        jsonString = details_qnapi_p_h::callMethod<String>(value.Env().Global(), "JSON.stringify", {value});
    } catch (...) {
        jsonString = "<stringify-error>";
    }
    return jsonString;
}

}

QT_END_NAMESPACE

#endif // QNAPI_P_H
