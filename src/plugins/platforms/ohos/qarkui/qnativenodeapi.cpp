// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/qnativenodeapi.h>

#include <QtCore/private/qohoslogger_p.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <qarkui/qarkuiutils.h>
#include <qohosutils.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {
namespace {

const ::ArkUI_NativeNodeAPI_1 &nativeNodeApi()
{
    static const ::ArkUI_NativeNodeAPI_1 api = []() {
        // NOTE - The pointer ownership here is unclear.
        // Because there is no information in the docs, assuming that the native interface
        // manages the pointer.
        auto *nativeNodeApi = reinterpret_cast<::ArkUI_NativeNodeAPI_1 *>(
            ::OH_ArkUI_QueryModuleInterfaceByName(::ARKUI_NATIVE_NODE, "ArkUI_NativeNodeAPI_1"));

        if (nativeNodeApi == nullptr)
            qOhosReportFatalErrorAndAbort("::OH_ArkUI_QueryModuleInterface failed");

        static constexpr auto requiredNonNullFunctionPointersMembers = std::make_tuple(
            &::ArkUI_NativeNodeAPI_1::addChild,
            &::ArkUI_NativeNodeAPI_1::addNodeEventReceiver,
            &::ArkUI_NativeNodeAPI_1::createNode,
            &::ArkUI_NativeNodeAPI_1::disposeNode,
            &::ArkUI_NativeNodeAPI_1::getAttribute,
            &::ArkUI_NativeNodeAPI_1::getChildAt,
            &::ArkUI_NativeNodeAPI_1::getParent,
            &::ArkUI_NativeNodeAPI_1::getTotalChildCount,
            &::ArkUI_NativeNodeAPI_1::registerNodeEvent,
            &::ArkUI_NativeNodeAPI_1::removeChild,
            &::ArkUI_NativeNodeAPI_1::removeNodeEventReceiver,
            &::ArkUI_NativeNodeAPI_1::setAttribute,
            &::ArkUI_NativeNodeAPI_1::unregisterNodeEvent);

        QtOhos::tupleForEach(requiredNonNullFunctionPointersMembers, [&](auto functionMemberPointer) {
            if ((nativeNodeApi->*functionMemberPointer) == nullptr)
                qOhosReportFatalErrorAndAbort("Required function is null");
        });

        return *nativeNodeApi;
    }();

    return api;
}

::ArkUI_NumberValue toArkUiNumberValue(std::int32_t value)
{
    return ::ArkUI_NumberValue {
        .i32 = value,
    };
}

::ArkUI_NumberValue toArkUiNumberValue(std::uint32_t value)
{
    return ::ArkUI_NumberValue {
        .u32 = value,
    };
}

::ArkUI_NumberValue toArkUiNumberValue(float value)
{
    return ::ArkUI_NumberValue {
        .f32 = value,
    };
}

void setNodeAttributeOrFail(
    ::ArkUI_NodeHandle node,
    ::ArkUI_NodeAttributeType attribute,
    std::initializer_list<::ArkUI_AttributeItem> values)
{
    auto errorCode = nativeNodeApi().setAttribute(node, attribute, values.begin());
    if (errorCode != ::ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "Failed to set attribute: %d on node: %p with error: %d",
            attribute,
            node,
            errorCode);
    }
}

void setNodeAttributeOrFail(
    ::ArkUI_NodeHandle node,
    ::ArkUI_NodeAttributeType attribute,
    std::initializer_list<::ArkUI_NumberValue> values)
{
    ::ArkUI_AttributeItem item = {
        .value = values.begin(),
        .size = static_cast<std::int32_t>(values.size()),
    };
    setNodeAttributeOrFail(node, attribute, {item});
}

