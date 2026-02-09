// Copyright (C) 2023 David Reondo <kde@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandxdgdialogv1_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandXdgDialogV1::QWaylandXdgDialogV1(::xdg_dialog_v1 *object) : xdg_dialog_v1(object) { }

QWaylandXdgDialogV1::~QWaylandXdgDialogV1()
{
    xdg_dialog_v1_destroy(object());
}

QWaylandXdgDialogWmV1::QWaylandXdgDialogWmV1(wl_registry *registry, uint32_t id, int version)
    : xdg_wm_dialog_v1(registry, id, version)
{
}

QWaylandXdgDialogWmV1::~QWaylandXdgDialogWmV1()
{
    destroy();
}
QWaylandXdgDialogV1 *QWaylandXdgDialogWmV1::getDialog(xdg_toplevel *toplevel)
{
    return new QWaylandXdgDialogV1(get_xdg_dialog(toplevel));
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
