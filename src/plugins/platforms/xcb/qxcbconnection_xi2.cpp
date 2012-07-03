/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbconnection.h"
#include "qxcbwindow.h"
#include <qpa/qwindowsysteminterface.h>

#ifdef XCB_USE_XINPUT2

#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>

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
            // Tablet support: Figure out the stylus-related devices. We will
            // only select events on this device. Press, motion, etc. events
            // must never be selected for _all_ devices as that would render
            // the standard XCB_MOTION_NOTIFY and similar handlers useless and
            // we have no intention to infect all the pure xcb code with
            // Xlib-based XI2.
            xi2SetupTabletDevices();
#endif // QT_NO_TABLETEVENT
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

#ifndef QT_NO_TABLETEVENT
    // Tablets.
    if (!m_tabletData.isEmpty()) {
        Display *xDisplay = static_cast<Display *>(m_xlib_display);
        QVector<XIEventMask> xiEventMask(m_tabletData.count());
        unsigned int bitMask = 0;
        unsigned char *xiBitMask = reinterpret_cast<unsigned char *>(&bitMask);
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

void QXcbConnection::xi2HandleEvent(xcb_ge_event_t *event)
{
    if (xi2PrepareXIGenericDeviceEvent(event, m_xiOpCode)) {
        xXIGenericDeviceEvent *xiEvent = reinterpret_cast<xXIGenericDeviceEvent *>(event);
#ifndef QT_NO_TABLETEVENT
        for (int i = 0; i < m_tabletData.count(); ++i) {
            if (m_tabletData.at(i).deviceId == xiEvent->deviceid) {
                xi2HandleTabletEvent(xiEvent, &m_tabletData[i]);
                return;
            }
        }
#endif // QT_NO_TABLETEVENT
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
                    m_tabletData.append(tabletData);
                }
                XIFreeDeviceInfo(dev);
            }
        }
        XIFreeDeviceInfo(devices);
    }
}

void QXcbConnection::xi2HandleTabletEvent(void *event, TabletData *tabletData)
{
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
        // No possiblity to report proximity motion (no suitable Qt event exists yet).
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
        break;
    }
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
