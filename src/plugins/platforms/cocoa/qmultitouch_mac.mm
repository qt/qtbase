// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qmultitouch_mac_p.h"
#include "qcocoahelpers.h"
#include "qcocoascreen.h"
#include <private/qpointingdevice_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_CONSTINIT QHash<qint64, QCocoaTouch*> QCocoaTouch::_currentTouches;
Q_CONSTINIT QHash<quint64, QPointingDevice*> QCocoaTouch::_touchDevices;
Q_CONSTINIT QPointF QCocoaTouch::_screenReferencePos;
Q_CONSTINIT QPointF QCocoaTouch::_trackpadReferencePos;
int QCocoaTouch::_idAssignmentCount = 0;
int QCocoaTouch::_touchCount = 0;
bool QCocoaTouch::_updateInternalStateOnly = true;

QCocoaTouch::QCocoaTouch(NSTouch *nstouch)
{
    if (_currentTouches.size() == 0)
        _idAssignmentCount = 0;

    _touchPoint.id = _idAssignmentCount++;
    _touchPoint.pressure = 1.0;
    _identity = qint64([nstouch identity]);
    _currentTouches.insert(_identity, this);
    updateTouchData(nstouch, NSTouchPhaseBegan);
}

QCocoaTouch::~QCocoaTouch()
{
    _currentTouches.remove(_identity);
}

void QCocoaTouch::updateTouchData(NSTouch *nstouch, NSTouchPhase phase)
{
    _touchPoint.state = toTouchPointState(phase);

    // From the normalized position on the trackpad, calculate
    // where on screen the touchpoint should be according to the
    // reference position:
    NSPoint npos = [nstouch normalizedPosition];
    QPointF qnpos = QPointF(npos.x, 1 - npos.y);
    _touchPoint.normalPosition = qnpos;

    if (_touchPoint.id == 0 && phase == NSTouchPhaseBegan) {
        _trackpadReferencePos = qnpos;
        _screenReferencePos = QCocoaScreen::mapFromNative([NSEvent mouseLocation]);
    }

    QPointF screenPos = _screenReferencePos;

    NSSize dsize = [nstouch deviceSize];
    float ppiX = (qnpos.x() - _trackpadReferencePos.x()) * dsize.width;
    float ppiY = (qnpos.y() - _trackpadReferencePos.y()) * dsize.height;
    QPointF relativePos = _trackpadReferencePos - QPointF(ppiX, ppiY);
    screenPos -= relativePos;
    // Mac does not support area touch, only points, hence set width/height to 1.
    // The touch point is supposed to be in the center of '_touchPoint.area', and
    // since width/height is 1 it means we must subtract 0.5 from x and y.
    screenPos.rx() -= 0.5;
    screenPos.ry() -= 0.5;
    _touchPoint.area = QRectF(screenPos, QSize(1, 1));
}

QCocoaTouch *QCocoaTouch::findQCocoaTouch(NSTouch *nstouch)
{
    qint64 identity = qint64([nstouch identity]);
    if (_currentTouches.contains(identity))
        return _currentTouches.value(identity);
    return nullptr;
}

QEventPoint::State QCocoaTouch::toTouchPointState(NSTouchPhase nsState)
{
    QEventPoint::State qtState = QEventPoint::State::Released;
    switch (nsState) {
        case NSTouchPhaseBegan:
            qtState = QEventPoint::State::Pressed;
            break;
        case NSTouchPhaseMoved:
            qtState = QEventPoint::State::Updated;
            break;
        case NSTouchPhaseStationary:
            qtState = QEventPoint::State::Stationary;
            break;
        case NSTouchPhaseEnded:
        case NSTouchPhaseCancelled:
            qtState = QEventPoint::State::Released;
            break;
        default:
            break;
    }
    return qtState;
}

