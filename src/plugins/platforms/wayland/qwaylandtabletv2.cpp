// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandtabletv2_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandsurface_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandbuffer_p.h"
#include "qwaylandcursorsurface_p.h"
#include "qwaylandcursor_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/qpa/qwindowsysteminterface_p.h>

#include <wayland-cursor.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

using namespace Qt::StringLiterals;

#if QT_CONFIG(cursor)
int QWaylandTabletToolV2::idealCursorScale() const
{
    if (m_tabletSeat->seat()->mQDisplay->compositor()->version() < 3) {
        return 1;
    }

    if (auto *s = mCursor.surface.get()) {
        if (s->outputScale() > 0)
            return s->outputScale();
    }

    return m_tabletSeat->seat()->mCursor.fallbackOutputScale;
}

void QWaylandTabletToolV2::updateCursorTheme()
{
    QString cursorThemeName;
    QSize cursorSize;

    if (const QPlatformTheme *platformTheme = QGuiApplicationPrivate::platformTheme()) {
        cursorThemeName = platformTheme->themeHint(QPlatformTheme::MouseCursorTheme).toString();
        cursorSize = platformTheme->themeHint(QPlatformTheme::MouseCursorSize).toSize();
    }

    if (cursorThemeName.isEmpty())
        cursorThemeName = QStringLiteral("default");
    if (cursorSize.isEmpty())
        cursorSize = QSize(24, 24);

    int scale = idealCursorScale();
    int pixelSize = cursorSize.width() * scale;
    auto *display = m_tabletSeat->seat()->mQDisplay;
    mCursor.theme = display->loadCursorTheme(cursorThemeName, pixelSize);

    if (!mCursor.theme)
        return; // A warning has already been printed in loadCursorTheme

    if (auto *arrow = mCursor.theme->cursor(Qt::ArrowCursor)) {
        int arrowPixelSize = qMax(arrow->images[0]->width,
                                  arrow->images[0]->height); // Not all cursor themes are square
        while (scale > 1 && arrowPixelSize / scale < cursorSize.width())
            --scale;
    } else {
        qCWarning(lcQpaWayland) << "Cursor theme does not support the arrow cursor";
    }
    mCursor.themeBufferScale = scale;
}

void QWaylandTabletToolV2::updateCursor()
{
    if (mEnterSerial == 0)
        return;

    auto shape = m_tabletSeat->seat()->mCursor.shape;

    if (shape == Qt::BlankCursor) {
        if (mCursor.surface)
            mCursor.surface->reset();
        set_cursor(mEnterSerial, nullptr, 0, 0);
        return;
    }

    if (shape == Qt::BitmapCursor) {
        auto buffer = m_tabletSeat->seat()->mCursor.bitmapBuffer;
        if (!buffer) {
            qCWarning(lcQpaWayland) << "No buffer for bitmap cursor, can't set cursor";
            return;
        }
        auto hotspot = m_tabletSeat->seat()->mCursor.hotspot;
        int bufferScale = m_tabletSeat->seat()->mCursor.bitmapScale;
        getOrCreateCursorSurface()->update(buffer->buffer(), hotspot, buffer->size(), bufferScale);
        return;
    }

    if (mCursor.shape) {
        if (mCursor.surface) {
            mCursor.surface->reset();
        }
        mCursor.shape->setShape(mEnterSerial, shape);
        return;
    }

    if (!mCursor.theme || idealCursorScale() != mCursor.themeBufferScale)
        updateCursorTheme();

    if (!mCursor.theme)
        return;

    // Set from shape using theme
    QElapsedTimer &timer = m_tabletSeat->seat()->mCursor.animationTimer;
    const uint time = timer.isValid() ? timer.elapsed() : 0;

    if (struct ::wl_cursor *waylandCursor = mCursor.theme->cursor(shape)) {
        uint duration = 0;
        int frame = wl_cursor_frame_and_duration(waylandCursor, time, &duration);
        ::wl_cursor_image *image = waylandCursor->images[frame];

        struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);
        if (!buffer) {
            qCWarning(lcQpaWayland) << "Could not find buffer for cursor" << shape;
            return;
        }
        int bufferScale = mCursor.themeBufferScale;
        QPoint hotspot = QPoint(image->hotspot_x, image->hotspot_y) / bufferScale;
        QSize size = QSize(image->width, image->height) / bufferScale;
        bool animated = duration > 0;
        if (animated) {
            mCursor.gotFrameCallback = false;
            mCursor.gotTimerCallback = false;
            mCursor.frameTimer.start(duration);
        }
        getOrCreateCursorSurface()->update(buffer, hotspot, size, bufferScale, animated);
        return;
    }

    qCWarning(lcQpaWayland) << "Unable to change to cursor" << shape;
}

