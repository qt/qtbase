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

#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>

void QXcbConnection::initializeXInput2()
{
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    if (XQueryExtension(xDisplay, "XInputExtension", &m_xiOpCode, &m_xiEventBase, &m_xiErrorBase)) {
        int xiMajor = 2;
#if defined(XCB_USE_XINPUT22)
        m_xi2Minor = 2; // for touch support 2.2 is enough
#elif defined(XCB_USE_XINPUT21)
        m_xi2Minor = 1; // for smooth scrolling 2.1 is enough
#else
        m_xi2Minor = 0; // for tablet support 2.0 is enough
#endif
        qCDebug(lcQpaXInput, "Plugin build with support for XInput 2 version up "
                             "to %d.%d", xiMajor, m_xi2Minor);

        switch (XIQueryVersion(xDisplay, &xiMajor, &m_xi2Minor)) {
        case Success:
            // Server's supported version can be lower than the version we have
            // announced to support. In this case Qt client will be limited by
            // X server's supported version.
            qCDebug(lcQpaXInput, "Using XInput version %d.%d", xiMajor, m_xi2Minor);
            m_xi2Enabled = true;
            xi2SetupDevices();
            xi2SelectStateEvents();
            break;
        case BadRequest: // Must be an X server with XInput 1
            qCDebug(lcQpaXInput, "X server does not support XInput 2");
            break;
        default: // BadValue
            qCDebug(lcQpaXInput, "Internal error");
            break;
        }
    }
}

void QXcbConnection::xi2SelectStateEvents()
{
    // These state events do not depend on a specific X window, but are global
    // for the X client's (application's) state.
    unsigned int bitMask = 0;
    unsigned char *xiBitMask = reinterpret_cast<unsigned char *>(&bitMask);
    XIEventMask xiEventMask;
    bitMask = XI_HierarchyChangedMask;
    bitMask |= XI_DeviceChangedMask;
    bitMask |= XI_PropertyEventMask;
    xiEventMask.deviceid = XIAllDevices;
    xiEventMask.mask_len = sizeof(bitMask);
    xiEventMask.mask = xiBitMask;
    Display *dpy = static_cast<Display *>(m_xlib_display);
    XISelectEvents(dpy, DefaultRootWindow(dpy), &xiEventMask, 1);
}

void QXcbConnection::xi2SelectDeviceEvents(xcb_window_t window)
{
    if (window == rootWindow())
        return;

    unsigned int bitMask = 0;
    unsigned char *xiBitMask = reinterpret_cast<unsigned char *>(&bitMask);
    bitMask |= XI_ButtonPressMask;
    bitMask |= XI_ButtonReleaseMask;
    bitMask |= XI_MotionMask;
    // There is a check for enter/leave events in plain xcb enter/leave event handler,
    // core enter/leave events will be ignored in this case.
    bitMask |= XI_EnterMask;
    bitMask |= XI_LeaveMask;
#ifdef XCB_USE_XINPUT22
    if (isAtLeastXI22()) {
        bitMask |= XI_TouchBeginMask;
        bitMask |= XI_TouchUpdateMask;
        bitMask |= XI_TouchEndMask;
    }
#endif

    XIEventMask mask;
    mask.mask_len = sizeof(bitMask);
    mask.mask = xiBitMask;
    mask.deviceid = XIAllMasterDevices;
    Display *dpy = static_cast<Display *>(m_xlib_display);
    Status result = XISelectEvents(dpy, window, &mask, 1);
    if (result == Success)
        QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
    else
        qCDebug(lcQpaXInput, "failed to select events, window %x, result %d", window, result);
}

