/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
