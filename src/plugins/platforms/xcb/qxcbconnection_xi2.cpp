/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qtouchdevice.h"
#include <qpa/qwindowsysteminterface.h>
//#define XI2_TOUCH_DEBUG
#ifdef XI2_TOUCH_DEBUG
#include <QDebug>
#endif

#ifdef XCB_USE_XINPUT2

#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#define FINGER_MAX_WIDTH_MM 10

struct XInput2DeviceData {
    XInput2DeviceData()
    : xiDeviceInfo(0)
    , qtTouchDevice(0)
    {
    }
    XIDeviceInfo *xiDeviceInfo;
    QTouchDevice *qtTouchDevice;
};

#ifndef QT_NO_TABLETEVENT
static inline bool q_xi2_is_tablet(XIDeviceInfo *dev)
{
    QByteArray name(dev->name);
    name = name.toLower();
    // Cannot just check for "wacom" because that would also pick up the touch and tablet-button devices.
    return name.contains("stylus") || name.contains("eraser");
}
#endif // QT_NO_TABLETEVENT

void QXcbConnection::initializeXInput2()
{
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    if (XQueryExtension(xDisplay, "XInputExtension", &m_xiOpCode, &m_xiEventBase, &m_xiErrorBase)) {
        int xiMajor = 2;
        m_xi2Minor = 2; // try 2.2 first, needed for TouchBegin/Update/End
        if (XIQueryVersion(xDisplay, &xiMajor, &m_xi2Minor) == BadRequest) {
            m_xi2Minor = 0; // for tablet support 2.0 is enough
            m_xi2Enabled = XIQueryVersion(xDisplay, &xiMajor, &m_xi2Minor) != BadRequest;
        } else {
            m_xi2Enabled = true;
        }
        if (m_xi2Enabled) {
#ifndef QT_NO_TABLETEVENT
            // Tablet support: Find the stylus-related devices.
            xi2SetupTabletDevices();
#endif // QT_NO_TABLETEVENT
#ifdef XI2_TOUCH_DEBUG
            qDebug("XInput version %d.%d is supported", xiMajor, m_xi2Minor);
#endif
        }
    }
}

void QXcbConnection::finalizeXInput2()
{
}

void QXcbConnection::xi2Select(xcb_window_t window)
{
    if (!m_xi2Enabled)
        return;

    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    unsigned int bitMask = 0;
    unsigned char *xiBitMask = reinterpret_cast<unsigned char *>(&bitMask);

#ifdef XCB_USE_XINPUT22
    // Select touch events on all master devices indiscriminately.
    bitMask |= XI_TouchBeginMask;
    bitMask |= XI_TouchUpdateMask;
    bitMask |= XI_TouchEndMask;
    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = sizeof(bitMask);
    mask.mask = xiBitMask;
    Status result = XISelectEvents(xDisplay, window, &mask, 1);
    // If we have XInput 2.2 and successfully enable touch on the master
    // devices, then evdev touchscreens will provide touch only. In most other
    // cases, there will be emulated mouse events, because true X11 touch
    // support is so new that for the older drivers, mouse emulation was the
    // only way; and it's still the fallback even with the modern evdev driver.
    // But if neither Qt nor X11 does mouse emulation, it will not be possible
    // to interact with mouse-oriented QWidgets; so we have to let Qt do it.
    if (m_xi2Minor >= 2 && result == Success)
        has_touch_without_mouse_emulation = true;
#endif

#ifndef QT_NO_TABLETEVENT
    // For each tablet, select some additional event types.
    // Press, motion, etc. events must never be selected for _all_ devices
    // as that would render the standard XCB_MOTION_NOTIFY and
    // similar handlers useless and we have no intention to infect
    // all the pure xcb code with Xlib-based XI2.
    if (!m_tabletData.isEmpty()) {
        QVector<XIEventMask> xiEventMask(m_tabletData.count());
        bitMask |= XI_ButtonPressMask;
        bitMask |= XI_ButtonReleaseMask;
        bitMask |= XI_MotionMask;
        bitMask |= XI_PropertyEventMask;
        for (int i = 0; i < m_tabletData.count(); ++i) {
            xiEventMask[i].deviceid = m_tabletData.at(i).deviceId;
            xiEventMask[i].mask_len = sizeof(bitMask);
            xiEventMask[i].mask = xiBitMask;
        }
        XISelectEvents(xDisplay, window, xiEventMask.data(), m_tabletData.count());
    }
#endif // QT_NO_TABLETEVENT
}

