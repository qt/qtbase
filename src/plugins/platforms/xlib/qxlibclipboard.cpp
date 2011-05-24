/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxlibclipboard.h"

#include "qxlibscreen.h"
#include "qxlibmime.h"
#include "qxlibdisplay.h"

#include <private/qapplication_p.h>

#include <QtCore/QDebug>

class QXlibClipboardMime : public QXlibMime
{
    Q_OBJECT
public:
    QXlibClipboardMime(QClipboard::Mode mode, QXlibClipboard *clipboard)
        : QXlibMime()
        , m_clipboard(clipboard)
    {
        switch (mode) {
        case QClipboard::Selection:
            modeAtom = XA_PRIMARY;
            break;

        case QClipboard::Clipboard:
            modeAtom = QXlibStatic::atom(QXlibStatic::CLIPBOARD);
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
            QXlibClipboardMime *that = const_cast<QXlibClipboardMime *>(this);
            // get the list of targets from the current clipboard owner - we do this
            // once so that multiple calls to this function don't require multiple
            // server round trips...
            that->format_atoms = m_clipboard->getDataInFormat(modeAtom,QXlibStatic::atom(QXlibStatic::TARGETS));

            if (format_atoms.size() > 0) {
                Atom *targets = (Atom *) format_atoms.data();
                int size = format_atoms.size() / sizeof(Atom);

                for (int i = 0; i < size; ++i) {
                    if (targets[i] == 0)
                        continue;

                    QStringList formatsForAtom = mimeFormatsForAtom(m_clipboard->screen()->display()->nativeDisplay(),targets[i]);
                    for (int j = 0; j < formatsForAtom.size(); ++j) {
                        if (!formatList.contains(formatsForAtom.at(j)))
                            that->formatList.append(formatsForAtom.at(j));
                    }
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

        QList<Atom> atoms;
        Atom *targets = (Atom *) format_atoms.data();
        int size = format_atoms.size() / sizeof(Atom);
        for (int i = 0; i < size; ++i)
            atoms.append(targets[i]);

        QByteArray encoding;
        Atom fmtatom = mimeAtomForFormat(m_clipboard->screen()->display()->nativeDisplay(),fmt, requestedType, atoms, &encoding);

        if (fmtatom == 0)
            return QVariant();

        return mimeConvertToFormat(m_clipboard->screen()->display()->nativeDisplay(),fmtatom, m_clipboard->getDataInFormat(modeAtom,fmtatom), fmt, requestedType, encoding);
    }
private:
    bool empty() const
    {
        Window win = XGetSelectionOwner(m_clipboard->screen()->display()->nativeDisplay(), modeAtom);

        return win == XNone;
    }


    Atom modeAtom;
    QXlibClipboard *m_clipboard;
    QStringList formatList;
    QByteArray format_atoms;
};

const int QXlibClipboard::clipboard_timeout = 5000;

QXlibClipboard::QXlibClipboard(QXlibScreen *screen)
    : QPlatformClipboard()
    , m_screen(screen)
    , m_xClipboard(0)
    , m_clientClipboard(0)
    , m_xSelection(0)
    , m_clientSelection(0)
    , m_requestor(XNone)
    , m_owner(XNone)
{
}

QMimeData * QXlibClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode == QClipboard::Clipboard) {
        if (!m_xClipboard) {
            m_xClipboard = new QXlibClipboardMime(mode, this);
        }
        Window clipboardOwner = XGetSelectionOwner(screen()->display()->nativeDisplay(),QXlibStatic::atom(QXlibStatic::CLIPBOARD));
        if (clipboardOwner == owner()) {
            return m_clientClipboard;
        } else {
            return m_xClipboard;
        }
    } else if (mode == QClipboard::Selection) {
        if (!m_xSelection) {
            m_xSelection = new QXlibClipboardMime(mode, this);
        }
        Window clipboardOwner = XGetSelectionOwner(screen()->display()->nativeDisplay(),XA_PRIMARY);
        if (clipboardOwner == owner()) {
            return m_clientSelection;
        } else {
            return m_xSelection;
        }
    }
    return 0;
}

void QXlibClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    Atom modeAtom;
    QMimeData **d;
    switch (mode) {
    case QClipboard::Selection:
        modeAtom = XA_PRIMARY;
        d = &m_clientSelection;
        break;

    case QClipboard::Clipboard:
        modeAtom = QXlibStatic::atom(QXlibStatic::CLIPBOARD);
        d = &m_clientClipboard;
        break;

    default:
        qWarning("QClipboard::setMimeData: unsupported mode '%d'", mode);
        return;
    }

