// Copyright (C) 2026 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandcutouts_p.h"
#include "qwaylandwindow_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

using namespace QtWaylandClient;

Cutouts::Cutouts(::xx_cutouts_v1 *object, QWaylandWindow *window)
    : QtWayland::xx_cutouts_v1(object)
    , m_window(window)
{
}

void Cutouts::xx_cutouts_v1_cutout_box(int32_t x, int32_t y, int32_t width, int32_t height, [[maybe_unused]] uint32_t type, [[maybe_unused]] uint32_t id)
{
    // compute the minimum screen area we would lose on each side, and pick the minimum one
    // this produces optimal/expected results also for less-obvious cases like square cutouts towards
    // a screen corner, but you can find a set of multiple cutouts where it wont, as it doesn't do
    // a global optimization. That's not a practically relevant scenario so far though.
    std::array<int32_t, 4> areas; // top, left, bottom, right
    areas[0] = (y + height) * m_window->window()->width();
    areas[1] = (x + width) * m_window->window()->height();
    areas[2] = (m_window->window()->height() - y) * m_window->window()->width();
    areas[3] = (m_window->window()->width() - x) * m_window->window()->height();
    const auto minSide = std::distance(areas.begin(), std::min_element(areas.begin(), areas.end()));

    switch (minSide) {
    case 0:
        m_margins.setTop(std::max(m_margins.top(), y + height));
        break;
    case 1:
        m_margins.setLeft(std::max(m_margins.left(), x + width));
        break;
    case 2:
        m_margins.setBottom(std::max(m_margins.bottom(), y));
        break;
    case 3:
        m_margins.setRight(std::max(m_margins.right(), x));
        break;
    }
}

void Cutouts::xx_cutouts_v1_cutout_corner([[maybe_unused]] uint32_t position, [[maybe_unused]] uint32_t radius, uint32_t id)
{
    // corners are currently not considered as the impact on screen area
    // loss is disproportianl to the risk of covering relevant application details.
    // This matches behavior observed on Android.
    // If we want to change this we should also do global optimziation over all cutouts,
    // as a notch on one side can effectively cover corners already as well, reducing
    // the amount of margin needed on their respective other sides.

    m_unhandled.push_back(id);
}

void Cutouts::xx_cutouts_v1_configure()
{
    m_window->setSafeAreaMargins(m_margins);
    m_margins = {};

    if (!m_unhandled.empty()) {
        const auto data = QByteArray::fromRawData(reinterpret_cast<const char*>(m_unhandled.data()), sizeof(uint32_t) * m_unhandled.size());
        set_unhandled(data);
        m_unhandled.clear();
    }
}

QT_END_NAMESPACE
