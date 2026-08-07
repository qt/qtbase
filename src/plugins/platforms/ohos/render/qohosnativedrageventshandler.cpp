// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qmimedata.h>
#include <QtGui/private/qdnd_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <algorithm>
#include <arkui/drag_and_drop.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <arkui/ui_input_event.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <database/udmf/udmf.h>
#include <deque>
#include <functional>
#include <future>
#include <info/application_target_sdk_version.h>
#include <memory>
#include <optional>
#include <qarkui/qarkuiutils.h>
#include <qarkui/qnativenodeapi.h>
#include <qohosjsutils.h>
#include <qohosplatformdrag.h>
#include <qohosudmf.h>
#include <qohosudmfconversions.h>
#include <qohosutils.h>
#include <qpa/qplatformdrag.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qwindowsysteminterface.h>
#include <render/qohosbatchingrequestshandler.h>
#include <render/qohosdrageventutils.h>
#include <render/qohosnativedrageventshandler.h>
#include <string>
#include <type_traits>
#include <vector>

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace {

// The following DragResult constants are listed in the JS documentation,
// but they are not included as ArkUI_DragResult enumerators (despite the
// fact that they actually work according to the JS documentation):
//  - DROP_ENABLED = 3
//  - DROP_DISABLED = 4
constexpr auto Q_DROP_ENABLED = static_cast<ArkUI_DragResult>(3);
constexpr auto Q_DROP_DISABLED = static_cast<ArkUI_DragResult>(4);

struct DragEventInfo
{
    QPoint localDropPos;
    Qt::DropActions dropActions;
    Qt::KeyboardModifiers keyboardModifiers;
};

template<typename T>
QOhosSupplier<T> makeImplicitlySharedSupplier(QOhosSupplier<T> baseSupplier)
{
    auto sharedBaseSupplier = QtOhos::moveToSharedPtr(std::move(baseSupplier));
    return [sharedBaseSupplier]() {
        return (*sharedBaseSupplier)();
    };
}

std::int32_t getDragEventDataTypeCount(::ArkUI_DragEvent *dragEvent)
{
    std::int32_t dataTypeCount = 0;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_GetDataTypeCount),
        dragEvent, &dataTypeCount);

    return dataTypeCount;
}

std::vector<std::string> getDragEventDataTypes(::ArkUI_DragEvent *dragEvent)
{
    constexpr auto dataTypeMaxLength = 128;

    auto dataTypeCount = getDragEventDataTypeCount(dragEvent);
    if (dataTypeCount == 0)
        return {};

    std::vector<std::array<char, dataTypeMaxLength + 1>> dataTypesStringData;
    dataTypesStringData.resize(dataTypeCount);

    std::vector<char *> dataTypesStringPointers;
    for (std::int32_t i = 0; i < dataTypeCount; ++i)
        dataTypesStringPointers.push_back(dataTypesStringData[i].data());

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_GetDataTypes),
        dragEvent, dataTypesStringPointers.data(),  dataTypesStringPointers.size(), dataTypeMaxLength);

    return {dataTypesStringPointers.begin(), dataTypesStringPointers.end()};
}

std::shared_ptr<QOhosUdmfData> tryGetDragEventUdmfDataOrNull(::ArkUI_DragEvent *dragEvent)
{
    QOhosUdmfData udmfData;
    auto getUdmfDataRes = QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_GetUdmfData),
        dragEvent, udmfData.nativePtr());

    return getUdmfDataRes == ARKUI_ERROR_CODE_NO_ERROR
        ? QtOhos::moveToSharedPtr(std::move(udmfData))
        : nullptr;
}

void setDragEventSuggestedDropOperationIfAvailable(
    ::ArkUI_DragEvent *dragEvent, std::optional<::ArkUI_DropOperation> optDropOperation)
{
    if (optDropOperation.has_value()) {
        QArkUi::callArkUiOrFailOnErrorResult(
            Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_SetSuggestedDropOperation),
            dragEvent, optDropOperation.value());
    }
}