    Window newOwner;

    if (! data) { // no data, clear clipboard contents
        newOwner = XNone;
    } else {
        newOwner = owner();

        *d = data;
    }

    XSetSelectionOwner(m_screen->display()->nativeDisplay(), modeAtom, newOwner, CurrentTime);

    if (XGetSelectionOwner(m_screen->display()->nativeDisplay(), modeAtom) != newOwner) {
        qWarning("QClipboard::setData: Cannot set X11 selection owner");
    }

}

bool QXlibClipboard::supportsMode(QClipboard::Mode mode) const
{
    if (mode == QClipboard::Clipboard || mode == QClipboard::Selection)
        return true;
    return false;
}


QXlibScreen * QXlibClipboard::screen() const
{
    return m_screen;
}

Window QXlibClipboard::requestor() const
{
    if (!m_requestor) {
        int x = 0, y = 0, w = 3, h = 3;
        QXlibClipboard *that = const_cast<QXlibClipboard *>(this);
        Window window  = XCreateSimpleWindow(m_screen->display()->nativeDisplay(), m_screen->rootWindow(),
                                       x, y, w, h, 0 /*border_width*/,
                                       m_screen->blackPixel(), m_screen->whitePixel());
        that->setRequestor(window);
    }
    return m_requestor;
}

void QXlibClipboard::setRequestor(Window window)
{
    if (m_requestor != XNone) {
        XDestroyWindow(m_screen->display()->nativeDisplay(),m_requestor);
    }
    m_requestor = window;
}

Window QXlibClipboard::owner() const
{
    if (!m_owner) {
        int x = 0, y = 0, w = 3, h = 3;
        QXlibClipboard *that = const_cast<QXlibClipboard *>(this);
        Window window  = XCreateSimpleWindow(m_screen->display()->nativeDisplay(), m_screen->rootWindow(),
                                       x, y, w, h, 0 /*border_width*/,
                                       m_screen->blackPixel(), m_screen->whitePixel());
        that->setOwner(window);
    }
    return m_owner;
}

void QXlibClipboard::setOwner(Window window)
{
    if (m_owner != XNone){
        XDestroyWindow(m_screen->display()->nativeDisplay(),m_owner);
    }
    m_owner = window;
}

Atom QXlibClipboard::sendTargetsSelection(QMimeData *d, Window window, Atom property)
{
    QVector<Atom> types;
    QStringList formats = QInternalMimeData::formatsHelper(d);
    for (int i = 0; i < formats.size(); ++i) {
        QList<Atom> atoms = QXlibMime::mimeAtomsForFormat(screen()->display()->nativeDisplay(),formats.at(i));
        for (int j = 0; j < atoms.size(); ++j) {
            if (!types.contains(atoms.at(j)))
                types.append(atoms.at(j));
        }
    }
    types.append(QXlibStatic::atom(QXlibStatic::TARGETS));
    types.append(QXlibStatic::atom(QXlibStatic::MULTIPLE));
    types.append(QXlibStatic::atom(QXlibStatic::TIMESTAMP));
    types.append(QXlibStatic::atom(QXlibStatic::SAVE_TARGETS));

    XChangeProperty(screen()->display()->nativeDisplay(), window, property, XA_ATOM, 32,
                    PropModeReplace, (uchar *) types.data(), types.size());
    return property;
}

Atom QXlibClipboard::sendSelection(QMimeData *d, Atom target, Window window, Atom property)
{
    Atom atomFormat = target;
    int dataFormat = 0;
    QByteArray data;

    QString fmt = QXlibMime::mimeAtomToString(screen()->display()->nativeDisplay(), target);
    if (fmt.isEmpty()) { // Not a MIME type we have
        qDebug() << "QClipboard: send_selection(): converting to type '%s' is not supported" << fmt.data();
        return XNone;
    }
    qDebug() << "QClipboard: send_selection(): converting to type '%s'" << fmt.data();

    if (QXlibMime::mimeDataForAtom(screen()->display()->nativeDisplay(),target, d, &data, &atomFormat, &dataFormat)) {

         // don't allow INCR transfers when using MULTIPLE or to
        // Motif clients (since Motif doesn't support INCR)
        static Atom motif_clip_temporary = QXlibStatic::atom(QXlibStatic::CLIP_TEMPORARY);
        bool allow_incr = property != motif_clip_temporary;

        // X_ChangeProperty protocol request is 24 bytes
        const int increment = (XMaxRequestSize(screen()->display()->nativeDisplay()) * 4) - 24;
        if (data.size() > increment && allow_incr) {
            long bytes = data.size();
            XChangeProperty(screen()->display()->nativeDisplay(), window, property,
                            QXlibStatic::atom(QXlibStatic::INCR), 32, PropModeReplace, (uchar *) &bytes, 1);

//            (void)new QClipboardINCRTransaction(window, property, atomFormat, dataFormat, data, increment);
            qDebug() << "not implemented INCRT just YET!";
            return property;
        }

        // make sure we can perform the XChangeProperty in a single request
        if (data.size() > increment)
            return XNone; // ### perhaps use several XChangeProperty calls w/ PropModeAppend?
        int dataSize = data.size() / (dataFormat / 8);
        // use a single request to transfer data
        XChangeProperty(screen()->display()->nativeDisplay(), window, property, atomFormat,
                        dataFormat, PropModeReplace, (uchar *) data.data(),
                        dataSize);
    }
    return property;
}

