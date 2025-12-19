// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"

#include <qwayland-server-tablet-unstable-v2.h>

#include <QtGui/QRasterWindow>

using namespace MockCompositor;

constexpr int tabletVersion = 1; // protocol VERSION, not the name suffix (_v2)

class TabletManagerV2;
class TabletSeatV2;

class TabletV2 : public QObject, public QtWaylandServer::zwp_tablet_v2
{
    Q_OBJECT
public:
    explicit TabletV2(TabletSeatV2 *tabletSeat)
        : m_tabletSeat(tabletSeat)
    {
    }

    void send_removed() = delete;
    void send_removed(struct ::wl_resource *resource) = delete;
    void sendRemoved();

    QPointer<TabletSeatV2> m_tabletSeat; // destroy order is not guaranteed
protected:
    void zwp_tablet_v2_destroy(Resource *resource) override;
};

class TabletToolV2 : public QObject, public QtWaylandServer::zwp_tablet_tool_v2
{
    Q_OBJECT
public:
    using ToolType = QtWaylandServer::zwp_tablet_tool_v2::type;
    explicit TabletToolV2(TabletSeatV2 *tabletSeat, ToolType toolType, quint64 hardwareSerial)
        : m_tabletSeat(tabletSeat)
        , m_toolType(toolType)
        , m_hardwareSerial(hardwareSerial)
    {
    }

    wl_resource *toolResource() // for convenience
    {
        Q_ASSERT(resourceMap().size() == 1);
        // Strictly speaking, there may be more than one resource for the tool, for intsance if
        // if there are multiple clients, or a client has called get_tablet_seat multiple times.
        // For now we'll pretend there can only be one resource.
        return resourceMap().first()->handle;
    }

    void send_removed() = delete;
    void send_removed(struct ::wl_resource *resource) = delete;
    void sendRemoved();

    uint sendProximityIn(TabletV2 *tablet, Surface *surface);
    void sendProximityOut();
    void sendMotion(QPointF position)
    {
        Q_ASSERT(m_proximitySurface);
        for (auto *resource : resourceMap())
            send_motion(resource->handle, wl_fixed_from_double(position.x()), wl_fixed_from_double(position.y()));
    }
    uint sendDown();
    void sendUp() { send_up(toolResource()); }
    void sendPressure(uint pressure);
    void sendTilt(qreal tiltX, qreal tiltY) { send_tilt(toolResource(), wl_fixed_from_double(tiltX), wl_fixed_from_double(tiltY)); }
    void sendRotation(qreal rotation) { send_rotation(toolResource(), wl_fixed_from_double(rotation)); }
    uint sendButton(uint button, bool pressed);
    uint sendFrame();

    QPointer<TabletSeatV2> m_tabletSeat; // destruction order is not guaranteed
    ToolType m_toolType = ToolType::type_pen;
    quint64 m_hardwareSerial = 0;
    QPointer<Surface> m_proximitySurface;
protected:
    void zwp_tablet_tool_v2_destroy(Resource *resource) override;
};

class TabletPadV2 : public QObject, public QtWaylandServer::zwp_tablet_pad_v2
{
    Q_OBJECT
public:
    explicit TabletPadV2(TabletSeatV2 *tabletSeat)
        : m_tabletSeat(tabletSeat)
    {
    }

    void send_removed() = delete;
    void send_removed(struct ::wl_resource *resource) = delete;
    void sendRemoved();

    QPointer<TabletSeatV2> m_tabletSeat; // destroy order is not guaranteed
protected:
    void zwp_tablet_pad_v2_destroy(Resource *resource) override;
};

class TabletSeatV2 : public QObject, public QtWaylandServer::zwp_tablet_seat_v2
{
    Q_OBJECT
public:
    explicit TabletSeatV2(TabletManagerV2 *manager, Seat *seat)
        : m_manager(manager)
        , m_seat(seat)
    {}
    TabletV2 *addTablet()
    {
        auto *tablet = new TabletV2(this);
        m_tablets.append(tablet);
        for (auto *resource : resourceMap())
            sendTabletAdded(resource, tablet);
        return tablet;
    }

    void sendTabletAdded(Resource *resource, TabletV2 *tablet)
    {
        // Although, not necessarily correct, assuming just one tablet_seat per client
        auto *tabletResource = tablet->add(resource->client(), resource->version());
        zwp_tablet_seat_v2::send_tablet_added(resource->handle, tabletResource->handle);
        // TODO: send extra stuff before done?
        tablet->send_done(tabletResource->handle);
    }