std::uint64_t getDragEventModifierKeyStates(::ArkUI_DragEvent *dragEvent)
{
    std::uint64_t keys;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_GetModifierKeyStates),
        dragEvent, &keys);
    return keys;
}

Qt::KeyboardModifiers mapArkUiModifierKeyStatesToQt(std::uint64_t modifierKeyStates)
{
    static const std::pair<std::uint64_t, Qt::KeyboardModifier> arkUiToQtModifiersMap[] = {
        {::ARKUI_MODIFIER_KEY_CTRL, Qt::KeyboardModifier::ControlModifier},
        {::ARKUI_MODIFIER_KEY_SHIFT, Qt::KeyboardModifier::ShiftModifier},
        {::ARKUI_MODIFIER_KEY_ALT, Qt::KeyboardModifier::AltModifier},
        {::ARKUI_MODIFIER_KEY_FN, Qt::KeyboardModifier::MetaModifier},
    };

    Qt::KeyboardModifiers modifiers;
    for (const auto &mod : arkUiToQtModifiersMap)
        modifiers.setFlag(mod.second, (modifierKeyStates & mod.first) != 0);

    return modifiers;
}

template<typename Context, typename Result>
std::optional<Result> tryRunInQtThreadAndGetResult(
    QtOhos::QThreadSafeRef<Context> context, std::function<Result(Context &)> qtThreadProcessFunc)
{
    constexpr auto maxResultWaitTime = ch::milliseconds(50);

    auto resultPromise = std::make_shared<std::promise<Result>>();
    auto resultFuture = resultPromise->get_future();

    context.visitInQtThreadIfAlive(
        [qtThreadProcessFunc = std::move(qtThreadProcessFunc), resultPromise](Context &context) {
            resultPromise->set_value(qtThreadProcessFunc(context));
        });

    return
        resultFuture.wait_for(maxResultWaitTime) == std::future_status::ready
            ? std::optional<Result>(resultFuture.get())
            : std::nullopt;
}

template<typename Context, typename Result>
std::function<std::optional<Result>(std::function<Result(Context &)>)>
makeBestEffortQtThreadFunctionsExecutor(
    QtOhos::QThreadSafeRef<Context> contextRef,
    QOhosSupplier<ch::nanoseconds> timeoutsSupplier)
{
    auto batchUpdater = makeQtOhosBatchingMTRequestsHandler<std::function<void(Context &)>>(
        [contextRef](std::function<void()> task) {
            contextRef.visitInQtThreadIfAlive([task = std::move(task)](Context &) {
                task();
            });
        },
        [contextRef](std::function<void(Context &)> &&request) {
            request(*contextRef.data());
        });

    struct ExecutorContext {
        decltype(batchUpdater) batchUpdater;
        QOhosSupplier<ch::nanoseconds> timeoutsSupplier;
    };

    auto executorContext = QtOhos::moveToSharedPtr(
        ExecutorContext{
            .batchUpdater = std::move(batchUpdater),
            .timeoutsSupplier = std::move(timeoutsSupplier),
        });

    return [executorContext](std::function<Result(Context &)> qtThreadProcessFunc) {
        const auto maxResultWaitTime = executorContext->timeoutsSupplier();

        auto promise = std::make_shared<std::promise<Result>>();
        auto future = promise->get_future();

        executorContext->batchUpdater(
            [&](std::function<void(Context &)> &request) {
                request = [qtThreadProcessFunc = std::move(qtThreadProcessFunc), promise](Context &context) {
                    promise->set_value(qtThreadProcessFunc(context));
                };
            });

        return future.wait_for(maxResultWaitTime) == std::future_status::ready
            ? std::optional<Result>(future.get())
            : std::nullopt;
    };
}

QOhosPlatformDrag *getQOhosPlatformDrag()
{
    return static_cast<QOhosPlatformDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
}

