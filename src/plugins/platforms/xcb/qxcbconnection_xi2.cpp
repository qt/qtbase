/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbconnection.h"
#include "qxcbkeyboard.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qtouchdevice.h"
#include "QtCore/qmetaobject.h"
#include <qpa/qwindowsysteminterface_p.h>
#include <QDebug>
#include <cmath>

#include <xcb/xinput.h>

using qt_xcb_input_device_event_t = xcb_input_button_press_event_t;

struct qt_xcb_input_event_mask_t {
    xcb_input_event_mask_t header;
    uint32_t               mask;
};

void QXcbConnection::xi2SelectStateEvents()
{
    // These state events do not depend on a specific X window, but are global
    // for the X client's (application's) state.
    qt_xcb_input_event_mask_t xiEventMask;
    xiEventMask.header.deviceid = XCB_INPUT_DEVICE_ALL;
    xiEventMask.header.mask_len = 1;
    xiEventMask.mask = XCB_INPUT_XI_EVENT_MASK_HIERARCHY;
    xiEventMask.mask |= XCB_INPUT_XI_EVENT_MASK_DEVICE_CHANGED;
    xiEventMask.mask |= XCB_INPUT_XI_EVENT_MASK_PROPERTY;
    xcb_input_xi_select_events(xcb_connection(), rootWindow(), 1, &xiEventMask.header);
}

void QXcbConnection::xi2SelectDeviceEvents(xcb_window_t window)
{
    if (window == rootWindow())
        return;

    uint32_t bitMask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
    bitMask |= XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
    bitMask |= XCB_INPUT_XI_EVENT_MASK_MOTION;
    // There is a check for enter/leave events in plain xcb enter/leave event handler,
    // core enter/leave events will be ignored in this case.
    bitMask |= XCB_INPUT_XI_EVENT_MASK_ENTER;
    bitMask |= XCB_INPUT_XI_EVENT_MASK_LEAVE;
    if (isAtLeastXI22()) {
        bitMask |= XCB_INPUT_XI_EVENT_MASK_TOUCH_BEGIN;
        bitMask |= XCB_INPUT_XI_EVENT_MASK_TOUCH_UPDATE;
        bitMask |= XCB_INPUT_XI_EVENT_MASK_TOUCH_END;
    }

    qt_xcb_input_event_mask_t mask;
    mask.header.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
    mask.header.mask_len = 1;
    mask.mask = bitMask;
    xcb_void_cookie_t cookie =
            xcb_input_xi_select_events_checked(xcb_connection(), window, 1, &mask.header);
    xcb_generic_error_t *error = xcb_request_check(xcb_connection(), cookie);
    if (error) {
        qCDebug(lcQpaXInput, "failed to select events, window %x, error code %d", window, error->error_code);
        free(error);
    } else {
        QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
    }
}

static inline qreal fixed3232ToReal(xcb_input_fp3232_t val)
{
    return qreal(val.integral) + qreal(val.frac) / (1ULL << 32);
}

void QXcbConnection::xi2SetupDevice(void *info, bool removeExisting)
{
    auto *deviceInfo = reinterpret_cast<xcb_input_xi_device_info_t *>(info);
    if (removeExisting) {
#if QT_CONFIG(tabletevent)
        for (int i = 0; i < m_tabletData.count(); ++i) {
            if (m_tabletData.at(i).deviceId == deviceInfo->deviceid) {
                m_tabletData.remove(i);
                break;
            }
        }
#endif
        m_scrollingDevices.remove(deviceInfo->deviceid);
        m_touchDevices.remove(deviceInfo->deviceid);
    }

    qCDebug(lcQpaXInputDevices) << "input device " << xcb_input_xi_device_info_name(deviceInfo) << "ID" << deviceInfo->deviceid;
#if QT_CONFIG(tabletevent)
    TabletData tabletData;
#endif
    ScrollingDevice scrollingDevice;
    auto classes_it = xcb_input_xi_device_info_classes_iterator(deviceInfo);
    for (; classes_it.rem; xcb_input_device_class_next(&classes_it)) {
        xcb_input_device_class_t *classinfo = classes_it.data;
        switch (classinfo->type) {
        case XCB_INPUT_DEVICE_CLASS_TYPE_VALUATOR: {
            auto *vci = reinterpret_cast<xcb_input_valuator_class_t *>(classinfo);
            const int valuatorAtom = qatom(vci->label);
            qCDebug(lcQpaXInputDevices) << "   has valuator" << atomName(vci->label) << "recognized?" << (valuatorAtom < QXcbAtom::NAtoms);
#if QT_CONFIG(tabletevent)
            if (valuatorAtom < QXcbAtom::NAtoms) {
                TabletData::ValuatorClassInfo info;
                info.minVal = fixed3232ToReal(vci->min);
                info.maxVal = fixed3232ToReal(vci->max);
                info.number = vci->number;
                tabletData.valuatorInfo[valuatorAtom] = info;
            }
#endif // QT_CONFIG(tabletevent)
            if (valuatorAtom == QXcbAtom::RelHorizScroll || valuatorAtom == QXcbAtom::RelHorizWheel)
                scrollingDevice.lastScrollPosition.setX(fixed3232ToReal(vci->value));
            else if (valuatorAtom == QXcbAtom::RelVertScroll || valuatorAtom == QXcbAtom::RelVertWheel)
                scrollingDevice.lastScrollPosition.setY(fixed3232ToReal(vci->value));
            break;
        }
        case XCB_INPUT_DEVICE_CLASS_TYPE_SCROLL: {
            auto *sci = reinterpret_cast<xcb_input_scroll_class_t *>(classinfo);
            if (sci->scroll_type == XCB_INPUT_SCROLL_TYPE_VERTICAL) {
                scrollingDevice.orientations |= Qt::Vertical;
                scrollingDevice.verticalIndex = sci->number;
                scrollingDevice.verticalIncrement = fixed3232ToReal(sci->increment);
            } else if (sci->scroll_type == XCB_INPUT_SCROLL_TYPE_HORIZONTAL) {
                scrollingDevice.orientations |= Qt::Horizontal;
                scrollingDevice.horizontalIndex = sci->number;
                scrollingDevice.horizontalIncrement = fixed3232ToReal(sci->increment);
            }
            break;
        }
        case XCB_INPUT_DEVICE_CLASS_TYPE_BUTTON: {
            auto *bci = reinterpret_cast<xcb_input_button_class_t *>(classinfo);
            xcb_atom_t *labels = nullptr;
            if (bci->num_buttons >= 5) {
                labels = xcb_input_button_class_labels(bci);
                xcb_atom_t label4 = labels[3];
                xcb_atom_t label5 = labels[4];
                // Some drivers have no labels on the wheel buttons, some have no label on just one and some have no label on
                // button 4 and the wrong one on button 5. So we just check that they are not labelled with unrelated buttons.
                if ((!label4 || qatom(label4) == QXcbAtom::ButtonWheelUp || qatom(label4) == QXcbAtom::ButtonWheelDown) &&
                    (!label5 || qatom(label5) == QXcbAtom::ButtonWheelUp || qatom(label5) == QXcbAtom::ButtonWheelDown))
                    scrollingDevice.legacyOrientations |= Qt::Vertical;
            }
            if (bci->num_buttons >= 7) {
                xcb_atom_t label6 = labels[5];
                xcb_atom_t label7 = labels[6];
                if ((!label6 || qatom(label6) == QXcbAtom::ButtonHorizWheelLeft) && (!label7 || qatom(label7) == QXcbAtom::ButtonHorizWheelRight))
                    scrollingDevice.legacyOrientations |= Qt::Horizontal;
            }
            qCDebug(lcQpaXInputDevices, "   has %d buttons", bci->num_buttons);
            break;
        }
        case XCB_INPUT_DEVICE_CLASS_TYPE_KEY:
            qCDebug(lcQpaXInputDevices) << "   it's a keyboard";
            break;
        case XCB_INPUT_DEVICE_CLASS_TYPE_TOUCH:
            // will be handled in populateTouchDevices()
            break;
        default:
            qCDebug(lcQpaXInputDevices) << "   has class" << classinfo->type;
            break;
        }
    }
    bool isTablet = false;
#if QT_CONFIG(tabletevent)
    // If we have found the valuators which we expect a tablet to have, it might be a tablet.
    if (tabletData.valuatorInfo.contains(QXcbAtom::AbsX) &&
            tabletData.valuatorInfo.contains(QXcbAtom::AbsY) &&
            tabletData.valuatorInfo.contains(QXcbAtom::AbsPressure))
        isTablet = true;

    // But we need to be careful not to take the touch and tablet-button devices as tablets.
    QByteArray name = QByteArray(xcb_input_xi_device_info_name(deviceInfo),
                                 xcb_input_xi_device_info_name_length(deviceInfo)).toLower();
    QString dbgType = QLatin1String("UNKNOWN");
    if (name.contains("eraser")) {
        isTablet = true;
        tabletData.pointerType = QTabletEvent::Eraser;
        dbgType = QLatin1String("eraser");
    } else if (name.contains("cursor") && !(name.contains("cursor controls") && name.contains("trackball"))) {
        isTablet = true;
        tabletData.pointerType = QTabletEvent::Cursor;
        dbgType = QLatin1String("cursor");
    } else if (name.contains("wacom") && name.contains("finger touch")) {
        isTablet = false;
    } else if ((name.contains("pen") || name.contains("stylus")) && isTablet) {
        tabletData.pointerType = QTabletEvent::Pen;
        dbgType = QLatin1String("pen");
    } else if (name.contains("wacom") && isTablet && !name.contains("touch")) {
        // combined device (evdev) rather than separate pen/eraser (wacom driver)
        tabletData.pointerType = QTabletEvent::Pen;
        dbgType = QLatin1String("pen");
    } else if (name.contains("aiptek") /* && device == QXcbAtom::KEYBOARD */) {
        // some "Genius" tablets
        isTablet = true;
        tabletData.pointerType = QTabletEvent::Pen;
        dbgType = QLatin1String("pen");
    } else if (name.contains("waltop") && name.contains("tablet")) {
        // other "Genius" tablets
        // WALTOP International Corp. Slim Tablet
        isTablet = true;
        tabletData.pointerType = QTabletEvent::Pen;
        dbgType = QLatin1String("pen");
    } else if (name.contains("uc-logic") && isTablet) {
        tabletData.pointerType = QTabletEvent::Pen;
        dbgType = QLatin1String("pen");
    } else if (name.contains("ugee")) {
        isTablet = true;
        tabletData.pointerType = QTabletEvent::Pen;
        dbgType = QLatin1String("pen");
    } else {
        isTablet = false;
    }

    if (isTablet) {
        tabletData.deviceId = deviceInfo->deviceid;
        m_tabletData.append(tabletData);
        qCDebug(lcQpaXInputDevices) << "   it's a tablet with pointer type" << dbgType;
    }
#endif // QT_CONFIG(tabletevent)

    if (scrollingDevice.orientations || scrollingDevice.legacyOrientations) {
        scrollingDevice.deviceId = deviceInfo->deviceid;
        // Only use legacy wheel button events when we don't have real scroll valuators.
        scrollingDevice.legacyOrientations &= ~scrollingDevice.orientations;
        m_scrollingDevices.insert(scrollingDevice.deviceId, scrollingDevice);
        qCDebug(lcQpaXInputDevices) << "   it's a scrolling device";
    }

    if (!isTablet) {
        TouchDeviceData *dev = populateTouchDevices(deviceInfo);
        if (dev && lcQpaXInputDevices().isDebugEnabled()) {
            if (dev->qtTouchDevice->type() == QTouchDevice::TouchScreen)
                qCDebug(lcQpaXInputDevices, "   it's a touchscreen with type %d capabilities 0x%X max touch points %d",
                        dev->qtTouchDevice->type(), (unsigned int)dev->qtTouchDevice->capabilities(),
                        dev->qtTouchDevice->maximumTouchPoints());
            else if (dev->qtTouchDevice->type() == QTouchDevice::TouchPad)
                qCDebug(lcQpaXInputDevices, "   it's a touchpad with type %d capabilities 0x%X max touch points %d size %f x %f",
                        dev->qtTouchDevice->type(), (unsigned int)dev->qtTouchDevice->capabilities(),
                        dev->qtTouchDevice->maximumTouchPoints(),
                        dev->size.width(), dev->size.height());
        }
    }

}

