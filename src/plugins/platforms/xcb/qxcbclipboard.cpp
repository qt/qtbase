/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbclipboard.h"

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbmime.h"

#include <private/qguiapplication_p.h>
#include <QElapsedTimer>

#include <QtCore/QDebug>

#include <xcb/xcb_icccm.h>

class QXcbClipboardMime : public QXcbMime
{
    Q_OBJECT
public:
    QXcbClipboardMime(QClipboard::Mode mode, QXcbClipboard *clipboard)
        : QXcbMime()
        , m_clipboard(clipboard)
    {
        switch (mode) {
        case QClipboard::Selection:
            modeAtom = QXcbAtom::XA_PRIMARY;
            break;

        case QClipboard::Clipboard:
            modeAtom = m_clipboard->connection()->atom(QXcbAtom::CLIPBOARD);
            break;

        default:
            qWarning("QTestLiteMime: Internal error: Unsupported clipboard mode");
            break;
        }
    }

protected:
    QStringList formats_sys() const
    {
        if (empty())
            return QStringList();

        if (!formatList.count()) {
            QXcbClipboardMime *that = const_cast<QXcbClipboardMime *>(this);
            // get the list of targets from the current clipboard owner - we do this
            // once so that multiple calls to this function don't require multiple
            // server round trips...
            that->format_atoms = m_clipboard->getDataInFormat(modeAtom, m_clipboard->connection()->atom(QXcbAtom::TARGETS));

            if (format_atoms.size() > 0) {
                xcb_atom_t *targets = (xcb_atom_t *) format_atoms.data();
                int size = format_atoms.size() / sizeof(xcb_atom_t);

                for (int i = 0; i < size; ++i) {
                    if (targets[i] == 0)
                        continue;

                    QString format = mimeAtomToString(m_clipboard->connection(), targets[i]);
                    if (!formatList.contains(format))
                        that->formatList.append(format);
                }
            }
        }

        return formatList;
    }

    bool hasFormat_sys(const QString &format) const
    {
        QStringList list = formats();
        return list.contains(format);
    }

    QVariant retrieveData_sys(const QString &fmt, QVariant::Type requestedType) const
    {
        if (fmt.isEmpty() || empty())
            return QByteArray();

        (void)formats(); // trigger update of format list

        QList<xcb_atom_t> atoms;
        xcb_atom_t *targets = (xcb_atom_t *) format_atoms.data();
        int size = format_atoms.size() / sizeof(xcb_atom_t);
        for (int i = 0; i < size; ++i)
            atoms.append(targets[i]);

        QByteArray encoding;
        xcb_atom_t fmtatom = mimeAtomForFormat(m_clipboard->connection(), fmt, requestedType, atoms, &encoding);

        if (fmtatom == 0)
            return QVariant();

        return mimeConvertToFormat(m_clipboard->connection(), fmtatom, m_clipboard->getDataInFormat(modeAtom, fmtatom), fmt, requestedType, encoding);
    }
private:
    bool empty() const
    {
        return m_clipboard->getSelectionOwner(modeAtom) == XCB_NONE;
    }


    xcb_atom_t modeAtom;
    QXcbClipboard *m_clipboard;
    QStringList formatList;
    QByteArray format_atoms;
};

const int QXcbClipboard::clipboard_timeout = 5000;

QXcbClipboard::QXcbClipboard(QXcbConnection *connection)
    : QPlatformClipboard()
    , m_connection(connection)
    , m_xClipboard(0)
    , m_clientClipboard(0)
    , m_xSelection(0)
    , m_clientSelection(0)
    , m_requestor(XCB_NONE)
    , m_owner(XCB_NONE)
{
    m_screen = m_connection->screens().at(m_connection->primaryScreen());
}

xcb_window_t QXcbClipboard::getSelectionOwner(xcb_atom_t atom) const
{
    xcb_connection_t *c = m_connection->xcb_connection();
    xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(c, atom);
    xcb_get_selection_owner_reply_t *reply;
    reply = xcb_get_selection_owner_reply(c, cookie, 0);
    xcb_window_t win = reply->owner;
    free(reply);
    return win;
}