void QXcbConnection::xi2SetupDevice(void *info, bool removeExisting)
{
    XIDeviceInfo *deviceInfo = reinterpret_cast<XIDeviceInfo *>(info);
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

    qCDebug(lcQpaXInputDevices) << "input device " << deviceInfo->name << "ID" << deviceInfo->deviceid;
#if QT_CONFIG(tabletevent)
    TabletData tabletData;
#endif
    ScrollingDevice scrollingDevice;
    for (int c = 0; c < deviceInfo->num_classes; ++c) {
        XIAnyClassInfo *classinfo = deviceInfo->classes[c];
        switch (classinfo->type) {
        case XIValuatorClass: {
            XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(classinfo);
            const int valuatorAtom = qatom(vci->label);
            qCDebug(lcQpaXInputDevices) << "   has valuator" << atomName(vci->label) << "recognized?" << (valuatorAtom < QXcbAtom::NAtoms);
#if QT_CONFIG(tabletevent)
            if (valuatorAtom < QXcbAtom::NAtoms) {
                TabletData::ValuatorClassInfo info;
                info.minVal = vci->min;
                info.maxVal = vci->max;
                info.number = vci->number;
                tabletData.valuatorInfo[valuatorAtom] = info;
            }
#endif // QT_CONFIG(tabletevent)
            if (valuatorAtom == QXcbAtom::RelHorizScroll || valuatorAtom == QXcbAtom::RelHorizWheel)
                scrollingDevice.lastScrollPosition.setX(vci->value);
            else if (valuatorAtom == QXcbAtom::RelVertScroll || valuatorAtom == QXcbAtom::RelVertWheel)
                scrollingDevice.lastScrollPosition.setY(vci->value);
            break;
        }
#ifdef XCB_USE_XINPUT21
        case XIScrollClass: {
            XIScrollClassInfo *sci = reinterpret_cast<XIScrollClassInfo *>(classinfo);
            if (sci->scroll_type == XIScrollTypeVertical) {
                scrollingDevice.orientations |= Qt::Vertical;
                scrollingDevice.verticalIndex = sci->number;
                scrollingDevice.verticalIncrement = sci->increment;
            }
            else if (sci->scroll_type == XIScrollTypeHorizontal) {
                scrollingDevice.orientations |= Qt::Horizontal;
                scrollingDevice.horizontalIndex = sci->number;
                scrollingDevice.horizontalIncrement = sci->increment;
            }
            break;
        }
        case XIButtonClass: {
            XIButtonClassInfo *bci = reinterpret_cast<XIButtonClassInfo *>(classinfo);
            if (bci->num_buttons >= 5) {
                Atom label4 = bci->labels[3];
                Atom label5 = bci->labels[4];
                // Some drivers have no labels on the wheel buttons, some have no label on just one and some have no label on
                // button 4 and the wrong one on button 5. So we just check that they are not labelled with unrelated buttons.
                if ((!label4 || qatom(label4) == QXcbAtom::ButtonWheelUp || qatom(label4) == QXcbAtom::ButtonWheelDown) &&
                    (!label5 || qatom(label5) == QXcbAtom::ButtonWheelUp || qatom(label5) == QXcbAtom::ButtonWheelDown))
                    scrollingDevice.legacyOrientations |= Qt::Vertical;
            }
            if (bci->num_buttons >= 7) {
                Atom label6 = bci->labels[5];
                Atom label7 = bci->labels[6];
                if ((!label6 || qatom(label6) == QXcbAtom::ButtonHorizWheelLeft) && (!label7 || qatom(label7) == QXcbAtom::ButtonHorizWheelRight))
                    scrollingDevice.legacyOrientations |= Qt::Horizontal;
            }
            qCDebug(lcQpaXInputDevices, "   has %d buttons", bci->num_buttons);
            break;
        }
#endif
        case XIKeyClass:
            qCDebug(lcQpaXInputDevices) << "   it's a keyboard";
            break;
#ifdef XCB_USE_XINPUT22
        case XITouchClass:
            // will be handled in populateTouchDevices()
            break;
#endif
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
    QByteArray name = QByteArray(deviceInfo->name).toLower();
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
    } else {
        isTablet = false;
    }

    if (isTablet) {
        tabletData.deviceId = deviceInfo->deviceid;
        m_tabletData.append(tabletData);
        qCDebug(lcQpaXInputDevices) << "   it's a tablet with pointer type" << dbgType;
    }
#endif // QT_CONFIG(tabletevent)

#ifdef XCB_USE_XINPUT21
    if (scrollingDevice.orientations || scrollingDevice.legacyOrientations) {
        scrollingDevice.deviceId = deviceInfo->deviceid;
        // Only use legacy wheel button events when we don't have real scroll valuators.
        scrollingDevice.legacyOrientations &= ~scrollingDevice.orientations;
        m_scrollingDevices.insert(scrollingDevice.deviceId, scrollingDevice);
        qCDebug(lcQpaXInputDevices) << "   it's a scrolling device";
    }
#endif

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

    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    int deviceCount = 0;
    XIDeviceInfo *devices = XIQueryDevice(xDisplay, XIAllDevices, &deviceCount);
    m_xiMasterPointerIds.clear();
    for (int i = 0; i < deviceCount; ++i) {
        XIDeviceInfo deviceInfo = devices[i];
        if (deviceInfo.use == XIMasterPointer) {
            m_xiMasterPointerIds.append(deviceInfo.deviceid);
            continue;
        }
        if (deviceInfo.use == XISlavePointer) // only slave pointer devices are relevant here
            xi2SetupDevice(&deviceInfo, false);
    }
    if (m_xiMasterPointerIds.size() > 1)
        qCDebug(lcQpaXInputDevices) << "multi-pointer X detected";
    XIFreeDeviceInfo(devices);
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

    unsigned int mask = 0;
    unsigned char *bitMask = reinterpret_cast<unsigned char *>(&mask);
    Display *dpy = static_cast<Display *>(m_xlib_display);

#ifdef XCB_USE_XINPUT22
    if (isAtLeastXI22()) {
        mask |= XI_TouchBeginMask;
        mask |= XI_TouchUpdateMask;
        mask |= XI_TouchEndMask;

        XIEventMask xiMask;
        xiMask.mask_len = sizeof(mask);
        xiMask.mask = bitMask;
        xiMask.deviceid = XIAllMasterDevices;
        Status result = XISelectEvents(dpy, window, &xiMask, 1);
        if (result == Success)
            QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
        else
            qCDebug(lcQpaXInput, "failed to select events, window %x, result %d", window, result);
    }
#endif

    mask = XI_ButtonPressMask;
    mask |= XI_ButtonReleaseMask;
    mask |= XI_MotionMask;

#if QT_CONFIG(tabletevent)
    QSet<int> tabletDevices;
    if (!m_tabletData.isEmpty()) {
        const int nrTablets = m_tabletData.count();
        QVector<XIEventMask> xiEventMask(nrTablets);
        for (int i = 0; i < nrTablets; ++i) {
            int deviceId = m_tabletData.at(i).deviceId;
            tabletDevices.insert(deviceId);
            xiEventMask[i].deviceid = deviceId;
            xiEventMask[i].mask_len = sizeof(mask);
            xiEventMask[i].mask = bitMask;
        }
        XISelectEvents(dpy, window, xiEventMask.data(), nrTablets);
    }
#endif

#ifdef XCB_USE_XINPUT21
    if (!m_scrollingDevices.isEmpty()) {
        QVector<XIEventMask> xiEventMask(m_scrollingDevices.size());
        int i = 0;
        for (const ScrollingDevice& scrollingDevice : qAsConst(m_scrollingDevices)) {
#if QT_CONFIG(tabletevent)
            if (tabletDevices.contains(scrollingDevice.deviceId))
                continue; // All necessary events are already captured.
#endif
            xiEventMask[i].deviceid = scrollingDevice.deviceId;
            xiEventMask[i].mask_len = sizeof(mask);
            xiEventMask[i].mask = bitMask;
            i++;
        }
        XISelectEvents(dpy, window, xiEventMask.data(), i);
    }
#endif

#if !QT_CONFIG(tabletevent) && !defined(XCB_USE_XINPUT21)
    Q_UNUSED(bitMask);
    Q_UNUSED(dpy);
#endif
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
    XIDeviceInfo *deviceinfo = reinterpret_cast<XIDeviceInfo *>(info);
    QTouchDevice::Capabilities caps = 0;
    int type = -1;
    int maxTouchPoints = 1;
    bool isTouchDevice = false;
    bool hasRelativeCoords = false;
    TouchDeviceData dev;
    for (int i = 0; i < deviceinfo->num_classes; ++i) {
        XIAnyClassInfo *classinfo = deviceinfo->classes[i];
        switch (classinfo->type) {
#ifdef XCB_USE_XINPUT22
        case XITouchClass: {
            XITouchClassInfo *tci = reinterpret_cast<XITouchClassInfo *>(classinfo);
            maxTouchPoints = tci->num_touches;
            qCDebug(lcQpaXInputDevices, "   has touch class with mode %d", tci->mode);
            switch (tci->mode) {
            case XIDependentTouch:
                type = QTouchDevice::TouchPad;
                break;
            case XIDirectTouch:
                type = QTouchDevice::TouchScreen;
                break;
            }
            break;
        }
#endif // XCB_USE_XINPUT22
        case XIValuatorClass: {
            XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(classinfo);
            const QXcbAtom::Atom valuatorAtom = qatom(vci->label);
            if (valuatorAtom < QXcbAtom::NAtoms) {
                TouchDeviceData::ValuatorClassInfo info;
                info.min = vci->min;
                info.max = vci->max;
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
                dev.size.setWidth((vci->max - vci->min) * 1000.0 / vciResolution);
            } else if (valuatorAtom == QXcbAtom::RelY) {
                hasRelativeCoords = true;
                dev.size.setHeight((vci->max - vci->min) * 1000.0 / vciResolution);
            } else if (valuatorAtom == QXcbAtom::AbsX) {
                caps |= QTouchDevice::Position;
                dev.size.setWidth((vci->max - vci->min) * 1000.0 / vciResolution);
            } else if (valuatorAtom == QXcbAtom::AbsY) {
                caps |= QTouchDevice::Position;
                dev.size.setHeight((vci->max - vci->min) * 1000.0 / vciResolution);
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
        dev.qtTouchDevice->setName(QString::fromUtf8(deviceinfo->name));
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

#if defined(XCB_USE_XINPUT21) || QT_CONFIG(tabletevent)
static inline qreal fixed1616ToReal(FP1616 val)
{
    return qreal(val) / 0x10000;
}
#endif // defined(XCB_USE_XINPUT21) || QT_CONFIG(tabletevent)

void QXcbConnection::xi2HandleEvent(xcb_ge_event_t *event)
{
    xi2PrepareXIGenericDeviceEvent(event);
    xXIGenericDeviceEvent *xiEvent = reinterpret_cast<xXIGenericDeviceEvent *>(event);
    int sourceDeviceId = xiEvent->deviceid; // may be the master id
    xXIDeviceEvent *xiDeviceEvent = 0;
    xXIEnterEvent *xiEnterEvent = 0;
    QXcbWindowEventListener *eventListener = 0;

    switch (xiEvent->evtype) {
    case XI_ButtonPress:
    case XI_ButtonRelease:
    case XI_Motion:
#ifdef XCB_USE_XINPUT22
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd:
#endif
    {
        xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
        eventListener = windowEventListenerFromId(xiDeviceEvent->event);
        sourceDeviceId = xiDeviceEvent->sourceid; // use the actual device id instead of the master
        break;
    }
    case XI_Enter:
    case XI_Leave: {
        xiEnterEvent = reinterpret_cast<xXIEnterEvent *>(event);
        eventListener = windowEventListenerFromId(xiEnterEvent->event);
        sourceDeviceId = xiEnterEvent->sourceid; // use the actual device id instead of the master
        break;
    }
    case XI_HierarchyChanged:
        xi2HandleHierarchyEvent(xiEvent);
        return;
    case XI_DeviceChanged:
        xi2HandleDeviceChangedEvent(xiEvent);
        return;
    default:
        break;
    }

    if (eventListener) {
        long result = 0;
        if (eventListener->handleGenericEvent(reinterpret_cast<xcb_generic_event_t *>(event), &result))
            return;
    }

#if QT_CONFIG(tabletevent)
    if (!xiEnterEvent) {
        QXcbConnection::TabletData *tablet = tabletDataForDevice(sourceDeviceId);
        if (tablet && xi2HandleTabletEvent(xiEvent, tablet))
            return;
    }
#endif // QT_CONFIG(tabletevent)

#ifdef XCB_USE_XINPUT21
    if (ScrollingDevice *device = scrollingDeviceForId(sourceDeviceId))
        xi2HandleScrollEvent(xiEvent, *device);
#endif // XCB_USE_XINPUT21

#ifdef XCB_USE_XINPUT22
    if (xiDeviceEvent) {
        switch (xiDeviceEvent->evtype) {
        case XI_ButtonPress:
        case XI_ButtonRelease:
        case XI_Motion:
            if (!xi2MouseEventsDisabled() && eventListener && !(xiDeviceEvent->flags & XIPointerEmulated))
                eventListener->handleXIMouseEvent(event);
            break;

        case XI_TouchBegin:
        case XI_TouchUpdate:
        case XI_TouchEnd:
            if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
                qCDebug(lcQpaXInputEvents, "XI2 touch event type %d seq %d detail %d pos %6.1f, %6.1f root pos %6.1f, %6.1f on window %x",
                        event->event_type, xiDeviceEvent->sequenceNumber, xiDeviceEvent->detail,
                        fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y),
                        fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y),xiDeviceEvent->event);
            if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event))
                xi2ProcessTouch(xiDeviceEvent, platformWindow);
            break;
        }
    } else if (xiEnterEvent && !xi2MouseEventsDisabled() && eventListener) {
        switch (xiEnterEvent->evtype) {
        case XI_Enter:
        case XI_Leave:
            eventListener->handleXIEnterLeave(event);
            break;
        }
    }
#endif // XCB_USE_XINPUT22
}

bool QXcbConnection::xi2MouseEventsDisabled() const
{
    static bool xi2MouseDisabled = qEnvironmentVariableIsSet("QT_XCB_NO_XI2_MOUSE");
    // FIXME: Don't use XInput2 mouse events when Xinerama extension
    // is enabled, because it causes problems with multi-monitor setup.
    return xi2MouseDisabled || has_xinerama_extension;
}

#ifdef XCB_USE_XINPUT22
bool QXcbConnection::isTouchScreen(int id)
{
    auto device = touchDeviceForId(id);
    return device && device->qtTouchDevice->type() == QTouchDevice::TouchScreen;
}

void QXcbConnection::xi2ProcessTouch(void *xiDevEvent, QXcbWindow *platformWindow)
{
    xXIDeviceEvent *xiDeviceEvent = static_cast<xXIDeviceEvent *>(xiDevEvent);
    TouchDeviceData *dev = touchDeviceForId(xiDeviceEvent->sourceid);
    Q_ASSERT(dev);
    const bool firstTouch = dev->touchPoints.isEmpty();
    if (xiDeviceEvent->evtype == XI_TouchBegin) {
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
    for (const TouchDeviceData::ValuatorClassInfo vci : dev->valuatorInfo) {
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
    if (xiDeviceEvent->evtype != XI_TouchEnd) {
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

    switch (xiDeviceEvent->evtype) {
    case XI_TouchBegin:
        if (firstTouch) {
            dev->firstPressedPosition = QPointF(x, y);
            dev->firstPressedNormalPosition = QPointF(nx, ny);
        }
        dev->pointPressedPosition.insert(touchPoint.id, QPointF(x, y));

        // Touches must be accepted when we are grabbing touch events. Otherwise the entire sequence
        // will get replayed when the grab ends.
        if (m_xiGrab) {
            // Note that XIAllowTouchEvents is known to deadlock with older libXi versions,
            // for details see qtbase/src/plugins/platforms/xcb/README. This has nothing to
            // do with the XInput protocol version, but is a bug in libXi implementation instead.
            XIAllowTouchEvents(static_cast<Display *>(m_xlib_display), xiDeviceEvent->deviceid,
                               xiDeviceEvent->detail, xiDeviceEvent->event, XIAcceptTouch);
        }
        break;
    case XI_TouchUpdate:
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
                XIAllowTouchEvents(static_cast<Display *>(m_xlib_display), xiDeviceEvent->deviceid,
                                   xiDeviceEvent->detail, xiDeviceEvent->event, XIRejectTouch);
                window->doStartSystemMoveResize(QPoint(x, y), m_startSystemMoveResizeInfo.corner);
                m_startSystemMoveResizeInfo.window = XCB_NONE;
            }
        }
        break;
    case XI_TouchEnd:
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
    Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective_mods);
    QWindowSystemInterface::handleTouchEvent(platformWindow->window(), xiDeviceEvent->time, dev->qtTouchDevice, dev->touchPoints.values(), modifiers);
    if (touchPoint.state == Qt::TouchPointReleased)
        // If a touchpoint was released, we can forget it, because the ID won't be reused.
        dev->touchPoints.remove(touchPoint.id);
    else
        // Make sure that we don't send TouchPointPressed/Moved in more than one QTouchEvent
        // with this touch point if the next XI2 event is about a different touch point.
        touchPoint.state = Qt::TouchPointStationary;
}

bool QXcbConnection::startSystemMoveResizeForTouchBegin(xcb_window_t window, const QPoint &point, int corner)
{
    QHash<int, TouchDeviceData>::const_iterator devIt = m_touchDevices.constBegin();
    for (; devIt != m_touchDevices.constEnd(); ++devIt) {
        TouchDeviceData deviceData = devIt.value();
        if (deviceData.qtTouchDevice->type() == QTouchDevice::TouchScreen) {
            QHash<int, QPointF>::const_iterator pointIt = deviceData.pointPressedPosition.constBegin();
            for (; pointIt != deviceData.pointPressedPosition.constEnd(); ++pointIt) {
                if (pointIt.value().toPoint() == point) {
                    m_startSystemMoveResizeInfo.window = window;
                    m_startSystemMoveResizeInfo.deviceid = devIt.key();
                    m_startSystemMoveResizeInfo.pointid = pointIt.key();
                    m_startSystemMoveResizeInfo.corner = corner;
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
#endif // XCB_USE_XINPUT22

bool QXcbConnection::xi2SetMouseGrabEnabled(xcb_window_t w, bool grab)
{
    Display *xDisplay = static_cast<Display *>(xlib_display());
    bool ok = false;

    if (grab) { // grab
        XIEventMask evmask;
        unsigned char mask[XIMaskLen(XI_LASTEVENT)];
        evmask.mask = mask;
        evmask.mask_len = sizeof(mask);
        memset(mask, 0, sizeof(mask));
        XISetMask(mask, XI_ButtonPress);
        XISetMask(mask, XI_ButtonRelease);
        XISetMask(mask, XI_Motion);
        XISetMask(mask, XI_Enter);
        XISetMask(mask, XI_Leave);
        XISetMask(mask, XI_TouchBegin);
        XISetMask(mask, XI_TouchUpdate);
        XISetMask(mask, XI_TouchEnd);

        for (int id : m_xiMasterPointerIds) {
            evmask.deviceid = id;
            Status result = XIGrabDevice(xDisplay, id, w, CurrentTime, None,
                                         XIGrabModeAsync, XIGrabModeAsync, False, &evmask);
            if (result != Success) {
                qCDebug(lcQpaXInput, "failed to grab events for device %d on window %x"
                                     "(result %d)", id, w, result);
            } else {
                // Managed to grab at least one of master pointers, that should be enough
                // to properly dismiss windows that rely on mouse grabbing.
                ok = true;
            }
        }
    } else { // ungrab
        for (int id : m_xiMasterPointerIds) {
            Status result = XIUngrabDevice(xDisplay, id, CurrentTime);
            if (result != Success)
                qCDebug(lcQpaXInput, "XIUngrabDevice failed - id: %d (result %d)", id, result);
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
    xXIHierarchyEvent *xiEvent = reinterpret_cast<xXIHierarchyEvent *>(event);
    // We only care about hotplugged devices
    if (!(xiEvent->flags & (XISlaveRemoved | XISlaveAdded)))
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
    xXIDeviceChangedEvent *xiEvent = reinterpret_cast<xXIDeviceChangedEvent *>(event);
    switch (xiEvent->reason) {
    case XIDeviceChange: {
        int nrDevices = 0;
        Display *dpy = static_cast<Display *>(m_xlib_display);
        XIDeviceInfo* deviceInfo = XIQueryDevice(dpy, xiEvent->sourceid, &nrDevices);
        if (nrDevices <= 0)
            return;
        xi2SetupDevice(deviceInfo);
        XIFreeDeviceInfo(deviceInfo);
        break;
    }
    case XISlaveSwitch: {
#ifdef XCB_USE_XINPUT21
        if (ScrollingDevice *scrollingDevice = scrollingDeviceForId(xiEvent->sourceid))
            xi2UpdateScrollingDevice(*scrollingDevice);
#endif
        break;
    }
    default:
        qCDebug(lcQpaXInputEvents, "unknown device-changed-event (device %d)", xiEvent->sourceid);
        break;
    }
}

#ifdef XCB_USE_XINPUT21
void QXcbConnection::xi2UpdateScrollingDevice(ScrollingDevice &scrollingDevice)
{
    int nrDevices = 0;
    Display *dpy = static_cast<Display *>(m_xlib_display);
    XIDeviceInfo* deviceInfo = XIQueryDevice(dpy, scrollingDevice.deviceId, &nrDevices);
    if (nrDevices <= 0) {
        qCDebug(lcQpaXInputDevices, "scrolling device %d no longer present", scrollingDevice.deviceId);
        return;
    }
    QPointF lastScrollPosition;
    if (lcQpaXInputEvents().isDebugEnabled())
        lastScrollPosition = scrollingDevice.lastScrollPosition;
    for (int c = 0; c < deviceInfo->num_classes; ++c) {
        XIAnyClassInfo *classInfo = deviceInfo->classes[c];
        if (classInfo->type == XIValuatorClass) {
            XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(classInfo);
            const int valuatorAtom = qatom(vci->label);
            if (valuatorAtom == QXcbAtom::RelHorizScroll || valuatorAtom == QXcbAtom::RelHorizWheel)
                scrollingDevice.lastScrollPosition.setX(vci->value);
            else if (valuatorAtom == QXcbAtom::RelVertScroll || valuatorAtom == QXcbAtom::RelVertWheel)
                scrollingDevice.lastScrollPosition.setY(vci->value);
        }
    }
    if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled() && lastScrollPosition != scrollingDevice.lastScrollPosition))
        qCDebug(lcQpaXInputEvents, "scrolling device %d moved from (%f, %f) to (%f, %f)", scrollingDevice.deviceId,
                lastScrollPosition.x(), lastScrollPosition.y(),
                scrollingDevice.lastScrollPosition.x(),
                scrollingDevice.lastScrollPosition.y());

    XIFreeDeviceInfo(deviceInfo);
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
    xXIGenericDeviceEvent *xiEvent = reinterpret_cast<xXIGenericDeviceEvent *>(event);

    if (xiEvent->evtype == XI_Motion && scrollingDevice.orientations) {
        xXIDeviceEvent* xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
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
                Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective_mods);
                if (modifiers & Qt::AltModifier) {
                    std::swap(angleDelta.rx(), angleDelta.ry());
                    std::swap(rawDelta.rx(), rawDelta.ry());
                }
                qCDebug(lcQpaXInputEvents) << "scroll wheel @ window pos" << local << "delta px" << rawDelta << "angle" << angleDelta;
                QWindowSystemInterface::handleWheelEvent(platformWindow->window(), xiEvent->time, local, global, rawDelta, angleDelta, modifiers);
            }
        }
    } else if (xiEvent->evtype == XI_ButtonRelease && scrollingDevice.legacyOrientations) {
        xXIDeviceEvent* xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
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
                Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(xiDeviceEvent->mods.effective_mods);
                if (modifiers & Qt::AltModifier)
                    std::swap(angleDelta.rx(), angleDelta.ry());
                qCDebug(lcQpaXInputEvents) << "scroll wheel (button" << xiDeviceEvent->detail << ") @ window pos" << local << "delta angle" << angleDelta;
                QWindowSystemInterface::handleWheelEvent(platformWindow->window(), xiEvent->time, local, global, QPoint(), angleDelta, modifiers);
            }
        }
    }
}
#endif // XCB_USE_XINPUT21

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
    const xXIDeviceEvent *xideviceevent = static_cast<const xXIDeviceEvent *>(event);
    const unsigned char *buttonsMaskAddr = (const unsigned char*)&xideviceevent[1];
    const unsigned char *valuatorsMaskAddr = buttonsMaskAddr + xideviceevent->buttons_len * 4;
    FP3232 *valuatorsValuesAddr = (FP3232*)(valuatorsMaskAddr + xideviceevent->valuators_len * 4);

    int valuatorOffset = xi2ValuatorOffset(valuatorsMaskAddr, xideviceevent->valuators_len, valuatorNum);
    if (valuatorOffset < 0)
        return false;

    *value = valuatorsValuesAddr[valuatorOffset].integral;
    *value += ((double)valuatorsValuesAddr[valuatorOffset].frac / (1 << 16) / (1 << 16));
    return true;
}

void QXcbConnection::xi2PrepareXIGenericDeviceEvent(xcb_ge_event_t *event)
{
    // xcb event structs contain stuff that wasn't on the wire, the full_sequence field
    // adds an extra 4 bytes and generic events cookie data is on the wire right after the standard 32 bytes.
    // Move this data back to have the same layout in memory as it was on the wire
    // and allow casting, overwriting the full_sequence field.
    memmove((char*) event + 32, (char*) event + 36, event->length * 4);
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
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    const xXIGenericDeviceEvent *xiEvent = static_cast<const xXIGenericDeviceEvent *>(event);
    const xXIDeviceEvent *xiDeviceEvent = reinterpret_cast<const xXIDeviceEvent *>(xiEvent);

    switch (xiEvent->evtype) {
    case XI_ButtonPress: {
        Qt::MouseButton b = xiToQtMouseButton(xiDeviceEvent->detail);
        tabletData->buttons |= b;
        xi2ReportTabletEvent(xiEvent, tabletData);
        break;
    }
    case XI_ButtonRelease: {
        Qt::MouseButton b = xiToQtMouseButton(xiDeviceEvent->detail);
        tabletData->buttons ^= b;
        xi2ReportTabletEvent(xiEvent, tabletData);
        break;
    }
    case XI_Motion:
        xi2ReportTabletEvent(xiEvent, tabletData);
        break;
    case XI_PropertyEvent: {
        // This is the wacom driver's way of reporting tool proximity.
        // The evdev driver doesn't do it this way.
        const xXIPropertyEvent *ev = reinterpret_cast<const xXIPropertyEvent *>(event);
        if (ev->what == XIPropertyModified) {
            if (ev->property == atom(QXcbAtom::WacomSerialIDs)) {
                enum WacomSerialIndex {
                    _WACSER_USB_ID = 0,
                    _WACSER_LAST_TOOL_SERIAL,
                    _WACSER_LAST_TOOL_ID,
                    _WACSER_TOOL_SERIAL,
                    _WACSER_TOOL_ID,
                    _WACSER_COUNT
                };
                Atom propType;
                int propFormat;
                unsigned long numItems, bytesAfter;
                unsigned char *data;
                if (XIGetProperty(xDisplay, tabletData->deviceId, ev->property, 0, 100,
                                  0, AnyPropertyType, &propType, &propFormat,
                                  &numItems, &bytesAfter, &data) == Success) {
                    if (propType == atom(QXcbAtom::INTEGER) && propFormat == 32 && numItems == _WACSER_COUNT) {
                        quint32 *ptr = reinterpret_cast<quint32 *>(data);
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
                    XFree(data);
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

void QXcbConnection::xi2ReportTabletEvent(const void *event, TabletData *tabletData)
{
    const xXIDeviceEvent *ev = reinterpret_cast<const xXIDeviceEvent *>(event);
    QXcbWindow *xcbWindow = platformWindowFromId(ev->event);
    if (!xcbWindow)
        return;
    QWindow *window = xcbWindow->window();
    const Qt::KeyboardModifiers modifiers = keyboard()->translateModifiers(ev->mods.effective_mods);
    QPointF local(fixed1616ToReal(ev->event_x), fixed1616ToReal(ev->event_y));
    QPointF global(fixed1616ToReal(ev->root_x), fixed1616ToReal(ev->root_y));
    double pressure = 0, rotation = 0, tangentialPressure = 0;
    int xTilt = 0, yTilt = 0;

    for (QHash<int, TabletData::ValuatorClassInfo>::iterator it = tabletData->valuatorInfo.begin(),
            ite = tabletData->valuatorInfo.end(); it != ite; ++it) {
        int valuator = it.key();
        TabletData::ValuatorClassInfo &classInfo(it.value());
        xi2GetValuatorValueIfSet(event, classInfo.number, &classInfo.curVal);
        double normalizedValue = (classInfo.curVal - classInfo.minVal) / (classInfo.maxVal - classInfo.minVal);
        switch (valuator) {
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
            ev->sequenceNumber, ev->detail, ev->time,
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
