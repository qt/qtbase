// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSUTILS_H
#define QOHOSUTILS_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlogging.h>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <qohosplugincore.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtOhos
{

namespace qohosutils_details {

QOhosOptional<std::uintmax_t> tryParseStringAsUIntMax(const std::string &inputString);

}

template<typename IdValueType, typename TypeTag>
class TypedId
{
public:
    using ValueType = IdValueType;

    explicit TypedId(IdValueType value);
    TypedId() = default;

    TypedId(const TypedId &other);
    TypedId &operator=(const TypedId &other);

    TypedId(TypedId &&other);
    TypedId &operator=(TypedId &&other);

    ~TypedId() = default;

    bool operator==(const TypedId &other) const;
    bool operator!=(const TypedId &other) const;
    bool operator<(const TypedId &other) const;

    IdValueType value() const;

private:
    IdValueType m_value;
};

template<typename ...Ts>
using Conjuction = std::integral_constant<
    bool, std::make_tuple(bool(Ts::value)...) == std::make_tuple(((void) Ts::value, true)...)>;

template<typename ForwardIt, typename Predicate>
ForwardIt removeMatchingWithLookahead(ForwardIt firstIt, ForwardIt lastIt, Predicate &&predicate);

struct EvaluateSequentially
{
    template<typename... Args>
    EvaluateSequentially(Args && ...) {}
};

template<typename UnaryFunc, typename... TupleElements, std::size_t... TupleIndices>
void tupleForEach(const std::tuple<TupleElements...> &tuple, UnaryFunc func, std::index_sequence<TupleIndices...>);

template<typename UnaryFunc, typename... TupleElements>
void tupleForEach(const std::tuple<TupleElements...> &tuple, UnaryFunc func);

template<typename T>
QOhosConsumer<T> makeCompressingAsyncConsumer(
    QOhosConsumer<T> baseConsumer, QOhosConsumer<std::function<void()>> asyncExecutor);

template<typename ...Ts, typename BaseConsumer>
std::enable_if_t<std::is_assignable<QOhosConsumer<Ts...>, BaseConsumer>::value, std::function<bool(Ts...)>>
makeCallOnceConsumerWrapper(BaseConsumer &&baseConsumer);

QOhosOptional<double> tryParseStringAsFiniteDouble(const std::string &inputString);

template<typename T>
std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, QOhosOptional<T>>
tryParseStringAsUnsignedInteger(const std::string &inputString);

std::string printfToString(const char *format, ...) Q_ATTRIBUTE_FORMAT_PRINTF(1, 2);

const char *mapBoolToTrueFalseStr(bool value);

std::shared_ptr<QtOhos::QAbilityPeer> tryMapOptMainWindowToAbilityPeer(
    QtOhos::JsState &jsState, QOhosOptional<QtOhos::QObjectThreadSafeRef> optInstanceMainWindowRef);

template<typename ForwardIt, typename Predicate>
ForwardIt removeMatchingWithLookahead(ForwardIt firstIt, ForwardIt lastIt, Predicate &&predicate)
{
    if (firstIt == lastIt)
        return firstIt;

    auto outputIt = firstIt;
    auto currentIt = firstIt;
    auto nextIt = currentIt;
    ++nextIt;

    while (nextIt != lastIt) {
        if (!predicate(*currentIt, *nextIt)) {
            if (outputIt != currentIt) {
                *outputIt = std::move(*currentIt);
            }
            ++outputIt;
        }
        ++currentIt;
        ++nextIt;
    }

    if (outputIt != currentIt) {
        *outputIt = std::move(*currentIt);
    }
    ++outputIt;

    return outputIt;
}

template<typename UnaryFunc, typename... TupleElements, std::size_t... TupleIndices>
void tupleForEach(const std::tuple<TupleElements...> &tuple, UnaryFunc func, std::index_sequence<TupleIndices...>)
{
    EvaluateSequentially { (func(std::get<TupleIndices>(tuple)), 0)... };
}

template<typename UnaryFunc, typename... TupleElements>
void tupleForEach(const std::tuple<TupleElements...> &tuple, UnaryFunc func)
{
    tupleForEach(tuple, func, std::make_index_sequence<sizeof...(TupleElements)>());
}

template<typename T>
constexpr T makeCopyByValue(T value)
{
    return value;
}

template<typename IdValueType, typename Tag>
TypedId<IdValueType, Tag>::TypedId(IdValueType value)
    : m_value(value)
{
}

template<typename IdValueType, typename Tag>
bool TypedId<IdValueType, Tag>::operator==(const TypedId &other) const
{
    return m_value == other.m_value;
}

template<typename IdValueType, typename Tag>
bool TypedId<IdValueType, Tag>::operator!=(const TypedId &other) const
{
    return m_value != other.m_value;
}

template<typename IdValueType, typename Tag>
bool TypedId<IdValueType, Tag>::operator<(const TypedId &other) const
{
    return m_value < other.m_value;
}

template<typename IdValueType, typename Tag>
IdValueType TypedId<IdValueType, Tag>::value() const
{
    return m_value;
}

template<typename IdValueType, typename Tag>
TypedId<IdValueType, Tag>::TypedId(const TypedId &other) = default;

template<typename IdValueType, typename Tag>
TypedId<IdValueType, Tag> &TypedId<IdValueType, Tag>::operator=(const TypedId &other) = default;

template<typename IdValueType, typename Tag>
TypedId<IdValueType, Tag>::TypedId(TypedId &&other) = default;

template<typename IdValueType, typename Tag>
TypedId<IdValueType, Tag> &TypedId<IdValueType, Tag>::operator=(TypedId &&other) = default;

template<typename T>
QOhosConsumer<T> makeCompressingAsyncConsumer(
    QOhosConsumer<T> baseConsumer, QOhosConsumer<std::function<void()>> asyncExecutor)
{
    struct Context
    {
        QOhosConsumer<T> baseConsumer;
        QOhosConsumer<std::function<void()>> asyncExecutor;
        std::mutex pendingValueMutex;
        QOhosOptional<T> pendingValue;
    };

    auto context = std::make_shared<Context>();
    context->baseConsumer = std::move(baseConsumer);
    context->asyncExecutor = std::move(asyncExecutor);

    return [context](T value) {
        std::lock_guard<std::mutex> pendingValueLock(context->pendingValueMutex);
        if (!context->pendingValue.hasValue()) {
            context->asyncExecutor(
                [context]() {
                    QOhosOptional<T> pendingValue;
                    {
                        std::lock_guard<std::mutex> pendingValueLock(context->pendingValueMutex);
                        std::swap(pendingValue, context->pendingValue);
                    }
                    context->baseConsumer(pendingValue.value());
                });
        }
        context->pendingValue.emplace(std::move(value));
    };
}

template<typename ...Ts, typename BaseConsumer>
std::enable_if_t<std::is_assignable<QOhosConsumer<Ts...>, BaseConsumer>::value, std::function<bool(Ts...)>>
makeCallOnceConsumerWrapper(BaseConsumer &&baseConsumer)
{
    return [baseConsumer = QOhosConsumer<Ts...>(std::move(baseConsumer))](Ts &&...args) mutable {
        if (baseConsumer) {
            std::exchange(baseConsumer, nullptr)(std::forward<Ts>(args)...);
            return true;
        } else {
            return false;
        }
    };
}

template<typename T>
std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, QOhosOptional<T>>
tryParseStringAsUnsignedInteger(const std::string &inputString)
{
    auto parsedValue = qohosutils_details::tryParseStringAsUIntMax(inputString);
    bool valueValidForType = parsedValue.hasValue()
        && parsedValue.value() <= std::numeric_limits<T>::max();
    return valueValidForType
        ? makeQOhosOptional(static_cast<T>(parsedValue.value()))
        : makeEmptyQOhosOptional();
}

}

QT_END_NAMESPACE

#endif // QOHOSUTILS_H
