// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibinputtouch_p.h"
#include "qlibinputhandler_p.h"
#include "qoutputmapping_p.h"
#include <libinput.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QPointingDevice>
#include <QtGui/QScreen>
#include <QtGui/QPointingDevice>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/private/qpointingdevice_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcLibInputEvents, "qt.qpa.input.events")

QWindowSystemInterface::TouchPoint *QLibInputTouch::DeviceState::point(int32_t slot)
{
    const int id = qMax(0, slot);

    for (int i = 0; i < m_points.size(); ++i)
        if (m_points.at(i).id == id)
            return &m_points[i];

    return nullptr;
}

QLibInputTouch::DeviceState *QLibInputTouch::deviceState(libinput_event_touch *e)
{
    libinput_device *dev = libinput_event_get_device(libinput_event_touch_get_base_event(e));
    return &m_devState[dev];
}

QRect QLibInputTouch::screenGeometry(DeviceState *state)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!state->m_screenName.isEmpty()) {
        if (!m_screen) {
            const QList<QScreen *> screens = QGuiApplication::screens();
            for (QScreen *s : screens) {
                if (s->name() == state->m_screenName) {
                    m_screen = s;
                    break;
                }
            }
        }
        if (m_screen)
            screen = m_screen;
    }
    return screen ? QHighDpi::toNativePixels(screen->geometry(), screen) : QRect();
}

QPointF QLibInputTouch::getPos(libinput_event_touch *e)
{
    DeviceState *state = deviceState(e);
    QRect geom = screenGeometry(state);
    const double x = libinput_event_touch_get_x_transformed(e, geom.width());
    const double y = libinput_event_touch_get_y_transformed(e, geom.height());
    return geom.topLeft() + QPointF(x, y);
}

static void setMatrix(libinput_device *dev)
{
    if (libinput_device_config_calibration_has_matrix(dev)) {
        QByteArray env = qgetenv("QT_QPA_LIBINPUT_TOUCH_MATRIX");
        env = env.simplified();
        if (env.size()) {
            float matrix[6];
            QList<QByteArray> list = env.split(' ');
            if (list.length() != 6) {
                qCWarning(qLcLibInput, "matrix length %" PRIdQSIZETYPE " wrong, should be 6",
                          list.length());
                return;
            }
            for (int i = 0; i < 6; i++) {
                bool ok = true;
                matrix[i] = list[i].toFloat(&ok);
                if (!ok) {
                    qCWarning(qLcLibInput, "Invalid matrix entry %d %s ", i, list[i].constData());
                    return;
                }
            }
            if (libinput_device_config_calibration_set_matrix(dev, matrix) != LIBINPUT_CONFIG_STATUS_SUCCESS)
                qCWarning(qLcLibInput, "Failed to set libinput calibration matrix ");
        }
    } else {
        qCWarning(qLcLibInput, "Touch device doesn't support matrix");
    }
}
void QLibInputTouch::registerDevice(libinput_device *dev)
{
    struct udev_device *udev_device;
    udev_device = libinput_device_get_udev_device(dev);
    QString devNode = QString::fromUtf8(udev_device_get_devnode(udev_device));
    QString devName = QString::fromUtf8(libinput_device_get_name(dev));

    qCDebug(qLcLibInput, "libinput: registerDevice %s - %s",
            qPrintable(devNode), qPrintable(devName));

    QOutputMapping *mapping = QOutputMapping::get();
    QRect geom;
    if (mapping->load()) {
        m_devState[dev].m_screenName = mapping->screenNameForDeviceNode(devNode);
        if (!m_devState[dev].m_screenName.isEmpty()) {
            geom = screenGeometry(&m_devState[dev]);
            qCDebug(qLcLibInput) << "libinput: Mapping device" << devNode
                                 << "to screen" << m_devState[dev].m_screenName
                                 << "with geometry" << geom;
        }
    }

    QPointingDevice *&td = m_devState[dev].m_touchDevice;
    td = new QPointingDevice(devName, udev_device_get_devnum(udev_device),
                             QInputDevice::DeviceType::TouchScreen, QPointingDevice::PointerType::Finger,
                             QPointingDevice::Capability::Position | QPointingDevice::Capability::Area, 16, 0);
    auto devPriv = QPointingDevicePrivate::get(td);
    devPriv->busId = QString::fromLocal8Bit(udev_device_get_syspath(udev_device)); // TODO is that the best to choose?
    if (!geom.isNull())
        devPriv->setAvailableVirtualGeometry(geom);
    QWindowSystemInterface::registerInputDevice(td);
    setMatrix(dev);
}

