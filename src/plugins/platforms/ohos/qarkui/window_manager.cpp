// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/window_manager.h>

#include <cstdint>
#include <qohosdisplayinfo.h>
#include <multimodalinput/oh_input_manager.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <render/qohosbatchingrequestshandler.h>
#include <window_manager/oh_display_info.h>
#include <window_manager/oh_window_comm.h>
#include <window_manager/oh_window_event_filter.h>
#include <memory>
#include <tuple>

QT_BEGIN_NAMESPACE

namespace QArkUi {

namespace {

using JsDisplayId = QOhosDisplayInfo::JsDisplayId;

struct MouseEventFilterRegistryTraits
{
    using RawEventType = ::Input_MouseEvent;
    using MappedEventType = MouseEvent;

    static constexpr auto registerEventFilterFunc = ::OH_NativeWindowManager_RegisterMouseEventFilter;
    static constexpr auto unregisterEventFilterFunc = ::OH_NativeWindowManager_UnregisterMouseEventFilter;

    static JsWindowId extractWindowIdFromEvent(RawEventType *event);
};

struct KeyEventFilterRegistryTraits
{
    using RawEventType = ::Input_KeyEvent;
    using MappedEventType = KeyEvent;

    static constexpr auto registerEventFilterFunc = ::OH_NativeWindowManager_RegisterKeyEventFilter;
    static constexpr auto unregisterEventFilterFunc = ::OH_NativeWindowManager_UnregisterKeyEventFilter;

    static JsWindowId extractWindowIdFromEvent(RawEventType *event);
};

struct TouchEventFilterRegistryTraits
{
    using RawEventType = ::Input_TouchEvent;
    using MappedEventType = TouchEvent;

    static constexpr auto registerEventFilterFunc = ::OH_NativeWindowManager_RegisterTouchEventFilter;
    static constexpr auto unregisterEventFilterFunc = ::OH_NativeWindowManager_UnregisterTouchEventFilter;

    static JsWindowId extractWindowIdFromEvent(RawEventType *event);
};

template<typename Traits>
class EventConsumersMap
{
public:
    using RawEventType = typename Traits::RawEventType;
    using MappedEventType = typename Traits::MappedEventType;

    static EventConsumersMap<Traits> &instance();

    EventConsumersMap(const EventConsumersMap &) = delete;
    EventConsumersMap(EventConsumersMap &&) = delete;
    EventConsumersMap &operator=(const EventConsumersMap &) = delete;
    EventConsumersMap &operator=(EventConsumersMap &&) = delete;

    std::shared_ptr<void> registerEventsConsumer(
        JsWindowId jsWindowId,
        QOhosConsumer<const MappedEventType &> eventsConsumer);

private:
    static bool filterEvent(typename Traits::RawEventType *event);
    void consumeEvent(JsWindowId jsWindowId, typename Traits::MappedEventType mappedEvent);

    EventConsumersMap();