void QXcbConnection::xi2SetupDevices()
{
#if QT_CONFIG(tabletevent)
    m_tabletData.clear();
#endif
    m_scrollingDevices.clear();
    m_touchDevices.clear();
    m_xiMasterPointerIds.clear();

    auto reply = Q_XCB_REPLY(xcb_input_xi_query_device, xcb_connection(), XCB_INPUT_DEVICE_ALL);
    if (!reply) {
        qCDebug(lcQpaXInputDevices) << "failed to query devices";
        return;
    }

    auto it = xcb_input_xi_query_device_infos_iterator(reply.get());
    for (; it.rem; xcb_input_xi_device_info_next(&it)) {
        xcb_input_xi_device_info_t *deviceInfo = it.data;
        if (deviceInfo->type == XCB_INPUT_DEVICE_TYPE_MASTER_POINTER) {
            m_xiMasterPointerIds.append(deviceInfo->deviceid);
            continue;
        }
        // only slave pointer devices are relevant here
        if (deviceInfo->type == XCB_INPUT_DEVICE_TYPE_SLAVE_POINTER)
            xi2SetupDevice(deviceInfo, false);
    }

    if (m_xiMasterPointerIds.size() > 1)
        qCDebug(lcQpaXInputDevices) << "multi-pointer X detected";
}