Qt::DropAction processDropInQWindow(
    QWindow &qWindow, const DragEventInfo &dragEventInfo,
    QOhosSupplier<std::unique_ptr<QMimeData>> dropDataFactory)
{
    QDrag *currentDrag = QDragManager::self()->object();
    if (currentDrag != nullptr)
        getQOhosPlatformDrag()->handlePreDrop();
    QPlatformDropQtResponse qtResponse = QWindowSystemInterface::handleDrop(
        &qWindow,
        currentDrag != nullptr ? currentDrag->mimeData() : dropDataFactory().get(),
        dragEventInfo.localDropPos,
        currentDrag != nullptr ? currentDrag->supportedActions() : dragEventInfo.dropActions,
        Qt::LeftButton, dragEventInfo.keyboardModifiers);
    auto updatedDropAction =
        qtResponse.isAccepted()
            ? qtResponse.acceptedAction()
            : Qt::IgnoreAction;
    if (currentDrag != nullptr)
        getQOhosPlatformDrag()->updateDropAction(updatedDropAction);
    return updatedDropAction;
}

void processPendingDropRequestAsynchronously(
    QtOhos::JsState &jsState, QtOhos::QThreadSafeRef<QWindow> qWindowRef, const DragEventInfo &dragEventInfo,
    QOhosSupplier<std::unique_ptr<QMimeData>> dropDataFactory, std::int32_t pendingDropRequestId)
{
    qOhosPrintfDebug("%s: async processing of drop request with id=%d", Q_FUNC_INFO, pendingDropRequestId);

    auto qtDropActionConsumer = moveToSharedPtr(
        QtOhos::makeCallOnceConsumerWrapper<QtOhos::JsState &, Qt::DropAction>(
            [pendingDropRequestId](QtOhos::JsState &, Qt::DropAction qtDropAction) {
                qOhosPrintfDebug(
                    "%s: got qtDropAction=%d for drop request with id=%d",
                    Q_FUNC_INFO, static_cast<int>(qtDropAction), pendingDropRequestId);
                QArkUi::callArkUiOrFailOnErrorResult(
                    Q_OHOS_NAMED_FUNC(::OH_ArkUI_NotifyDragResult),
                    pendingDropRequestId,
                    qtDropAction != Qt::IgnoreAction
                        ? ::ARKUI_DRAG_RESULT_SUCCESSFUL
                        : ::ARKUI_DRAG_RESULT_FAILED);
                QArkUi::callArkUiOrFailOnErrorResult(
                    Q_OHOS_NAMED_FUNC(::OH_ArkUI_NotifyDragEndPendingDone),
                    pendingDropRequestId);
            }));

    constexpr auto notifyDragEndPendingTimeout = ch::milliseconds(1500);
    QtOhos::setJsTimeout(
        jsState,
        [pendingDropRequestId, qtDropActionConsumer](const QtOhos::CallbackInfo &cbInfo) {
            if ((*qtDropActionConsumer)(cbInfo.jsState(), Qt::IgnoreAction))
                qOhosPrintfDebug("%s: used timeout action for drop request with id=%d", Q_FUNC_INFO, pendingDropRequestId);
        },
        notifyDragEndPendingTimeout);

    qWindowRef.visitInQtThreadIfAlive(
        [dragEventInfo, dropDataFactory = std::move(dropDataFactory), qtDropActionConsumer](QWindow &qWindow) mutable {
            auto dropAction = processDropInQWindow(qWindow, dragEventInfo, std::move(dropDataFactory));
            QtOhos::runInJsThreadAndWait(
                [&](QtOhos::JsState &jsState) {
                    (*qtDropActionConsumer)(jsState, dropAction);
                },
                Q_FUNC_INFO);
        });
}

bool isAsyncDropHandlingAllowed()
{
    return qEnvironmentVariableIntValue("IO__QT__USE_ASYNC_DROP_END_HANDLING") != 0;
}