    using ToolType = QtWaylandServer::zwp_tablet_tool_v2::type;
    TabletToolV2 *addTool(ToolType toolType = ToolType::type_pen, quint64 hardwareSerial = 0)
    {
        auto *tool = new TabletToolV2(this, toolType, hardwareSerial);
        m_tools.append(tool);
        for (auto *resource : resourceMap())
            sendToolAdded(resource, tool);
        return tool;
    }

    void sendToolAdded(Resource *resource, TabletToolV2 *tool)
    {
        // Although, not necessarily correct, assuming just one tablet_seat per client
        auto *toolResource = tool->add(resource->client(), resource->version())->handle;
        zwp_tablet_seat_v2::send_tool_added(resource->handle, toolResource);
        tool->send_type(toolResource, tool->m_toolType);
        if (tool->m_hardwareSerial) {
            const uint hi = tool->m_hardwareSerial >> 32;
            const uint lo = tool->m_hardwareSerial & 0xffffffff;
            tool->send_hardware_serial(toolResource, hi, lo);
        }
        tool->send_done(toolResource);
    }

    TabletPadV2 *addPad()
    {
        auto *pad = new TabletPadV2(this);
        m_pads.append(pad);
        for (auto *resource : resourceMap())
            sendPadAdded(resource, pad);
        return pad;
    }

    void sendPadAdded(Resource *resource, TabletPadV2 *pad)
    {
        // Although, not necessarily correct, assuming just one tablet_seat per client
        auto *padResource = pad->add(resource->client(), resource->version())->handle;
        zwp_tablet_seat_v2::send_pad_added(resource->handle, padResource);
        pad->send_done(padResource);
    }

    void removeAll()
    {
        const auto tools = m_tools;
        for (auto *tool : tools)
            tool->sendRemoved();

        const auto tablets = m_tablets;
        for (auto *tablet : tablets)
            tablet->sendRemoved();

        const auto pads = m_pads;
        for (auto *pad : pads)
            pad->sendRemoved();
    }

    TabletManagerV2 *m_manager = nullptr;
    Seat *m_seat = nullptr;
    QList<TabletV2 *> m_tablets;
    QList<TabletV2 *> m_tabletsWaitingForDestroy;
    QList<TabletToolV2 *> m_tools;
    QList<TabletToolV2::Resource *> m_toolsWaitingForDestroy;
    QList<TabletPadV2 *> m_pads;
    QList<TabletPadV2::Resource *> m_padsWaitingForDestroy;

protected:
    void zwp_tablet_seat_v2_bind_resource(Resource *resource) override
    {
        for (auto *tablet : m_tablets)
            sendTabletAdded(resource, tablet);
        for (auto *tool : m_tools)
            sendToolAdded(resource, tool);
        for (auto *pad : m_pads)
            sendPadAdded(resource, pad);
    }
};

class TabletManagerV2 : public Global, public QtWaylandServer::zwp_tablet_manager_v2
{
    Q_OBJECT
public:
    explicit TabletManagerV2(CoreCompositor *compositor, int version = 1)
        : QtWaylandServer::zwp_tablet_manager_v2(compositor->m_display, version)
        , m_version(version)
    {}
    bool isClean() override
    {
        for (auto *seat : m_tabletSeats) {
            if (!seat->m_tabletsWaitingForDestroy.empty())
                return false;
            if (!seat->m_toolsWaitingForDestroy.empty())
                return false;
            if (!seat->m_padsWaitingForDestroy.empty())
                return false;
        }
        return true;
    }

    TabletSeatV2 *tabletSeatFor(Seat *seat)
    {
        Q_ASSERT(seat);
        if (auto *tabletSeat = m_tabletSeats.value(seat, nullptr))
            return tabletSeat;

        auto *tabletSeat = new TabletSeatV2(this, seat);
        m_tabletSeats[seat] = tabletSeat;
        return tabletSeat;
    }

    int m_version = 1; // TODO: Remove on libwayland upgrade
    QMap<Seat *, TabletSeatV2 *> m_tabletSeats;

protected:
    void zwp_tablet_manager_v2_destroy(Resource *resource) override
    {
        // tablet_seats created from this object are unaffected and should be destroyed separately.
        wl_resource_destroy(resource->handle);
    }