QMimeData * QXcbClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode == QClipboard::Clipboard) {
        if (!m_xClipboard) {
            m_xClipboard = new QXcbClipboardMime(mode, this);
        }
        xcb_window_t clipboardOwner = getSelectionOwner(m_connection->atom(QXcbAtom::CLIPBOARD));
        if (clipboardOwner == owner()) {
            return m_clientClipboard;
        } else {
            return m_xClipboard;
        }
    } else if (mode == QClipboard::Selection) {
        if (!m_xSelection) {
            m_xSelection = new QXcbClipboardMime(mode, this);
        }
        xcb_window_t clipboardOwner = getSelectionOwner(QXcbAtom::XA_PRIMARY);
        if (clipboardOwner == owner()) {
            return m_clientSelection;
        } else {
            return m_xSelection;
        }
    }
    return 0;
}

void QXcbClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    xcb_atom_t modeAtom;
    QMimeData **d;
    switch (mode) {
    case QClipboard::Selection:
        modeAtom = QXcbAtom::XA_PRIMARY;
        d = &m_clientSelection;
        break;

    case QClipboard::Clipboard:
        modeAtom = m_connection->atom(QXcbAtom::CLIPBOARD);
        d = &m_clientClipboard;
        break;

    default:
        qWarning("QClipboard::setMimeData: unsupported mode '%d'", mode);
        return;
    }

    xcb_window_t newOwner;

    if (! data) { // no data, clear clipboard contents
        newOwner = XCB_NONE;
    } else {
        newOwner = owner();

        *d = data;
    }

    xcb_set_selection_owner(m_connection->xcb_connection(), newOwner, modeAtom, m_connection->time());

    if (getSelectionOwner(modeAtom) != newOwner) {
        qWarning("QClipboard::setData: Cannot set X11 selection owner");
    }

}

bool QXcbClipboard::supportsMode(QClipboard::Mode mode) const
{
    if (mode == QClipboard::Clipboard || mode == QClipboard::Selection)
        return true;
    return false;
}

xcb_window_t QXcbClipboard::requestor() const
{
    if (!m_requestor) {
        const int x = 0, y = 0, w = 3, h = 3;
        QXcbClipboard *that = const_cast<QXcbClipboard *>(this);

        xcb_window_t window = xcb_generate_id(m_connection->xcb_connection());
        Q_XCB_CALL(xcb_create_window(m_connection->xcb_connection(),
                                     XCB_COPY_FROM_PARENT,            // depth -- same as root
                                     window,                        // window id
                                     m_screen->screen()->root,                   // parent window id
                                     x, y, w, h,
                                     0,                               // border width
                                     XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                     m_screen->screen()->root_visual, // visual
                                     0,                               // value mask
                                     0));                             // value list

        uint32_t mask = XCB_EVENT_MASK_PROPERTY_CHANGE;
        xcb_change_window_attributes(m_connection->xcb_connection(), window, XCB_CW_EVENT_MASK, &mask);

        that->setRequestor(window);
    }
    return m_requestor;
}

void QXcbClipboard::setRequestor(xcb_window_t window)
{
    if (m_requestor != XCB_NONE) {
        xcb_destroy_window(m_connection->xcb_connection(), m_requestor);
    }
    m_requestor = window;
}

xcb_window_t QXcbClipboard::owner() const
{
    if (!m_owner) {
        int x = 0, y = 0, w = 3, h = 3;
        QXcbClipboard *that = const_cast<QXcbClipboard *>(this);

        xcb_window_t window = xcb_generate_id(m_connection->xcb_connection());
        Q_XCB_CALL(xcb_create_window(m_connection->xcb_connection(),
                                     XCB_COPY_FROM_PARENT,            // depth -- same as root
                                     window,                        // window id
                                     m_screen->screen()->root,                   // parent window id
                                     x, y, w, h,
                                     0,                               // border width
                                     XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                     m_screen->screen()->root_visual, // visual
                                     0,                               // value mask
                                     0));                             // value list

        that->setOwner(window);
    }
    return m_owner;
}

