// Copyright (C) 2022 David Reondo <kde@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <qwayland-xdg-dialog-v1.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandXdgDialogV1 : public QtWayland::xdg_dialog_v1
{
public:
    QWaylandXdgDialogV1(::xdg_dialog_v1 *object);
    ~QWaylandXdgDialogV1() override;
};

class QWaylandXdgDialogWmV1 : public QtWayland::xdg_wm_dialog_v1
{
public:
    QWaylandXdgDialogWmV1(wl_registry *registry, uint32_t id, int version);
    ~QWaylandXdgDialogWmV1() override;
    QWaylandXdgDialogV1 *getDialog(xdg_toplevel *toplevel);
};

} // namespace QtWaylandClient

QT_END_NAMESPACE