    void zwp_tablet_manager_v2_get_tablet_seat(Resource *resource, uint32_t id, ::wl_resource *seatResource) override
    {
        auto *seat = fromResource<Seat>(seatResource);
        QVERIFY(seat);
        auto *tabletSeat = tabletSeatFor(seat);
        tabletSeat->add(resource->client(), id, resource->version());
    }
};

void TabletV2::sendRemoved()
{
    for (auto *resource : resourceMap())
        zwp_tablet_v2_send_removed(resource->handle);
    bool removed = m_tabletSeat->m_tablets.removeOne(this);
    QVERIFY(removed);
    m_tabletSeat->m_tabletsWaitingForDestroy.append(this);
}

void TabletV2::zwp_tablet_v2_destroy(QtWaylandServer::zwp_tablet_v2::Resource *resource)
{
    Q_UNUSED(resource);
    if (m_tabletSeat) {
        bool removed = m_tabletSeat->m_tabletsWaitingForDestroy.removeOne(this);
        QVERIFY(removed);
    }
    wl_resource_destroy(resource->handle);
}

void TabletToolV2::sendRemoved()
{
    for (auto *resource : resourceMap()) {
        zwp_tablet_tool_v2_send_removed(resource->handle);
        m_tabletSeat->m_toolsWaitingForDestroy.append(resource);
    }
    bool removed = m_tabletSeat->m_tools.removeOne(this);
    QVERIFY(removed);
}

uint TabletToolV2::sendProximityIn(TabletV2 *tablet, Surface *surface)
{
    Q_ASSERT(!m_proximitySurface);
    m_proximitySurface = surface;
    uint serial = m_tabletSeat->m_seat->m_compositor->nextSerial();
    auto *client = surface->resource()->client();
    auto tabletResource = tablet->resourceMap().value(client)->handle;
    send_proximity_in(toolResource(), serial, tabletResource, surface->resource()->handle);
    return serial;
}

void TabletToolV2::sendProximityOut()
{
    Q_ASSERT(m_proximitySurface);
    send_proximity_out(toolResource());
    m_proximitySurface = nullptr;
}

uint TabletToolV2::sendDown()
{
    uint serial = m_tabletSeat->m_seat->m_compositor->nextSerial();
    send_down(toolResource(), serial);
    return serial;
}

void TabletToolV2::sendPressure(uint pressure)
{
    Q_ASSERT(m_proximitySurface);
    auto *client = m_proximitySurface->resource()->client();
    auto toolResource = resourceMap().value(client)->handle;
    send_pressure(toolResource, pressure);
}

uint TabletToolV2::sendButton(uint button, bool pressed)
{
    button_state state = pressed ? button_state_pressed : button_state_released;
    uint serial = m_tabletSeat->m_seat->m_compositor->nextSerial();
    send_button(toolResource(), serial, button, state);
    return serial;
}

uint TabletToolV2::sendFrame()
{
    uint time = m_tabletSeat->m_seat->m_compositor->currentTimeMilliseconds();
    for (auto *resource : resourceMap())
        send_frame(resource->handle, time);
    return time;
}

void TabletToolV2::zwp_tablet_tool_v2_destroy(QtWaylandServer::zwp_tablet_tool_v2::Resource *resource)
{
    if (m_tabletSeat) {
        m_tabletSeat->m_toolsWaitingForDestroy.removeOne(resource);
    }
    wl_resource_destroy(resource->handle);
}

void TabletPadV2::sendRemoved()
{
    for (auto *resource : resourceMap()) {
        zwp_tablet_pad_v2_send_removed(resource->handle);
        m_tabletSeat->m_padsWaitingForDestroy.append(resource);
    }
    bool removed = m_tabletSeat->m_pads.removeOne(this);
    QVERIFY(removed);
}

void TabletPadV2::zwp_tablet_pad_v2_destroy(QtWaylandServer::zwp_tablet_pad_v2::Resource *resource)
{
    if (m_tabletSeat) {
        m_tabletSeat->m_padsWaitingForDestroy.removeOne(resource);
    }
    wl_resource_destroy(resource->handle);
}