CursorSurface<QWaylandTabletToolV2> *QWaylandTabletToolV2::getOrCreateCursorSurface()
{
    if (!mCursor.surface)
        mCursor.surface.reset(
                new CursorSurface<QWaylandTabletToolV2>(this, m_tabletSeat->seat()->mQDisplay));
    return mCursor.surface.get();
}

void QWaylandTabletToolV2::cursorTimerCallback()
{
    mCursor.gotTimerCallback = true;
    if (mCursor.gotFrameCallback)
        updateCursor();
}

void QWaylandTabletToolV2::cursorFrameCallback()
{
    mCursor.gotFrameCallback = true;
    if (mCursor.gotTimerCallback)
        updateCursor();
}

#endif // QT_CONFIG(cursor)

QWaylandTabletManagerV2::QWaylandTabletManagerV2(QWaylandDisplay *display, uint id, uint version)
    : zwp_tablet_manager_v2(display->wl_registry(), id, qMin(version, uint(1)))
{
    qCDebug(lcQpaInputDevices, "new tablet manager: ID %d version %d", id, version);
}

QWaylandTabletManagerV2::~QWaylandTabletManagerV2()
{
    destroy();
}

QWaylandTabletSeatV2::QWaylandTabletSeatV2(QWaylandTabletManagerV2 *manager, QWaylandInputDevice *seat)
    : QtWayland::zwp_tablet_seat_v2(manager->get_tablet_seat(seat->wl_seat()))
    , m_seat(seat)
{
    qCDebug(lcQpaInputDevices) << "new tablet seat" << seat->seatname() << "id" << seat->id();
}

QWaylandTabletSeatV2::~QWaylandTabletSeatV2()
{
    qDeleteAll(m_tablets);
    qDeleteAll(m_tools);
    qDeleteAll(m_deadTools);
    qDeleteAll(m_pads);
    destroy();
}

void QWaylandTabletSeatV2::zwp_tablet_seat_v2_tablet_added(zwp_tablet_v2 *id)
{
    auto *tablet = new QWaylandTabletV2(id, m_seat->seatname());
    qCDebug(lcQpaInputDevices) << "seat" << this << id << "has tablet" << tablet;
    tablet->setParent(this);
    m_tablets.push_back(tablet);
    connect(tablet, &QWaylandTabletV2::destroyed, this, [this, tablet] { m_tablets.removeOne(tablet); });
}

void QWaylandTabletSeatV2::zwp_tablet_seat_v2_tool_added(zwp_tablet_tool_v2 *id)
{
    qDeleteAll(m_deadTools);
    auto *tool = new QWaylandTabletToolV2(this, id);
    if (m_tablets.size() == 1) {
        tool->setParent(m_tablets.first());
        QPointingDevicePrivate *d = QPointingDevicePrivate::get(tool);
        d->name = m_tablets.first()->name() + u" stylus";
    } else {
        qCDebug(lcQpaInputDevices) << "seat" << this << "has tool" << tool << "for one of these tablets:" << m_tablets;
        // TODO identify which tablet if there are several; then tool->setParent(tablet)
    }
    m_tools.push_back(tool);
    connect(tool, &QWaylandTabletToolV2::destroyed, this, [this, tool] {
        m_tools.removeOne(tool);
        m_deadTools.removeOne(tool);
    });
}

void QWaylandTabletSeatV2::zwp_tablet_seat_v2_pad_added(zwp_tablet_pad_v2 *id)
{
    auto *pad = new QWaylandTabletPadV2(id);
    if (m_tablets.size() == 1) {
        pad->setParent(m_tablets.first());
        QPointingDevicePrivate *d = QPointingDevicePrivate::get(pad);
        d->name = m_tablets.first()->name() + u" touchpad";
    } else {
        qCDebug(lcQpaInputDevices) << "seat" << this << "has touchpad" << pad << "for one of these tablets:" << m_tablets;
        // TODO identify which tablet if there are several
    }
    m_pads.push_back(pad);
    connect(pad, &QWaylandTabletPadV2::destroyed, this, [this, pad] { m_pads.removeOne(pad); });
}

