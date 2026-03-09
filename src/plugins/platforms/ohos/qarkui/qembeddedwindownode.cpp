// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <arkui/native_type.h>
#include <arkui/ui_input_event.h>
#include <cstdint>
#include <qarkui/qembeddedwindownode.h>

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/native_gesture.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <native_window/external_window.h>
#include <qarkui/qarkuiutils.h>
#include <qarkui/qohosdragaction.h>
#include <qohosutils.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>
#include <render/qxcomponent.h>
#include <tuple>

QT_BEGIN_NAMESPACE

namespace QArkUi {

namespace {

std::shared_ptr<ArkUI_DragPreviewOption> makeDragPreviewOption()
{
    return {
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_ArkUI_CreateDragPreviewOption)),
        [](ArkUI_DragPreviewOption *option) {
            QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragPreviewOption_Dispose),
                option);
        }
    };
}

class NativeNodeCallbackDispatcher
{
public:
    static std::shared_ptr<void> registerCallbackReceiver(
        ::ArkUI_NodeHandle nodeHandle, QEmbeddedWindowNode *embeddedWindowNode);

private:
    static void handleGestureEvent(
        ::ArkUI_GestureEvent *event, void *extraParams, NativeGestureInfo::GestureType gestureType);
    static void handlePinchGestureEvent(::ArkUI_GestureEvent *event, void *extraParam);
    static void handleRotationGestureEvent(::ArkUI_GestureEvent *event, void *extraParam);
};

std::shared_ptr<void> NativeNodeCallbackDispatcher::registerCallbackReceiver(
    ::ArkUI_NodeHandle nodeHandle, QEmbeddedWindowNode *embeddedWindowNode)
{
    auto *gestureApi = reinterpret_cast<ArkUI_NativeGestureAPI_1 *>(
        ::OH_ArkUI_QueryModuleInterfaceByName(ARKUI_NATIVE_GESTURE, "ArkUI_NativeGestureAPI_1"));

    if (gestureApi == nullptr) {
        qOhosPrintfError(
            "::OH_ArkUI_QueryModuleInterface failed to return ArkUI_NativeGestureAPI_1 module");
        return {};
    }

    ArkUI_GestureEventActionTypeMask action_type = GESTURE_EVENT_ACTION_ACCEPT
        | GESTURE_EVENT_ACTION_UPDATE | GESTURE_EVENT_ACTION_END | GESTURE_EVENT_ACTION_CANCEL;

    constexpr int32_t fingersNum = 2;
    constexpr double minimumDistanceToTriggerGesture = 5.0;
    auto *pinchGesture =
        gestureApi->createPinchGesture(fingersNum, minimumDistanceToTriggerGesture);
    gestureApi->setGestureEventTarget(
        pinchGesture, action_type, embeddedWindowNode,
        &NativeNodeCallbackDispatcher::handlePinchGestureEvent);

    const double minimumAngleToTriggerGesture = 1.0;
    auto *rotationGesture =
        gestureApi->createRotationGesture(fingersNum, minimumAngleToTriggerGesture);
    gestureApi->setGestureEventTarget(
        rotationGesture, action_type, embeddedWindowNode,
        &NativeNodeCallbackDispatcher::handleRotationGestureEvent);

    auto *groupGesture = gestureApi->createGroupGesture(::PARALLEL_GROUP);
    gestureApi->addChildGesture(groupGesture, pinchGesture);
    gestureApi->addChildGesture(groupGesture, rotationGesture);
    gestureApi->addGestureToNode(nodeHandle, groupGesture, ::PARALLEL, ::NORMAL_GESTURE_MASK);

    return QtOhos::makeDestroyNotifier(
        [nodeHandle, gestureApi, groupGesture, pinchGesture, rotationGesture]() {
            qOhosPrintfDebug("Unregistering gestures for NativeNode %p", nodeHandle);
            gestureApi->removeGestureFromNode(nodeHandle, groupGesture);
            gestureApi->removeChildGesture(groupGesture, pinchGesture);
            gestureApi->removeChildGesture(groupGesture, rotationGesture);
            gestureApi->dispose(pinchGesture);
            gestureApi->dispose(rotationGesture);
            gestureApi->dispose(groupGesture);
        });
}

