// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include "qxcbobject.h"
#include "qxcbconnection.h"
#include <qlist.h>

QT_BEGIN_NAMESPACE

class QXcbWMSupport : public QXcbObject
{
public:
    QXcbWMSupport(QXcbConnection *c);


    bool isSupportedByWM(xcb_atom_t atom) const;
    const QList<xcb_window_t> &virtualRoots() const { return net_virtual_roots; }

private:
    friend class QXcbConnection;
    void updateNetWMAtoms();
    void updateVirtualRoots();

    QList<xcb_atom_t> net_wm_atoms;
    QList<xcb_window_t> net_virtual_roots;
};

QT_END_NAMESPACE