QWaylandTabletV2::QWaylandTabletV2(::zwp_tablet_v2 *tablet, const QString &seatName)
    : QPointingDevice(u"unknown"_s, -1, DeviceType::Stylus, PointerType::Pen,
                      Capability::Position | Capability::Hover,
                      1, 1, seatName)
    , QtWayland::zwp_tablet_v2(tablet)
{
    qCDebug(lcQpaInputDevices) << "new tablet on seat" << seatName;
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->seatName = seatName;
}

QWaylandTabletV2::~QWaylandTabletV2()
{
    destroy();
}

void QWaylandTabletV2::zwp_tablet_v2_name(const QString &name)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->name = name;
}

void QWaylandTabletV2::zwp_tablet_v2_id(uint32_t vid, uint32_t pid)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->systemId = (quint64(vid) << 32) | pid;
    qCDebug(lcQpaInputDevices) << "vid" << vid << "pid" << pid << "stored as systemId in" << this;
}

void QWaylandTabletV2::zwp_tablet_v2_path(const QString &path)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->busId = path;
}

void QWaylandTabletV2::zwp_tablet_v2_done()
{
    QWindowSystemInterface::registerInputDevice(this);
}

void QWaylandTabletSeatV2::updateCursor()
{
    for (auto tool : m_tools)
        tool->updateCursor();
}

void QWaylandTabletSeatV2::toolRemoved(QWaylandTabletToolV2 *tool)
{
    m_tools.removeOne(tool);
    m_deadTools.append(tool);
}

void QWaylandTabletV2::zwp_tablet_v2_removed()
{
    deleteLater();
}

QWaylandTabletToolV2::QWaylandTabletToolV2(QWaylandTabletSeatV2 *tabletSeat, ::zwp_tablet_tool_v2 *tool)
    : QPointingDevice(u"tool"_s, -1, DeviceType::Stylus, PointerType::Pen,
                      Capability::Position | Capability::Hover,
                      1, 1, tabletSeat->seat()->seatname())
    , QtWayland::zwp_tablet_tool_v2(tool)
    , m_tabletSeat(tabletSeat)
{
    // TODO get the number of buttons somehow?

#if QT_CONFIG(cursor)
    if (auto cursorShapeManager = m_tabletSeat->seat()->mQDisplay->cursorShapeManager()) {
        mCursor.shape.reset(
                new QWaylandCursorShape(cursorShapeManager->get_tablet_tool_v2(object())));
    }

    mCursor.frameTimer.setSingleShot(true);
    mCursor.frameTimer.callOnTimeout(this, [&]() { cursorTimerCallback(); });
#endif
}