    QOhosConsumer<std::tuple<JsWindowId, MappedEventType>> m_mtSafeEventsConsumerProxy;
    std::map<JsWindowId, std::shared_ptr<QOhosConsumer<const MappedEventType &>>> m_consumersMap;
};

using MouseEventFilterRegistry = EventConsumersMap<MouseEventFilterRegistryTraits>;
using KeyEventFilterRegistry = EventConsumersMap<KeyEventFilterRegistryTraits>;
using TouchEventFilterRegistry = EventConsumersMap<TouchEventFilterRegistryTraits>;

JsWindowId MouseEventFilterRegistryTraits::extractWindowIdFromEvent(::Input_MouseEvent *event)
{
    return JsWindowId(::OH_Input_GetMouseEventWindowId(event));
}

JsWindowId KeyEventFilterRegistryTraits::extractWindowIdFromEvent(::Input_KeyEvent *event)
{
    return JsWindowId(::OH_Input_GetKeyEventWindowId(event));
}

JsWindowId TouchEventFilterRegistryTraits::extractWindowIdFromEvent(::Input_TouchEvent *event)
{
    return JsWindowId(::OH_Input_GetTouchEventWindowId(event));
}

template<typename Traits>
EventConsumersMap<Traits> &EventConsumersMap<Traits>::instance()
{
    static EventConsumersMap<Traits> eventConsumersMap;
    return eventConsumersMap;
}

template<typename Traits>
EventConsumersMap<Traits>::EventConsumersMap()
    : m_mtSafeEventsConsumerProxy(
        makeQtOhosSimpleBatchingMTRequestsHandler<std::tuple<JsWindowId, MappedEventType>>(
            [](std::function<void()> task) {
                QtOhos::invokeInJsThread(
                    [task = std::move(task)](QtOhos::JsState &) {
                        task();
                    });
            },
            [this](std::vector<std::tuple<JsWindowId, MappedEventType>> eventsBatch) {
                for (const auto &event : eventsBatch)
                    consumeEvent(std::get<JsWindowId>(event), std::get<MappedEventType>(event));
            }))
{
}

template<typename Traits>
std::shared_ptr<void> EventConsumersMap<Traits>::registerEventsConsumer(
    JsWindowId jsWindowId,
    QOhosConsumer<const MappedEventType &> eventsConsumer)
{
    if (m_consumersMap.find(jsWindowId) != m_consumersMap.end()) {
        qOhosReportFatalErrorAndAbort(
            "%s: Duplicate event consumer for jsWindowId: %f", Q_FUNC_INFO, jsWindowId.value());
    }

    auto errorCode = (*Traits::registerEventFilterFunc)(
        static_cast<std::int32_t>(jsWindowId.value()), &EventConsumersMap<Traits>::filterEvent);
    if (errorCode != ::OK) {
        qOhosReportFatalErrorAndAbort(
            "%s: failed to register event filter with error: %d",
            Q_FUNC_INFO, errorCode);
    }
    auto unregisterHandle = QtOhos::makeDestroyNotifier([jsWindowId]() {
        qOhosPrintfWarning("%s - %f", Q_FUNC_INFO, jsWindowId.value());
        (*Traits::unregisterEventFilterFunc)(static_cast<std::int32_t>(jsWindowId.value()));
    });

    m_consumersMap[jsWindowId] =
        QtOhos::makeSharedPtrWithAttachedExtraData(
            QtOhos::moveToSharedPtr(std::move(eventsConsumer)),
            std::move(unregisterHandle));

    return QtOhos::makeDestroyNotifier([this, jsWindowId]() {
        qOhosPrintfWarning("%s: %f", Q_FUNC_INFO, jsWindowId.value());
        std::ignore = m_consumersMap.erase(jsWindowId);
    });
}

template<typename Traits>
bool EventConsumersMap<Traits>::filterEvent(typename Traits::RawEventType *event)
{
    auto jsWindowId = Traits::extractWindowIdFromEvent(event);
    auto optMappedEvent = MappedEventType::createFromNativeEvent(event);
    if (optMappedEvent.hasValue())
        instance().m_mtSafeEventsConsumerProxy(std::make_tuple(jsWindowId, optMappedEvent.value()));
    else
        qOhosPrintfWarning("%s: jsWindowId: %f, Failed to map native event type", Q_FUNC_INFO, jsWindowId.value());
    return false;
}

template<typename Traits>
void EventConsumersMap<Traits>::consumeEvent(JsWindowId jsWindowId, MappedEventType mappedEvent)
{
    auto consumerIter = m_consumersMap.find(jsWindowId);
    if (consumerIter == m_consumersMap.end()) {
        qOhosPrintfWarning(
            "%s: received event for jsWindowId: %f which does not contain events consumer",
            Q_FUNC_INFO, jsWindowId.value());
        return;
    }

    auto &eventConsumerFunc = *(consumerIter->second);
    (eventConsumerFunc)(mappedEvent);
}

}

std::shared_ptr<void> registerMouseEventsConsumer(
    JsWindowId jsWindowId,
    QOhosConsumer<const MouseEvent &> eventsConsumer)
{
    return MouseEventFilterRegistry::instance()
        .registerEventsConsumer(jsWindowId, std::move(eventsConsumer));
}

std::shared_ptr<void> registerKeyEventsConsumer(
    JsWindowId jsWindowId,
    QOhosConsumer<const KeyEvent &> eventsConsumer)
{
    return KeyEventFilterRegistry::instance()
        .registerEventsConsumer(jsWindowId, std::move(eventsConsumer));
}

std::shared_ptr<void> registerTouchEventsConsumer(
    JsWindowId jsWindowId,
    QOhosConsumer<const TouchEvent &> eventsConsumer)
{
    return TouchEventFilterRegistry::instance()
        .registerEventsConsumer(jsWindowId, std::move(eventsConsumer));
}

}

QT_END_NAMESPACE