XInput2DeviceData *QXcbConnection::deviceForId(int id)
{
    XInput2DeviceData *dev = m_touchDevices[id];
    if (!dev) {
        int unused = 0;
        QTouchDevice::Capabilities caps = 0;
        dev = new XInput2DeviceData;
        dev->xiDeviceInfo = XIQueryDevice(static_cast<Display *>(m_xlib_display), id, &unused);
        dev->qtTouchDevice = new QTouchDevice;
        for (int i = 0; i < dev->xiDeviceInfo->num_classes; ++i) {
            XIAnyClassInfo *classinfo = dev->xiDeviceInfo->classes[i];
            switch (classinfo->type) {
#ifdef XCB_USE_XINPUT22
            case XITouchClass: {
                XITouchClassInfo *tci = reinterpret_cast<XITouchClassInfo *>(classinfo);
                switch (tci->mode) {
                case XIModeRelative:
                    dev->qtTouchDevice->setType(QTouchDevice::TouchPad);
                    break;
                case XIModeAbsolute:
                    dev->qtTouchDevice->setType(QTouchDevice::TouchScreen);
                    break;
                }
            } break;
#endif
            case XIValuatorClass: {
                XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(classinfo);
                if (vci->label == atom(QXcbAtom::AbsMTPositionX))
                    caps |= QTouchDevice::Position | QTouchDevice::NormalizedPosition;
                else if (vci->label == atom(QXcbAtom::AbsMTTouchMajor))
                    caps |= QTouchDevice::Area;
                else if (vci->label == atom(QXcbAtom::AbsMTPressure) || vci->label == atom(QXcbAtom::AbsPressure))
                    caps |= QTouchDevice::Pressure;
            } break;
            }
        }
        dev->qtTouchDevice->setCapabilities(caps);
        dev->qtTouchDevice->setName(dev->xiDeviceInfo->name);
        if (caps != 0)
            QWindowSystemInterface::registerTouchDevice(dev->qtTouchDevice);
#ifdef XI2_TOUCH_DEBUG
        qDebug("registered new device %s with %d classes and %d max touch points",
            dev->xiDeviceInfo->name, dev->xiDeviceInfo->num_classes, dev->qtTouchDevice->maxTouchPoints());
#endif
        m_touchDevices[id] = dev;
    }
    return dev;
}

#ifdef XCB_USE_XINPUT22
static qreal fixed1616ToReal(FP1616 val)
{
    return (qreal(val >> 16)) + (val & 0xFF) / (qreal)0xFF;
}

static qreal valuatorNormalized(double value, XIValuatorClassInfo *vci)
{
    if (value > vci->max)
        value = vci->max;
    if (value < vci->min)
        value = vci->min;
    return (value - vci->min) / (vci->max - vci->min);
}
#endif

