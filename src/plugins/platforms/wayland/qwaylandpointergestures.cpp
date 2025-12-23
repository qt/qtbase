// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandpointergestures_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandsurface_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandPointerGestures::QWaylandPointerGestures(QWaylandDisplay *display, uint id, uint version)
    : zwp_pointer_gestures_v1(display->wl_registry(), id, qMin(version, uint(1)))
{
}

QWaylandPointerGestures::~QWaylandPointerGestures() noexcept
{
    if (version() >= ZWP_POINTER_GESTURES_V1_RELEASE_SINCE_VERSION)
        release();
    else
        zwp_pointer_gestures_v1_destroy(object());
}

QWaylandPointerGestureSwipe *
    QWaylandPointerGestures::createPointerGestureSwipe(QWaylandInputDevice *device)
{
    return new QWaylandPointerGestureSwipe(device);
}

QWaylandPointerGesturePinch *
    QWaylandPointerGestures::createPointerGesturePinch(QWaylandInputDevice *device)
{
    return new QWaylandPointerGesturePinch(device);
}

QWaylandPointerGestureSwipe::QWaylandPointerGestureSwipe(QWaylandInputDevice *p)
    : mParent(p)
{
}

QWaylandPointerGestureSwipe::~QWaylandPointerGestureSwipe()
{
    destroy();
}

void QWaylandPointerGestureSwipe::zwp_pointer_gesture_swipe_v1_begin(uint32_t serial, uint32_t time,
                                                                     struct ::wl_surface *surface,
                                                                     uint32_t fingers)
{
#ifndef QT_NO_GESTURES
    mFocus = QWaylandSurface::fromWlSurface(surface);
    if (!mFocus || !mFocus->waylandWindow()) {
        return;
    }
    mParent->mSerial = serial;
    mFingers = fingers;

    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_begin @ "
                               << pointer->mSurfacePos << "fingers" << fingers;

    auto e = QWaylandPointerGestureSwipeEvent(mFocus, Qt::GestureStarted, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF());

    mFocus->waylandWindow()->handleSwipeGesture(mParent, e);
#endif
}

void QWaylandPointerGestureSwipe::zwp_pointer_gesture_swipe_v1_update(uint32_t time,
                                                                      wl_fixed_t dx, wl_fixed_t dy)
{
#ifndef QT_NO_GESTURES
    if (!mFocus || !mFocus->waylandWindow()) {
        return;
    }
    const auto* pointer = mParent->pointer();

    const QPointF delta = QPointF(wl_fixed_to_double(dx), wl_fixed_to_double(dy));
    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_update @ "
                               << pointer->mSurfacePos << "delta" << delta;

    auto e = QWaylandPointerGestureSwipeEvent(mFocus, Qt::GestureUpdated, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers, delta);

    mFocus->waylandWindow()->handleSwipeGesture(mParent, e);
#endif
}

void QWaylandPointerGestureSwipe::zwp_pointer_gesture_swipe_v1_end(uint32_t serial, uint32_t time,
                                                                   int32_t cancelled)
{
#ifndef QT_NO_GESTURES
    if (!mFocus || !mFocus->waylandWindow()) {
        return;
    }
    mParent->mSerial = serial;
    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_end @ "
                               << pointer->mSurfacePos << (cancelled ? "CANCELED" : "");

    auto gestureType = cancelled ? Qt::GestureFinished : Qt::GestureCanceled;

    auto e = QWaylandPointerGestureSwipeEvent(mFocus, gestureType, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF());

    mFocus->waylandWindow()->handleSwipeGesture(mParent, e);

    mFocus.clear();
    mFingers = 0;
#endif
}

QWaylandPointerGesturePinch::QWaylandPointerGesturePinch(QWaylandInputDevice *p)
    : mParent(p)
{
}

QWaylandPointerGesturePinch::~QWaylandPointerGesturePinch()
{
    destroy();
}

void QWaylandPointerGesturePinch::zwp_pointer_gesture_pinch_v1_begin(uint32_t serial, uint32_t time,
                                                                     struct ::wl_surface *surface,
                                                                     uint32_t fingers)
{
#ifndef QT_NO_GESTURES
    mFocus = QWaylandSurface::fromWlSurface(surface);
    if (!mFocus || !mFocus->waylandWindow()) {
        return;
    }
    mParent->mSerial = serial;
    mFingers = fingers;
    mLastScale = 1;
    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_pinch_v1_begin @ "
                               << pointer->mSurfacePos << "fingers" << fingers;

    auto e = QWaylandPointerGesturePinchEvent(mFocus, Qt::GestureStarted, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF(), 0, 0);

    mFocus->waylandWindow()->handlePinchGesture(mParent, e);
#endif
}

void QWaylandPointerGesturePinch::zwp_pointer_gesture_pinch_v1_update(uint32_t time,
                                                                      wl_fixed_t dx, wl_fixed_t dy,
                                                                      wl_fixed_t scale,
                                                                      wl_fixed_t rotation)
{
#ifndef QT_NO_GESTURES
    if (!mFocus || !mFocus->waylandWindow()) {
        return;
    }
    const auto* pointer = mParent->pointer();

    const qreal rscale = wl_fixed_to_double(scale);
    const qreal rot = wl_fixed_to_double(rotation);
    const QPointF delta = QPointF(wl_fixed_to_double(dx), wl_fixed_to_double(dy));
    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_pinch_v1_update @ "
                               << pointer->mSurfacePos << "delta" << delta
                               << "scale" << mLastScale << "->" << rscale
                               << "delta" << rscale - mLastScale << "rot" << rot;

    auto e = QWaylandPointerGesturePinchEvent(mFocus, Qt::GestureUpdated, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              delta, rscale - mLastScale, rot);

    mFocus->waylandWindow()->handlePinchGesture(mParent, e);

    mLastScale = rscale;
#endif
}

void QWaylandPointerGesturePinch::zwp_pointer_gesture_pinch_v1_end(uint32_t serial, uint32_t time,
                                                                   int32_t cancelled)
{
#ifndef QT_NO_GESTURES
    if (!mFocus || !mFocus->waylandWindow()) {
        return;
    }
    mParent->mSerial = serial;
    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_end @ "
                               << pointer->mSurfacePos << (cancelled ? "CANCELED" : "");

    auto gestureType = cancelled ? Qt::GestureFinished : Qt::GestureCanceled;

    auto e = QWaylandPointerGesturePinchEvent(mFocus, gestureType, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF(), 0, 0);

    mFocus->waylandWindow()->handlePinchGesture(mParent, e);

    mFocus.clear();
    mFingers = 0;
    mLastScale = 1;
#endif
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