/*! \internal

    Notes on QT_XCB_NO_XI2_MOUSE Handling:

    Here we don't select pointer button press/release and motion events on master devices, instead
    we select these events directly on slave devices. This means that a master device will fallback
    to sending core events for every XI_* event that is sent directly by a slave device. For more
    details see "Event processing for attached slave devices" in XInput2 specification. To prevent
    handling of the same event twice, we have checks for xi2MouseEventsDisabled() in XI2 event
    handlers (but this is somewhat inconsistent in some situations). If the purpose for
    QT_XCB_NO_XI2_MOUSE was so that an application using QAbstractNativeEventFilter would see core
    mouse events before they are handled by Qt then QT_XCB_NO_XI2_MOUSE won't always work as
    expected (e.g. we handle scroll event directly from a slave device event, before an application
    has seen the fallback core event from a master device).

    The commit introducing QT_XCB_NO_XI2_MOUSE also states that setting this envvar "restores the
    old behavior with broken grabbing". It did not elaborate why grabbing was not fixed for this
    code path. The issue that this envvar tries to solve seem to be less important than broken
    grabbing (broken apparently only for touch events). Thus, if you really want core mouse events
    in your application and do not care about broken touch, then use QT_XCB_NO_XI2 (more on this
    below) to disable the extension all together. The reason why grabbing might have not been fixed
    is that calling XIGrabDevice with this code path for some reason always returns AlreadyGrabbed
    (by debugging X server's code it appears that when we call XIGrabDevice, an X server first grabs
    pointer via core pointer and then fails to do XI2 grab with AlreadyGrabbed; disclaimer - I did
    not debug this in great detail). When we try supporting odd setups like QT_XCB_NO_XI2_MOUSE, we
    are asking for trouble anyways.

    In conclusion, introduction of QT_XCB_NO_XI2_MOUSE causes more issues than solves - the above
    mentioned inconsistencies, maintenance of this code path and that QT_XCB_NO_XI2_MOUSE replaces
    less important issue with somewhat more important issue. It also makes us to use less optimal
    code paths in certain situations (see xi2HandleHierarchyEvent). Using of QT_XCB_NO_XI2 has its
    drawbacks too - no tablet and touch events. So the only real fix in this case is at an
    application side (teach the application about xcb_ge_event_t events). Based on this,
    QT_XCB_NO_XI2_MOUSE will be removed in ### Qt 6. It should not have existed in the first place,
    native events seen by QAbstractNativeEventFilter is not really a public API, applications should
    expect changes at this level and do ifdefs if something changes between Qt version.
*/
void QXcbConnection::xi2SelectDeviceEventsCompatibility(xcb_window_t window)
{
    if (window == rootWindow())
        return;

    uint32_t mask = 0;

    if (isAtLeastXI22()) {
        mask |= XCB_INPUT_XI_EVENT_MASK_TOUCH_BEGIN;
        mask |= XCB_INPUT_XI_EVENT_MASK_TOUCH_UPDATE;
        mask |= XCB_INPUT_XI_EVENT_MASK_TOUCH_END;

        qt_xcb_input_event_mask_t xiMask;
        xiMask.header.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
        xiMask.header.mask_len = 1;
        xiMask.mask = mask;

        xcb_void_cookie_t cookie =
                xcb_input_xi_select_events_checked(xcb_connection(), window, 1, &xiMask.header);
        xcb_generic_error_t *error = xcb_request_check(xcb_connection(), cookie);
        if (error) {
            qCDebug(lcQpaXInput, "failed to select events, window %x, error code %d", window, error->error_code);
            free(error);
        } else {
            QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
        }
    }

    mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
    mask |= XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
    mask |= XCB_INPUT_XI_EVENT_MASK_MOTION;

#if QT_CONFIG(tabletevent)
    QSet<int> tabletDevices;
    if (!m_tabletData.isEmpty()) {
        const int nrTablets = m_tabletData.count();
        QVector<qt_xcb_input_event_mask_t> xiEventMask(nrTablets);
        for (int i = 0; i < nrTablets; ++i) {
            int deviceId = m_tabletData.at(i).deviceId;
            tabletDevices.insert(deviceId);
            xiEventMask[i].header.deviceid = deviceId;
            xiEventMask[i].header.mask_len = 1;
            xiEventMask[i].mask = mask;
        }
        xcb_input_xi_select_events(xcb_connection(), window, nrTablets, &(xiEventMask.data()->header));
    }
#endif

    if (!m_scrollingDevices.isEmpty()) {
        QVector<qt_xcb_input_event_mask_t> xiEventMask(m_scrollingDevices.size());
        int i = 0;
        for (const ScrollingDevice& scrollingDevice : qAsConst(m_scrollingDevices)) {
#if QT_CONFIG(tabletevent)
            if (tabletDevices.contains(scrollingDevice.deviceId))
                continue; // All necessary events are already captured.
#endif
            xiEventMask[i].header.deviceid = scrollingDevice.deviceId;
            xiEventMask[i].header.mask_len = 1;
            xiEventMask[i].mask = mask;
            i++;
        }
        xcb_input_xi_select_events(xcb_connection(), window, i, &(xiEventMask.data()->header));
    }
}

QXcbConnection::TouchDeviceData *QXcbConnection::touchDeviceForId(int id)
{
    TouchDeviceData *dev = nullptr;
    if (m_touchDevices.contains(id))
        dev = &m_touchDevices[id];
    return dev;
}

QXcbConnection::TouchDeviceData *QXcbConnection::populateTouchDevices(void *info)
{
    auto *deviceinfo = reinterpret_cast<xcb_input_xi_device_info_t *>(info);
    QTouchDevice::Capabilities caps;
    int type = -1;
    int maxTouchPoints = 1;
    bool isTouchDevice = false;
    bool hasRelativeCoords = false;
    TouchDeviceData dev;
    auto classes_it = xcb_input_xi_device_info_classes_iterator(deviceinfo);
    for (; classes_it.rem; xcb_input_device_class_next(&classes_it)) {
        xcb_input_device_class_t *classinfo = classes_it.data;
        switch (classinfo->type) {
        case XCB_INPUT_DEVICE_CLASS_TYPE_TOUCH: {
            auto *tci = reinterpret_cast<xcb_input_touch_class_t *>(classinfo);
            maxTouchPoints = tci->num_touches;
            qCDebug(lcQpaXInputDevices, "   has touch class with mode %d", tci->mode);
            switch (tci->mode) {
            case XCB_INPUT_TOUCH_MODE_DEPENDENT:
                type = QTouchDevice::TouchPad;
                break;
            case XCB_INPUT_TOUCH_MODE_DIRECT:
                type = QTouchDevice::TouchScreen;
                break;
            }
            break;
        }
        case XCB_INPUT_DEVICE_CLASS_TYPE_VALUATOR: {
            auto *vci = reinterpret_cast<xcb_input_valuator_class_t *>(classinfo);
            const QXcbAtom::Atom valuatorAtom = qatom(vci->label);
            if (valuatorAtom < QXcbAtom::NAtoms) {
                TouchDeviceData::ValuatorClassInfo info;
                info.min = fixed3232ToReal(vci->min);
                info.max = fixed3232ToReal(vci->max);
                info.number = vci->number;
                info.label = valuatorAtom;
                dev.valuatorInfo.append(info);
            }
            // Some devices (mice) report a resolution of 0; they will be excluded later,
            // for now just prevent a division by zero
            const int vciResolution = vci->resolution ? vci->resolution : 1;
            if (valuatorAtom == QXcbAtom::AbsMTPositionX)
                caps |= QTouchDevice::Position | QTouchDevice::NormalizedPosition;
            else if (valuatorAtom == QXcbAtom::AbsMTTouchMajor)
                caps |= QTouchDevice::Area;
            else if (valuatorAtom == QXcbAtom::AbsMTOrientation)
                dev.providesTouchOrientation = true;
            else if (valuatorAtom == QXcbAtom::AbsMTPressure || valuatorAtom == QXcbAtom::AbsPressure)
                caps |= QTouchDevice::Pressure;
            else if (valuatorAtom == QXcbAtom::RelX) {
                hasRelativeCoords = true;
                dev.size.setWidth((fixed3232ToReal(vci->max) - fixed3232ToReal(vci->min)) * 1000.0 / vciResolution);
            } else if (valuatorAtom == QXcbAtom::RelY) {
                hasRelativeCoords = true;
                dev.size.setHeight((fixed3232ToReal(vci->max) - fixed3232ToReal(vci->min)) * 1000.0 / vciResolution);
            } else if (valuatorAtom == QXcbAtom::AbsX) {
                caps |= QTouchDevice::Position;
                dev.size.setWidth((fixed3232ToReal(vci->max) - fixed3232ToReal(vci->min)) * 1000.0 / vciResolution);
            } else if (valuatorAtom == QXcbAtom::AbsY) {
                caps |= QTouchDevice::Position;
                dev.size.setHeight((fixed3232ToReal(vci->max) - fixed3232ToReal(vci->min)) * 1000.0 / vciResolution);
            }
            break;
        }
        default:
            break;
        }
    }
    if (type < 0 && caps && hasRelativeCoords) {
        type = QTouchDevice::TouchPad;
        if (dev.size.width() < 10 || dev.size.height() < 10 ||
                dev.size.width() > 10000 || dev.size.height() > 10000)
            dev.size = QSizeF(130, 110);
    }
    if (!isAtLeastXI22() || type == QTouchDevice::TouchPad)
        caps |= QTouchDevice::MouseEmulation;

    if (type >= QTouchDevice::TouchScreen && type <= QTouchDevice::TouchPad) {
        dev.qtTouchDevice = new QTouchDevice;
        dev.qtTouchDevice->setName(QString::fromUtf8(xcb_input_xi_device_info_name(deviceinfo),
                                                     xcb_input_xi_device_info_name_length(deviceinfo)));
        dev.qtTouchDevice->setType((QTouchDevice::DeviceType)type);
        dev.qtTouchDevice->setCapabilities(caps);
        dev.qtTouchDevice->setMaximumTouchPoints(maxTouchPoints);
        if (caps != 0)
            QWindowSystemInterface::registerTouchDevice(dev.qtTouchDevice);
        m_touchDevices[deviceinfo->deviceid] = dev;
        isTouchDevice = true;
    }

    return isTouchDevice ? &m_touchDevices[deviceinfo->deviceid] : nullptr;
}

