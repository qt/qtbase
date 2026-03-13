// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qnativenode.h>

#include <EGL/eglplatform.h>
#include <QPointer>
#include <qohosutils.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qguiapplication.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <arkui/ui_input_event.h>
#include <native_window/external_window.h>
#include <qarkui/qembeddedwindownode.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosjsmain.h>
#include <qohospixelmapconversions.h>
#include <qohosplatformintegration.h>
#include <qohosplatformwindow.h>
#include <qohosudmf.h>
#include <qohosudmfconversions.h>
#include <qpa/qwindowsysteminterface.h>
#include <render/qohosarkuinativegestureshandler.h>
#include <render/qohosdrageventutils.h>
#include <render/qohoshovereventsgenerator.h>
#include <render/qohosnativeaxiseventhandler.h>
#include <render/qohosnativedrageventshandler.h>
#include <render/qohosnativekeyeventshandler.h>
#include <render/qohosnativemouseeventshandler.h>
#include <render/qohosnativexcomponentinputhandler.h>
#include <render/qohosview.h>
#include <render/qxcomponent.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>

QT_BEGIN_NAMESPACE

namespace {

using InputEventType = QArkUi::QXComponentCallbackReceiver::InputEventType;
using SurfaceEventType = QArkUi::QXComponentCallbackReceiver::SurfaceEventType;

class CallbackReceiver final : public QArkUi::QXComponentCallbackReceiver
{
public:
    explicit CallbackReceiver(
        QXComponentRender xComponent,
        QtOhos::QThreadSafeRef<QWindow> windowRef,
        QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef,
        std::function<void(SurfaceEventType, ::OHNativeWindow *, QOhosOptional<QSize>)> surfaceEventHandler);

    void onSurfaceEvent(SurfaceEventType surfaceEventType, ::OHNativeWindow *nativeWindow) override;
    void onInputEvent(InputEventType inputEventType, ::OHNativeWindow *window) override;
    void onHoverEvent(bool isHover) override;

    ~CallbackReceiver() override = default;
private:
    QOhosOptional<void *> m_lastWindowArgReceivedViaAnyCallback;
    std::function<void(SurfaceEventType, ::OHNativeWindow *, QOhosOptional<QSize>)> m_surfaceEventHandler;
    QSharedPointer<QOhosNativeXComponentInputHandler> m_xComponentInputHandler;
};

CallbackReceiver::CallbackReceiver(
    QXComponentRender xComponent,
    QtOhos::QThreadSafeRef<QWindow> windowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef,
    std::function<void(SurfaceEventType, ::OHNativeWindow *, QOhosOptional<QSize>)> surfaceEventHandler)
    : m_lastWindowArgReceivedViaAnyCallback()
    , m_surfaceEventHandler(std::move(surfaceEventHandler))
    , m_xComponentInputHandler(
        QSharedPointer<QOhosNativeXComponentInputHandler>::create(
            xComponent, windowRef, imEventHandlerRef))
{
}

void CallbackReceiver::onSurfaceEvent(SurfaceEventType surfaceEventType, ::OHNativeWindow *nativeWindow)
{
    m_lastWindowArgReceivedViaAnyCallback = nativeWindow;
    m_surfaceEventHandler(surfaceEventType, nativeWindow, QOhosSurface::tryGetBufferGeometryForWindow(nativeWindow));
}

void CallbackReceiver::onInputEvent(InputEventType inputEventType, ::OHNativeWindow *window)
{
    m_lastWindowArgReceivedViaAnyCallback = window;

    switch (inputEventType) {
    case QArkUi::QXComponentCallbackReceiver::InputEventType::Mouse:
        m_xComponentInputHandler->handleMouseEvent(window);
        break;
    case QArkUi::QXComponentCallbackReceiver::InputEventType::Touch:
        m_xComponentInputHandler->handleTouchEvent(window);
        break;
    case QArkUi::QXComponentCallbackReceiver::InputEventType::Keyboard:
        m_xComponentInputHandler->handleKeyEvent();
        break;
    }
}

void CallbackReceiver::onHoverEvent(bool isHover)
{
    if (!m_lastWindowArgReceivedViaAnyCallback.hasValue()) {
        qOhosPrintfWarning("Received hover event before without existing surface");
        return;
    }

    m_xComponentInputHandler->handleHoverEvent(m_lastWindowArgReceivedViaAnyCallback.value(), isHover);
}

QOhosOptional<::ArkUI_RenderFit> tryMapRenderFitPolicyToArkUi(
    QOhosPlatformWindow::NativeNodeRenderFitPolicy renderFitPolicy)
{
    switch (renderFitPolicy) {
    case QOhosPlatformWindow::NativeNodeRenderFitPolicy::TopLeft:
        return makeQOhosOptional(::ARKUI_RENDER_FIT_TOP_LEFT);
    case QOhosPlatformWindow::NativeNodeRenderFitPolicy::Fill:
        return makeQOhosOptional(::ARKUI_RENDER_FIT_RESIZE_FILL);
    }

    return makeEmptyQOhosOptional();
}

}

