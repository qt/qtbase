// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNATIVENODE_API
#define QNATIVENODE_API

#include <QtCore/qglobal.h>
#include <QtCore/private/qohoscommon_p.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>

QT_BEGIN_NAMESPACE

namespace QArkUi {

class Node
{
public:
    static std::unique_ptr<Node> createOrFail(::ArkUI_NodeType type);
    static std::unique_ptr<Node> takeOwnershipOfExternalNode(::ArkUI_NodeHandle nodeHandle);
    static bool isQtManagedNode(::ArkUI_NodeHandle nodeHandle);
    static QOhosOptional<::ArkUI_NodeHandle> tryfindChild(
        ::ArkUI_NodeHandle nodeHandle, const std::function<bool(::ArkUI_NodeHandle)> predicate);

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    Node(Node &&) = delete;
    Node &operator=(Node &&) = delete;

    ::ArkUI_NodeHandle handle() const;

    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType, float value);
    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType, std::uint32_t value);
    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType, std::int32_t value);
    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType, bool value);
    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType, const std::string &value);

    template<std::size_t N>
    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType,
        const std::array<float, N> &value);
    template<std::size_t N>
    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType,
        const std::array<std::int32_t, N> &value);
    template<std::size_t N>
    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType,
        const std::array<std::uint32_t, N> &value);

    template<typename ...EnumTypes>
    std::enable_if_t<QtOhos::Conjuction<std::is_enum<EnumTypes>...>::value, void>
    setAttributeOrFail(::ArkUI_NodeAttributeType attributeType, const std::tuple<EnumTypes ...> &enumValuesTuple);

    template<typename T>
    std::enable_if_t<std::is_enum<T>::value, void>
    setAttributeOrFail(::ArkUI_NodeAttributeType attributeType, T value);

    void addChildOrFail(Node &child);
    void removeChildOrFail(Node &child);

    void setEventHandler(
        ::ArkUI_NodeEventType eventType,
        std::function<void(::ArkUI_NodeEvent *)> eventHandler);

    template<typename T>
    T getAttributeOrFail(::ArkUI_NodeAttributeType attributeType) const;

    template<>
    std::int32_t getAttributeOrFail<std::int32_t>(::ArkUI_NodeAttributeType attributeType) const;

    void setLengthMetricUnitOrFail(::ArkUI_LengthMetricUnit unit);

    bool hasParent() const;

    std::uint32_t siblingsCount() const;

private:
    static std::set<::ArkUI_NodeHandle> qtManagedNodes;

    explicit Node(::ArkUI_NodeHandle handle);

    void setAttributeOrFail(
        ::ArkUI_NodeAttributeType attributeType,
        std::size_t numberCount, const ::ArkUI_NumberValue *numbers);

    ::ArkUI_NumberValue getNumberValueAttributeOrFail(::ArkUI_NodeAttributeType attributeType) const;

    std::unique_ptr<::ArkUI_Node, void (*)(::ArkUI_Node *)> m_node;
    std::shared_ptr<void> m_eventReceiverHandle;
    std::unordered_map<::ArkUI_NodeEventType, std::function<void(ArkUI_NodeEvent *)>> m_eventHandlers;
};

template<typename ...EnumTypes>
std::enable_if_t<QtOhos::Conjuction<std::is_enum<EnumTypes>...>::value, void>
Node::setAttributeOrFail(::ArkUI_NodeAttributeType attributeType, const std::tuple<EnumTypes ...> &enumValuesTuple)
{
    constexpr auto argCount = sizeof...(EnumTypes);

    std::array<std::int32_t, argCount> values;
    std::size_t valueIndex = 0;
    QtOhos::tupleForEach(
        enumValuesTuple,
        [&](auto enumValue) {
            values[valueIndex++] = static_cast<std::int32_t>(enumValue);
        });

    setAttributeOrFail(attributeType, values);
}

template<typename T>
std::enable_if_t<std::is_enum<T>::value, void>
Node::setAttributeOrFail(::ArkUI_NodeAttributeType attributeType, T value)
{
    setAttributeOrFail(attributeType, static_cast<std::int32_t>(value));
}


template<std::size_t N>
void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, const std::array<float, N> &value)
{
    std::array<::ArkUI_NumberValue, N> numberValues;
    std::transform(
        value.begin(), value.end(),
        numberValues.begin(), [](float value) {
            return ::ArkUI_NumberValue {
                .f32  = value,
            };
        });
    setAttributeOrFail(attributeType, numberValues.size(), numberValues.data());
}

template<std::size_t N>
void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, const std::array<std::int32_t, N> &value)
{
    std::array<::ArkUI_NumberValue, N> numberValues;
    std::transform(
        value.begin(), value.end(),
        numberValues.begin(), [](std::int32_t value) {
            return ::ArkUI_NumberValue {
                .i32  = value,
            };
        });
    setAttributeOrFail(attributeType, numberValues.size(), numberValues.data());
}

template<std::size_t N>
void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, const std::array<std::uint32_t, N> &value)
{
    std::array<::ArkUI_NumberValue, N> numberValues;
    std::transform(
        value.begin(), value.end(),
        numberValues.begin(), [](std::uint32_t value) {
            return ::ArkUI_NumberValue {
                .u32  = value,
            };
        });
    setAttributeOrFail(attributeType, numberValues.size(), numberValues.data());
}

}

QT_END_NAMESPACE

#endif