std::shared_ptr<void> registerNodeEvent(
    ::ArkUI_NodeHandle node, ::ArkUI_NodeEventType eventType, void *userData)
{
    auto registerEventRes = nativeNodeApi().registerNodeEvent(
        node, eventType, static_cast<std::int32_t>(eventType), userData);
    if (registerEventRes != ::ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "QArkUi: registerNodeEvent(%p, %d) failed with error: %d",
            node, eventType, registerEventRes);
    }

    return QtOhos::makeDestroyNotifier(
        [node, eventType]() {
            nativeNodeApi().unregisterNodeEvent(node, eventType);
        });
}

std::shared_ptr<void> addNodeEventReceiver(
    ::ArkUI_NodeHandle node, void (*eventReceiver)(::ArkUI_NodeEvent *event))
{
    auto addEventReceiverRes = nativeNodeApi().addNodeEventReceiver(node, eventReceiver);
    if (addEventReceiverRes != ::ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "QArkUi: addNodeEventReceiver(%p, ...) failed with error: %d",
            node, addEventReceiverRes);
    }

    return QtOhos::makeDestroyNotifier(
        [node, eventReceiver]() {
            nativeNodeApi().removeNodeEventReceiver(node, eventReceiver);
        });
}

}

std::set<::ArkUI_NodeHandle> Node::qtManagedNodes;

void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, float value)
{
    setNodeAttributeOrFail(handle(), attributeType, {toArkUiNumberValue(value)});
}

void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, std::int32_t value)
{
    setNodeAttributeOrFail(handle(), attributeType, {toArkUiNumberValue(value)});
}

void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, std::uint32_t value)
{
    setNodeAttributeOrFail(handle(), attributeType, {toArkUiNumberValue(value)});
}

void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, bool value)
{
    std::int32_t intValue = value ? 1 : 0;
    setNodeAttributeOrFail(handle(), attributeType, {toArkUiNumberValue(intValue)});
}

void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType, const std::string &value)
{
    ::ArkUI_AttributeItem item = {
        .string = value.data(),
    };
    setNodeAttributeOrFail(handle(), attributeType, {item});
}

void Node::addChildOrFail(Node &child)
{
    auto errorCode = nativeNodeApi().addChild(handle(), child.handle());
    if (errorCode != ::ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "addChild failed for parent: %p, child: %p with error: %d",
            handle(),
            child.handle(),
            errorCode);
    }
}

::ArkUI_NodeHandle Node::handle() const
{
    return m_node.get();
}

std::unique_ptr<Node> Node::createOrFail(::ArkUI_NodeType type)
{
    ::ArkUI_NodeHandle nativeNode = nativeNodeApi().createNode(type);
    if (nativeNode == nullptr)
        qOhosReportFatalErrorAndAbort("Failed to create native node");

    return std::unique_ptr<Node>(new Node(nativeNode));
}

Node::Node(::ArkUI_NodeHandle handle)
    : m_node(
        handle,
        [](auto handle) {
            auto *parentNode = nativeNodeApi().getParent(handle);
            if (parentNode != nullptr)
                nativeNodeApi().removeChild(parentNode, handle);
            nativeNodeApi().disposeNode(handle);
            std::ignore = qtManagedNodes.erase(handle);
        })
{
    bool added = false;
    std::tie(std::ignore, added) = qtManagedNodes.insert(handle);
    if (!added)
        qOhosReportFatalErrorAndAbort("Attempted to take ownership of ::ArkUI_NodeHandle for a second time.");
}

void Node::removeChildOrFail(Node &child)
{
    auto errorCode = nativeNodeApi().removeChild(handle(), child.handle());
    if (errorCode != ::ARKUI_ERROR_CODE_NO_ERROR)
        qOhosReportFatalErrorAndAbort("removeChild failed with error: %d", errorCode);
}