QNativeNode::QNativeNode(const CreateInfo &nativeNodeCreateInfo)
    : QObject()
{
    auto qWindow = QPointer<QWindow>(nativeNodeCreateInfo.window);
    auto selfRef = QtOhos::makeQThreadSafeRef(this);
    auto windowId = QOhosPlatformWindow::fromQWindow(qWindow)->internalWindowId();

    auto platformWindowFlags = QOhosPlatformWindow::platformWindowFlagsForQWindow(qWindow);
    bool focusable = !platformWindowFlags.testFlag(Qt::WindowDoesNotAcceptFocus);

    auto renderFit = nativeNodeCreateInfo.renderFitPolicyHint
        .andThen(tryMapRenderFitPolicyToArkUi).valueOr(::ARKUI_RENDER_FIT_TOP_LEFT);

    connect(
        qGuiApp, &QGuiApplication::focusWindowChanged,
        this, [this, qWindow](QWindow *focusedWindow) {
            if (focusedWindow == qWindow)
                setFocused(true);
        });

    auto qWindowRef = QtOhos::makeQThreadSafeRef(qWindow.data());

    auto *ohosPlatformIntegration = QOhosPlatformIntegration::instance();
    auto *inputMethodEventHandler = ohosPlatformIntegration->inputMethodEventHandler();
    if (inputMethodEventHandler == nullptr)
        qOhosReportFatalErrorAndAbort("QOhosInputMethodEventHandler is null!");
    auto imEventHandlerRef = QtOhos::makeQThreadSafeRef(inputMethodEventHandler);

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData = QtOhos::makeProxyWithJsThreadDeleter(std::make_shared<JsStateData>());

        using ParentDescriptor = QArkUi::QEmbeddedWindowNode::ParentDescriptor;

        QOhosOptional<ParentDescriptor> optParentDescriptor;
        if (nativeNodeCreateInfo.optParent.hasValue()) {
            auto *parentNode = nativeNodeCreateInfo.optParent.value()->m_jsStateData->embeddedWindow.get();
            optParentDescriptor = ParentDescriptor(*parentNode);
        }

        QArkUi::QQtEmbeddedWindowNode::CreateInfo createInfo = {
            .offset = nativeNodeCreateInfo.geometry.topLeft(),
            .size = nativeNodeCreateInfo.geometry.size(),
            .xComponentId = QXComponentId::createForRenderXComponent(windowId),
            .xComponentType = ::ArkUI_XComponentType::ARKUI_XCOMPONENT_TYPE_SURFACE,
            .focusOnTouch = false,
            .focusable = focusable,
            .optParent = optParentDescriptor,
            .sizePolicy = QArkUi::QEmbeddedWindowNode::SizePolicy::Points,
            .renderFit = renderFit,
            .backgroundColor = nativeNodeCreateInfo.backgroundColor,
        };

        m_jsStateData->embeddedWindow = QArkUi::QQtEmbeddedWindowNode::createOrFail(createInfo);
        m_jsStateData->embeddedWindow->setCallbackReceiver(
            std::make_unique<CallbackReceiver>(
                m_jsStateData->embeddedWindow->renderXComponent(),
                qWindowRef, imEventHandlerRef,
                [selfRef](SurfaceEventType type, ::OHNativeWindow *window, QOhosOptional<QSize> optSurfaceSize) {
                    selfRef.visitInQtThreadIfAlive(
                        [type, window, optSurfaceSize](auto &self) {
                            self.handleSurfaceEvent(type, window, optSurfaceSize);
                        });
                }));

        m_jsStateData->embeddedWindow->setTouchInterceptReceiver(
            [selfRef, embeddedWindow = m_jsStateData->embeddedWindow.get()](const ::ArkUI_UIInputEvent *event) {
                auto eventAction = ::OH_ArkUI_UIInputEvent_GetAction(event);

                if (eventAction != ::UI_MOUSE_EVENT_ACTION_PRESS && eventAction != ::UI_TOUCH_EVENT_ACTION_DOWN)
                    return;

                if (!embeddedWindow->hasNonQtManagedChildren())
                    return;

                selfRef.visitInQtThreadIfAlive(
                    [](QNativeNode &node) {
                        Q_EMIT node.externalContentClickDetected();
                    });
            });

        m_jsStateData->embeddedWindow->setAxisEventsHandler(
            makeQOhosNativeAxisEventHandler(qWindowRef, imEventHandlerRef));
        m_jsStateData->embeddedWindow->setGesturesHandler(
            makeQOhosArkUiNativeGesturesHandler(qWindowRef));
        m_jsStateData->embeddedWindow->setDragEventsHandler(makeQOhosNativeDragEventsHandler(qWindowRef));
        if (QtOhos::isNativeNodeApiKeyEventsEnabled())
            m_jsStateData->embeddedWindow->setKeyEventsHandler(makeQOhosNativeKeyEventsHandler(qWindowRef, imEventHandlerRef));
        if (QtOhos::isNativeNodeApiMouseEventsEnabled()) {
            auto hoverEventsGenerator = makeQOhosHoverEventsGenerator(qWindowRef, imEventHandlerRef);
            m_jsStateData->embeddedWindow->setMouseEventsHandler(
                makeQOhosNativeMouseEventsHandler(qWindowRef, imEventHandlerRef, hoverEventsGenerator));
            m_jsStateData->embeddedWindow->setHoverEventsHandler(
                [hoverEventsGenerator](QArkUi::NativeNodeHoverEvent hoverEvent) {
                    hoverEventsGenerator->handleQOhosHoverEvent(hoverEvent.isHovered);
                });
        }
    });
}