void QXcbConnection::xi2HandleEvent(xcb_ge_event_t *event)
{
    if (xi2PrepareXIGenericDeviceEvent(event, m_xiOpCode)) {
        xXIGenericDeviceEvent *xiEvent = reinterpret_cast<xXIGenericDeviceEvent *>(event);

#ifndef QT_NO_TABLETEVENT
        for (int i = 0; i < m_tabletData.count(); ++i) {
            if (m_tabletData.at(i).deviceId == xiEvent->deviceid) {
                if (xi2HandleTabletEvent(xiEvent, &m_tabletData[i]))
                    return;
            }
        }
#endif // QT_NO_TABLETEVENT

#ifdef XCB_USE_XINPUT22
        if (xiEvent->evtype == XI_TouchBegin || xiEvent->evtype == XI_TouchUpdate || xiEvent->evtype == XI_TouchEnd) {
            xXIDeviceEvent* xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
#ifdef XI2_TOUCH_DEBUG
            qDebug("XI2 event type %d seq %d detail %d pos 0x%X,0x%X %f,%f root pos %f,%f",
                event->event_type, xiEvent->sequenceNumber, xiDeviceEvent->detail,
                xiDeviceEvent->event_x, xiDeviceEvent->event_y,
                fixed1616ToReal(xiDeviceEvent->event_x), fixed1616ToReal(xiDeviceEvent->event_y),
                fixed1616ToReal(xiDeviceEvent->root_x), fixed1616ToReal(xiDeviceEvent->root_y) );
#endif

            if (QXcbWindow *platformWindow = platformWindowFromId(xiDeviceEvent->event)) {
                XInput2DeviceData *dev = deviceForId(xiEvent->deviceid);
                if (xiEvent->evtype == XI_TouchBegin) {
                    QWindowSystemInterface::TouchPoint tp;
                    tp.id = xiDeviceEvent->detail % INT_MAX;
                    tp.state = Qt::TouchPointPressed;
                    tp.pressure = -1.0;
                    m_touchPoints[tp.id] = tp;
                }
                QWindowSystemInterface::TouchPoint &touchPoint = m_touchPoints[xiDeviceEvent->detail];
                qreal x = fixed1616ToReal(xiDeviceEvent->root_x);
                qreal y = fixed1616ToReal(xiDeviceEvent->root_y);
                qreal nx = -1.0, ny = -1.0, w = 0.0, h = 0.0;
                QXcbScreen* screen = m_screens.at(0);
                for (int i = 0; i < dev->xiDeviceInfo->num_classes; ++i) {
                    XIAnyClassInfo *classinfo = dev->xiDeviceInfo->classes[i];
                    if (classinfo->type == XIValuatorClass) {
                        XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(classinfo);
                        int n = vci->number;
                        double value;
                        if (!xi2GetValuatorValueIfSet(xiDeviceEvent, n, &value))
                            continue;
#ifdef XI2_TOUCH_DEBUG
                        qDebug("   valuator class label %d value %lf from range %lf -> %lf name %s",
                            vci->label, value, vci->min, vci->max, XGetAtomName(static_cast<Display *>(m_xlib_display), vci->label) );
#endif
                        if (vci->label == atom(QXcbAtom::AbsMTPositionX)) {
                            nx = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::AbsMTPositionY)) {
                            ny = valuatorNormalized(value, vci);
                        } else if (vci->label == atom(QXcbAtom::AbsMTTouchMajor)) {
                            // Convert the value within its range as a fraction of a finger's max (contact patch)
                            //  width in mm, and from there to pixels depending on screen resolution
                            w = valuatorNormalized(value, vci) * FINGER_MAX_WIDTH_MM *
                                screen->geometry().width() / screen->physicalSize().width();
                        } else if (vci->label == atom(QXcbAtom::AbsMTTouchMinor)) {
                            h = valuatorNormalized(value, vci) * FINGER_MAX_WIDTH_MM *
                                screen->geometry().height() / screen->physicalSize().height();
                        } else if (vci->label == atom(QXcbAtom::AbsMTPressure) ||
                                   vci->label == atom(QXcbAtom::AbsPressure)) {
                            touchPoint.pressure = valuatorNormalized(value, vci);
                        }
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
                if (xiEvent->evtype != XI_TouchEnd) {
                    if (w == 0.0)
                        w = touchPoint.area.width();
                    if (h == 0.0)
                        h = touchPoint.area.height();
                }

                switch (xiEvent->evtype) {
                case XI_TouchUpdate:
                    if (touchPoint.area.center() != QPoint(x, y))
                        touchPoint.state = Qt::TouchPointMoved;
                    else
                        touchPoint.state = Qt::TouchPointStationary;
                    break;
                case XI_TouchEnd:
                    touchPoint.state = Qt::TouchPointReleased;
                }
                touchPoint.area = QRectF(x - w/2, y - h/2, w, h);
                touchPoint.normalPosition = QPointF(nx, ny);

#ifdef XI2_TOUCH_DEBUG
                qDebug() << "   tp "  << touchPoint.id << " state " << touchPoint.state << " pos norm " << touchPoint.normalPosition <<
                    " area " << touchPoint.area << " pressure " << touchPoint.pressure;
#endif
                QWindowSystemInterface::handleTouchEvent(platformWindow->window(), xiEvent->time, dev->qtTouchDevice, m_touchPoints.values());
                // If a touchpoint was released, we can forget it, because the ID won't be reused.
                if (touchPoint.state == Qt::TouchPointReleased)
                    m_touchPoints.remove(touchPoint.id);
            }
        }
#endif
    }
}

#ifndef QT_NO_TABLETEVENT
void QXcbConnection::xi2QueryTabletData(void *dev, TabletData *tabletData)
{
    XIDeviceInfo *device = static_cast<XIDeviceInfo *>(dev);
    tabletData->deviceId = device->deviceid;

    tabletData->pointerType = QTabletEvent::Pen;
    if (QByteArray(device->name).toLower().contains("eraser"))
        tabletData->pointerType = QTabletEvent::Eraser;

    for (int i = 0; i < device->num_classes; ++i) {
        switch (device->classes[i]->type) {
        case XIValuatorClass: {
            XIValuatorClassInfo *vci = reinterpret_cast<XIValuatorClassInfo *>(device->classes[i]);
            int val = 0;
            if (vci->label == atom(QXcbAtom::AbsX))
                val = QXcbAtom::AbsX;
            else if (vci->label == atom(QXcbAtom::AbsY))
                val = QXcbAtom::AbsY;
            else if (vci->label == atom(QXcbAtom::AbsPressure))
                val = QXcbAtom::AbsPressure;
            else if (vci->label == atom(QXcbAtom::AbsTiltX))
                val = QXcbAtom::AbsTiltX;
            else if (vci->label == atom(QXcbAtom::AbsTiltY))
                val = QXcbAtom::AbsTiltY;
            else if (vci->label == atom(QXcbAtom::AbsWheel))
                val = QXcbAtom::AbsWheel;
            else if (vci->label == atom(QXcbAtom::AbsDistance))
                val = QXcbAtom::AbsDistance;
            if (val) {
                TabletData::ValuatorClassInfo info;
                info.minVal = vci->min;
                info.maxVal = vci->max;
                info.number = vci->number;
                tabletData->valuatorInfo[val] = info;
            }
        }
            break;
        default:
            break;
        }
    }
}

void QXcbConnection::xi2SetupTabletDevices()
{
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    m_tabletData.clear();
    int deviceCount = 0;
    XIDeviceInfo *devices = XIQueryDevice(xDisplay, XIAllDevices, &deviceCount);
    if (devices) {
        for (int i = 0; i < deviceCount; ++i) {
            int unused = 0;
            XIDeviceInfo *dev = XIQueryDevice(xDisplay, devices[i].deviceid, &unused);
            if (dev) {
                if (q_xi2_is_tablet(dev)) {
                    TabletData tabletData;
                    xi2QueryTabletData(dev, &tabletData);
#ifdef XI2_TOUCH_DEBUG
                    qDebug() << "found tablet" << dev->name;
#endif
                    m_tabletData.append(tabletData);
                }
                XIFreeDeviceInfo(dev);
            }
        }
        XIFreeDeviceInfo(devices);
    }
}

bool QXcbConnection::xi2HandleTabletEvent(void *event, TabletData *tabletData)
{
    bool handled = true;
    Display *xDisplay = static_cast<Display *>(m_xlib_display);
    xXIGenericDeviceEvent *xiEvent = static_cast<xXIGenericDeviceEvent *>(event);
    switch (xiEvent->evtype) {
    case XI_ButtonPress: // stylus down
        if (reinterpret_cast<xXIDeviceEvent *>(event)->detail == 1) { // ignore the physical buttons on the stylus
            tabletData->down = true;
            xi2ReportTabletEvent(*tabletData, xiEvent);
        }
        break;
    case XI_ButtonRelease: // stylus up
        if (reinterpret_cast<xXIDeviceEvent *>(event)->detail == 1) {
            tabletData->down = false;
            xi2ReportTabletEvent(*tabletData, xiEvent);
        }
        break;
    case XI_Motion:
        // Report TabletMove only when the stylus is touching the tablet.
        // No possibility to report proximity motion (no suitable Qt event exists yet).
        if (tabletData->down)
            xi2ReportTabletEvent(*tabletData, xiEvent);
        break;
    case XI_PropertyEvent: {
        xXIPropertyEvent *ev = reinterpret_cast<xXIPropertyEvent *>(event);
        if (ev->what == XIPropertyModified) {
            if (ev->property == atom(QXcbAtom::WacomSerialIDs)) {
                Atom propType;
                int propFormat;
                unsigned long numItems, bytesAfter;
                unsigned char *data;
                if (XIGetProperty(xDisplay, tabletData->deviceId, ev->property, 0, 100,
                                  0, AnyPropertyType, &propType, &propFormat,
                                  &numItems, &bytesAfter, &data) == Success) {
                    if (propType == atom(QXcbAtom::INTEGER) && propFormat == 32) {
                        int *ptr = reinterpret_cast<int *>(data);
                        for (unsigned long i = 0; i < numItems; ++i)
                            tabletData->serialId |= qint64(ptr[i]) << (i * 32);
                    }
                    XFree(data);
                }
                // With recent-enough X drivers this property change event seems to come always
                // when entering and leaving proximity. Due to the lack of other options hook up
                // the enter/leave events to it.
                if (tabletData->inProximity) {
                    tabletData->inProximity = false;
                    QWindowSystemInterface::handleTabletLeaveProximityEvent(QTabletEvent::Stylus,
                                                                            tabletData->pointerType,
                                                                            tabletData->serialId);
                } else {
                    tabletData->inProximity = true;
                    QWindowSystemInterface::handleTabletEnterProximityEvent(QTabletEvent::Stylus,
                                                                            tabletData->pointerType,
                                                                            tabletData->serialId);
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

void QXcbConnection::xi2ReportTabletEvent(const TabletData &tabletData, void *event)
{
    xXIDeviceEvent *ev = reinterpret_cast<xXIDeviceEvent *>(event);
    QXcbWindow *xcbWindow = platformWindowFromId(ev->event);
    if (!xcbWindow)
        return;
    QWindow *window = xcbWindow->window();
    const double scale = 65536.0;
    QPointF local(ev->event_x / scale, ev->event_y / scale);
    QPointF global(ev->root_x / scale, ev->root_y / scale);
    double pressure = 0, rotation = 0;
    int xTilt = 0, yTilt = 0;

    for (QHash<int, TabletData::ValuatorClassInfo>::const_iterator it = tabletData.valuatorInfo.constBegin(),
            ite = tabletData.valuatorInfo.constEnd(); it != ite; ++it) {
        int valuator = it.key();
        const TabletData::ValuatorClassInfo &classInfo(it.value());
        double value;
        if (xi2GetValuatorValueIfSet(event, classInfo.number, &value)) {
            double normalizedValue = (value - classInfo.minVal) / double(classInfo.maxVal - classInfo.minVal);
            switch (valuator) {
            case QXcbAtom::AbsPressure:
                pressure = normalizedValue;
                break;
            case QXcbAtom::AbsTiltX:
                xTilt = value;
                break;
            case QXcbAtom::AbsTiltY:
                yTilt = value;
                break;
            case QXcbAtom::AbsWheel:
                rotation = value / 64.0;
                break;
            default:
                break;
            }
        }
    }

    QWindowSystemInterface::handleTabletEvent(window, tabletData.down, local, global,
                                              QTabletEvent::Stylus, tabletData.pointerType,
                                              pressure, xTilt, yTilt, 0,
                                              rotation, 0, tabletData.serialId);
}
#endif // QT_NO_TABLETEVENT

#endif // XCB_USE_XINPUT2