static inline qreal fixed1616ToReal(xcb_input_fp1616_t val)
{
    return qreal(val) / 0x10000;
}

void QXcbConnection::xi2HandleEvent(xcb_ge_event_t *event)
{
    auto *xiEvent = reinterpret_cast<qt_xcb_input_device_event_t *>(event);
    int sourceDeviceId = xiEvent->deviceid; // may be the master id
    qt_xcb_input_device_event_t *xiDeviceEvent = nullptr;
    xcb_input_enter_event_t *xiEnterEvent = nullptr;
    QXcbWindowEventListener *eventListener = nullptr;

    switch (xiEvent->event_type) {
    case XCB_INPUT_BUTTON_PRESS:
    case XCB_INPUT_BUTTON_RELEASE:
    case XCB_INPUT_MOTION:
    case XCB_INPUT_TOUCH_BEGIN:
    case XCB_INPUT_TOUCH_UPDATE:
    case XCB_INPUT_TOUCH_END:
    {
        xiDeviceEvent = xiEvent;
        eventListener = windowEventListenerFromId(xiDeviceEvent->event);
        sourceDeviceId = xiDeviceEvent->sourceid; // use the actual device id instead of the master
        break;
    }
    case XCB_INPUT_ENTER:
    case XCB_INPUT_LEAVE: {
        xiEnterEvent = reinterpret_cast<xcb_input_enter_event_t *>(event);
        eventListener = windowEventListenerFromId(xiEnterEvent->event);
        sourceDeviceId = xiEnterEvent->sourceid; // use the actual device id instead of the master
        break;
    }
    case XCB_INPUT_HIERARCHY:
        xi2HandleHierarchyEvent(event);
        return;
    case XCB_INPUT_DEVICE_CHANGED:
        xi2HandleDeviceChangedEvent(event);
        return;
    default:
        break;
    }

    if (eventListener) {
        if (eventListener->handleNativeEvent(reinterpret_cast<xcb_generic_event_t *>(event)))
            return;
    }

#if QT_CONFIG(tabletevent)
    if (!xiEnterEvent) {
        QXcbConnection::TabletData *tablet = tabletDataForDevice(sourceDeviceId);
        if (tablet && xi2HandleTabletEvent(event, tablet))
            return;
    }
#endif // QT_CONFIG(tabletevent)

    if (ScrollingDevice *device = scrollingDeviceForId(sourceDeviceId))
        xi2HandleScrollEvent(event, *device);

    if (xiDeviceEvent) {
        switch (xiDeviceEvent->event_type) {
        case XCB_INPUT_BUTTON_PRESS:
        case XCB_INPUT_BUTTON_RELEASE:
        case XCB_INPUT_MOTION:
            if (!xi2MouseEventsDisabled() && eventListener &&
                    !(xiDeviceEvent->flags & XCB_INPUT_POINTER_EVENT_FLAGS_POINTER_EMULATED))
                eventListener->handleXIMouseEvent(event);
            break;

        case XCB_INPUT_TOUCH_BEGIN:
        case XCB_INPUT_TOUCH_UPDATE:
        case XCB_INPUT_TOUCH_END:
            if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
                qCDebug(lcQpaXInputEvents, "XI2 touch event type %d seq %d detail %d pos %6.1f, %6.1f root pos %6.1f, %6.1f on window %x",
                        event->event_type, xiDeviceEvent->sequence, xiDeviceEvent->detail,
                        fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y),
                        fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y),xiDeviceEvent->event);
            if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event))
                xi2ProcessTouch(xiDeviceEvent, platformWindow);
            break;
        }
    } else if (xiEnterEvent && !xi2MouseEventsDisabled() && eventListener) {
        switch (xiEnterEvent->event_type) {
        case XCB_INPUT_ENTER:
        case XCB_INPUT_LEAVE:
            eventListener->handleXIEnterLeave(event);
            break;
        }
    }
}

bool QXcbConnection::xi2MouseEventsDisabled() const
{
    static bool xi2MouseDisabled = qEnvironmentVariableIsSet("QT_XCB_NO_XI2_MOUSE");
    // FIXME: Don't use XInput2 mouse events when Xinerama extension
    // is enabled, because it causes problems with multi-monitor setup.
    return xi2MouseDisabled || hasXinerama();
}

bool QXcbConnection::isTouchScreen(int id)
{
    auto device = touchDeviceForId(id);
    return device && device->qtTouchDevice->type() == QTouchDevice::TouchScreen;
}