void NativeNodeCallbackDispatcher::handleGestureEvent(
    ::ArkUI_GestureEvent *event, void *extraParams, NativeGestureInfo::GestureType gestureType)
{
    const auto *embeddedWindowNode = reinterpret_cast<const QEmbeddedWindowNode *>(extraParams);
    NativeGestureInfo nativeGestureInfo {
        .type = gestureType,
        .event = event,
    };
    embeddedWindowNode->handleGestureEvent(nativeGestureInfo);
}

void NativeNodeCallbackDispatcher::handlePinchGestureEvent(
    ::ArkUI_GestureEvent *event, void *extraParams)
{
    handleGestureEvent(event, extraParams, NativeGestureInfo::GestureType::Pinch);
}

void NativeNodeCallbackDispatcher::handleRotationGestureEvent(
    ::ArkUI_GestureEvent *event, void *extraParams)
{
    handleGestureEvent(event, extraParams, NativeGestureInfo::GestureType::Rotation);
}

void attachNativeRootNodeOrFail(QXComponentNode parent, ::ArkUI_NodeHandle node)
{
    qOhosPrintfDebug("%s: parentXc: %p node: %p", Q_FUNC_INFO, parent.handle(), node);
    auto errorCode = ::OH_NativeXComponent_AttachNativeRootNode(parent.handle(), node);
    if (errorCode != ::ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "OH_NativeXComponent_AttachNativeRootNode failed with error: %d", errorCode);
    }
}

void detachNativeRootNodeOrFail(QXComponentNode parent, ::ArkUI_NodeHandle node)
{
    qOhosPrintfDebug("%s: parentXc: %p node: %p", Q_FUNC_INFO, parent.handle(), node);
    auto errorCode = ::OH_NativeXComponent_DetachNativeRootNode(parent.handle(), node);
    if (errorCode != ARKUI_ERROR_CODE_NO_ERROR) {
        qOhosReportFatalErrorAndAbort(
            "OH_NativeXComponent_DetachNativeRootNode failed with error: %d", errorCode);
    }
}

std::shared_ptr<::OH_Pixelmap_ImageInfo> makeNativePixelMapImageInfo()
{
    ::OH_Pixelmap_ImageInfo *imageInfo;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_Create),
        &imageInfo);

    return {imageInfo, &OH_PixelmapImageInfo_Release};
}

std::shared_ptr<OH_Pixelmap_ImageInfo> getNativePixelMapImageInfo(::OH_PixelmapNative *pixelMap)
{
    auto imageInfo = makeNativePixelMapImageInfo();

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapNative_GetImageInfo),
        pixelMap, imageInfo.get());

    return imageInfo;
}

std::tuple<std::uint32_t, std::uint32_t> getNativePixelMapSize(::OH_PixelmapNative *pixelMap)
{
    auto imageInfo = getNativePixelMapImageInfo(pixelMap);

    std::uint32_t width;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_GetWidth),
        imageInfo.get(), &width);

    std::uint32_t height;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PixelmapImageInfo_GetHeight),
        imageInfo.get(), &height);

    return {width, height};
}

std::tuple<std::uint32_t, std::uint32_t> getNativePixelMapsIntersectionSize(
    const std::vector<std::shared_ptr<::OH_PixelmapNative>> &pixelMaps)
{
    if (pixelMaps.empty())
        return {0, 0};

    auto intersectionSize = std::make_tuple(
        std::numeric_limits<std::uint32_t>::max(),
        std::numeric_limits<std::uint32_t>::max());
    for (const auto &pixelMap : pixelMaps) {
        auto pixelMapSize = getNativePixelMapSize(pixelMap.get());
        intersectionSize = std::make_tuple(
            std::min(std::get<0>(intersectionSize), std::get<0>(pixelMapSize)),
            std::min(std::get<1>(intersectionSize), std::get<1>(pixelMapSize)));
    }

    return intersectionSize;
}

