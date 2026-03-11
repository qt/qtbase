// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/qqtembeddedwindownode.h>

#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <qarkui/qarkuiutils.h>
#include <qohosjsmain.h>
#include <qohosutils.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {
namespace {

class XComponentCallbackDispatcher
{
public:
    static XComponentCallbackDispatcher &instance();

    XComponentCallbackDispatcher(const XComponentCallbackDispatcher &) = delete;
    XComponentCallbackDispatcher(XComponentCallbackDispatcher &&) = delete;
    XComponentCallbackDispatcher &operator=(const XComponentCallbackDispatcher &) = delete;
    XComponentCallbackDispatcher &operator=(XComponentCallbackDispatcher &&) = delete;

    std::shared_ptr<void> registerCallbackReceiver(
        ::OH_NativeXComponent *xComponent,
        QXComponentCallbackReceiver *receiver);

private:
    XComponentCallbackDispatcher() = default;

    static void handleSurfaceCreated(::OH_NativeXComponent *xComponent, void *window);
    static void handleSurfaceChanged(::OH_NativeXComponent *xComponent, void *window);
    static void handleSurfaceDestroyed(::OH_NativeXComponent *xComponent, void *window);
    static void handleSurfaceTouchEvent(::OH_NativeXComponent *xComponent, void *window);
    static void handleMouseEvent(::OH_NativeXComponent *xComponent, void *window);
    static void handleHoverEvent(::OH_NativeXComponent *xComponent, bool hover);
    static void handleKeyEvent(::OH_NativeXComponent *xComponent, void *window);
    static QXComponentCallbackReceiver *findCallbackReceiverOrNull(::OH_NativeXComponent *xComponent);