bool tryStartAsyncProcessingOfDropEvent(
    QtOhos::JsState &jsState, ::ArkUI_DragEvent *dragEvent, QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    const DragEventInfo &dragEventInfo, QOhosSupplier<std::unique_ptr<QMimeData>> dropDataFactory)
{
    if (!isAsyncDropHandlingAllowed())
        return false;

    std::int32_t pendingDropRequestId = 0;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_RequestDragEndPending),
        dragEvent, &pendingDropRequestId);
    processPendingDropRequestAsynchronously(
        jsState, qWindowRef, dragEventInfo,
        std::move(dropDataFactory), pendingDropRequestId);

    return true;
}

QOhosSupplier<ch::nanoseconds> makeDragMoveQtThreadWaitTimeoutsSupplier()
{
    return [recentWaitTimeouts = std::deque<ch::steady_clock::time_point>()]() mutable -> ch::nanoseconds {
        constexpr auto waitTimeout = ch::milliseconds(50);
        constexpr auto noWaitTimeout = ch::milliseconds(0);
        constexpr auto maxWaitsPerSecond = 10;

        const auto now = ch::steady_clock::now();

        recentWaitTimeouts.erase(
            recentWaitTimeouts.begin(),
            std::lower_bound(
                recentWaitTimeouts.begin(), recentWaitTimeouts.end(),
                now - ch::seconds(1)));

        if (recentWaitTimeouts.size() < maxWaitsPerSecond) {
            recentWaitTimeouts.push_back(now);
            return waitTimeout;
        } else {
            return noWaitTimeout;
        }
    };
}

QPoint getDragEventTouchDisplayPosition(::ArkUI_DragEvent *dragEvent)
{
    return QPoint(
        ::OH_ArkUI_DragEvent_GetTouchPointXToDisplay(dragEvent),
        ::OH_ArkUI_DragEvent_GetTouchPointYToDisplay(dragEvent));
}

}