QRectF QNativeNode::geometry() const
{
    return m_nodeGeometry;
}

void QNativeNode::setSizeParentFillPercentNormalized(const QSizeF &size)
{
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setSizeParentFillPercentageNormalized(size);
        });
}

void QNativeNode::setSize(const QSizeF &size)
{
    m_nodeGeometry.setSize(size.toSize());
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setSize(size);
        });
}

void QNativeNode::setPosition(QPoint position)
{
    m_nodeGeometry.setTopLeft(position);
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setPosition(position);
        });
}

void QNativeNode::setVisibility(bool visible)
{
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setNodeVisibility(visible);
        });
}

QOhosSurface *QNativeNode::surfaceOrNull() const
{
    return m_optSurface.get();
}

void QNativeNode::fillToParent(const QSize &surfaceResolution)
{
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setPosition(QPointF(0, 0));
            m_jsStateData->embeddedWindow->setSizeParentFillPercentageNormalized(QSizeF(1.0, 1.0));
            m_jsStateData->embeddedWindow->setSurfaceResolution(
                static_cast<std::uint32_t>(surfaceResolution.width()),
                static_cast<std::uint32_t>(surfaceResolution.height()));
        });
}

void QNativeNode::handleSurfaceEvent(
    SurfaceEventType surfaceEventType,
    ::OHNativeWindow *nativeWindow,
    const QOhosOptional<QSize> &optSurfaceSize)
{
    qCDebug(QtForOhos, "Surface event: %d window: %p", surfaceEventType, nativeWindow);
    switch (surfaceEventType) {
    case SurfaceEventType::SurfaceChanged:
    {
        // NOTE - Its possible to receive surface changed event with null nativeWindow
        if (nativeWindow == nullptr) {
            m_optSurface.reset();
        } else if (m_optSurface) {
            m_optSurface->setNativeWindowSurface(nativeWindow, optSurfaceSize);
        } else {
            m_optSurface = std::make_unique<QOhosSurface>(nativeWindow);
        }
        break;
    }
    case SurfaceEventType::SurfaceCreated:
        m_optSurface = std::make_unique<QOhosSurface>(nativeWindow);
        break;
    case SurfaceEventType::SurfaceDestroyed:
        m_optSurface.reset();
        break;
    }
    Q_EMIT surfaceStatusChanged(optSurfaceSize);
}