void QXcbConnection::xi2ProcessTouch(void *xiDevEvent, QXcbWindow *platformWindow)
{
    auto *xiDeviceEvent = reinterpret_cast<xcb_input_touch_begin_event_t *>(xiDevEvent);
    TouchDeviceData *dev = touchDeviceForId(xiDeviceEvent->sourceid);
    Q_ASSERT(dev);
    const bool firstTouch = dev->touchPoints.isEmpty();
    if (xiDeviceEvent->event_type == XCB_INPUT_TOUCH_BEGIN) {
        QWindowSystemInterface::TouchPoint tp;
        tp.id = xiDeviceEvent->detail % INT_MAX;
        tp.state = Qt::TouchPointPressed;
        tp.pressure = -1.0;
        dev->touchPoints[tp.id] = tp;
    }
    QWindowSystemInterface::TouchPoint &touchPoint = dev->touchPoints[xiDeviceEvent->detail];
    QXcbScreen* screen = platformWindow->xcbScreen();
    qreal x = fixed1616ToReal(xiDeviceEvent->root_x);
    qreal y = fixed1616ToReal(xiDeviceEvent->root_y);
    qreal nx = -1.0, ny = -1.0;
    qreal w = 0.0, h = 0.0;
    bool majorAxisIsY = touchPoint.area.height() > touchPoint.area.width();
    for (const TouchDeviceData::ValuatorClassInfo &vci : qAsConst(dev->valuatorInfo)) {
        double value;
        if (!xi2GetValuatorValueIfSet(xiDeviceEvent, vci.number, &value))
            continue;
        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
            qCDebug(lcQpaXInputEvents, "   valuator %20s value %lf from range %lf -> %lf",
                    atomName(vci.label).constData(), value, vci.min, vci.max);
        if (value > vci.max)
            value = vci.max;
        if (value < vci.min)
            value = vci.min;
        qreal valuatorNormalized = (value - vci.min) / (vci.max - vci.min);
        if (vci.label == QXcbAtom::RelX) {
            nx = valuatorNormalized;
        } else if (vci.label == QXcbAtom::RelY) {
            ny = valuatorNormalized;
        } else if (vci.label == QXcbAtom::AbsX) {
            nx = valuatorNormalized;
        } else if (vci.label == QXcbAtom::AbsY) {
            ny = valuatorNormalized;
        } else if (vci.label == QXcbAtom::AbsMTPositionX) {
            nx = valuatorNormalized;
        } else if (vci.label == QXcbAtom::AbsMTPositionY) {
            ny = valuatorNormalized;
        } else if (vci.label == QXcbAtom::AbsMTTouchMajor) {
            const qreal sw = screen->geometry().width();
            const qreal sh = screen->geometry().height();
            w = valuatorNormalized * std::sqrt(sw * sw + sh * sh);
        } else if (vci.label == QXcbAtom::AbsMTTouchMinor) {
            const qreal sw = screen->geometry().width();
            const qreal sh = screen->geometry().height();
            h = valuatorNormalized * std::sqrt(sw * sw + sh * sh);
        } else if (vci.label == QXcbAtom::AbsMTOrientation) {
            // Find the closest axis.
            // 0 corresponds to the Y axis, vci.max to the X axis.
            // Flipping over the Y axis and rotating by 180 degrees
            // don't change the result, so normalize value to range
            // [0, vci.max] first.
            value = qAbs(value);
            while (value > vci.max)
                value -= 2 * vci.max;
            value = qAbs(value);
            majorAxisIsY = value < vci.max - value;
        } else if (vci.label == QXcbAtom::AbsMTPressure || vci.label == QXcbAtom::AbsPressure) {
            touchPoint.pressure = valuatorNormalized;
        }

    }
    // If any value was not updated, use the last-known value.
    if (nx == -1.0) {
        x = touchPoint.area.center().x();
        nx = x / screen->geometry().width();
    }
    if (ny == -1.0) {
        y = touchPoint.area.center().y();
        ny = y / screen->geometry().height();
    }
    if (xiDeviceEvent->event_type != XCB_INPUT_TOUCH_END) {
        if (!dev->providesTouchOrientation) {
            if (w == 0.0)
                w = touchPoint.area.width();
            h = w;
        } else {
            if (w == 0.0)
                w = qMax(touchPoint.area.width(), touchPoint.area.height());
            if (h == 0.0)
                h = qMin(touchPoint.area.width(), touchPoint.area.height());
            if (majorAxisIsY)
                qSwap(w, h);
        }
    }

    switch (xiDeviceEvent->event_type) {
    case XCB_INPUT_TOUCH_BEGIN:
        if (firstTouch) {
            dev->firstPressedPosition = QPointF(x, y);
            dev->firstPressedNormalPosition = QPointF(nx, ny);
        }
        dev->pointPressedPosition.insert(touchPoint.id, QPointF(x, y));

        // Touches must be accepted when we are grabbing touch events. Otherwise the entire sequence
        // will get replayed when the grab ends.
        if (m_xiGrab) {
            xcb_input_xi_allow_events(xcb_connection(), XCB_CURRENT_TIME, xiDeviceEvent->deviceid,
                                      XCB_INPUT_EVENT_MODE_ACCEPT_TOUCH,
                                      xiDeviceEvent->detail, xiDeviceEvent->event);
        }
        break;
    case XCB_INPUT_TOUCH_UPDATE:
        if (dev->qtTouchDevice->type() == QTouchDevice::TouchPad && dev->pointPressedPosition.value(touchPoint.id) == QPointF(x, y)) {
            qreal dx = (nx - dev->firstPressedNormalPosition.x()) *
                dev->size.width() * screen->geometry().width() / screen->physicalSize().width();
            qreal dy = (ny - dev->firstPressedNormalPosition.y()) *
                dev->size.height() * screen->geometry().height() / screen->physicalSize().height();
            x = dev->firstPressedPosition.x() + dx;
            y = dev->firstPressedPosition.y() + dy;
            touchPoint.state = Qt::TouchPointMoved;
        } else if (touchPoint.area.center() != QPoint(x, y)) {
            touchPoint.state = Qt::TouchPointMoved;
            if (dev->qtTouchDevice->type() == QTouchDevice::TouchPad)
                dev->pointPressedPosition[touchPoint.id] = QPointF(x, y);
        }

        if (dev->qtTouchDevice->type() == QTouchDevice::TouchScreen &&
            xiDeviceEvent->event == m_startSystemMoveResizeInfo.window &&
            xiDeviceEvent->sourceid == m_startSystemMoveResizeInfo.deviceid &&
            xiDeviceEvent->detail == m_startSystemMoveResizeInfo.pointid) {
            QXcbWindow *window = platformWindowFromId(m_startSystemMoveResizeInfo.window);
            if (window) {
                xcb_input_xi_allow_events(xcb_connection(), XCB_CURRENT_TIME, xiDeviceEvent->deviceid,
                                          XCB_INPUT_EVENT_MODE_REJECT_TOUCH,
                                          xiDeviceEvent->detail, xiDeviceEvent->event);
                window->doStartSystemMoveResize(QPoint(x, y), m_startSystemMoveResizeInfo.edges);
                m_startSystemMoveResizeInfo.window = XCB_NONE;
            }
        }
        break;
    case XCB_INPUT_TOUCH_END:
        touchPoint.state = Qt::TouchPointReleased;
        if (dev->qtTouchDevice->type() == QTouchDevice::TouchPad && dev->pointPressedPosition.value(touchPoint.id) == QPointF(x, y)) {
            qreal dx = (nx - dev->firstPressedNormalPosition.x()) *
                dev->size.width() * screen->geometry().width() / screen->physicalSize().width();
            qreal dy = (ny - dev->firstPressedNormalPosition.y()) *
                dev->size.width() * screen->geometry().width() / screen->physicalSize().width();
            x = dev->firstPressedPosition.x() + dx;
            y = dev->firstPressedPosition.y() + dy;
        }
        dev->pointPressedPosition.remove(touchPoint.id);
    }
    touchPoint.area = QRectF(x - w/2, y - h/2, w, h);
    touchPoint.normalPosition = QPointF(nx, ny);

    if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
        qCDebug(lcQpaXInputEvents) << "   touchpoint "  << touchPoint.id << " state " << touchPoint.state << " pos norm " << touchPoint.normalPosition <<
            " area " << touchPoint.area << " pressure " << touchPoint.pressure;
    Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective);
    QWindowSystemInterface::handleTouchEvent(platformWindow->window(), xiDeviceEvent->time, dev->qtTouchDevice, dev->touchPoints.values(), modifiers);
    if (touchPoint.state == Qt::TouchPointReleased)
        // If a touchpoint was released, we can forget it, because the ID won't be reused.
        dev->touchPoints.remove(touchPoint.id);
    else
        // Make sure that we don't send TouchPointPressed/Moved in more than one QTouchEvent
        // with this touch point if the next XI2 event is about a different touch point.
        touchPoint.state = Qt::TouchPointStationary;
}

bool QXcbConnection::startSystemMoveResizeForTouch(xcb_window_t window, int edges)
{
    QHash<int, TouchDeviceData>::const_iterator devIt = m_touchDevices.constBegin();
    for (; devIt != m_touchDevices.constEnd(); ++devIt) {
        TouchDeviceData deviceData = devIt.value();
        if (deviceData.qtTouchDevice->type() == QTouchDevice::TouchScreen) {
            auto pointIt = deviceData.touchPoints.constBegin();
            for (; pointIt != deviceData.touchPoints.constEnd(); ++pointIt) {
                Qt::TouchPointState state = pointIt.value().state;
                if (state == Qt::TouchPointMoved || state == Qt::TouchPointPressed || state == Qt::TouchPointStationary) {
                    m_startSystemMoveResizeInfo.window = window;
                    m_startSystemMoveResizeInfo.deviceid = devIt.key();
                    m_startSystemMoveResizeInfo.pointid = pointIt.key();
                    m_startSystemMoveResizeInfo.edges = edges;
                    return true;
                }
            }
        }
    }
    return false;
}

void QXcbConnection::abortSystemMoveResizeForTouch()
{
    m_startSystemMoveResizeInfo.window = XCB_NONE;
}