class TabletCompositor : public DefaultCompositor {
public:
    explicit TabletCompositor()
    {
        exec([this] {
            m_config.autoConfigure = true;
            add<TabletManagerV2>(tabletVersion);
        });
    }
    TabletSeatV2 *tabletSeat(int i = 0)
    {
        return get<TabletManagerV2>()->tabletSeatFor(get<Seat>(i));
    }
    TabletV2 *tablet(int i = 0, int iSeat = 0)
    {
        if (auto *ts = tabletSeat(iSeat))
            return ts->m_tablets.value(i, nullptr);
        return nullptr;
    }
    TabletToolV2 *tabletTool(int i = 0, int iSeat = 0)
    {
        if (auto *ts = tabletSeat(iSeat))
            return ts->m_tools.value(i, nullptr);
        return nullptr;
    }
    TabletPadV2 *tabletPad(int i = 0, int iSeat = 0)
    {
        if (auto *ts = tabletSeat(iSeat))
            return ts->m_pads.value(i, nullptr);
        return nullptr;
    }
};

Q_DECLARE_METATYPE(QtWaylandServer::zwp_tablet_tool_v2::type);
Q_DECLARE_METATYPE(QPointingDevice::PointerType);
Q_DECLARE_METATYPE(Qt::MouseButton);

class tst_tabletv2 : public QObject, private TabletCompositor
{
    using ToolType = QtWaylandServer::zwp_tablet_tool_v2::type;
    Q_OBJECT
private slots:
    void cleanup();
    void bindsToManager();
    void createsTabletSeat();
    void destroysTablet();
    void destroysTool();
    void destroysPad();
    void removeTabletBeforeTool();
    void removeTabletBeforePad();
    void proximityEvents();
    void moveEvent();
    void pointerType_data();
    void pointerType();
    void hardwareSerial();
    void buttons_data();
    void buttons();
    void tabletEvents();
};

class ProximityFilter : public QObject {
    Q_OBJECT
public:
    ProximityFilter() { qApp->installEventFilter(this); }
    ~ProximityFilter() override { qDeleteAll(m_events); }
    QList<QTabletEvent *> m_events;

    int nextEventIndex = 0;
    int numEvents() const { return m_events.size() - nextEventIndex; }
    QTabletEvent *popEvent()
    {
        auto *event = m_events.value(nextEventIndex, nullptr);
        if (event)
            ++nextEventIndex;
        return event;
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        Q_UNUSED(object);
        switch (event->type()) {
        case QEvent::TabletEnterProximity:
        case QEvent::TabletLeaveProximity: {
            auto *e = static_cast<QTabletEvent *>(event);
            auto *ev = new QTabletEvent(e->type(), e->pointingDevice(), e->position(), e->globalPosition(),
                                     e->pressure(), e->xTilt(), e->yTilt(),
                                     e->tangentialPressure(), e->rotation(), e->z(),
                                     Qt::KeyboardModifier::NoModifier,
                                     e->button(), e->buttons());
            m_events << ev;
            break;
        }
        default:
            break;
        }
        return false;
    }
};

void tst_tabletv2::cleanup()
{
    exec([&] {
        tabletSeat()->removeAll();
    });
    QCOMPOSITOR_COMPARE(get<TabletManagerV2>()->m_tabletSeats.size(), 1);
    QCOMPOSITOR_COMPARE(tabletSeat()->m_tablets.size(), 0);
    QCOMPOSITOR_COMPARE(tabletSeat()->m_tools.size(), 0);
    QCOMPOSITOR_COMPARE(tabletSeat()->m_pads.size(), 0);

    QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
}

void tst_tabletv2::bindsToManager()
{
    QCOMPOSITOR_TRY_COMPARE(get<TabletManagerV2>()->resourceMap().size(), 1);
    QCOMPOSITOR_TRY_COMPARE(get<TabletManagerV2>()->resourceMap().first()->version(), tabletVersion);
}

void tst_tabletv2::createsTabletSeat()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->resourceMap().contains(client()));
    QCOMPOSITOR_TRY_COMPARE(tabletSeat()->resourceMap().value(client())->version(), tabletVersion);
    //TODO: Maybe also assert some capability reported though qt APIs?
}

void tst_tabletv2::destroysTablet()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
    });
    QCOMPOSITOR_TRY_VERIFY(tablet());

    exec([&] {
        tablet()->sendRemoved();
    });

    QCOMPOSITOR_TRY_VERIFY(!tablet());
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->m_tabletsWaitingForDestroy.empty());
}

void tst_tabletv2::destroysTool()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool();
    });
    QCOMPOSITOR_TRY_VERIFY(tabletTool());

    exec([&] {
        tabletTool()->sendRemoved();
        tablet()->sendRemoved();
    });

    QCOMPOSITOR_TRY_VERIFY(!tabletTool());
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->m_toolsWaitingForDestroy.empty());
}