WId QNativeNode::windowId() const
{
    return QtOhos::evalInJsThread([&](QtOhos::JsState &) {
        return reinterpret_cast<WId>(m_jsStateData->embeddedWindow->qtWindowId());
    });
}

void QNativeNode::setParent(std::shared_ptr<QXComponentNode> xComponent)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &){
        m_jsStateData->embeddedWindow->setParentOrReparent(
            QArkUi::QEmbeddedWindowNode::ParentDescriptor(xComponent));
    });
}

void QNativeNode::setParent(QNativeNode &other)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &){
        m_jsStateData->embeddedWindow->setParentOrReparent(
            QArkUi::QEmbeddedWindowNode::ParentDescriptor(*other.m_jsStateData->embeddedWindow));
    });
}

void QNativeNode::raise()
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &){
        auto &window = *m_jsStateData->embeddedWindow;
        window.raise();
    });
}

void QNativeNode::lower()
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &){
        auto &window = *m_jsStateData->embeddedWindow;
        window.lower();
    });
}

void QNativeNode::detachFromParentIfPresent()
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        auto &window = *m_jsStateData->embeddedWindow;
        window.detachFromParentIfPresent();
    });
}

void QNativeNode::setFocused(bool focused)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setFocused(focused);
    });
}

void QNativeNode::setFocusable(bool focusable)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setFocusable(focusable);
    });
}

void QNativeNode::setBackgroundColor(const QColor &color)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setBackgroundColor(color);
    });
}

void QNativeNode::setBrightness(int brightness)
{
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setBrightness(brightness);
        });
}

void QNativeNode::setContrast(int contrast)
{
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setContrast(contrast);
        });
}

void QNativeNode::setSaturation(int saturation)
{
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->setSaturation(saturation);
        });
}

void QNativeNode::startDrag(
    const std::vector<QImage> &images, const QPointF &hotspot,
    const QMimeData &mimeData, QOhosConsumer<Qt::DropAction> dropActionConsumer)
{
    auto udmfDataFactory = makeUdmfDataFactoryFromQMimeData(mimeData, {});

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            qOhosPrintfDebug("%s: starting drag", Q_FUNC_INFO);

            std::vector<std::shared_ptr<::OH_PixelmapNative>> pixelMaps;
            std::transform(
                images.begin(), images.end(), std::back_inserter(pixelMaps),
                &createNativePixelMapFromQImage);

            struct DragContext
            {
                QOhosConsumer<Qt::DropAction> dropActionConsumer;
                std::shared_ptr<void> startedDragHandle;
            };
            auto context = std::make_shared<DragContext>();
            context->dropActionConsumer = std::move(dropActionConsumer);

            auto startedDragHandle = m_jsStateData->embeddedWindow->startDrag(
                std::move(pixelMaps), hotspot,
                udmfDataFactory(),
                [context](::ArkUI_DragAndDropInfo *dragAndDropInfo) mutable {
                    auto dragStatus = QArkUi::callArkUi(
                        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAndDropInfo_GetDragStatus),
                        dragAndDropInfo);
                    qOhosPrintfDebug("%s: drag status: %d", Q_FUNC_INFO, static_cast<int>(dragStatus));

                    if (dragStatus == ::ARKUI_DRAG_STATUS_ENDED && context) {
                        auto *dragEvent = QArkUi::callArkUiOrFailOnNullResult(
                            Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAndDropInfo_GetDragEvent),
                            dragAndDropInfo);

                        ::ArkUI_DragResult dragResult = ::ARKUI_DRAG_RESULT_FAILED;
                        QArkUi::callArkUiOrFailOnErrorResult(
                            Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_GetDragResult),
                            dragEvent, &dragResult);

                        auto qtDropAction =
                            dragResult == ::ARKUI_DRAG_RESULT_SUCCESSFUL
                                ? mapQOhosArkUiDropOperationToQt(
                                    getQOhosDragEventDropOperation(dragEvent))
                                : Qt::IgnoreAction;

                        qOhosPrintfDebug(
                            "%s: sending drag-end notification: %d / %d",
                            Q_FUNC_INFO, static_cast<int>(dragResult), static_cast<int>(qtDropAction));
                        QtOhos::invokeInQtThread(
                            [dropActionConsumer = std::move(context->dropActionConsumer), qtDropAction]() {
                                dropActionConsumer(qtDropAction);
                            });
                        context.reset();
                    }
                });
            context->startedDragHandle = startedDragHandle;
    });
}