    std::map<::OH_NativeXComponent *, QXComponentCallbackReceiver *> m_receivers;
};

XComponentCallbackDispatcher &XComponentCallbackDispatcher::instance()
{
    static XComponentCallbackDispatcher dispatcher;
    return dispatcher;
}

std::shared_ptr<void> XComponentCallbackDispatcher::registerCallbackReceiver(
    ::OH_NativeXComponent *xComponent, QXComponentCallbackReceiver *receiver)
{
    qOhosPrintfDebug("Registering callbacks for xcomponent %p", xComponent);

    if (m_receivers.find(xComponent) != m_receivers.end()) {
        qOhosPrintfError("XComponent %p callbacks were already registered", xComponent);
        throw std::runtime_error("Duplicate xcomponent");
    }

    m_receivers[xComponent] = receiver;

    static ::OH_NativeXComponent_Callback callbacks = {
        .OnSurfaceCreated = &XComponentCallbackDispatcher::handleSurfaceCreated,
        .OnSurfaceChanged = &XComponentCallbackDispatcher::handleSurfaceChanged,
        .OnSurfaceDestroyed = &XComponentCallbackDispatcher::handleSurfaceDestroyed,
        .DispatchTouchEvent = &XComponentCallbackDispatcher::handleSurfaceTouchEvent,
    };

    static ::OH_NativeXComponent_MouseEvent_Callback mouseEventCallbacks = {
        .DispatchMouseEvent = &XComponentCallbackDispatcher::handleMouseEvent,
        .DispatchHoverEvent = &XComponentCallbackDispatcher::handleHoverEvent,
    };

    auto lifecycleAndTouchCallbackRegisterStatus = ::OH_NativeXComponent_RegisterCallback(
        xComponent, &callbacks);
    if (lifecycleAndTouchCallbackRegisterStatus != ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosReportFatalErrorAndAbort(
            "OH_NativeXComponent_RegisterCallback failed with error: %d",
            lifecycleAndTouchCallbackRegisterStatus);
    }

    if (!QtOhos::isNativeNodeApiMouseEventsEnabled()) {
        auto mouseRegisterStatus = ::OH_NativeXComponent_RegisterMouseEventCallback(
            xComponent,
            &mouseEventCallbacks);
        if (mouseRegisterStatus != ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
            qOhosReportFatalErrorAndAbort(
                "OH_NativeXComponent_RegisterMouseEventCallback failed with error: %d",
                mouseRegisterStatus);
        }
    }

    if (!QtOhos::isNativeNodeApiKeyEventsEnabled()) {
        auto keyRegisterStatus  = ::OH_NativeXComponent_RegisterKeyEventCallback(
            xComponent, &XComponentCallbackDispatcher::handleKeyEvent);
        if (keyRegisterStatus != ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
            qOhosReportFatalErrorAndAbort(
                "OH_NativeXComponent_RegisterKeyEventCallback failed with error: %d",
                keyRegisterStatus);
        }
    }

    return QtOhos::makeDestroyNotifier([this, xComponent](){
        qOhosPrintfDebug("Unregistering callbacks for xcomponent %p", xComponent);
        m_receivers.erase(xComponent);
        ::OH_NativeXComponent_RegisterCallback(xComponent, nullptr);
        ::OH_NativeXComponent_RegisterMouseEventCallback(xComponent, nullptr);
        ::OH_NativeXComponent_RegisterKeyEventCallback(xComponent, nullptr);
    });
}

QXComponentCallbackReceiver *XComponentCallbackDispatcher::findCallbackReceiverOrNull(::OH_NativeXComponent *xComponent)
{
    auto &dispatcher = XComponentCallbackDispatcher::instance();
    auto it = dispatcher.m_receivers.find(xComponent);
    if (it == dispatcher.m_receivers.end()) {
        qOhosPrintfDebug("Error: xComponent %p does not have any assigned receiver", xComponent);
        return nullptr;
    }
    return it->second;
}

void XComponentCallbackDispatcher::handleSurfaceTouchEvent(::OH_NativeXComponent *xComponent, void *window)
{
    auto *receiver = findCallbackReceiverOrNull(xComponent);
    if (receiver != nullptr) {
        receiver->onInputEvent(
            QXComponentCallbackReceiver::InputEventType::Touch,
            reinterpret_cast<::OHNativeWindow *>(window));
    }
}

void XComponentCallbackDispatcher::handleHoverEvent(::OH_NativeXComponent *xComponent, bool isHover)
{
    auto *receiver = findCallbackReceiverOrNull(xComponent);
    if (receiver != nullptr)
        receiver->onHoverEvent(isHover);
}


void XComponentCallbackDispatcher::handleKeyEvent(::OH_NativeXComponent *xComponent, void *window)
{
    auto *receiver = findCallbackReceiverOrNull(xComponent);
    if (receiver != nullptr) {
        receiver->onInputEvent(
            QXComponentCallbackReceiver::InputEventType::Keyboard,
            reinterpret_cast<::OHNativeWindow *>(window));
    }
}

void XComponentCallbackDispatcher::handleMouseEvent(::OH_NativeXComponent *xComponent, void *window)
{
    auto *receiver = findCallbackReceiverOrNull(xComponent);
    if (receiver != nullptr) {
        receiver->onInputEvent(
            QXComponentCallbackReceiver::InputEventType::Mouse,
            reinterpret_cast<::OHNativeWindow *>(window));
    }
}

void XComponentCallbackDispatcher::handleSurfaceCreated(::OH_NativeXComponent *xComponent, void *window)
{
    qOhosPrintfDebug("%s: xc: %p window: %p", Q_FUNC_INFO, xComponent, window);
    auto *receiver = findCallbackReceiverOrNull(xComponent);
    if (receiver != nullptr) {
        receiver->onSurfaceEvent(
            QXComponentCallbackReceiver::SurfaceEventType::SurfaceCreated,
            reinterpret_cast<::OHNativeWindow*>(window));
    }
}

void XComponentCallbackDispatcher::handleSurfaceChanged(::OH_NativeXComponent *xComponent, void *window)
{
    qOhosPrintfDebug("%s: xc: %p window: %p", Q_FUNC_INFO, xComponent, window);
    auto *receiver = findCallbackReceiverOrNull(xComponent);
    if (receiver != nullptr) {
        receiver->onSurfaceEvent(
            QXComponentCallbackReceiver::SurfaceEventType::SurfaceChanged,
            reinterpret_cast<::OHNativeWindow*>(window));
    }
}

void XComponentCallbackDispatcher::handleSurfaceDestroyed(::OH_NativeXComponent *xComponent, void *window)
{
    qOhosPrintfDebug("%s: xc: %p window: %p", Q_FUNC_INFO, xComponent, window);
    auto *receiver = findCallbackReceiverOrNull(xComponent);
    if (receiver != nullptr) {
        receiver->onSurfaceEvent(
            QXComponentCallbackReceiver::SurfaceEventType::SurfaceDestroyed,
            reinterpret_cast<::OHNativeWindow*>(window));
    }
}

}