bool QXcbConnection::xi2SetMouseGrabEnabled(xcb_window_t w, bool grab)
{
    bool ok = false;

    if (grab) { // grab
        uint32_t mask = XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS
                | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE
                | XCB_INPUT_XI_EVENT_MASK_MOTION
                | XCB_INPUT_XI_EVENT_MASK_ENTER
                | XCB_INPUT_XI_EVENT_MASK_LEAVE
                | XCB_INPUT_XI_EVENT_MASK_TOUCH_BEGIN
                | XCB_INPUT_XI_EVENT_MASK_TOUCH_UPDATE
                | XCB_INPUT_XI_EVENT_MASK_TOUCH_END;

        for (int id : qAsConst(m_xiMasterPointerIds)) {
            xcb_generic_error_t *error = nullptr;
            auto cookie = xcb_input_xi_grab_device(xcb_connection(), w, XCB_CURRENT_TIME, XCB_CURSOR_NONE, id,
                                                   XCB_INPUT_GRAB_MODE_22_ASYNC, XCB_INPUT_GRAB_MODE_22_ASYNC,
                                                   false, 1, &mask);
            auto *reply = xcb_input_xi_grab_device_reply(xcb_connection(), cookie, &error);
            if (error) {
                qCDebug(lcQpaXInput, "failed to grab events for device %d on window %x"
                                     "(error code %d)", id, w, error->error_code);
                free(error);
            } else {
                // Managed to grab at least one of master pointers, that should be enough
                // to properly dismiss windows that rely on mouse grabbing.
                ok = true;
            }
            free(reply);
        }
    } else { // ungrab
        for (int id : qAsConst(m_xiMasterPointerIds)) {
            auto cookie = xcb_input_xi_ungrab_device_checked(xcb_connection(), XCB_CURRENT_TIME, id);
            xcb_generic_error_t *error = xcb_request_check(xcb_connection(), cookie);
            if (error) {
                qCDebug(lcQpaXInput, "XIUngrabDevice failed - id: %d (error code %d)", id, error->error_code);
                free(error);
            }
        }
        // XIUngrabDevice does not seem to wait for a reply from X server (similar to
        // xcb_ungrab_pointer). Ungrabbing won't fail, unless NoSuchExtension error
        // has occurred due to a programming error somewhere else in the stack. That
        // would mean that things will crash soon anyway.
        ok = true;
    }

    if (ok)
        m_xiGrab = grab;

    return ok;
}

void QXcbConnection::xi2HandleHierarchyEvent(void *event)
{
    auto *xiEvent = reinterpret_cast<xcb_input_hierarchy_event_t *>(event);
    // We only care about hotplugged devices
    if (!(xiEvent->flags & (XCB_INPUT_HIERARCHY_MASK_SLAVE_REMOVED | XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED)))
        return;

    xi2SetupDevices();

    if (xi2MouseEventsDisabled()) {
        // In compatibility mode (a.k.a xi2MouseEventsDisabled() mode) we select events for
        // each device separately. When a new device appears, we have to select events from
        // this device on all event-listening windows. This is not needed when events are
        // selected via XIAllDevices/XIAllMasterDevices (as in xi2SelectDeviceEvents()).
        for (auto it = m_mapper.cbegin(), end = m_mapper.cend(); it != end; ++it)
            xi2SelectDeviceEventsCompatibility(it.key());
    }
}

void QXcbConnection::xi2HandleDeviceChangedEvent(void *event)
{
    auto *xiEvent = reinterpret_cast<xcb_input_device_changed_event_t *>(event);
    switch (xiEvent->reason) {
    case XCB_INPUT_CHANGE_REASON_DEVICE_CHANGE: {
        auto reply = Q_XCB_REPLY(xcb_input_xi_query_device, xcb_connection(), xiEvent->sourceid);
        if (!reply || reply->num_infos <= 0)
            return;
        auto it = xcb_input_xi_query_device_infos_iterator(reply.get());
        xi2SetupDevice(it.data);
        break;
    }
    case XCB_INPUT_CHANGE_REASON_SLAVE_SWITCH: {
        if (ScrollingDevice *scrollingDevice = scrollingDeviceForId(xiEvent->sourceid))
            xi2UpdateScrollingDevice(*scrollingDevice);
        break;
    }
    default:
        qCDebug(lcQpaXInputEvents, "unknown device-changed-event (device %d)", xiEvent->sourceid);
        break;
    }
}

void QXcbConnection::xi2UpdateScrollingDevice(ScrollingDevice &scrollingDevice)
{
    auto reply = Q_XCB_REPLY(xcb_input_xi_query_device, xcb_connection(), scrollingDevice.deviceId);
    if (!reply || reply->num_infos <= 0) {
        qCDebug(lcQpaXInputDevices, "scrolling device %d no longer present", scrollingDevice.deviceId);
        return;
    }
    QPointF lastScrollPosition;
    if (lcQpaXInputEvents().isDebugEnabled())
        lastScrollPosition = scrollingDevice.lastScrollPosition;

    xcb_input_xi_device_info_t *deviceInfo = xcb_input_xi_query_device_infos_iterator(reply.get()).data;
    auto classes_it = xcb_input_xi_device_info_classes_iterator(deviceInfo);
    for (; classes_it.rem; xcb_input_device_class_next(&classes_it)) {
        xcb_input_device_class_t *classInfo = classes_it.data;
        if (classInfo->type == XCB_INPUT_DEVICE_CLASS_TYPE_VALUATOR) {
            auto *vci = reinterpret_cast<xcb_input_valuator_class_t *>(classInfo);
            const int valuatorAtom = qatom(vci->label);
            if (valuatorAtom == QXcbAtom::RelHorizScroll || valuatorAtom == QXcbAtom::RelHorizWheel)
                scrollingDevice.lastScrollPosition.setX(fixed3232ToReal(vci->value));
            else if (valuatorAtom == QXcbAtom::RelVertScroll || valuatorAtom == QXcbAtom::RelVertWheel)
                scrollingDevice.lastScrollPosition.setY(fixed3232ToReal(vci->value));
        }
    }
    if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled() && lastScrollPosition != scrollingDevice.lastScrollPosition))
        qCDebug(lcQpaXInputEvents, "scrolling device %d moved from (%f, %f) to (%f, %f)", scrollingDevice.deviceId,
                lastScrollPosition.x(), lastScrollPosition.y(),
                scrollingDevice.lastScrollPosition.x(),
                scrollingDevice.lastScrollPosition.y());
}

void QXcbConnection::xi2UpdateScrollingDevices()
{
    QHash<int, ScrollingDevice>::iterator it = m_scrollingDevices.begin();
    const QHash<int, ScrollingDevice>::iterator end = m_scrollingDevices.end();
    while (it != end) {
        xi2UpdateScrollingDevice(it.value());
        ++it;
    }
}

QXcbConnection::ScrollingDevice *QXcbConnection::scrollingDeviceForId(int id)
{
    ScrollingDevice *dev = nullptr;
    if (m_scrollingDevices.contains(id))
        dev = &m_scrollingDevices[id];
    return dev;
}