void tst_tabletv2::destroysPad()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addPad();
    });
    QCOMPOSITOR_TRY_VERIFY(tabletPad());

    exec([&] {
        tabletPad()->sendRemoved();
    });

    QCOMPOSITOR_TRY_VERIFY(!tabletPad());
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->m_padsWaitingForDestroy.empty());
}

void tst_tabletv2::removeTabletBeforeTool()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool();
    });
    QCOMPOSITOR_TRY_VERIFY(tablet());
    QCOMPOSITOR_TRY_VERIFY(tabletTool());

    exec([&] { tablet()->sendRemoved(); });
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->m_tabletsWaitingForDestroy.empty());

    exec([&] { tabletTool()->sendRemoved(); });
    QCOMPOSITOR_TRY_VERIFY(!tabletTool());
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->m_toolsWaitingForDestroy.empty());
}

void tst_tabletv2::removeTabletBeforePad()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addPad();
    });
    QCOMPOSITOR_TRY_VERIFY(tablet());
    QCOMPOSITOR_TRY_VERIFY(tabletPad());

    exec([&] { tablet()->sendRemoved(); });
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->m_tabletsWaitingForDestroy.empty());

    exec([&] { tabletPad()->sendRemoved(); });
    QCOMPOSITOR_TRY_VERIFY(!tabletPad());
    QCOMPOSITOR_TRY_VERIFY(tabletSeat()->m_padsWaitingForDestroy.empty());
}

void tst_tabletv2::proximityEvents()
{
    ProximityFilter filter;

    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool();
    });

    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QCOMPOSITOR_TRY_VERIFY(tablet());
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        auto *tool = tabletTool();
        tool->sendProximityIn(tablet(), surface);
        tool->sendFrame();
    });

    QTRY_COMPARE(filter.numEvents(), 1);
    QTabletEvent *enterEvent = filter.popEvent();
    QCOMPARE(enterEvent->type(), QEvent::TabletEnterProximity);

    exec([&] {
        auto *tool = tabletTool();
        tool->sendProximityOut();
        tool->sendFrame();
    });

    QTRY_COMPARE(filter.numEvents(), 1);
    QTabletEvent *leaveEvent = filter.popEvent();
    QCOMPARE(leaveEvent->type(), QEvent::TabletLeaveProximity);
}

class TabletWindow : public QRasterWindow {
    Q_OBJECT
public:
    ~TabletWindow() override { qDeleteAll(m_events); }

    void tabletEvent(QTabletEvent *e) override
    {
        m_events << new QTabletEvent(e->type(), e->pointingDevice(), e->position(), e->globalPosition(),
                                     e->pressure(), e->xTilt(), e->yTilt(),
                                     e->tangentialPressure(), e->rotation(), e->z(),
                                     Qt::KeyboardModifier::NoModifier,
                                     e->button(), e->buttons());
        emit tabletEventReceived(m_events.last());
    }
    int nextEventIndex = 0;
    int numEvents() const { return m_events.size() - nextEventIndex; }
    QTabletEvent *popEvent()
    {
        auto *event = m_events.value(nextEventIndex, nullptr);
        if (event)
            ++nextEventIndex;
        return event;
    }

signals:
    void tabletEventReceived(QTabletEvent *event);

private:
    QList<QTabletEvent *> m_events;
};

void tst_tabletv2::moveEvent()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool();
    });

    TabletWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QCOMPOSITOR_TRY_VERIFY(tablet());
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        auto *tool = tabletTool();
        tool->sendProximityIn(tablet(), surface);
        tool->sendMotion(QPointF(12, 34));
        tool->sendFrame();
    });
    QTRY_VERIFY(window.numEvents());
    QTabletEvent *event = window.popEvent();
    QCOMPARE(event->type(), QEvent::TabletMove);
    QCOMPARE(event->pressure(), 0);
    QCOMPARE(event->position(), QPointF(12, 34));
}