void QXlibClipboard::handleSelectionRequest(XEvent *xevent)
{
    XSelectionRequestEvent *req = &xevent->xselectionrequest;

    if (requestor() && req->requestor == requestor()) {
        qDebug() << "This should be caught before";
        return;
    }

    XEvent event;
    event.xselection.type      = SelectionNotify;
    event.xselection.display   = req->display;
    event.xselection.requestor = req->requestor;
    event.xselection.selection = req->selection;
    event.xselection.target    = req->target;
    event.xselection.property  = XNone;
    event.xselection.time      = req->time;

    QMimeData *d;
    if (req->selection == XA_PRIMARY) {
        d = m_clientSelection;
    } else if (req->selection == QXlibStatic::atom(QXlibStatic::CLIPBOARD)) {
        d = m_clientClipboard;
    } else {
        qWarning("QClipboard: Unknown selection '%lx'", req->selection);
        XSendEvent(screen()->display()->nativeDisplay(), req->requestor, False, NoEventMask, &event);
        return;
    }

    if (!d) {
        qWarning("QClipboard: Cannot transfer data, no data available");
        XSendEvent(screen()->display()->nativeDisplay(), req->requestor, False, NoEventMask, &event);
        return;
    }

    Atom xa_targets = QXlibStatic::atom(QXlibStatic::TARGETS);
    Atom xa_multiple = QXlibStatic::atom(QXlibStatic::MULTIPLE);
    Atom xa_timestamp = QXlibStatic::atom(QXlibStatic::TIMESTAMP);

    struct AtomPair { Atom target; Atom property; } *multi = 0;
    Atom multi_type = XNone;
    int multi_format = 0;
    int nmulti = 0;
    int imulti = -1;
    bool multi_writeback = false;

    if (req->target == xa_multiple) {
        QByteArray multi_data;
        if (req->property == XNone
            || !clipboardReadProperty(req->requestor, req->property, false, &multi_data,
                                           0, &multi_type, &multi_format)
            || multi_format != 32) {
            // MULTIPLE property not formatted correctly
            XSendEvent(screen()->display()->nativeDisplay(), req->requestor, False, NoEventMask, &event);
            return;
        }
        nmulti = multi_data.size()/sizeof(*multi);
        multi = new AtomPair[nmulti];
        memcpy(multi,multi_data.data(),multi_data.size());
        imulti = 0;
    }

    for (; imulti < nmulti; ++imulti) {
        Atom target;
        Atom property;

        if (multi) {
            target = multi[imulti].target;
            property = multi[imulti].property;
        } else {
            target = req->target;
            property = req->property;
            if (property == XNone) // obsolete client
                property = target;
        }

        Atom ret = XNone;
        if (target == XNone || property == XNone) {
            ;
        } else if (target == xa_timestamp) {
//            if (d->timestamp != CurrentTime) {
//                XChangeProperty(screen()->display()->nativeDisplay(), req->requestor, property, XA_INTEGER, 32,
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
            if (ret == XNone) {
                multi[imulti].property = XNone;
                multi_writeback = true;
            }
        } else {
            event.xselection.property = ret;
            break;
        }
    }

    if (nmulti > 0) {
        if (multi_writeback) {
            // according to ICCCM 2.6.2 says to put None back
            // into the original property on the requestor window
            XChangeProperty(screen()->display()->nativeDisplay(), req->requestor, req->property, multi_type, 32,
                            PropModeReplace, (uchar *) multi, nmulti * 2);
        }

        delete [] multi;
        event.xselection.property = req->property;
    }

    // send selection notify to requestor
    XSendEvent(screen()->display()->nativeDisplay(), req->requestor, False, NoEventMask, &event);
}

