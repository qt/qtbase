// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWAYLANDCURSORSURFACE_H
#define QWAYLANDCURSORSURFACE_H

#include "qwaylandsurface_p.h"
#include "qwaylandcallback_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

#if QT_CONFIG(cursor)
template <typename InputDevice>
class CursorSurface : public QWaylandSurface
{
public:
    explicit CursorSurface(InputDevice *pointer, QWaylandDisplay *display)
        : QWaylandSurface(display), m_pointer(pointer)
    {
        connect(this, &QWaylandSurface::screensChanged, m_pointer, &InputDevice::updateCursor);
    }

    void reset()
    {
        m_setSerial = 0;
        m_hotspot = QPoint();
    }

    // Size and hotspot are in surface coordinates
    void update(wl_buffer *buffer, const QPoint &hotspot, const QSize &size, int bufferScale,
                bool animated = false)
    {
        // Calling code needs to ensure buffer scale is supported if != 1
        Q_ASSERT(bufferScale == 1 || version() >= 3);

        auto enterSerial = m_pointer->mEnterSerial;
        if (m_setSerial < enterSerial || m_hotspot != hotspot) {
            m_pointer->set_cursor(m_pointer->mEnterSerial, object(), hotspot.x(), hotspot.y());
            m_setSerial = enterSerial;
            m_hotspot = hotspot;
        }

        if (version() >= 3)
            set_buffer_scale(bufferScale);

        attach(buffer, 0, 0);
        damage(0, 0, size.width(), size.height());
        m_frameCallback.reset();
        if (animated) {
            m_frameCallback.reset(new WlCallback(frame(), [this](uint32_t time) {
                Q_UNUSED(time);
                m_pointer->cursorFrameCallback();
            }));
        }
        commit();
    }

    int outputScale() const
    {
        int scale = 0;
        for (auto *screen : m_screens)
            scale = qMax(scale, screen->scale());
        return scale;
    }

private:
    QScopedPointer<WlCallback> m_frameCallback;
    InputDevice *m_pointer = nullptr;
    uint m_setSerial = 0;
    QPoint m_hotspot;
};

#endif // QT_CONFIG(cursor)

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDCURSORSURFACE_H