void QXcbClipboard::setOwner(xcb_window_t window)
{
    if (m_owner != XCB_NONE){
        xcb_destroy_window(m_connection->xcb_connection(), m_owner);
    }
    m_owner = window;
}

xcb_atom_t QXcbClipboard::sendTargetsSelection(QMimeData *d, xcb_window_t window, xcb_atom_t property)
{
    QVector<xcb_atom_t> types;
    QStringList formats = QInternalMimeData::formatsHelper(d);
    for (int i = 0; i < formats.size(); ++i) {
        QList<xcb_atom_t> atoms = QXcbMime::mimeAtomsForFormat(m_connection, formats.at(i));
        for (int j = 0; j < atoms.size(); ++j) {
            if (!types.contains(atoms.at(j)))
                types.append(atoms.at(j));
        }
    }
    types.append(m_connection->atom(QXcbAtom::TARGETS));
    types.append(m_connection->atom(QXcbAtom::MULTIPLE));
    types.append(m_connection->atom(QXcbAtom::TIMESTAMP));
    types.append(m_connection->atom(QXcbAtom::SAVE_TARGETS));

    xcb_change_property(m_connection->xcb_connection(), XCB_PROP_MODE_REPLACE, window, property, QXcbAtom::XA_ATOM,
                        32, types.size(), (const void *)types.constData());
    return property;
}

xcb_atom_t QXcbClipboard::sendSelection(QMimeData *d, xcb_atom_t target, xcb_window_t window, xcb_atom_t property)
{
    xcb_atom_t atomFormat = target;
    int dataFormat = 0;
    QByteArray data;

    QString fmt = QXcbMime::mimeAtomToString(m_connection, target);
    if (fmt.isEmpty()) { // Not a MIME type we have
        qDebug() << "QClipboard: send_selection(): converting to type '%s' is not supported" << fmt.data();
        return XCB_NONE;
    }
    qDebug() << "QClipboard: send_selection(): converting to type '%s'" << fmt.data();

    if (QXcbMime::mimeDataForAtom(m_connection, target, d, &data, &atomFormat, &dataFormat)) {

         // don't allow INCR transfers when using MULTIPLE or to
        // Motif clients (since Motif doesn't support INCR)
        static xcb_atom_t motif_clip_temporary = m_connection->atom(QXcbAtom::CLIP_TEMPORARY);
        bool allow_incr = property != motif_clip_temporary;

        // X_ChangeProperty protocol request is 24 bytes
        const int increment = (xcb_get_maximum_request_length(m_connection->xcb_connection()) * 4) - 24;
        if (data.size() > increment && allow_incr) {
            long bytes = data.size();
            xcb_change_property(m_connection->xcb_connection(), XCB_PROP_MODE_REPLACE, window, property,
                                m_connection->atom(QXcbAtom::INCR), 32, 1, (const void *)&bytes);

//            (void)new QClipboardINCRTransaction(window, property, atomFormat, dataFormat, data, increment);
            qDebug() << "not implemented INCRT just YET!";
            return property;
        }

        // make sure we can perform the XChangeProperty in a single request
        if (data.size() > increment)
            return XCB_NONE; // ### perhaps use several XChangeProperty calls w/ PropModeAppend?
        int dataSize = data.size() / (dataFormat / 8);
        // use a single request to transfer data
        xcb_change_property(m_connection->xcb_connection(), XCB_PROP_MODE_REPLACE, window, property, atomFormat,
                            dataFormat, dataSize, (const void *)data.constData());
    }
    return property;
}

