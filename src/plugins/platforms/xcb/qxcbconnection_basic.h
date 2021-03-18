/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QXCBBASICCONNECTION_H
#define QXCBBASICCONNECTION_H

#include "qxcbatom.h"
#include "qxcbexport.h"

#include <QtCore/QPair>
#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtGui/private/qtguiglobal_p.h>

#include <xcb/xcb.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaXcb)

class Q_XCB_EXPORT QXcbBasicConnection : public QObject
{
    Q_OBJECT
public:
    QXcbBasicConnection(const char *displayName);
    ~QXcbBasicConnection();

#if QT_CONFIG(xcb_xlib)
    void *xlib_display() const { return m_xlibDisplay; }
#endif
    const char *displayName() const { return m_displayName.constData(); }
    int primaryScreenNumber() const { return m_primaryScreenNumber; }
    xcb_connection_t *xcb_connection() const { return m_xcbConnection; }
    bool isConnected() const {
        return m_xcbConnection && !xcb_connection_has_error(m_xcbConnection);
    }
    const xcb_setup_t *setup() const { return m_setup; }

    size_t maxRequestDataBytes(size_t requestSize) const;

    inline xcb_atom_t atom(QXcbAtom::Atom qatom) const { return m_xcbAtom.atom(qatom); }
    QXcbAtom::Atom qatom(xcb_atom_t atom) const { return m_xcbAtom.qatom(atom); }
    xcb_atom_t internAtom(const char *name);
    QByteArray atomName(xcb_atom_t atom);

    bool hasXFixes() const { return m_hasXFixes; }
    bool hasXShape() const { return m_hasXhape; }
    bool hasXRandr() const { return m_hasXRandr; }
    bool hasInputShape() const { return m_hasInputShape; }
    bool hasXKB() const { return m_hasXkb; }
    bool hasXRender(int major = -1, int minor = -1) const {
        if (m_hasXRender && major != -1 && minor != -1)
            return m_xrenderVersion >= qMakePair(major, minor);

        return m_hasXRender;
    }
    bool hasXInput2() const { return m_xi2Enabled; }
    bool hasShm() const { return m_hasShm; }
    bool hasShmFd() const { return m_hasShmFd; }
    bool hasXSync() const { return m_hasXSync; }
    bool hasXinerama() const { return m_hasXinerama; }
    bool hasBigRequest() const;

    bool isAtLeastXI21() const { return m_xi2Enabled && m_xi2Minor >= 1; }
    bool isAtLeastXI22() const { return m_xi2Enabled && m_xi2Minor >= 2; }
    bool isXIEvent(xcb_generic_event_t *event) const;
    bool isXIType(xcb_generic_event_t *event, uint16_t type) const;

    bool isXFixesType(uint responseType, int eventType) const;
    bool isXRandrType(uint responseType, int eventType) const;
    bool isXkbType(uint responseType) const; // https://bugs.freedesktop.org/show_bug.cgi?id=51295

protected:
    void initializeShm();
    void initializeXFixes();
    void initializeXRender();
    void initializeXRandr();
    void initializeXinerama();
    void initializeXShape();
    void initializeXKB();
    void initializeXSync();
    void initializeXInput2();

private:
#if QT_CONFIG(xcb_xlib)
    void *m_xlibDisplay = nullptr;
#endif
    QByteArray m_displayName;
    xcb_connection_t *m_xcbConnection = nullptr;
    int m_primaryScreenNumber = 0;
    const xcb_setup_t *m_setup = nullptr;
    QXcbAtom m_xcbAtom;

    bool m_hasXFixes = false;
    bool m_hasXinerama = false;
    bool m_hasXhape = false;
    bool m_hasInputShape;
    bool m_hasXRandr = false;
    bool m_hasXkb = false;
    bool m_hasXRender = false;
    bool m_hasShm = false;
    bool m_hasShmFd = false;
    bool m_hasXSync = false;

    QPair<int, int> m_xrenderVersion;

    bool m_xi2Enabled = false;
    int m_xi2Minor = -1;
    int m_xiOpCode = -1;
    uint32_t m_xinputFirstEvent = 0;

    uint32_t m_xfixesFirstEvent = 0;
    uint32_t m_xrandrFirstEvent = 0;
    uint32_t m_xkbFirstEvent = 0;

    uint32_t m_maximumRequestLength = 0;
};

#define Q_XCB_REPLY_CONNECTION_ARG(connection, ...) connection

struct QStdFreeDeleter {
    void operator()(void *p) const noexcept { return std::free(p); }
};

#define Q_XCB_REPLY(call, ...) \
    std::unique_ptr<call##_reply_t, QStdFreeDeleter>( \
        call##_reply(Q_XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call(__VA_ARGS__), nullptr) \
    )

#define Q_XCB_REPLY_UNCHECKED(call, ...) \
    std::unique_ptr<call##_reply_t, QStdFreeDeleter>( \
        call##_reply(Q_XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call##_unchecked(__VA_ARGS__), nullptr) \
    )

QT_END_NAMESPACE

#endif // QXCBBASICCONNECTION_H
