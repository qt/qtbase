// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWAYLANDPOINTERGESTURES_P_H
#define QWAYLANDPOINTERGESTURES_P_H

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

#include <QtWaylandClient/private/qwayland-pointer-gestures-unstable-v1.h>

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandSurface;
class QWaylandInputDevice;
class QWaylandPointerGestureSwipe;
class QWaylandPointerGesturePinch;

class Q_WAYLANDCLIENT_EXPORT QWaylandPointerGestures : public QtWayland::zwp_pointer_gestures_v1
{
public:
    explicit QWaylandPointerGestures(QWaylandDisplay *display, uint id, uint version);
    ~QWaylandPointerGestures();

    QWaylandPointerGestureSwipe *createPointerGestureSwipe(QWaylandInputDevice *device);
    QWaylandPointerGesturePinch *createPointerGesturePinch(QWaylandInputDevice *device);

private:
    Q_DISABLE_COPY(QWaylandPointerGestures)
};

class Q_WAYLANDCLIENT_EXPORT QWaylandPointerGestureSwipe :
        public QtWayland::zwp_pointer_gesture_swipe_v1
{
public:
    QWaylandPointerGestureSwipe(QWaylandInputDevice *p);
    ~QWaylandPointerGestureSwipe() override;

    void zwp_pointer_gesture_swipe_v1_begin(uint32_t serial,
                                            uint32_t time,
                                            struct ::wl_surface *surface,
                                            uint32_t fingers) override;

    void zwp_pointer_gesture_swipe_v1_update(uint32_t time,
                                             wl_fixed_t dx,
                                             wl_fixed_t dy) override;

    void zwp_pointer_gesture_swipe_v1_end(uint32_t serial,
                                          uint32_t time,
                                          int32_t cancelled) override;

    struct ::zwp_pointer_gesture_swipe_v1 *zwp_pointer_gesture_swipe_v1()
    {
        return QtWayland::zwp_pointer_gesture_swipe_v1::object();
    }

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandSurface> mFocus;
    uint mFingers = 0;

private:
    Q_DISABLE_COPY(QWaylandPointerGestureSwipe)
};

class Q_WAYLANDCLIENT_EXPORT QWaylandPointerGesturePinch :
        public QtWayland::zwp_pointer_gesture_pinch_v1
{
public:
    QWaylandPointerGesturePinch(QWaylandInputDevice *p);
    ~QWaylandPointerGesturePinch() override;

    void zwp_pointer_gesture_pinch_v1_begin(uint32_t serial,
                                            uint32_t time,
                                            struct ::wl_surface *surface,
                                            uint32_t fingers) override;

    void zwp_pointer_gesture_pinch_v1_update(uint32_t time,
                                             wl_fixed_t dx,
                                             wl_fixed_t dy,
                                             wl_fixed_t scale,
                                             wl_fixed_t rotation) override;

    void zwp_pointer_gesture_pinch_v1_end(uint32_t serial,
                                          uint32_t time,
                                          int32_t cancelled) override;

    struct ::zwp_pointer_gesture_pinch_v1 *zwp_pointer_gesture_pinch_v1()
    {
        return QtWayland::zwp_pointer_gesture_pinch_v1::object();
    }

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandSurface> mFocus;
    uint mFingers = 0;

    // We need to convert between absolute scale provided by wayland/libinput and zoom deltas
    // that Qt expects. This stores the scale of the last pinch event or 1.0 if there was none.
    qreal mLastScale = 1;
private:
    Q_DISABLE_COPY(QWaylandPointerGesturePinch)
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDPOINTERGESTURES_P_H