bool dragHotspotFitsInPixelMaps(
    const std::vector<std::shared_ptr<::OH_PixelmapNative>> &pixelMaps,
    const QPointF &hotspot)
{
    if (hotspot.x() < 0 || hotspot.y() < 0)
        return false;

    auto bounds = getNativePixelMapsIntersectionSize(pixelMaps);
    return hotspot.x() + 1 <= std::get<0>(bounds) && hotspot.y() + 1 <= std::get<1>(bounds);
}

float mapValueFromRangeInToRangeOut(float value, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

}

const std::int32_t QEmbeddedWindowNode::minimumNodeZIndexValue = 1;

void QEmbeddedWindowNode::setNodeVisibility(bool visible)
{
    qOhosPrintfDebug("%s: %s", Q_FUNC_INFO, visible ? "visible" : "hidden");
    m_stackNode->setAttributeOrFail(
        ::NODE_VISIBILITY,
        visible
            ? ::ARKUI_VISIBILITY_VISIBLE
            : ::ARKUI_VISIBILITY_HIDDEN);
}

void QEmbeddedWindowNode::setSizeParentFillPercentageNormalized(const QSizeF &size)
{
    qOhosPrintfDebug("%s: %f,%f", Q_FUNC_INFO, size.width(), size.height());
    m_stackNode->setAttributeOrFail(
        ::NODE_WIDTH_PERCENT,
        static_cast<float>(size.width()));

    m_stackNode->setAttributeOrFail(
        ::NODE_HEIGHT_PERCENT,
        static_cast<float>(size.height()));
}

void QEmbeddedWindowNode::setSize(const QSizeF &size)
{
    qOhosPrintfDebug("%s: %f,%f", Q_FUNC_INFO, size.width(), size.height());
    m_stackNode->setAttributeOrFail(
        ::NODE_WIDTH,
        static_cast<float>(size.width()));
    m_stackNode->setAttributeOrFail(
        ::NODE_HEIGHT,
        static_cast<float>(size.height()));
}

void QEmbeddedWindowNode::setPosition(const QPointF &position)
{
    qOhosPrintfDebug("%s: %f,%f", Q_FUNC_INFO, position.x(), position.y());
    m_stackNode->setAttributeOrFail(::NODE_POSITION, toFloatArray(position));
}

QEmbeddedWindowNode::QEmbeddedWindowNode(
    std::unique_ptr<Node> stack,
    std::unique_ptr<Node> contentNode,
    std::unique_ptr<QtOhos::WindowIdStruct> windowId)
    : m_stackNode(std::move(stack))
    , m_contentNode(std::move(contentNode))
    , m_windowIdStruct(std::move(windowId))
    , m_nativeNodeDispatcherHandle(NativeNodeCallbackDispatcher::registerCallbackReceiver(
        m_contentNode->handle(), this))
{
}

void QEmbeddedWindowNode::setParentOrReparent(ParentDescriptor parentDescriptor)
{
    if (m_parentDescriptor == parentDescriptor)
        return;

    detachFromParentIfPresent();

    switch (parentDescriptor.type()) {
    case ParentDescriptor::Type::EmbeddedWindowNode:
    {
        auto ewParent = parentDescriptor.embeddedWindowNodeOrNull();
        qOhosPrintfDebug(
            "%s: EmbeddedWindowNode parent: %p",
            Q_FUNC_INFO,
            ewParent.get());
        if (ewParent)
            ewParent->m_stackNode->addChildOrFail(*m_stackNode);
        break;
    }
    case ParentDescriptor::Type::XComponentNode:
        auto xComponentNode = parentDescriptor.xComponentNodeOrNull();
        if (xComponentNode) {
            qOhosPrintfDebug(
                "%s: XComponentNode parent: %p",
                Q_FUNC_INFO,
                xComponentNode->handle());
            attachNativeRootNodeOrFail(
                *xComponentNode,
                m_stackNode->handle());
        }
        break;
    }

    m_parentDescriptor = parentDescriptor;
}