void tst_tabletv2::pointerType_data()
{
    QTest::addColumn<ToolType>("toolType");
    QTest::addColumn<QPointingDevice::PointerType>("pointerType");
    QTest::addColumn<QInputDevice::DeviceType>("tabletDevice");

    QTest::newRow("pen") << ToolType::type_pen << QPointingDevice::PointerType::Pen << QInputDevice::DeviceType::Stylus;
    QTest::newRow("eraser") << ToolType::type_eraser << QPointingDevice::PointerType::Eraser << QInputDevice::DeviceType::Stylus;
    QTest::newRow("pencil") << ToolType::type_pencil << QPointingDevice::PointerType::Pen << QInputDevice::DeviceType::Stylus;
    QTest::newRow("airbrush") << ToolType::type_airbrush << QPointingDevice::PointerType::Pen << QInputDevice::DeviceType::Airbrush;
    QTest::newRow("brush") << ToolType::type_brush << QPointingDevice::PointerType::Pen << QInputDevice::DeviceType::Stylus; // TODO: is TabletDevice::Stylus the right thing?
    QTest::newRow("lens") << ToolType::type_lens << QPointingDevice::PointerType::Cursor << QInputDevice::DeviceType::Puck;
    // TODO: also add tests for FourDMouse and RotationStylus (also need to send capabilities)

    // TODO: should these rather be mapped to touch/mouse events?
    QTest::newRow("finger") << ToolType::type_finger << QPointingDevice::PointerType::Unknown << QInputDevice::DeviceType::Unknown;
    QTest::newRow("mouse") << ToolType::type_mouse << QPointingDevice::PointerType::Cursor << QInputDevice::DeviceType::Unknown;
}

void tst_tabletv2::pointerType()
{
    using ToolType = QtWaylandServer::zwp_tablet_tool_v2::type;
    QFETCH(ToolType, toolType);
    QFETCH(QPointingDevice::PointerType, pointerType);
    QFETCH(QInputDevice::DeviceType, tabletDevice);

    ProximityFilter filter;

    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool(toolType);
    });

    TabletWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QCOMPOSITOR_TRY_VERIFY(tablet());
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        auto *tool = tabletTool();
        tool->sendProximityIn(tablet(), surface);
        tool->sendMotion(QPointF(12, 34));
        tool->sendFrame();
    });

    QTRY_COMPARE(filter.numEvents(), 1);
    QTabletEvent *event = filter.popEvent();
    QCOMPARE(event->pointerType(), pointerType);
    QCOMPARE(event->deviceType(), tabletDevice);

    QTRY_VERIFY(window.numEvents());
    event = window.popEvent();
    QCOMPARE(event->pointerType(), pointerType);
    QCOMPARE(event->deviceType(), tabletDevice);

    exec([&] {
        tabletTool()->sendProximityOut();
        tabletTool()->sendFrame();
    });

    QTRY_VERIFY(filter.numEvents());
    event = filter.popEvent();
    QCOMPARE(event->pointerType(), pointerType);
    QCOMPARE(event->deviceType(), tabletDevice);
}

void tst_tabletv2::hardwareSerial()
{
    ProximityFilter filter;
    const QPointingDeviceUniqueId uid = QPointingDeviceUniqueId::fromNumericId(0xbaba15dead15f00d);

    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool(ToolType::type_pen, uid.numericId());
    });

    TabletWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QCOMPOSITOR_TRY_VERIFY(tablet());
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        auto *tool = tabletTool();
        tool->sendProximityIn(tablet(), surface);
        tool->sendMotion(QPointF(12, 34));
        tool->sendFrame();
    });

    QTRY_COMPARE(filter.numEvents(), 1);
    QTabletEvent *event = filter.popEvent();
    QCOMPARE(event->pointingDevice()->uniqueId(), uid);

    QTRY_VERIFY(window.numEvents());
    event = window.popEvent();
    QCOMPARE(event->pointingDevice()->uniqueId(), uid);

    exec([&] {
        tabletTool()->sendProximityOut();
        tabletTool()->sendFrame();
    });

    QTRY_VERIFY(filter.numEvents());
    event = filter.popEvent();
    QCOMPARE(event->pointingDevice()->uniqueId(), uid);
}

// As defined in linux/input-event-codes.h
#ifndef BTN_STYLUS
#define BTN_STYLUS 0x14b
#endif
#ifndef BTN_STYLUS2
#define BTN_STYLUS2 0x14c
#endif

void tst_tabletv2::buttons_data()
{
    QTest::addColumn<uint>("tabletButton");
    QTest::addColumn<Qt::MouseButton>("mouseButton");

    QTest::newRow("BTN_STYLUS2") << uint(BTN_STYLUS2) << Qt::MouseButton::RightButton;
    QTest::newRow("BTN_STYLUS") << uint(BTN_STYLUS) << Qt::MouseButton::MiddleButton;
}