QList<QWindowSystemInterface::TouchPoint>
QCocoaTouch::getCurrentTouchPointList(NSEvent *event, bool acceptSingleTouch)
{
    QMap<int, QWindowSystemInterface::TouchPoint> touchPoints;
    NSSet *ended = [event touchesMatchingPhase:NSTouchPhaseEnded | NSTouchPhaseCancelled inView:nil];
    NSSet *active = [event
        touchesMatchingPhase:NSTouchPhaseBegan | NSTouchPhaseMoved | NSTouchPhaseStationary
        inView:nil];
    _touchCount = [active count];

    // First: remove touches that were ended by the user. If we are
    // currently not accepting single touches, a corresponding 'begin'
    // has never been send to the app for these events.
    // So should therefore not send the following removes either.

    for (int i=0; i<int([ended count]); ++i) {
        NSTouch *touch = [[ended allObjects] objectAtIndex:i];
        QCocoaTouch *qcocoaTouch = findQCocoaTouch(touch);
        if (qcocoaTouch) {
            qcocoaTouch->updateTouchData(touch, [touch phase]);
            if (!_updateInternalStateOnly)
                touchPoints.insert(qcocoaTouch->_touchPoint.id, qcocoaTouch->_touchPoint);
            delete qcocoaTouch;
        }
    }

    bool wasUpdateInternalStateOnly = _updateInternalStateOnly;
    _updateInternalStateOnly = !acceptSingleTouch && _touchCount < 2;

    // Next: update, or create, existing touches.
    // We always keep track of all touch points, even
    // when not accepting single touches.

    for (int i=0; i<int([active count]); ++i) {
        NSTouch *touch = [[active allObjects] objectAtIndex:i];
        QCocoaTouch *qcocoaTouch = findQCocoaTouch(touch);
        if (!qcocoaTouch)
            qcocoaTouch = new QCocoaTouch(touch);
        else
            qcocoaTouch->updateTouchData(touch, wasUpdateInternalStateOnly ? NSTouchPhaseBegan : [touch phase]);
        if (!_updateInternalStateOnly)
            touchPoints.insert(qcocoaTouch->_touchPoint.id, qcocoaTouch->_touchPoint);
    }

    // Next: sadly, we need to check that our touch hash is in
    // sync with cocoa. This is typically not the case after a system
    // gesture happened (like a four-finger-swipe to show expose).

    if (_touchCount != _currentTouches.size()) {
        // Remove all instances, and basically start from scratch:
        touchPoints.clear();
        // Deleting touch points will remove them from current touches,
        // so we make a copy of the touches before iterating them.
        const auto currentTouchesSnapshot = _currentTouches;
        for (QCocoaTouch *qcocoaTouch : currentTouchesSnapshot) {
            if (!_updateInternalStateOnly) {
                qcocoaTouch->_touchPoint.state = QEventPoint::State::Released;
                touchPoints.insert(qcocoaTouch->_touchPoint.id, qcocoaTouch->_touchPoint);
            }
            delete qcocoaTouch;
        }
        _currentTouches.clear();
        _updateInternalStateOnly = !acceptSingleTouch;
        return touchPoints.values();
    }

    // Finally: If this call _started_ to reject single
    // touches, we need to fake a release for the remaining
    // touch now (and refake a begin for it later, if needed).

    if (_updateInternalStateOnly && !wasUpdateInternalStateOnly && !_currentTouches.isEmpty()) {
        QCocoaTouch *qcocoaTouch = _currentTouches.cbegin().value();
        qcocoaTouch->_touchPoint.state = QEventPoint::State::Released;
        touchPoints.insert(qcocoaTouch->_touchPoint.id, qcocoaTouch->_touchPoint);
        // Since this last touch also will end up being the first
        // touch (if the user adds a second finger without lifting
        // the first), we promote it to be the primary touch:
        qcocoaTouch->_touchPoint.id = 0;
        _idAssignmentCount = 1;
    }

    return touchPoints.values();
}

QPointingDevice *QCocoaTouch::getTouchDevice(QInputDevice::DeviceType type, quint64 id)
{
    QPointingDevice *ret = _touchDevices.value(id);
    if (!ret) {
        ret = new QPointingDevice(type == QInputDevice::DeviceType::TouchScreen ? "touchscreen"_L1 : "trackpad"_L1,
                                  id, type, QPointingDevice::PointerType::Finger,
                                  QInputDevice::Capability::Position |
                                  QInputDevice::Capability::NormalizedPosition |
                                  QInputDevice::Capability::MouseEmulation,
                                  10, 0);
        QWindowSystemInterface::registerInputDevice(ret);
        _touchDevices.insert(id, ret);
    }
    return ret;
}

QT_END_NAMESPACE
