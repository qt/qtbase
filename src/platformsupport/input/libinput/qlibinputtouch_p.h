/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QLIBINPUTTOUCH_P_H
#define QLIBINPUTTOUCH_P_H

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <qpa/qwindowsysteminterface.h>

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
        QTouchDevice *m_touchDevice;
        QString m_screenName;
    };

    DeviceState *deviceState(libinput_event_touch *e);
    QPointF getPos(libinput_event_touch *e);

    QHash<libinput_device *, DeviceState> m_devState;
    mutable QPointer<QScreen> m_screen;
};

QT_END_NAMESPACE

#endif