bool QEmbeddedWindowNode::detachFromParentIfPresent()
{
    if (!m_parentDescriptor.hasValue())
        return false;

    qOhosPrintfDebug("%s", Q_FUNC_INFO);

    auto parentDescriptor = m_parentDescriptor.value();
    switch (parentDescriptor.type()) {
    case ParentDescriptor::Type::XComponentNode:
    {
        auto xComponentNode = parentDescriptor.xComponentNodeOrNull();
        if (xComponentNode)
            detachNativeRootNodeOrFail(*xComponentNode, m_stackNode->handle());
        break;
    }
    case ParentDescriptor::Type::EmbeddedWindowNode:
    {
        auto ewParent = parentDescriptor.embeddedWindowNodeOrNull();
        if (ewParent)
            ewParent->m_stackNode->removeChildOrFail(*m_stackNode);
        break;
    }
    }

    return true;
}

void QEmbeddedWindowNode::handleGestureEvent(const NativeGestureInfo &nativeGestureInfo) const
{
    if (m_gesturesHandler)
        m_gesturesHandler(nativeGestureInfo);
}

void *QEmbeddedWindowNode::qtWindowId()
{
    return m_windowIdStruct.get();
}

QEmbeddedWindowNode::ParentDescriptor::ParentDescriptor(QEmbeddedWindowNode &node)
    : m_type(QEmbeddedWindowNode::ParentDescriptor::Type::EmbeddedWindowNode)
    , m_embeddedWindowNode(node.shared_from_this())
{
}

QEmbeddedWindowNode::ParentDescriptor::ParentDescriptor(std::shared_ptr<QXComponentNode> node)
    : m_type(QEmbeddedWindowNode::ParentDescriptor::Type::XComponentNode)
    , m_xComponentNode(node)
{
}

QEmbeddedWindowNode::ParentDescriptor::Type QEmbeddedWindowNode::ParentDescriptor::type() const
{
    return m_type;
}

std::shared_ptr<QXComponentNode> QEmbeddedWindowNode::ParentDescriptor::xComponentNodeOrNull() const
{
    return m_xComponentNode.value().lock();
}

std::shared_ptr<QEmbeddedWindowNode> QEmbeddedWindowNode::ParentDescriptor::embeddedWindowNodeOrNull() const
{
    return m_embeddedWindowNode.value().lock();
}

bool QEmbeddedWindowNode::ParentDescriptor::operator==(const ParentDescriptor &other) const
{
    return m_type == other.m_type && [&]() {
        switch (m_type) {
        case Type::EmbeddedWindowNode:
            return (m_embeddedWindowNode.hasValue() && other.m_embeddedWindowNode.hasValue())
                && m_embeddedWindowNode.value().lock() == other.m_embeddedWindowNode.value().lock();
        case Type::XComponentNode:
            return (m_xComponentNode.hasValue() && other.m_xComponentNode.hasValue())
                && m_xComponentNode.value().lock() == other.m_xComponentNode.value().lock();
        }
    }();
}

std::int32_t QEmbeddedWindowNode::zIndex() const
{
    return m_stackNode->getAttributeOrFail<std::int32_t>(::NODE_Z_INDEX);
}

void QEmbeddedWindowNode::setZIndex(std::int32_t index)
{
    m_stackNode->setAttributeOrFail(::NODE_Z_INDEX, index);
}

void QEmbeddedWindowNode::raise()
{
    if (!m_stackNode->hasParent()) {
        qOhosPrintfDebug("%s: stack node has no parent, ignore raising it", Q_FUNC_INFO);
        return;
    }

    m_stackNode->moveTo(m_stackNode->siblingsCount());
}

void QEmbeddedWindowNode::lower()
{
    if (!m_stackNode->hasParent()) {
        qOhosPrintfDebug("%s: stack node has no parent, ignore lowering it", Q_FUNC_INFO);
        return;
    }

    m_stackNode->moveTo(0);
}

void QEmbeddedWindowNode::setFocused(bool focused)
{
    m_contentNode->setAttributeOrFail(::NODE_FOCUS_STATUS, focused);
}

void QEmbeddedWindowNode::setFocusable(bool focusable)
{
    m_contentNode->setAttributeOrFail(::NODE_FOCUSABLE, focusable);
}