void QLibInputTouch::unregisterDevice(libinput_device *dev)
{
    Q_UNUSED(dev);
    // There is no way to remove a QPointingDevice.
}

void QLibInputTouch::processTouchDown(libinput_event_touch *e)
{
    int slot = libinput_event_touch_get_slot(e);
    DeviceState *state = deviceState(e);
    QWindowSystemInterface::TouchPoint *tp = state->point(slot);
    if (tp) {
        qWarning("Incorrect touch state");
    } else {
        QWindowSystemInterface::TouchPoint newTp;
        newTp.id = qMax(0, slot);
        newTp.state = QEventPoint::State::Pressed;
        newTp.area = QRect(0, 0, 8, 8);
        newTp.area.moveCenter(getPos(e));
        state->m_points.append(newTp);
        qCDebug(qLcLibInputEvents) << "touch down" << newTp;
    }
}

void QLibInputTouch::processTouchMotion(libinput_event_touch *e)
{
    int slot = libinput_event_touch_get_slot(e);
    DeviceState *state = deviceState(e);
    QWindowSystemInterface::TouchPoint *tp = state->point(slot);
    if (tp) {
        QEventPoint::State tmpState = QEventPoint::State::Updated;
        const QPointF p = getPos(e);
        if (tp->area.center() == p)
            tmpState = QEventPoint::State::Stationary;
        else
            tp->area.moveCenter(p);
        // 'down' may be followed by 'motion' within the same "frame".
        // Handle this by compressing and keeping the Pressed state until the 'frame'.
        if (tp->state != QEventPoint::State::Pressed && tp->state != QEventPoint::State::Released)
            tp->state = tmpState;
        qCDebug(qLcLibInputEvents) << "touch move" << tp;
    } else {
        qWarning("Inconsistent touch state (got 'motion' without 'down')");
    }
}

void QLibInputTouch::processTouchUp(libinput_event_touch *e)
{
    int slot = libinput_event_touch_get_slot(e);
    DeviceState *state = deviceState(e);
    QWindowSystemInterface::TouchPoint *tp = state->point(slot);
    if (tp) {
        tp->state = QEventPoint::State::Released;
        // There may not be a Frame event after the last Up. Work this around.
        QEventPoint::States s;
        for (int i = 0; i < state->m_points.size(); ++i)
            s |= state->m_points.at(i).state;
        qCDebug(qLcLibInputEvents) << "touch up" << s << tp;
        if (s == QEventPoint::State::Released)
            processTouchFrame(e);
        else
            qCDebug(qLcLibInputEvents, "waiting for all points to be released");
    } else {
        qWarning("Inconsistent touch state (got 'up' without 'down')");
    }
}

void QLibInputTouch::processTouchCancel(libinput_event_touch *e)
{
    DeviceState *state = deviceState(e);
    qCDebug(qLcLibInputEvents) << "touch cancel" << state->m_points;
    if (state->m_touchDevice)
        QWindowSystemInterface::handleTouchCancelEvent(nullptr, state->m_touchDevice, QGuiApplication::keyboardModifiers());
    else
        qWarning("TouchCancel without registered device");
}

void QLibInputTouch::processTouchFrame(libinput_event_touch *e)
{
    DeviceState *state = deviceState(e);
    if (!state->m_touchDevice) {
        qWarning("TouchFrame without registered device");
        return;
    }
    qCDebug(qLcLibInputEvents) << "touch frame" << state->m_points;
    if (state->m_points.isEmpty())
        return;

    QWindowSystemInterface::handleTouchEvent(nullptr, state->m_touchDevice, state->m_points,
                                             QGuiApplication::keyboardModifiers());

    for (int i = 0; i < state->m_points.size(); ++i) {
        QWindowSystemInterface::TouchPoint &tp(state->m_points[i]);
        if (tp.state == QEventPoint::State::Released)
            state->m_points.removeAt(i--);
        else if (tp.state == QEventPoint::State::Pressed)
            tp.state = QEventPoint::State::Stationary;
    }
}

QT_END_NAMESPACE
