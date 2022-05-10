// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBOBJECT_H
#define QXCBOBJECT_H

#include "qxcbconnection.h"

QT_BEGIN_NAMESPACE

class QXcbObject
{
public:
    QXcbObject(QXcbConnection *connection = nullptr) : m_connection(connection) {}

    void setConnection(QXcbConnection *connection) { m_connection = connection; }
    QXcbConnection *connection() const { return m_connection; }

    xcb_atom_t atom(QXcbAtom::Atom atom) const { return m_connection->atom(atom); }
    xcb_connection_t *xcb_connection() const { return m_connection->xcb_connection(); }

private:
    QXcbConnection *m_connection;
};

QT_END_NAMESPACE

#endif