void QEmbeddedWindowNode::setBackgroundColor(const QColor &color)
{
    m_contentNode->setAttributeOrFail(::NODE_BACKGROUND_COLOR, color.rgba());
}

void QEmbeddedWindowNode::setBrightness(int brightness)
{
    std::pair<int, int> qtBrightnessRange(-100, 100);
    std::pair<float, float> ohosBrightnessRange(0, 2);

    m_contentNode->setAttributeOrFail(
        ::NODE_BRIGHTNESS,
        mapValueFromRangeInToRangeOut(
            brightness, qtBrightnessRange.first, qtBrightnessRange.second,
            ohosBrightnessRange.first, ohosBrightnessRange.second));
}

void QEmbeddedWindowNode::setContrast(int contrast)
{
    const int qtDefaultContrast = 0;
    std::pair<int, int> qtContrastRange(-100, 100);

    const int ohosDefaultContrast = 1;
    constexpr auto safeReductionOfTheUpperLimit = 0.99f;
    std::pair<float, float> ohosContrastRange(0, 10 * safeReductionOfTheUpperLimit);

    const float ohosContrastValue = contrast <= 0
        ? mapValueFromRangeInToRangeOut(
            contrast, qtContrastRange.first, qtDefaultContrast, ohosContrastRange.first,
            ohosDefaultContrast)
        : mapValueFromRangeInToRangeOut(
            contrast, qtDefaultContrast, qtContrastRange.second, ohosDefaultContrast,
            ohosContrastRange.second);
    m_contentNode->setAttributeOrFail(::NODE_CONTRAST, ohosContrastValue);
}

void QEmbeddedWindowNode::setSaturation(int saturation)
{
    const int qtDefaultSaturation = 0;
    std::pair<int, int> qtSaturationRange(-100, 100);

    const int ohosDefaultSaturation = 1;
    std::pair<float, float> ohosSaturationRange(0, 50);

    const float ohosSaturationValue = saturation <= 0
        ? mapValueFromRangeInToRangeOut(
            saturation, qtSaturationRange.first, qtDefaultSaturation, ohosSaturationRange.first,
            ohosDefaultSaturation)
        : mapValueFromRangeInToRangeOut(
            saturation, qtDefaultSaturation, qtSaturationRange.second, ohosDefaultSaturation,
            ohosSaturationRange.second);
    m_contentNode->setAttributeOrFail(::NODE_SATURATION, ohosSaturationValue);
}

void QEmbeddedWindowNode::setGesturesHandler(
    QOhosConsumer<const NativeGestureInfo &> gesturesHandler)
{
    m_gesturesHandler = gesturesHandler;
}

void QEmbeddedWindowNode::setDragEventsHandler(
    QOhosConsumer<::ArkUI_NodeEventType, ::ArkUI_DragEvent *> dragEventsHandler)
{
    static const ::ArkUI_NodeEventType dragEventTypes[] = {
        ::NODE_ON_DRAG_ENTER,
        ::NODE_ON_DRAG_MOVE,
        ::NODE_ON_DRAG_LEAVE,
        ::NODE_ON_DROP,
    };

    auto sharedDragEventsHandler = QtOhos::moveToSharedPtr(std::move(dragEventsHandler));

    for (auto dragEventType : dragEventTypes) {
        contentNode().setEventHandler(
            dragEventType,
            [dragEventType, sharedDragEventsHandler](::ArkUI_NodeEvent *nodeEvent) {
                auto *dragEvent = QArkUi::callArkUi(
                    Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeEvent_GetDragEvent), nodeEvent);
                if (dragEvent == nullptr) {
                    qOhosPrintfError("Got illegal drag event as ArkUI_NodeEvent, ignoring");
                    return;
                }
                (*sharedDragEventsHandler)(dragEventType, dragEvent);
            });
    }
}

void QEmbeddedWindowNode::setKeyEventsHandler(
    QOhosConsumer<::ArkUI_UIInputEvent *> keyEventsHandler)
{
    setContentNodeEventHandler(::NODE_ON_KEY_EVENT, "key", std::move(keyEventsHandler));
}