void tst_tabletv2::buttons()
{
    QFETCH(uint, tabletButton);
    QFETCH(Qt::MouseButton, mouseButton);

    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool();
    });

    TabletWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QCOMPOSITOR_TRY_VERIFY(tablet());
    exec([&] {
        tabletTool()->sendProximityIn(tablet(), xdgSurface()->m_surface);
        tabletTool()->sendMotion(QPointF(12, 34));
        tabletTool()->sendFrame();
    });

    QTRY_VERIFY(window.numEvents());
    window.popEvent();

    QCOMPOSITOR_TRY_VERIFY(tablet());
    exec([&] {
        tabletTool()->sendButton(tabletButton, true);
        tabletTool()->sendFrame();
        tabletTool()->sendButton(tabletButton, false);
        tabletTool()->sendFrame();
    });

    QTRY_VERIFY(window.numEvents());
    QTabletEvent *event = window.popEvent();
    QCOMPARE(event->buttons(), mouseButton);

    exec([&] {
        tabletTool()->sendProximityOut();
        tabletTool()->sendFrame();
    });
}

void tst_tabletv2::tabletEvents()
{
    QCOMPOSITOR_TRY_VERIFY(tabletSeat());
    exec([&] {
        tabletSeat()->addTablet();
        tabletSeat()->addTool();
    });

    TabletWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QCOMPOSITOR_TRY_VERIFY(tablet());
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        auto *tool = tabletTool();
        // TODO: encapsulate this into a helper function?
        tool->sendProximityIn(tablet(), surface);
        tool->sendMotion(QPointF(12, 34));
        tool->sendDown();
        tool->sendPressure(65535);
        tool->sendFrame();
    });

    QTRY_VERIFY(window.numEvents());
    QTabletEvent *event = window.popEvent();
    QCOMPARE(event->type(), QEvent::TabletPress);
    QCOMPARE(event->pressure(), 1.0);
    QCOMPARE(event->position(), QPointF(12, 34));

    // Values we didn't send should be 0
    QCOMPARE(event->rotation(), 0);
    QCOMPARE(event->xTilt(), 0);
    QCOMPARE(event->yTilt(), 0);

    exec([&] {
        tabletTool()->sendMotion(QPointF(45, 56));
        tabletTool()->sendPressure(65535/2);
        tabletTool()->sendRotation(90);
        tabletTool()->sendTilt(13, 37);
        tabletTool()->sendFrame();
    });

    QTRY_VERIFY(window.numEvents());
    event = window.popEvent();
    QCOMPARE(event->type(), QEvent::TabletMove);
    QVERIFY(qAbs(event->pressure() - 0.5) < 0.01);
    QVERIFY(qAbs(event->rotation() - 90) < 0.01);
    QVERIFY(qAbs(event->xTilt() - 13) < 0.01);
    QVERIFY(qAbs(event->yTilt() - 37) < 0.01);
    QCOMPARE(event->position(), QPointF(45, 56));

    // Verify that the values stay the same if we don't update them
    exec([&] {
        tabletTool()->sendMotion(QPointF(10, 11)); // Change position only
        tabletTool()->sendFrame();
    });
    QTRY_VERIFY(window.numEvents());
    event = window.popEvent();
    QCOMPARE(event->type(), QEvent::TabletMove);
    QVERIFY(qAbs(event->pressure() - 0.5) < 0.01);
    QVERIFY(qAbs(event->rotation() - 90) < 0.01);
    QVERIFY(qAbs(event->xTilt() - 13) < 0.01);
    QVERIFY(qAbs(event->yTilt() - 37) < 0.01);
    QCOMPARE(event->position(), QPointF(10, 11));

    exec([&] {
        tabletTool()->sendPressure(0);
        tabletTool()->sendUp();
        tabletTool()->sendFrame();

        tabletTool()->sendProximityOut();
        tabletTool()->sendFrame();
    });

    QTRY_VERIFY(window.numEvents());
    event = window.popEvent();
    QCOMPARE(event->type(), QEvent::TabletRelease);
    QCOMPARE(event->pressure(), 0);
    QCOMPARE(event->position(), QPointF(10, 11));
}

QCOMPOSITOR_TEST_MAIN(tst_tabletv2)
#include "tst_tabletv2.moc"