QWaylandTabletToolV2::~QWaylandTabletToolV2()
{
    destroy();
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_type(uint32_t tool_type)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);

    switch (tool_type) {
    case type_airbrush:
    case type_brush:
    case type_pencil:
    case type_pen:
        d->pointerType = QPointingDevice::PointerType::Pen;
        break;
    case type_eraser:
        d->pointerType = QPointingDevice::PointerType::Eraser;
        break;
    case type_mouse:
    case type_lens:
        d->pointerType = QPointingDevice::PointerType::Cursor;
        break;
    case type_finger:
        d->pointerType = QPointingDevice::PointerType::Unknown;
        break;
    }

    switch (tool_type) {
    case type::type_airbrush:
        d->deviceType = QInputDevice::DeviceType::Airbrush;
        d->capabilities.setFlag(QInputDevice::Capability::TangentialPressure);
        break;
    case type::type_brush:
    case type::type_pencil:
    case type::type_pen:
    case type::type_eraser:
        d->deviceType = QInputDevice::DeviceType::Stylus;
        break;
    case type::type_lens:
        d->deviceType = QInputDevice::DeviceType::Puck;
        break;
    case type::type_mouse:
    case type::type_finger:
        d->deviceType = QInputDevice::DeviceType::Unknown;
        break;
    }
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_hardware_serial(uint32_t hardware_serial_hi, uint32_t hardware_serial_lo)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->uniqueId = QPointingDeviceUniqueId::fromNumericId((quint64(hardware_serial_hi) << 32) + hardware_serial_lo);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_hardware_id_wacom(uint32_t hardware_id_hi, uint32_t hardware_id_lo)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->systemId = (quint64(hardware_id_hi) << 32) + hardware_id_lo;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_capability(uint32_t capability)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    switch (capability) {
    case capability_tilt:
        // no distinction... we have to assume it has both axes
        d->capabilities.setFlag(QInputDevice::Capability::XTilt);
        d->capabilities.setFlag(QInputDevice::Capability::YTilt);
        break;
    case capability_pressure:
        d->capabilities.setFlag(QInputDevice::Capability::Pressure);
        break;
    case capability_distance:
        d->capabilities.setFlag(QInputDevice::Capability::ZPosition);
        break;
    case capability_rotation:
        d->capabilities.setFlag(QInputDevice::Capability::Rotation);
        break;
    case capability_slider:
        // nothing to represent that so far
        break;
    case capability_wheel:
        d->capabilities.setFlag(QInputDevice::Capability::Scroll);
        d->capabilities.setFlag(QInputDevice::Capability::PixelScroll);
        break;
    }
    qCDebug(lcQpaInputDevices) << capability << "->" << this;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_done()
{
    QWindowSystemInterface::registerInputDevice(this);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_removed()
{
    m_tabletSeat->toolRemoved(this);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_proximity_in(uint32_t serial, zwp_tablet_v2 *tablet, wl_surface *surface)
{
    Q_UNUSED(tablet);

    m_tabletSeat->seat()->mSerial = serial;
    mEnterSerial = serial;

    if (Q_UNLIKELY(!surface)) {
        qCDebug(lcQpaWayland) << "Ignoring zwp_tablet_tool_v2_proximity_v2 with no surface";
        return;
    }
    m_pending.enteredSurface = true;
    m_pending.proximitySurface = QWaylandSurface::fromWlSurface(surface);

#if QT_CONFIG(cursor)
    // Depends on mEnterSerial being updated
    updateCursor();
#endif
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_proximity_out()
{
    m_pending.enteredSurface = false;
    m_pending.proximitySurface = nullptr;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_down(uint32_t serial)
{
    m_pending.down = true;

    m_tabletSeat->seat()->mSerial = serial;

    if (m_pending.proximitySurface) {
        if (QWaylandSurface *window = m_pending.proximitySurface) {
            QWaylandInputDevice *seat = m_tabletSeat->seat();
            seat->display()->setLastInputDevice(seat, serial, window);
        }
    }
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_up()
{
    m_pending.down = false;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_motion(wl_fixed_t x, wl_fixed_t y)
{
    m_pending.surfacePosition = QPointF(wl_fixed_to_double(x), wl_fixed_to_double(y));
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_pressure(uint32_t pressure)
{
    const int maxPressure = 65535;
    m_pending.pressure = qreal(pressure)/maxPressure;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_distance(uint32_t distance)
{
    m_pending.distance = distance;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_tilt(wl_fixed_t tilt_x, wl_fixed_t tilt_y)
{
    m_pending.xTilt = wl_fixed_to_double(tilt_x);
    m_pending.yTilt = wl_fixed_to_double(tilt_y);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_rotation(wl_fixed_t degrees)
{
    m_pending.rotation = wl_fixed_to_double(degrees);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_slider(int32_t position)
{
    m_pending.slider = qreal(position) / 65535;
}

static Qt::MouseButton mouseButtonFromTablet(uint button)
{
    switch (button) {
    case 0x110: return Qt::MouseButton::LeftButton; // BTN_LEFT
    case 0x14b: return Qt::MouseButton::MiddleButton; // BTN_STYLUS
    case 0x14c: return Qt::MouseButton::RightButton; // BTN_STYLUS2
    default:
        return Qt::NoButton;
    }
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_button(uint32_t serial, uint32_t button, uint32_t state)
{
    m_tabletSeat->seat()->mSerial = serial;

    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    Qt::MouseButton mouseButton = mouseButtonFromTablet(button);
    if (state == button_state_pressed)
        m_pending.buttons |= mouseButton;
    else
        m_pending.buttons &= ~mouseButton;
    // ideally we'd get button count when the tool is discovered; seems to be a shortcoming in tablet-unstable-v2
    // but if we get events from buttons we didn't know existed, increase it
    if (mouseButton == Qt::RightButton)
        d->buttonCount = qMax(d->buttonCount, 2);
    else if (mouseButton == Qt::MiddleButton)
        d->buttonCount = qMax(d->buttonCount, 3);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_frame(uint32_t time)
{
    if (!m_pending.proximitySurface) {
        if (m_applied.enteredSurface) {
            // leaving proximity
            QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(nullptr, this, false);
            m_pending = State(); // Don't leave pressure etc. lying around when we enter the next surface
            m_applied = State();
        } else {
            qCWarning(lcQpaWayland) << "Can't send tablet event with no proximity surface, ignoring";
        }
        return;
    }

    QWaylandWindow *waylandWindow = QWaylandWindow::fromWlSurface(m_pending.proximitySurface->object());
    QWindow *window = waylandWindow->window();
    const bool needsTabletEvent = !(m_pending == m_applied);

    if (!m_applied.proximitySurface || needsTabletEvent) {
        ulong timestamp = time;
        const QPointF localPosition = waylandWindow->mapFromWlSurface(m_pending.surfacePosition);
        const QPointF globalPosition = waylandWindow->mapToGlobalF(localPosition);
        const Qt::MouseButtons buttons = m_pending.buttons |
                (m_pending.down ? Qt::MouseButton::LeftButton : Qt::MouseButton::NoButton);
        const qreal pressure = m_pending.pressure;
        const qreal xTilt = m_pending.xTilt;
        const qreal yTilt = m_pending.yTilt;
        const qreal tangentialPressure = m_pending.slider;
        const qreal rotation = m_pending.rotation;
        const int z = int(m_pending.distance);

        if (!m_applied.proximitySurface) {
            QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(window, this, true, localPosition, globalPosition,
                                                                         buttons, xTilt, yTilt, tangentialPressure, rotation, z,
                                                                         m_tabletSeat->seat()->modifiers());
            m_applied.proximitySurface = m_pending.proximitySurface;
        }

        if (needsTabletEvent) {
            // do not use localPosition here since that is in Qt window coordinates
            // but we need surface coordinates to include the decoration
            bool decorationHandledEvent = waylandWindow->handleTabletEventDecoration(
                    m_tabletSeat->seat(), m_pending.surfacePosition,
                    window->mapToGlobal(m_pending.surfacePosition), buttons,
                    m_tabletSeat->seat()->modifiers());

            if (!decorationHandledEvent) {
                QWindowSystemInterface::handleTabletEvent(window, timestamp, this, localPosition, globalPosition,
                                                          buttons, pressure,
                                                          xTilt, yTilt, tangentialPressure, rotation, z,
                                                          m_tabletSeat->seat()->modifiers());
            }
        }
    }

    m_applied = m_pending;
}

// TODO: delete when upgrading to c++20
bool QWaylandTabletToolV2::State::operator==(const QWaylandTabletToolV2::State &o) const {
    return
            down == o.down &&
            proximitySurface.data() == o.proximitySurface.data() &&
            enteredSurface == o.enteredSurface &&
            surfacePosition == o.surfacePosition &&
            distance == o.distance &&
            pressure == o.pressure &&
            rotation == o.rotation &&
            xTilt == o.xTilt &&
            yTilt == o.yTilt &&
            slider == o.slider &&
            buttons == o.buttons;
}

QWaylandTabletPadV2::QWaylandTabletPadV2(::zwp_tablet_pad_v2 *pad)
    : QPointingDevice(u"tablet touchpad"_s, -1, DeviceType::TouchPad, PointerType::Finger,
                      Capability::Position,
                      1, 1)
    , QtWayland::zwp_tablet_pad_v2(pad)
{
}

QWaylandTabletPadV2::~QWaylandTabletPadV2()
{
    destroy();
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_path(const QString &path)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->busId = path;
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_buttons(uint32_t buttons)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->buttonCount = buttons;
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_group(zwp_tablet_pad_group_v2 *pad_group)
{
    // As of writing Qt does not handle tablet pads group and the controls on it
    // This proxy is server created so it is just deleted here to not leak it
    zwp_tablet_pad_group_v2_destroy(pad_group);
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_done()
{
    QWindowSystemInterface::registerInputDevice(this);
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_removed()
{
    delete this;
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

#include "moc_qwaylandtabletv2_p.cpp"
