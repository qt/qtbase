// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLIBINPUTTOUCH_P_H
#define QLIBINPUTTOUCH_P_H

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <qpa/qwindowsysteminterface.h>
#include <private/qglobal_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

struct libinput_event_touch;
struct libinput_device;

QT_BEGIN_NAMESPACE

class QScreen;
class QLibInputTouch
{
public:
    void registerDevice(libinput_device *dev);
    void unregisterDevice(libinput_device *dev);
    void processTouchDown(libinput_event_touch *e);
    void processTouchMotion(libinput_event_touch *e);
    void processTouchUp(libinput_event_touch *e);
    void processTouchCancel(libinput_event_touch *e);
    void processTouchFrame(libinput_event_touch *e);

private:
    struct DeviceState {
        DeviceState() : m_touchDevice(nullptr), m_screenName() { }
        QWindowSystemInterface::TouchPoint *point(int32_t slot);
        QList<QWindowSystemInterface::TouchPoint> m_points;
        QPointingDevice *m_touchDevice;
        QString m_screenName;
    };

    DeviceState *deviceState(libinput_event_touch *e);
    QRect screenGeometry(DeviceState *state);
    QPointF getPos(libinput_event_touch *e);

    QHash<libinput_device *, DeviceState> m_devState;
    mutable QPointer<QScreen> m_screen;
};

QT_END_NAMESPACE

#endif