static inline int maxSelectionIncr(Display *dpy)
{ return XMaxRequestSize(dpy) > 65536 ? 65536*4 : XMaxRequestSize(dpy)*4 - 100; }

bool QXlibClipboard::clipboardReadProperty(Window win, Atom property, bool deleteProperty, QByteArray *buffer, int *size, Atom *type, int *format) const
{
    int    maxsize = maxSelectionIncr(screen()->display()->nativeDisplay());
    ulong  bytes_left; // bytes_after
    ulong  length;     // nitems
    uchar *data;
    Atom   dummy_type;
    int    dummy_format;
    int    r;

    if (!type)                                // allow null args
        type = &dummy_type;
    if (!format)
        format = &dummy_format;

    // Don't read anything, just get the size of the property data
    r = XGetWindowProperty(screen()->display()->nativeDisplay(), win, property, 0, 0, False,
                            AnyPropertyType, type, format,
                            &length, &bytes_left, &data);
    if (r != Success || (type && *type == XNone)) {
        buffer->resize(0);
        return false;
    }
    XFree((char*)data);

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

            r = XGetWindowProperty(screen()->display()->nativeDisplay(), win, property, offset, maxsize/4,
                                   False, AnyPropertyType, type, format,
                                   &length, &bytes_left, &data);
            if (r != Success || (type && *type == XNone))
                break;

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

            XFree((char*)data);
        }

        if (*format == 8 && *type == QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT)) {
            // convert COMPOUND_TEXT to a multibyte string
            XTextProperty textprop;
            textprop.encoding = *type;
            textprop.format = *format;
            textprop.nitems = buffer_offset;
            textprop.value = (unsigned char *) buffer->data();

            char **list_ret = 0;
            int count;
            if (XmbTextPropertyToTextList(screen()->display()->nativeDisplay(), &textprop, &list_ret,
                         &count) == Success && count && list_ret) {
                offset = buffer_offset = strlen(list_ret[0]);
                buffer->resize(offset);
                memcpy(buffer->data(), list_ret[0], offset);
            }
            if (list_ret) XFreeStringList(list_ret);
        }
    }

    // correct size, not 0-term.
    if (size)
        *size = buffer_offset;

    if (deleteProperty)
        XDeleteProperty(screen()->display()->nativeDisplay(), win, property);

    screen()->display()->flush();

    return ok;
}

QByteArray QXlibClipboard::clipboardReadIncrementalProperty(Window win, Atom property, int nbytes, bool nullterm)
{
    XEvent event;

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
        screen()->display()->flush();
        if (!screen()->waitForClipboardEvent(win,PropertyNotify,&event,clipboard_timeout))
            break;
        if (event.xproperty.atom != property ||
             event.xproperty.state != PropertyNewValue)
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
    }

    // timed out ... create a new requestor window, otherwise the requestor
    // could consider next request to be still part of this timed out request
    setRequestor(0);

    return QByteArray();
}

QByteArray QXlibClipboard::getDataInFormat(Atom modeAtom, Atom fmtatom)
{
    QByteArray buf;

    Window   win = requestor();

    XSelectInput(screen()->display()->nativeDisplay(), win, NoEventMask); // don't listen for any events

    XDeleteProperty(screen()->display()->nativeDisplay(), win, QXlibStatic::atom(QXlibStatic::_QT_SELECTION));
    XConvertSelection(screen()->display()->nativeDisplay(), modeAtom, fmtatom, QXlibStatic::atom(QXlibStatic::_QT_SELECTION), win, CurrentTime);
    screen()->display()->sync();

    XEvent xevent;
    if (!screen()->waitForClipboardEvent(win,SelectionNotify,&xevent,clipboard_timeout) ||
         xevent.xselection.property == XNone) {
        return buf;
    }

    Atom   type;
    XSelectInput(screen()->display()->nativeDisplay(), win, PropertyChangeMask);

    if (clipboardReadProperty(win, QXlibStatic::atom(QXlibStatic::_QT_SELECTION), true, &buf, 0, &type, 0)) {
        if (type == QXlibStatic::atom(QXlibStatic::INCR)) {
            int nbytes = buf.size() >= 4 ? *((int*)buf.data()) : 0;
            buf = clipboardReadIncrementalProperty(win, QXlibStatic::atom(QXlibStatic::_QT_SELECTION), nbytes, false);
        }
    }

    XSelectInput(screen()->display()->nativeDisplay(), win, NoEventMask);


    return buf;
}

#include "qxlibclipboard.moc"
