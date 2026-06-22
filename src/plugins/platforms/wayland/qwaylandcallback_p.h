// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWAYLANDCALLBACK_H
#define QWAYLANDCALLBACK_H

#include "qwayland-wayland.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class WlCallback : public QtWayland::wl_callback
{
public:
    explicit WlCallback(::wl_callback *callback, std::function<void(uint32_t)> fn)
        : QtWayland::wl_callback(callback), m_fn(fn)
    {
    }
    ~WlCallback() override { wl_callback_destroy(object()); }
    void callback_done(uint32_t callback_data) override { m_fn(callback_data); }

private:
    Q_DISABLE_COPY(WlCallback)

    std::function<void(uint32_t)> m_fn;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDCALLBACK_H
