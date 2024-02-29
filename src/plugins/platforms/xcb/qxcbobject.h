// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

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