void QNativeNode::addForeignWindowChild(QOhosForeignWindow *foreignWindow)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        auto &child = foreignWindow->embeddedWindowNodeInJsThread();
        auto parentDescriptor = QArkUi::QEmbeddedWindowNode::ParentDescriptor(*m_jsStateData->embeddedWindow);
        child.setParentOrReparent(parentDescriptor);
    });
}

void QNativeNode::setTransparentForInput(bool transparentForInput)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setHitTestMode(
            transparentForInput
                ? ::ARKUI_HIT_TEST_MODE_NONE
                : ::ARKUI_HIT_TEST_MODE_DEFAULT);
    });
}

void QNativeNode::setNodeAreaChangeHandler(QOhosConsumer<QArkUi::QQtEmbeddedWindowNode::AreaChangeEvent> areaChangeEventConsumer)
{
    auto selfRef = QtOhos::makeQThreadSafeRef(this);
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setAreaChangeReceiver(
            QtOhos::makeCompressingAsyncConsumer(
                std::move(areaChangeEventConsumer),
                [selfRef](auto task) {
                    selfRef.visitInQtThreadIfAlive([task = std::move(task)](QNativeNode &) {
                        task();
                    });
                }));
    });
}

void QNativeNode::setNodeFocusChangeHandler(QOhosConsumer<bool> focusedChangedConsumer)
{
    auto selfRef = QtOhos::makeQThreadSafeRef(this);
    auto sharedConsumer = QtOhos::moveToSharedPtr(std::move(focusedChangedConsumer));
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setFocusedChangeReceiver(
            [selfRef, sharedConsumer = std::move(sharedConsumer)](bool focused) {
                selfRef.visitInQtThreadIfAlive([focused, sharedConsumer](QNativeNode &) {
                    (*sharedConsumer)(focused);
                });
            });
    });
}

void QNativeNode::setNodeVisibilityChangeHandler(QOhosConsumer<bool> visibilityChangedConsumer)
{
    auto selfRef = QtOhos::makeQThreadSafeRef(this);
    auto sharedConsumer = QtOhos::moveToSharedPtr(std::move(visibilityChangedConsumer));
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setVisibilityChangeReceiver(
            [selfRef, sharedConsumer = std::move(sharedConsumer)](bool visibile) {
                selfRef.visitInQtThreadIfAlive([visibile, sharedConsumer](QNativeNode &) {
                    (*sharedConsumer)(visibile);
                });
            });
    });
}

QRect QNativeNode::nodeScreenGeometryPixels() const
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            return m_jsStateData->embeddedWindow->nodeScreenGeometryPixels();
        });
}

QRect QNativeNode::nodeParentRelativeGeometryPixels() const
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            return QRect(
                m_jsStateData->embeddedWindow->parentRelativeOffsetPixels(),
                m_jsStateData->embeddedWindow->nodeScreenGeometryPixels().size());
        });
}

QT_END_NAMESPACE

