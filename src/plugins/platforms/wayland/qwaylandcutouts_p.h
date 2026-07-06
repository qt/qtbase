// Copyright (C) 2026 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDCUTOUTS_H
#define QWAYLANDCUTOUTS_H

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

#include <QtWaylandClient/private/qwayland-xx-cutouts-v1.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandWindow;

class Cutouts : public QtWayland::xx_cutouts_v1
{
public:
    Cutouts(::xx_cutouts_v1 *object, QWaylandWindow *window);

protected:
    void xx_cutouts_v1_cutout_box(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t type, uint32_t id) override;
    void xx_cutouts_v1_cutout_corner(uint32_t position, uint32_t radius, uint32_t id) override;
    void xx_cutouts_v1_configure() override;

private:
    QWaylandWindow *m_window;
    std::vector<uint32_t> m_unhandled;
    QMargins m_margins;
};

}

QT_END_NAMESPACE

#endif