void QXcbClipboard::handleSelectionRequest(xcb_selection_request_event_t *req)
{
    if (requestor() && req->requestor == requestor()) {
        qDebug() << "This should be caught before";
        return;
    }

    xcb_selection_notify_event_t event;
    event.response_type = XCB_SELECTION_NOTIFY;
    event.requestor = req->requestor;
    event.selection = req->selection;
    event.target    = req->target;
    event.property  = XCB_NONE;
    event.time      = req->time;

    QMimeData *d;
    if (req->selection == QXcbAtom::XA_PRIMARY) {
        d = m_clientSelection;
    } else if (req->selection == m_connection->atom(QXcbAtom::CLIPBOARD)) {
        d = m_clientClipboard;
    } else {
        qWarning() << "QClipboard: Unknown selection" << m_connection->atomName(req->selection);
        xcb_send_event(m_connection->xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
        return;
    }

    if (!d) {
        qWarning("QClipboard: Cannot transfer data, no data available");
        xcb_send_event(m_connection->xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
        return;
    }

    xcb_atom_t xa_targets = m_connection->atom(QXcbAtom::TARGETS);
    xcb_atom_t xa_multiple = m_connection->atom(QXcbAtom::MULTIPLE);
    xcb_atom_t xa_timestamp = m_connection->atom(QXcbAtom::TIMESTAMP);

    struct AtomPair { xcb_atom_t target; xcb_atom_t property; } *multi = 0;
    xcb_atom_t multi_type = XCB_NONE;
    int multi_format = 0;
    int nmulti = 0;
    int imulti = -1;
    bool multi_writeback = false;

    if (req->target == xa_multiple) {
        QByteArray multi_data;
        if (req->property == XCB_NONE
            || !clipboardReadProperty(req->requestor, req->property, false, &multi_data,
                                           0, &multi_type, &multi_format)
            || multi_format != 32) {
            // MULTIPLE property not formatted correctly
            xcb_send_event(m_connection->xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
            return;
        }
        nmulti = multi_data.size()/sizeof(*multi);
        multi = new AtomPair[nmulti];
        memcpy(multi,multi_data.data(),multi_data.size());
        imulti = 0;
    }

    for (; imulti < nmulti; ++imulti) {
        xcb_atom_t target;
        xcb_atom_t property;

        if (multi) {
            target = multi[imulti].target;
            property = multi[imulti].property;
        } else {
            target = req->target;
            property = req->property;
            if (property == XCB_NONE) // obsolete client
                property = target;
        }

        xcb_atom_t ret = XCB_NONE;
        if (target == XCB_NONE || property == XCB_NONE) {
            ;
        } else if (target == xa_timestamp) {
//            if (d->timestamp != CurrentTime) {
//                XChangeProperty(DISPLAY_FROM_XCB(m_connection), req->requestor, property, QXcbAtom::XA_INTEGER, 32,
//                                PropModeReplace, CurrentTime, 1);
//                ret = property;
//            } else {
//                qWarning("QClipboard: Invalid data timestamp");
//            }
        } else if (target == xa_targets) {
            ret = sendTargetsSelection(d, req->requestor, property);
        } else {
            ret = sendSelection(d, target, req->requestor, property);
        }

        if (nmulti > 0) {
            if (ret == XCB_NONE) {
                multi[imulti].property = XCB_NONE;
                multi_writeback = true;
            }
        } else {
            event.property = ret;
            break;
        }
    }

    if (nmulti > 0) {
        if (multi_writeback) {
            // according to ICCCM 2.6.2 says to put None back
            // into the original property on the requestor window
            xcb_change_property(m_connection->xcb_connection(), XCB_PROP_MODE_REPLACE, req->requestor, req->property,
                                multi_type, 32, nmulti*2, (const void *)multi);
        }

        delete [] multi;
        event.property = req->property;
    }

    // send selection notify to requestor
    xcb_send_event(m_connection->xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
}

static inline int maxSelectionIncr(xcb_connection_t *c)
{
    int l = xcb_get_maximum_request_length(c);
    return (l > 65536 ? 65536*4 : l*4) - 100;
}

bool QXcbClipboard::clipboardReadProperty(xcb_window_t win, xcb_atom_t property, bool deleteProperty, QByteArray *buffer, int *size, xcb_atom_t *type, int *format) const
{
    int    maxsize = maxSelectionIncr(m_connection->xcb_connection());
    ulong  bytes_left; // bytes_after
    xcb_atom_t   dummy_type;
    int    dummy_format;

    if (!type)                                // allow null args
        type = &dummy_type;
    if (!format)
        format = &dummy_format;

    // Don't read anything, just get the size of the property data
    xcb_get_property_cookie_t cookie = Q_XCB_CALL(xcb_get_property(m_connection->xcb_connection(), false, win, property, XCB_GET_PROPERTY_TYPE_ANY, 0, 0));
    xcb_get_property_reply_t *reply = xcb_get_property_reply(m_connection->xcb_connection(), cookie, 0);
    if (!reply || reply->type == XCB_NONE) {
        buffer->resize(0);
        return false;
    }
    *type = reply->type;
    *format = reply->format;
    bytes_left = reply->bytes_after;
    free(reply);

    int  offset = 0, buffer_offset = 0, format_inc = 1, proplen = bytes_left;

    switch (*format) {
    case 8:
    default:
        format_inc = sizeof(char) / 1;
        break;

    case 16:
        format_inc = sizeof(short) / 2;
        proplen *= sizeof(short) / 2;
        break;

    case 32:
        format_inc = sizeof(long) / 4;
        proplen *= sizeof(long) / 4;
        break;
    }

    int newSize = proplen;
    buffer->resize(newSize);

    bool ok = (buffer->size() == newSize);

    if (ok && newSize) {
        // could allocate buffer

        while (bytes_left) {
            // more to read...

            xcb_get_property_cookie_t cookie = Q_XCB_CALL(xcb_get_property(m_connection->xcb_connection(), false, win, property, XCB_GET_PROPERTY_TYPE_ANY, offset, maxsize/4));
            reply = xcb_get_property_reply(m_connection->xcb_connection(), cookie, 0);
            if (!reply || reply->type == XCB_NONE) {
                free(reply);
                break;
            }
            *type = reply->type;
            *format = reply->format;
            bytes_left = reply->bytes_after;
            char *data = (char *)xcb_get_property_value(reply);
            int length = xcb_get_property_value_length(reply);

            offset += length / (32 / *format);
            length *= format_inc * (*format) / 8;

            // Here we check if we get a buffer overflow and tries to
            // recover -- this shouldn't normally happen, but it doesn't
            // hurt to be defensive
            if ((int)(buffer_offset + length) > buffer->size()) {
                length = buffer->size() - buffer_offset;

                // escape loop
                bytes_left = 0;
            }

            memcpy(buffer->data() + buffer_offset, data, length);
            buffer_offset += length;

            free(reply);
        }
    }


    // correct size, not 0-term.
    if (size)
        *size = buffer_offset;

    if (deleteProperty)
        xcb_delete_property(m_connection->xcb_connection(), win, property);

    m_connection->flush();

    return ok;
}


namespace
{
    class Notify {
    public:
        Notify(xcb_window_t win, int t)
            : window(win), type(t) {}
        xcb_window_t window;
        int type;
        bool check(xcb_generic_event_t *event) const {
            if (!event)
                return false;
            int t = event->response_type & 0x7f;
            if (t != type)
                return false;
            if (t == XCB_PROPERTY_NOTIFY) {
                xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)event;
                if (pn->window == window)
                    return true;
            } else if (t == XCB_SELECTION_NOTIFY) {
                xcb_selection_notify_event_t *sn = (xcb_selection_notify_event_t *)event;
                if (sn->requestor == window)
                    return true;
            }
            return false;
        }
    };
    class ClipboardEvent {
    public:
        ClipboardEvent(QXcbConnection *c)
        { clipboard = c->internAtom("CLIPBOARD"); }
        xcb_atom_t clipboard;
        bool check(xcb_generic_event_t *e) const {
            if (!e)
                return false;
            int type = e->response_type & 0x7f;
            if (type == XCB_SELECTION_REQUEST) {
                xcb_selection_request_event_t *sr = (xcb_selection_request_event_t *)e;
                return sr->selection == QXcbAtom::XA_PRIMARY || sr->selection == clipboard;
            } else if (type == XCB_SELECTION_CLEAR) {
                xcb_selection_clear_event_t *sc = (xcb_selection_clear_event_t *)e;
                return sc->selection == QXcbAtom::XA_PRIMARY || sc->selection == clipboard;
            }
            return false;
        }
    };
}

xcb_generic_event_t *QXcbClipboard::waitForClipboardEvent(xcb_window_t win, int type, int timeout)
{
    QElapsedTimer timer;
    timer.start();
    do {
        Notify notify(win, type);
        xcb_generic_event_t *e = m_connection->checkEvent(notify);
        if (e)
            return e;

        // process other clipboard events, since someone is probably requesting data from us
        ClipboardEvent clipboard(m_connection);
        e = m_connection->checkEvent(clipboard);
        if (e) {
            m_connection->handleXcbEvent(e);
            free(e);
        }

        m_connection->flush();

        // sleep 50 ms, so we don't use up CPU cycles all the time.
        struct timeval usleep_tv;
        usleep_tv.tv_sec = 0;
        usleep_tv.tv_usec = 50000;
        select(0, 0, 0, 0, &usleep_tv);
    } while (timer.elapsed() < timeout);

    return 0;
}

QByteArray QXcbClipboard::clipboardReadIncrementalProperty(xcb_window_t win, xcb_atom_t property, int nbytes, bool nullterm)
{
    QByteArray buf;
    QByteArray tmp_buf;
    bool alloc_error = false;
    int  length;
    int  offset = 0;

    if (nbytes > 0) {
        // Reserve buffer + zero-terminator (for text data)
        // We want to complete the INCR transfer even if we cannot
        // allocate more memory
        buf.resize(nbytes+1);
        alloc_error = buf.size() != nbytes+1;
    }

    for (;;) {
        m_connection->flush();
        xcb_generic_event_t *ge = waitForClipboardEvent(win, XCB_PROPERTY_NOTIFY, clipboard_timeout);
        if (!ge)
            break;

        xcb_property_notify_event_t *event = (xcb_property_notify_event_t *)ge;
        if (event->atom != property || event->state != XCB_PROPERTY_NEW_VALUE)
            continue;
        if (clipboardReadProperty(win, property, true, &tmp_buf, &length, 0, 0)) {
            if (length == 0) {                // no more data, we're done
                if (nullterm) {
                    buf.resize(offset+1);
                    buf[offset] = '\0';
                } else {
                    buf.resize(offset);
                }
                return buf;
            } else if (!alloc_error) {
                if (offset+length > (int)buf.size()) {
                    buf.resize(offset+length+65535);
                    if (buf.size() != offset+length+65535) {
                        alloc_error = true;
                        length = buf.size() - offset;
                    }
                }
                memcpy(buf.data()+offset, tmp_buf.constData(), length);
                tmp_buf.resize(0);
                offset += length;
            }
        } else {
            break;
        }

        free(ge);
    }

    // timed out ... create a new requestor window, otherwise the requestor
    // could consider next request to be still part of this timed out request
    setRequestor(0);

    return QByteArray();
}

QByteArray QXcbClipboard::getDataInFormat(xcb_atom_t modeAtom, xcb_atom_t fmtAtom)
{
    return getSelection(modeAtom, fmtAtom, m_connection->atom(QXcbAtom::_QT_SELECTION));
}

QByteArray QXcbClipboard::getSelection(xcb_atom_t selection, xcb_atom_t target, xcb_atom_t property)
{
    QByteArray buf;
    xcb_window_t win = requestor();

    xcb_delete_property(m_connection->xcb_connection(), win, property);
    xcb_convert_selection(m_connection->xcb_connection(), win, selection, target, property, m_connection->time());

    m_connection->sync();

    xcb_generic_event_t *ge = waitForClipboardEvent(win, XCB_SELECTION_NOTIFY, clipboard_timeout);
    bool no_selection = !ge || ((xcb_selection_notify_event_t *)ge)->property == XCB_NONE;
    free(ge);

    if (no_selection)
        return buf;

    xcb_atom_t type;
    if (clipboardReadProperty(win, property, true, &buf, 0, &type, 0)) {
        if (type == m_connection->atom(QXcbAtom::INCR)) {
            int nbytes = buf.size() >= 4 ? *((int*)buf.data()) : 0;
            buf = clipboardReadIncrementalProperty(win, property, nbytes, false);
        }
    }

    return buf;
}

#include "qxcbclipboard.moc"