QXComponentCallbackReceiver::~QXComponentCallbackReceiver() = default;

std::shared_ptr<QQtEmbeddedWindowNode> QQtEmbeddedWindowNode::createOrFail(const CreateInfo &createInfo)
{
    constexpr std::uint32_t transparentArgb8888 = 0;

    auto stackNode = Node::createOrFail(::ARKUI_NODE_STACK);
    stackNode->setAttributeOrFail(::NODE_STACK_ALIGN_CONTENT, ::ARKUI_ALIGNMENT_TOP_START);
    stackNode->setAttributeOrFail(::NODE_Z_INDEX, createInfo.zIndex);
    stackNode->setAttributeOrFail(::NODE_BACKGROUND_COLOR, transparentArgb8888);
    stackNode->setAttributeOrFail(
        ::NODE_BLEND_MODE,
        std::make_tuple(::ARKUI_BLEND_MODE_SRC_OVER, ::BLEND_APPLY_TYPE_FAST));
    stackNode->setLengthMetricUnitOrFail(::ARKUI_LENGTH_METRIC_UNIT_PX);

    ::ArkUI_NodeAttributeType widthAttribute;
    ::ArkUI_NodeAttributeType heightAttribute;

    switch (createInfo.sizePolicy) {
    case SizePolicy::Points:
        widthAttribute = ::NODE_WIDTH;
        heightAttribute = ::NODE_HEIGHT;
        break;
    case SizePolicy::PercentNormalized:
        widthAttribute = ::NODE_WIDTH_PERCENT;
        heightAttribute = ::NODE_HEIGHT_PERCENT;
        break;
    }

    stackNode->setAttributeOrFail(
        widthAttribute,
        static_cast<float>(createInfo.size.width()));
    stackNode->setAttributeOrFail(
        heightAttribute,
        static_cast<float>(createInfo.size.height()));
    stackNode->setAttributeOrFail(
        ::NODE_POSITION,
        toFloatArray(createInfo.offset));

    auto xComponentNode = Node::createOrFail(::ARKUI_NODE_XCOMPONENT);
    xComponentNode->setAttributeOrFail(::NODE_XCOMPONENT_TYPE, createInfo.xComponentType);
    xComponentNode->setAttributeOrFail(::NODE_FOCUS_ON_TOUCH, createInfo.focusOnTouch);
    xComponentNode->setAttributeOrFail(
        ::NODE_BACKGROUND_COLOR,
        createInfo.backgroundColor.hasValue()
            ? createInfo.backgroundColor.value().rgba()
            : transparentArgb8888);
    xComponentNode->setAttributeOrFail(
        ::NODE_XCOMPONENT_ID,
        createInfo.xComponentId.stringId());
    xComponentNode->setAttributeOrFail(
        ::NODE_BLEND_MODE,
        std::make_tuple(::ARKUI_BLEND_MODE_SRC_OVER, ::BLEND_APPLY_TYPE_FAST));
    xComponentNode->setAttributeOrFail(::NODE_FOCUSABLE, createInfo.focusable);
    xComponentNode->setAttributeOrFail(::NODE_RENDER_FIT, createInfo.renderFit);
    xComponentNode->setAttributeOrFail(::NODE_Z_INDEX, createInfo.zIndex);
    xComponentNode->setLengthMetricUnitOrFail(::ARKUI_LENGTH_METRIC_UNIT_PX);

    stackNode->addChildOrFail(*xComponentNode);

    auto windowId = std::make_unique<QtOhos::WindowIdStruct>(QtOhos::WindowIdStruct{
        .nodeType = ::ArkUI_NodeType::ARKUI_NODE_XCOMPONENT,
        .content = xComponentNode->handle(),
        .stack = stackNode->handle(),
        .nodeOwner = "QT",
        .nodePrivate = nullptr,
    });

    auto node = std::shared_ptr<QQtEmbeddedWindowNode>(
        new QQtEmbeddedWindowNode(std::move(stackNode), std::move(xComponentNode), std::move(windowId)));
    if (createInfo.optParent.hasValue())
        node->setParentOrReparent(createInfo.optParent.value());

    node->setNodeVisibility(false);

    return node;
}