QOhosConsumer<::ArkUI_NodeEvent *> makeQOhosNativeDragEventsHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef)
{
    auto qtThreadMoveEventsProcessor = makeBestEffortQtThreadFunctionsExecutor<QWindow, Qt::DropAction>(
        qWindowRef, makeDragMoveQtThreadWaitTimeoutsSupplier());
    auto eventsHandler = [qWindowRef, qtThreadMoveEventsProcessor = std::move(qtThreadMoveEventsProcessor)](
        QtOhos::JsState &jsState, ::ArkUI_NodeEvent *nodeEvent) {
        auto eventType = QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(OH_ArkUI_NodeEvent_GetEventType), nodeEvent);
        auto *dragEvent = QArkUi::callArkUiOrFailOnNullResult(Q_OHOS_NAMED_FUNC(::OH_ArkUI_NodeEvent_GetDragEvent), nodeEvent);
        auto node = QArkUi::callArkUiOrFailOnNullResult(Q_OHOS_NAMED_FUNC(OH_ArkUI_NodeEvent_GetNodeHandle), nodeEvent);

        auto touchDisplayPosition = getDragEventTouchDisplayPosition(dragEvent);
        auto nodeDisplayPosition = QArkUi::Node::nodeDisplayPosition(node);
        auto localPosition = touchDisplayPosition - nodeDisplayPosition;

        DragEventInfo dragEventInfo = {
            .localDropPos = localPosition,
            .dropActions = mapQOhosArkUiDropOperationToQt(getQOhosDragEventDropOperation(dragEvent)),
            .keyboardModifiers = mapArkUiModifierKeyStatesToQt(getDragEventModifierKeyStates(dragEvent)),
        };

        qOhosPrintfDebug("QNativeNode: got drag event: %d, (%d,%d)", eventType, dragEventInfo.localDropPos.x(), dragEventInfo.localDropPos.y());

        switch (eventType) {
        case ::NODE_ON_DRAG_ENTER:
        case ::NODE_ON_DRAG_MOVE:
            {
                auto dropDataFactory = makeDummyQMimeDataFactoryFromUdmfDataTypes(
                    getDragEventDataTypes(dragEvent));
                auto qtDropAction = qtThreadMoveEventsProcessor(
                    [dragEventInfo, dropDataFactory = std::move(dropDataFactory)](QWindow &qWindow) {
                        QDrag *currentDrag = QDragManager::self()->object();
                        QPlatformDragQtResponse qtResponse = QWindowSystemInterface::handleDrag(
                            &qWindow,
                            currentDrag != nullptr ? currentDrag->mimeData() : dropDataFactory().get(),
                            dragEventInfo.localDropPos,
                            currentDrag != nullptr ? currentDrag->supportedActions() : dragEventInfo.dropActions,
                            Qt::LeftButton, dragEventInfo.keyboardModifiers);
                        if (currentDrag != nullptr && qtResponse.isAccepted() && qtResponse.acceptedAction() != Qt::IgnoreAction)
                            getQOhosPlatformDrag()->updateDropAction(qtResponse.acceptedAction());
                        return qtResponse.acceptedAction();
                    });
                QArkUi::callArkUiOrFailOnErrorResult(
                    Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_SetDragResult),
                    dragEvent,
                    qtDropAction.value_or(Qt::IgnoreAction) != Qt::IgnoreAction
                        ? Q_DROP_ENABLED
                        : Q_DROP_DISABLED);
                setDragEventSuggestedDropOperationIfAvailable(
                    dragEvent, qAndThen(qtDropAction, &tryMapQOhosArkUiDropOperationFromQt));
            }
            break;
        case ::NODE_ON_DRAG_LEAVE:
            qWindowRef.visitInQtThreadIfAlive(
                [](QWindow &qWindow) {
                    std::ignore = QWindowSystemInterface::handleDrag(
                        &qWindow, nullptr, QPoint(), Qt::IgnoreAction, Qt::MouseButtons(), Qt::KeyboardModifiers());
                });
            break;
        case ::NODE_ON_DROP:
            {
                QOhosSupplier<std::unique_ptr<QMimeData>> dropDataFactory;
                if (getDragEventDataTypeCount(dragEvent) != 0) {
                    auto optDragUdmfData = tryGetDragEventUdmfDataOrNull(dragEvent);
                    dropDataFactory = optDragUdmfData
                        ? createQMimeDataFactoryFromUdmfData(std::move(*optDragUdmfData))
                        : &std::make_unique<QMimeData>;
                } else {
                    dropDataFactory = &std::make_unique<QMimeData>;
                }
                auto copyableDropDataFactory = makeImplicitlySharedSupplier(std::move(dropDataFactory));

                bool asyncProcessingStarted = tryStartAsyncProcessingOfDropEvent(
                    jsState, dragEvent, qWindowRef, dragEventInfo, copyableDropDataFactory);

                if (!asyncProcessingStarted) {
                    auto qtDropAction = tryRunInQtThreadAndGetResult<QWindow, Qt::DropAction>(
                        qWindowRef,
                        [dragEventInfo, copyableDropDataFactory](QWindow &qWindow) {
                            return processDropInQWindow(qWindow, dragEventInfo, copyableDropDataFactory);
                        });
                    QArkUi::callArkUiOrFailOnErrorResult(
                        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_SetDragResult),
                        dragEvent,
                        qtDropAction.value_or(Qt::IgnoreAction) != Qt::IgnoreAction
                            ? ::ARKUI_DRAG_RESULT_SUCCESSFUL
                            : ::ARKUI_DRAG_RESULT_FAILED);
                    setDragEventSuggestedDropOperationIfAvailable(
                        dragEvent, qAndThen(qtDropAction, &tryMapQOhosArkUiDropOperationFromQt));
                }
            }
            break;
        default:
            break;
        }
    };

    return [eventsHandler = std::move(eventsHandler)](::ArkUI_NodeEvent *nodeEvent) {
        QtOhos::runInJsThreadAndWait(
            [&](QtOhos::JsState &jsState) {
                eventsHandler(jsState, nodeEvent);
            },
            Q_FUNC_INFO);
    };
}

QT_END_NAMESPACE