void QXcbConnection::xi2HandleScrollEvent(void *event, ScrollingDevice &scrollingDevice)
{
    auto *xiDeviceEvent = reinterpret_cast<qt_xcb_input_device_event_t *>(event);

    if (xiDeviceEvent->event_type == XCB_INPUT_MOTION && scrollingDevice.orientations) {
        if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event)) {
            QPoint rawDelta;
            QPoint angleDelta;
            double value;
            if (scrollingDevice.orientations & Qt::Vertical) {
                if (xi2GetValuatorValueIfSet(xiDeviceEvent, scrollingDevice.verticalIndex, &value)) {
                    double delta = scrollingDevice.lastScrollPosition.y() - value;
                    scrollingDevice.lastScrollPosition.setY(value);
                    angleDelta.setY((delta / scrollingDevice.verticalIncrement) * 120);
                    // With most drivers the increment is 1 for wheels.
                    // For libinput it is hardcoded to a useless 15.
                    // For a proper touchpad driver it should be in the same order of magnitude as 120
                    if (scrollingDevice.verticalIncrement > 15)
                        rawDelta.setY(delta);
                    else if (scrollingDevice.verticalIncrement < -15)
                        rawDelta.setY(-delta);
                }
            }
            if (scrollingDevice.orientations & Qt::Horizontal) {
                if (xi2GetValuatorValueIfSet(xiDeviceEvent, scrollingDevice.horizontalIndex, &value)) {
                    double delta = scrollingDevice.lastScrollPosition.x() - value;
                    scrollingDevice.lastScrollPosition.setX(value);
                    angleDelta.setX((delta / scrollingDevice.horizontalIncrement) * 120);
                    // See comment under vertical
                    if (scrollingDevice.horizontalIncrement > 15)
                        rawDelta.setX(delta);
                    else if (scrollingDevice.horizontalIncrement < -15)
                        rawDelta.setX(-delta);
                }
            }
            if (!angleDelta.isNull()) {
                QPoint local(fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y));
                QPoint global(fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y));
                Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective);
                if (modifiers & Qt::AltModifier) {
                    angleDelta = angleDelta.transposed();
                    rawDelta = rawDelta.transposed();
                }
                qCDebug(lcQpaXInputEvents) << "scroll wheel @ window pos" << local << "delta px" << rawDelta << "angle" << angleDelta;
                QWindowSystemInterface::handleWheelEvent(platformWindow->window(), xiDeviceEvent->time, local, global, rawDelta, angleDelta, modifiers);
            }
        }
    } else if (xiDeviceEvent->event_type == XCB_INPUT_BUTTON_RELEASE && scrollingDevice.legacyOrientations) {
        if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event)) {
            QPoint angleDelta;
            if (scrollingDevice.legacyOrientations & Qt::Vertical) {
                if (xiDeviceEvent->detail == 4)
                    angleDelta.setY(120);
                else if (xiDeviceEvent->detail == 5)
                    angleDelta.setY(-120);
            }
            if (scrollingDevice.legacyOrientations & Qt::Horizontal) {
                if (xiDeviceEvent->detail == 6)
                    angleDelta.setX(120);
                else if (xiDeviceEvent->detail == 7)
                    angleDelta.setX(-120);
            }
            if (!angleDelta.isNull()) {
                QPoint local(fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y));
                QPoint global(fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y));
                Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective);
                if (modifiers & Qt::AltModifier)
                    angleDelta = angleDelta.transposed();
                qCDebug(lcQpaXInputEvents) << "scroll wheel (button" << xiDeviceEvent->detail << ") @ window pos" << local << "delta angle" << angleDelta;
                QWindowSystemInterface::handleWheelEvent(platformWindow->window(), xiDeviceEvent->time, local, global, QPoint(), angleDelta, modifiers);
            }
        }
    }
}

static int xi2ValuatorOffset(const unsigned char *maskPtr, int maskLen, int number)
{
    int offset = 0;
    for (int i = 0; i < maskLen; i++) {
        if (number < 8) {
            if ((maskPtr[i] & (1 << number)) == 0)
                return -1;
        }
        for (int j = 0; j < 8; j++) {
            if (j == number)
                return offset;
            if (maskPtr[i] & (1 << j))
                offset++;
        }
        number -= 8;
    }
    return -1;
}

bool QXcbConnection::xi2GetValuatorValueIfSet(const void *event, int valuatorNum, double *value)
{
    auto *xideviceevent = static_cast<const qt_xcb_input_device_event_t *>(event);
    auto *buttonsMaskAddr = reinterpret_cast<const unsigned char *>(&xideviceevent[1]);
    auto *valuatorsMaskAddr = buttonsMaskAddr + xideviceevent->buttons_len * 4;
    auto *valuatorsValuesAddr = reinterpret_cast<const xcb_input_fp3232_t *>(valuatorsMaskAddr + xideviceevent->valuators_len * 4);

    int valuatorOffset = xi2ValuatorOffset(valuatorsMaskAddr, xideviceevent->valuators_len, valuatorNum);
    if (valuatorOffset < 0)
        return false;

    *value = valuatorsValuesAddr[valuatorOffset].integral;
    *value += ((double)valuatorsValuesAddr[valuatorOffset].frac / (1 << 16) / (1 << 16));
    return true;
}

Qt::MouseButton QXcbConnection::xiToQtMouseButton(uint32_t b)
{
    switch (b) {
    case 1: return Qt::LeftButton;
    case 2: return Qt::MiddleButton;
    case 3: return Qt::RightButton;
    // 4-7 are for scrolling
    default: break;
    }
    if (b >= 8 && b <= Qt::MaxMouseButton)
        return static_cast<Qt::MouseButton>(Qt::BackButton << (b - 8));
    return Qt::NoButton;
}

#if QT_CONFIG(tabletevent)
static QTabletEvent::TabletDevice toolIdToTabletDevice(quint32 toolId) {
    // keep in sync with wacom_intuos_inout() in Linux kernel driver wacom_wac.c
    switch (toolId) {
    case 0xd12:
    case 0x912:
    case 0x112:
    case 0x913: /* Intuos3 Airbrush */
    case 0x91b: /* Intuos3 Airbrush Eraser */
    case 0x902: /* Intuos4/5 13HD/24HD Airbrush */
    case 0x90a: /* Intuos4/5 13HD/24HD Airbrush Eraser */
    case 0x100902: /* Intuos4/5 13HD/24HD Airbrush */
    case 0x10090a: /* Intuos4/5 13HD/24HD Airbrush Eraser */
        return QTabletEvent::Airbrush;
    case 0x007: /* Mouse 4D and 2D */
    case 0x09c:
    case 0x094:
        return QTabletEvent::FourDMouse;
    case 0x017: /* Intuos3 2D Mouse */
    case 0x806: /* Intuos4 Mouse */
    case 0x096: /* Lens cursor */
    case 0x097: /* Intuos3 Lens cursor */
    case 0x006: /* Intuos4 Lens cursor */
        return QTabletEvent::Puck;
    case 0x885:    /* Intuos3 Art Pen (Marker Pen) */
    case 0x100804: /* Intuos4/5 13HD/24HD Art Pen */
    case 0x10080c: /* Intuos4/5 13HD/24HD Art Pen Eraser */
        return QTabletEvent::RotationStylus;
    case 0:
        return QTabletEvent::NoDevice;
    }
    return QTabletEvent::Stylus;  // Safe default assumption if nonzero
}

static const char *toolName(QTabletEvent::TabletDevice tool) {
    static const QMetaObject *metaObject = qt_getEnumMetaObject(tool);
    static const QMetaEnum me = metaObject->enumerator(metaObject->indexOfEnumerator(qt_getEnumName(tool)));
    return me.valueToKey(tool);
}

static const char *pointerTypeName(QTabletEvent::PointerType ptype) {
    static const QMetaObject *metaObject = qt_getEnumMetaObject(ptype);
    static const QMetaEnum me = metaObject->enumerator(metaObject->indexOfEnumerator(qt_getEnumName(ptype)));
    return me.valueToKey(ptype);
}