void Node::setEventHandler(
    ::ArkUI_NodeEventType eventType,
    std::function<void(::ArkUI_NodeEvent *)> eventHandler)
{
    if (!m_eventReceiverHandle) {
        m_eventReceiverHandle = addNodeEventReceiver(
            handle(),
            [](ArkUI_NodeEvent *event) {
                void *userData = ::OH_ArkUI_NodeEvent_GetUserData(event);
                if (userData == nullptr) {
                    qOhosPrintfWarning("QArkUi: got node event with null userData, ignoring");
                    return;
                }

                auto *self = reinterpret_cast<Node *>(userData);

                auto eventType = ::OH_ArkUI_NodeEvent_GetEventType(event);

                auto eventHandlerIter = self->m_eventHandlers.find(eventType);
                if (eventHandlerIter != self->m_eventHandlers.end())
                    eventHandlerIter->second(event);
                else
                    qOhosPrintfWarning("QArkUi: got node event type %d with no handler set, ignoring", eventType);
            });
    }

    m_eventHandlers.erase(eventType);

    auto eventRegistrationHandle = registerNodeEvent(handle(), eventType, this);

    m_eventHandlers.emplace(
        eventType,
        [eventRegistrationHandle, eventHandler = std::move(eventHandler)](ArkUI_NodeEvent *event) {
            eventHandler(event);
        });
}

void Node::setAttributeOrFail(
    ::ArkUI_NodeAttributeType attributeType,
    std::size_t numberCount, const ::ArkUI_NumberValue *numbers)
{
    ::ArkUI_AttributeItem item = {
        .value = numbers,
        .size = static_cast<std::int32_t>(numberCount),
    };

    setNodeAttributeOrFail(handle(), attributeType, {item});
}

std::unique_ptr<Node> Node::takeOwnershipOfExternalNode(::ArkUI_NodeHandle nodeHandle)
{
    if (nodeHandle == nullptr)
        qOhosReportFatalErrorAndAbort("nodeHandle must not be null");

    return std::unique_ptr<Node>(new Node(nodeHandle));
}

bool Node::isQtManagedNode(::ArkUI_NodeHandle nodeHandle)
{
    return qtManagedNodes.find(nodeHandle) != qtManagedNodes.end();
}

::ArkUI_NumberValue Node::getNumberValueAttributeOrFail(::ArkUI_NodeAttributeType attributeType) const
{
    const auto *item = nativeNodeApi().getAttribute(handle(), attributeType);
    if (item == nullptr) {
        qOhosReportFatalErrorAndAbort(
            "QArkUi: Failed to retrieve node: %p attribute: %d", handle(), attributeType);
    }
    return *item->value;
}

template<>
std::int32_t Node::getAttributeOrFail<std::int32_t>(::ArkUI_NodeAttributeType attributeType) const
{
    return getNumberValueAttributeOrFail(attributeType).i32;
}

void Node::setLengthMetricUnitOrFail(::ArkUI_LengthMetricUnit unit)
{
    auto errorCode = nativeNodeApi().setLengthMetricUnit(handle(), unit);
    if (errorCode != ::ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "setLengthMetricUnit failed for node: %p with error: %d", handle(), errorCode);
    }
}

bool Node::hasParent() const
{
    return nativeNodeApi().getParent(handle()) != nullptr;
}

std::uint32_t Node::siblingsCount() const
{
    auto *parentHandle = nativeNodeApi().getParent(handle());
    return parentHandle != nullptr
        ? nativeNodeApi().getTotalChildCount(parentHandle) - 1
        : 0;
}

void Node::moveTo(std::uint32_t index)
{
    QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeUtils_MoveTo),
        handle(), nativeNodeApi().getParent(handle()), index);
}

QOhosOptional<::ArkUI_NodeHandle> Node::tryfindChild(
    ::ArkUI_NodeHandle nodeHandle, const std::function<bool(::ArkUI_NodeHandle)> predicate)
{
    const auto childCount = nativeNodeApi().getTotalChildCount(nodeHandle);
    for (std::uint32_t i = 0; i < childCount; ++i) {
        auto *childNodeHandle = nativeNodeApi().getChildAt(nodeHandle, i);
        if (predicate(childNodeHandle))
            return makeQOhosOptional(childNodeHandle);
    }
    return makeEmptyQOhosOptional();
}

}

QT_END_NAMESPACE