QXComponentRender QQtEmbeddedWindowNode::renderXComponent() const
{
    auto *xComponent = ::OH_NativeXComponent_GetNativeXComponent(contentNode().handle());
    if (xComponent == nullptr)
        qOhosReportFatalErrorAndAbort("::OH_NativeXComponent_GetNativeXComponent failed");
    return QXComponentRender(xComponent);
}

void QQtEmbeddedWindowNode::setCallbackReceiver(
    std::unique_ptr<QXComponentCallbackReceiver> callbackReceiver)
{
    m_callbackReceiver = std::move(callbackReceiver);
}

void QQtEmbeddedWindowNode::onSurfaceEvent(
    SurfaceEventType eventType,
    ::OHNativeWindow *nativeWindow)
{
    qOhosPrintfDebug("%s: node: %p eventType: %d surface: %p", Q_FUNC_INFO, this, eventType, nativeWindow);
    switch (eventType) {
    case SurfaceEventType::SurfaceCreated:
    case SurfaceEventType::SurfaceChanged:
        m_nativeWindow = nativeWindow;
        break;
    case SurfaceEventType::SurfaceDestroyed:
        m_nativeWindow = nullptr;
        break;
    }

    if (m_callbackReceiver)
        m_callbackReceiver->onSurfaceEvent(eventType, nativeWindow);
}

void QQtEmbeddedWindowNode::onInputEvent(
    InputEventType inputEventType,
    ::OHNativeWindow *nativeWindow)
{
    if (m_callbackReceiver)
        m_callbackReceiver->onInputEvent(inputEventType, nativeWindow);
}

void QQtEmbeddedWindowNode::onHoverEvent(bool isHover)
{
    if (m_callbackReceiver)
        m_callbackReceiver->onHoverEvent(isHover);
}

void QQtEmbeddedWindowNode::setSurfaceResolution(std::uint32_t width, std::uint32_t height)
{
    contentNode().setAttributeOrFail(
        ::NODE_XCOMPONENT_SURFACE_SIZE,
        std::array<std::uint32_t, 2>{width, height});
}

QQtEmbeddedWindowNode::QQtEmbeddedWindowNode(
    std::unique_ptr<Node> stackNode,
    std::unique_ptr<Node> xComponentNode,
    std::unique_ptr<QtOhos::WindowIdStruct> windowId)
    : QEmbeddedWindowNode(std::move(stackNode), std::move(xComponentNode), std::move(windowId))
    , m_xComponentCallbackDispatcherToken(
        XComponentCallbackDispatcher::instance().registerCallbackReceiver(
            renderXComponent().handle(), this))
{
}

QQtEmbeddedWindowNode::~QQtEmbeddedWindowNode() = default;

void QQtEmbeddedWindowNode::setAreaChangeReceiver(QOhosConsumer<NodeAreaInfo> areaChangeReceiver)
{
    m_optAreaChangedReceiver = std::move(areaChangeReceiver);

    stackNode().setEventHandler(
        ::ArkUI_NodeEventType::NODE_EVENT_ON_AREA_CHANGE,
        [this](::ArkUI_NodeEvent *) {
            m_optAreaChangedReceiver(nodeAreaInfo());
        });
}

void QQtEmbeddedWindowNode::setFocusedChangeReceiver(QOhosConsumer<bool> focusChangedReceiver)
{
    m_optFocusedChangedReceiver = std::move(focusChangedReceiver);

    stackNode().setEventHandler(
        ::ArkUI_NodeEventType::NODE_ON_FOCUS,
        [this](::ArkUI_NodeEvent *) {
            m_optFocusedChangedReceiver(true);
        });

    stackNode().setEventHandler(
        ::ArkUI_NodeEventType::NODE_ON_BLUR,
        [this](::ArkUI_NodeEvent *) {
            m_optFocusedChangedReceiver(false);
        });
}