void QEmbeddedWindowNode::setHoverEventsHandler(QOhosConsumer<NativeNodeHoverEvent> hoverEventsHandler)
{
    setContentNodeEventHandler(
        ::NODE_ON_HOVER_EVENT,
        "hover",
        [hoverEventsHandler = std::move(hoverEventsHandler)](::ArkUI_UIInputEvent *rawHoverEvent) {
            hoverEventsHandler(NativeNodeHoverEvent::makeFromUiInputEvent(rawHoverEvent));
        });
}

void QEmbeddedWindowNode::setAxisEventsHandler(QOhosConsumer<::ArkUI_UIInputEvent *> axisEventsHandler)
{
    setContentNodeEventHandler(::NODE_ON_AXIS, "axis", std::move(axisEventsHandler));
}

void QEmbeddedWindowNode::setMouseEventsHandler(QOhosConsumer<NativeNodeMouseEvent> mouseEventsHandler)
{
    setContentNodeEventHandler(
        ::NODE_ON_MOUSE,
        "mouse",
        [mouseEventsHandler = std::move(mouseEventsHandler)](::ArkUI_UIInputEvent *rawMouseEvent) {
            mouseEventsHandler(NativeNodeMouseEvent::makeFromUiInputEvent(rawMouseEvent));
        });
}

std::shared_ptr<void> QEmbeddedWindowNode::startDrag(
    std::vector<std::shared_ptr<::OH_PixelmapNative>> pixelMaps, const QPointF &hotspot,
    QOhosUdmfData udmfData, std::function<void(::ArkUI_DragAndDropInfo *)> statusListener)
{
    bool hotspotFitsInPixelMaps = dragHotspotFitsInPixelMaps(pixelMaps, hotspot);

    auto dragAction = std::make_shared<QArkUi::DragAction>(contentNode().handle());
    dragAction->setPointerId(0);
    if (hotspotFitsInPixelMaps && !hotspot.isNull()) {
        dragAction->setTouchPoint(int(hotspot.x()), int(hotspot.y()));
    } else if (!hotspot.isNull()) {
        qOhosPrintfDebug(
            "%s: ignoring out-of-bounds hotspot: %fx%f",
            Q_FUNC_INFO, hotspot.x(), hotspot.y());
    }
    dragAction->setPixelMaps(std::move(pixelMaps));
    dragAction->setData(std::move(udmfData));
    dragAction->setStatusListener(std::move(statusListener));

    auto previewOption = makeDragPreviewOption();
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragPreviewOption_SetNumberBadgeEnabled),
        previewOption.get(), false);
    dragAction->setDragPreviewOption(*previewOption);

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_StartDrag),
        dragAction->nativePtr());

    return dragAction;
}

QEmbeddedWindowNode::~QEmbeddedWindowNode() = default;

Node &QEmbeddedWindowNode::contentNode()
{
    return *m_contentNode;
}

const Node &QEmbeddedWindowNode::contentNode() const
{
    return *m_contentNode;
}

Node &QEmbeddedWindowNode::stackNode()
{
    return *m_stackNode;
}

const Node &QEmbeddedWindowNode::stackNode() const
{
    return *m_stackNode;
}

void QEmbeddedWindowNode::setHitTestMode(::ArkUI_HitTestMode hitTestMode)
{
    m_stackNode->setAttributeOrFail(::NODE_HIT_TEST_BEHAVIOR, hitTestMode);
}

void QEmbeddedWindowNode::setContentNodeEventHandler(
    ::ArkUI_NodeEventType eventType, const char *eventTypeName,
    QOhosConsumer<::ArkUI_UIInputEvent *> inputEventsHandler)
{
    contentNode().setEventHandler(
        eventType,
        [inputEventsHandler = std::move(inputEventsHandler), eventTypeName](::ArkUI_NodeEvent *nodeEvent) {
            auto *inputEvent = QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeEvent_GetInputEvent), nodeEvent);
            if (inputEvent == nullptr) {
                qOhosPrintfError("Got null %s event as ArkUI_NodeEvent, ignoring", eventTypeName);
                return;
            }
            inputEventsHandler(inputEvent);
        });
}

}

QT_END_NAMESPACE