bool QXcbConnection::xi2HandleTabletEvent(const void *event, TabletData *tabletData)
{
    bool handled = true;
    const auto *xiDeviceEvent = reinterpret_cast<const qt_xcb_input_device_event_t *>(event);

    switch (xiDeviceEvent->event_type) {
    case XCB_INPUT_BUTTON_PRESS: {
        Qt::MouseButton b = xiToQtMouseButton(xiDeviceEvent->detail);
        tabletData->buttons |= b;
        xi2ReportTabletEvent(event, tabletData);
        break;
    }
    case XCB_INPUT_BUTTON_RELEASE: {
        Qt::MouseButton b = xiToQtMouseButton(xiDeviceEvent->detail);
        tabletData->buttons ^= b;
        xi2ReportTabletEvent(event, tabletData);
        break;
    }
    case XCB_INPUT_MOTION:
        xi2ReportTabletEvent(event, tabletData);
        break;
    case XCB_INPUT_PROPERTY: {
        // This is the wacom driver's way of reporting tool proximity.
        // The evdev driver doesn't do it this way.
        const auto *ev = reinterpret_cast<const xcb_input_property_event_t *>(event);
        if (ev->what == XCB_INPUT_PROPERTY_FLAG_MODIFIED) {
            if (ev->property == atom(QXcbAtom::WacomSerialIDs)) {
                enum WacomSerialIndex {
                    _WACSER_USB_ID = 0,
                    _WACSER_LAST_TOOL_SERIAL,
                    _WACSER_LAST_TOOL_ID,
                    _WACSER_TOOL_SERIAL,
                    _WACSER_TOOL_ID,
                    _WACSER_COUNT
                };

                auto reply = Q_XCB_REPLY(xcb_input_xi_get_property, xcb_connection(), tabletData->deviceId, 0,
                                         ev->property, XCB_GET_PROPERTY_TYPE_ANY, 0, 100);
                if (reply) {
                    if (reply->type == atom(QXcbAtom::INTEGER) && reply->format == 32 && reply->num_items == _WACSER_COUNT) {
                        quint32 *ptr = reinterpret_cast<quint32 *>(xcb_input_xi_get_property_items(reply.get()));
                        quint32 tool = ptr[_WACSER_TOOL_ID];
                        // Workaround for http://sourceforge.net/p/linuxwacom/bugs/246/
                        // e.g. on Thinkpad Helix, tool ID will be 0 and serial will be 1
                        if (!tool && ptr[_WACSER_TOOL_SERIAL])
                            tool = ptr[_WACSER_TOOL_SERIAL];

                        // The property change event informs us which tool is in proximity or which one left proximity.
                        if (tool) {
                            tabletData->inProximity = true;
                            tabletData->tool = toolIdToTabletDevice(tool);
                            tabletData->serialId = qint64(ptr[_WACSER_USB_ID]) << 32 | qint64(ptr[_WACSER_TOOL_SERIAL]);
                            QWindowSystemInterface::handleTabletEnterProximityEvent(ev->time,
                                tabletData->tool, tabletData->pointerType, tabletData->serialId);
                        } else {
                            tabletData->inProximity = false;
                            tabletData->tool = toolIdToTabletDevice(ptr[_WACSER_LAST_TOOL_ID]);
                            // Workaround for http://sourceforge.net/p/linuxwacom/bugs/246/
                            // e.g. on Thinkpad Helix, tool ID will be 0 and serial will be 1
                            if (!tabletData->tool)
                                tabletData->tool = toolIdToTabletDevice(ptr[_WACSER_LAST_TOOL_SERIAL]);
                            tabletData->serialId = qint64(ptr[_WACSER_USB_ID]) << 32 | qint64(ptr[_WACSER_LAST_TOOL_SERIAL]);
                            QWindowSystemInterface::handleTabletLeaveProximityEvent(ev->time,
                                tabletData->tool, tabletData->pointerType, tabletData->serialId);
                        }
                        // TODO maybe have a hash of tabletData->deviceId to device data so we can
                        // look up the tablet name here, and distinguish multiple tablets
                        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
                            qCDebug(lcQpaXInputEvents, "XI2 proximity change on tablet %d (USB %x): last tool: %x id %x current tool: %x id %x %s",
                                    tabletData->deviceId, ptr[_WACSER_USB_ID], ptr[_WACSER_LAST_TOOL_SERIAL], ptr[_WACSER_LAST_TOOL_ID],
                                    ptr[_WACSER_TOOL_SERIAL], ptr[_WACSER_TOOL_ID], toolName(tabletData->tool));
                    }
                }
            }
        }
        break;
    }
    default:
        handled = false;
        break;
    }

    return handled;
}

inline qreal scaleOneValuator(qreal normValue, qreal screenMin, qreal screenSize)
{
    return screenMin + normValue * screenSize;
}

void QXcbConnection::xi2ReportTabletEvent(const void *event, TabletData *tabletData)
{
    auto *ev = reinterpret_cast<const qt_xcb_input_device_event_t *>(event);
    QXcbWindow *xcbWindow = platformWindowFromId(ev->event);
    if (!xcbWindow)
        return;
    QWindow *window = xcbWindow->window();
    const Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(ev->mods.effective);
    QPointF local(fixed1616ToReal(ev->event_x), fixed1616ToReal(ev->event_y));
    QPointF global(fixed1616ToReal(ev->root_x), fixed1616ToReal(ev->root_y));
    double pressure = 0, rotation = 0, tangentialPressure = 0;
    int xTilt = 0, yTilt = 0;
    static const bool useValuators = !qEnvironmentVariableIsSet("QT_XCB_TABLET_LEGACY_COORDINATES");

    // Valuators' values are relative to the physical size of the current virtual
    // screen. Therefore we cannot use QScreen/QWindow geometry and should use
    // QPlatformWindow/QPlatformScreen instead.
    QRect physicalScreenArea;
    if (Q_LIKELY(useValuators)) {
        const QList<QPlatformScreen *> siblings = window->screen()->handle()->virtualSiblings();
        for (const QPlatformScreen *screen : siblings)
            physicalScreenArea |= screen->geometry();
    }

    for (QHash<int, TabletData::ValuatorClassInfo>::iterator it = tabletData->valuatorInfo.begin(),
            ite = tabletData->valuatorInfo.end(); it != ite; ++it) {
        int valuator = it.key();
        TabletData::ValuatorClassInfo &classInfo(it.value());
        xi2GetValuatorValueIfSet(event, classInfo.number, &classInfo.curVal);
        double normalizedValue = (classInfo.curVal - classInfo.minVal) / (classInfo.maxVal - classInfo.minVal);
        switch (valuator) {
        case QXcbAtom::AbsX:
            if (Q_LIKELY(useValuators)) {
                const qreal value = scaleOneValuator(normalizedValue, physicalScreenArea.x(), physicalScreenArea.width());
                global.setX(value);
                // mapFromGlobal is ok for nested/embedded windows, but works only with whole-number QPoint;
                // so map it first, then add back the sub-pixel position
                local.setX(window->mapFromGlobal(QPoint(int(value), 0)).x() + (value - int(value)));
            }
            break;
        case QXcbAtom::AbsY:
            if (Q_LIKELY(useValuators)) {
                qreal value = scaleOneValuator(normalizedValue, physicalScreenArea.y(), physicalScreenArea.height());
                global.setY(value);
                local.setY(window->mapFromGlobal(QPoint(0, int(value))).y() + (value - int(value)));
            }
            break;
        case QXcbAtom::AbsPressure:
            pressure = normalizedValue;
            break;
        case QXcbAtom::AbsTiltX:
            xTilt = classInfo.curVal;
            break;
        case QXcbAtom::AbsTiltY:
            yTilt = classInfo.curVal;
            break;
        case QXcbAtom::AbsWheel:
            switch (tabletData->tool) {
            case QTabletEvent::Airbrush:
                tangentialPressure = normalizedValue * 2.0 - 1.0; // Convert 0..1 range to -1..+1 range
                break;
            case QTabletEvent::RotationStylus:
                rotation = normalizedValue * 360.0 - 180.0; // Convert 0..1 range to -180..+180 degrees
                break;
            default:    // Other types of styli do not use this valuator
                break;
            }
            break;
        default:
            break;
        }
    }

    if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
        qCDebug(lcQpaXInputEvents, "XI2 event on tablet %d with tool %s type %s seq %d detail %d time %d "
            "pos %6.1f, %6.1f root pos %6.1f, %6.1f buttons 0x%x pressure %4.2lf tilt %d, %d rotation %6.2lf modifiers 0x%x",
            tabletData->deviceId, toolName(tabletData->tool), pointerTypeName(tabletData->pointerType),
            ev->sequence, ev->detail, ev->time,
            local.x(), local.y(), global.x(), global.y(),
            (int)tabletData->buttons, pressure, xTilt, yTilt, rotation, (int)modifiers);

    QWindowSystemInterface::handleTabletEvent(window, ev->time, local, global,
                                              tabletData->tool, tabletData->pointerType,
                                              tabletData->buttons, pressure,
                                              xTilt, yTilt, tangentialPressure,
                                              rotation, 0, tabletData->serialId, modifiers);
}

QXcbConnection::TabletData *QXcbConnection::tabletDataForDevice(int id)
{
    for (int i = 0; i < m_tabletData.count(); ++i) {
        if (m_tabletData.at(i).deviceId == id)
            return &m_tabletData[i];
    }
    return nullptr;
}

#endif // QT_CONFIG(tabletevent)