void QQtEmbeddedWindowNode::setVisibilityChangeReceiver(QOhosConsumer<bool> visibilityChangedReceiver)
{
    m_optVisibilityChangedReceiver = std::move(visibilityChangedReceiver);

    stackNode().setEventHandler(
        ::ArkUI_NodeEventType::NODE_EVENT_ON_APPEAR,
        [this](::ArkUI_NodeEvent *) {
            m_optVisibilityChangedReceiver(true);
        });

    stackNode().setEventHandler(
        ::ArkUI_NodeEventType::NODE_EVENT_ON_DISAPPEAR,
        [this](::ArkUI_NodeEvent *) {
            m_optVisibilityChangedReceiver(false);
        });
}

void QQtEmbeddedWindowNode::setTouchInterceptReceiver(
    QOhosConsumer<const ::ArkUI_UIInputEvent *> touchInterceptReceiver)
{
    m_optTouchInterceptReceiver = std::move(touchInterceptReceiver);
    stackNode().setEventHandler(
        ::ArkUI_NodeEventType::NODE_ON_TOUCH_INTERCEPT,
        [this](::ArkUI_NodeEvent *nodeEvent) {
            auto *uiInputEvent = ::OH_ArkUI_NodeEvent_GetInputEvent(nodeEvent);
            if (uiInputEvent == nullptr)
                return;
            m_optTouchInterceptReceiver(uiInputEvent);
        });
}

QRect QQtEmbeddedWindowNode::nodeScreenGeometryPixels() const
{
    ::ArkUI_IntOffset screenPositionPx;
    callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeUtils_GetLayoutPositionInScreen),
        stackNode().handle(), &screenPositionPx);

    ::ArkUI_IntSize screenSizePx;
    callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeUtils_GetLayoutSize),
        stackNode().handle(), &screenSizePx);

    return QRect(
        QPoint(screenPositionPx.x, screenPositionPx.y),
        QSize(screenSizePx.width, screenSizePx.height));
}

bool QQtEmbeddedWindowNode::hasNonQtManagedChildren() const
{
    return QArkUi::Node::tryfindChild(
        stackNode().handle(),
        [&](::ArkUI_NodeHandle nodeHandle) {
            return !QArkUi::Node::isQtManagedNode(nodeHandle);
        }).hasValue();
}

QQtEmbeddedWindowNode::NodeAreaInfo QQtEmbeddedWindowNode::nodeAreaInfo() const
{
    return NodeAreaInfo {
        .screenGeometryPixels = nodeScreenGeometryPixels(),
        .windowRelativeOffsetPixels = windowRelativeOffsetPixels(),
        .parentRelativeOffsetPixels = parentRelativeOffsetPixels(),
        .globalRelativeOffsetPixels = globalRelativeOffsetPixels(),
    };
}

QPoint QQtEmbeddedWindowNode::windowRelativeOffsetPixels() const
{
    ::ArkUI_IntOffset windowPositionPx;
    callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeUtils_GetLayoutPositionInWindow),
        stackNode().handle(), &windowPositionPx);
    return QPoint(windowPositionPx.x, windowPositionPx.y);
}

QPoint QQtEmbeddedWindowNode::parentRelativeOffsetPixels() const
{
    ::ArkUI_IntOffset parentPositionPx;
    callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeUtils_GetLayoutPosition),
        stackNode().handle(), &parentPositionPx);
    return QPoint(parentPositionPx.x, parentPositionPx.y);
}

QPoint QQtEmbeddedWindowNode::globalRelativeOffsetPixels() const
{
    ::ArkUI_IntOffset globalPositionPx;
    callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeUtils_GetLayoutPositionInGlobalDisplay),
        stackNode().handle(), &globalPositionPx);
    return QPoint(globalPositionPx.x, globalPositionPx.y);
}

}

QT_END_NAMESPACE
