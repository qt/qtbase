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

#include "qxlibstatic.h"
#include "qxlibscreen.h"
#include "qxlibdisplay.h"

#include <qplatformdefs.h>

#include <QtGui/private/qapplication_p.h>
#include <QtCore/QBuffer>
#include <QtCore/QLibrary>

#include <QDebug>

static const char * x11_atomnames = {
    // window-manager <-> client protocols
    "WM_PROTOCOLS\0"
    "WM_DELETE_WINDOW\0"
    "WM_TAKE_FOCUS\0"
    "_NET_WM_PING\0"
    "_NET_WM_CONTEXT_HELP\0"
    "_NET_WM_SYNC_REQUEST\0"
    "_NET_WM_SYNC_REQUEST_COUNTER\0"

    // ICCCM window state
    "WM_STATE\0"
    "WM_CHANGE_STATE\0"

    // Session management
    "WM_CLIENT_LEADER\0"
    "WM_WINDOW_ROLE\0"
    "SM_CLIENT_ID\0"

    // Clipboard
    "CLIPBOARD\0"
    "INCR\0"
    "TARGETS\0"
    "MULTIPLE\0"
    "TIMESTAMP\0"
    "SAVE_TARGETS\0"
    "CLIP_TEMPORARY\0"
    "_QT_SELECTION\0"
    "_QT_CLIPBOARD_SENTINEL\0"
    "_QT_SELECTION_SENTINEL\0"
    "CLIPBOARD_MANAGER\0"

    "RESOURCE_MANAGER\0"

    "_XSETROOT_ID\0"

    "_QT_SCROLL_DONE\0"
    "_QT_INPUT_ENCODING\0"

    "_MOTIF_WM_HINTS\0"

    "DTWM_IS_RUNNING\0"
    "ENLIGHTENMENT_DESKTOP\0"
    "_DT_SAVE_MODE\0"
    "_SGI_DESKS_MANAGER\0"

    // EWMH (aka NETWM)
    "_NET_SUPPORTED\0"
    "_NET_VIRTUAL_ROOTS\0"
    "_NET_WORKAREA\0"

    "_NET_MOVERESIZE_WINDOW\0"
    "_NET_WM_MOVERESIZE\0"

    "_NET_WM_NAME\0"
    "_NET_WM_ICON_NAME\0"
    "_NET_WM_ICON\0"

    "_NET_WM_PID\0"

    "_NET_WM_WINDOW_OPACITY\0"

    "_NET_WM_STATE\0"
    "_NET_WM_STATE_ABOVE\0"
    "_NET_WM_STATE_BELOW\0"
    "_NET_WM_STATE_FULLSCREEN\0"
    "_NET_WM_STATE_MAXIMIZED_HORZ\0"
    "_NET_WM_STATE_MAXIMIZED_VERT\0"
    "_NET_WM_STATE_MODAL\0"
    "_NET_WM_STATE_STAYS_ON_TOP\0"
    "_NET_WM_STATE_DEMANDS_ATTENTION\0"

    "_NET_WM_USER_TIME\0"
    "_NET_WM_USER_TIME_WINDOW\0"
    "_NET_WM_FULL_PLACEMENT\0"

    "_NET_WM_WINDOW_TYPE\0"
    "_NET_WM_WINDOW_TYPE_DESKTOP\0"
    "_NET_WM_WINDOW_TYPE_DOCK\0"
    "_NET_WM_WINDOW_TYPE_TOOLBAR\0"
    "_NET_WM_WINDOW_TYPE_MENU\0"
    "_NET_WM_WINDOW_TYPE_UTILITY\0"
    "_NET_WM_WINDOW_TYPE_SPLASH\0"
    "_NET_WM_WINDOW_TYPE_DIALOG\0"
    "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU\0"
    "_NET_WM_WINDOW_TYPE_POPUP_MENU\0"
    "_NET_WM_WINDOW_TYPE_TOOLTIP\0"
    "_NET_WM_WINDOW_TYPE_NOTIFICATION\0"
    "_NET_WM_WINDOW_TYPE_COMBO\0"
    "_NET_WM_WINDOW_TYPE_DND\0"
    "_NET_WM_WINDOW_TYPE_NORMAL\0"
    "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE\0"

    "_KDE_NET_WM_FRAME_STRUT\0"

    "_NET_STARTUP_INFO\0"
    "_NET_STARTUP_INFO_BEGIN\0"

    "_NET_SUPPORTING_WM_CHECK\0"

    "_NET_WM_CM_S0\0"

    "_NET_SYSTEM_TRAY_VISUAL\0"

    "_NET_ACTIVE_WINDOW\0"

    // Property formats
    "COMPOUND_TEXT\0"
    "TEXT\0"
    "UTF8_STRING\0"

    // xdnd
    "XdndEnter\0"
    "XdndPosition\0"
    "XdndStatus\0"
    "XdndLeave\0"
    "XdndDrop\0"
    "XdndFinished\0"
    "XdndTypeList\0"
    "XdndActionList\0"

    "XdndSelection\0"

    "XdndAware\0"
    "XdndProxy\0"

    "XdndActionCopy\0"
    "XdndActionLink\0"
    "XdndActionMove\0"
    "XdndActionPrivate\0"

    // Motif DND
    "_MOTIF_DRAG_AND_DROP_MESSAGE\0"
    "_MOTIF_DRAG_INITIATOR_INFO\0"
    "_MOTIF_DRAG_RECEIVER_INFO\0"
    "_MOTIF_DRAG_WINDOW\0"
    "_MOTIF_DRAG_TARGETS\0"

    "XmTRANSFER_SUCCESS\0"
    "XmTRANSFER_FAILURE\0"

    // Xkb
    "_XKB_RULES_NAMES\0"

    // XEMBED
    "_XEMBED\0"
    "_XEMBED_INFO\0"

    // Wacom old. (before version 0.10)
    "Wacom Stylus\0"
    "Wacom Cursor\0"
    "Wacom Eraser\0"

    // Tablet
    "STYLUS\0"
    "ERASER\0"
};

/*!
    \internal
    Try to resolve a \a symbol from \a library with the version specified
    by \a vernum.

    Note that, in the case of the Xfixes library, \a vernum is not the same as
    \c XFIXES_MAJOR - it is a part of soname and may differ from the Xfixes
    version.
*/
static void* qt_load_library_runtime(const char *library, int vernum,
                                     int highestVernum, const char *symbol)
{
    QList<int> versions;
    // we try to load in the following order:
    // explicit version -> the default one -> (from the highest (highestVernum) to the lowest (vernum) )
    if (vernum != -1)
        versions << vernum;
    versions << -1;
    if (vernum != -1) {
        for(int i = highestVernum; i > vernum; --i)
            versions << i;
    }
    Q_FOREACH(int version, versions) {
        QLatin1String libName(library);
        QLibrary xfixesLib(libName, version);
        void *ptr = xfixesLib.resolve(symbol);
        if (ptr)
            return ptr;
    }
    return 0;
}

#  define XFIXES_LOAD_RUNTIME(vernum, symbol, symbol_type) \
    (symbol_type)qt_load_library_runtime("libXfixes", vernum, 4, #symbol);
#  define XFIXES_LOAD_V1(symbol) \
    XFIXES_LOAD_RUNTIME(1, symbol, Ptr##symbol)
#  define XFIXES_LOAD_V2(symbol) \
    XFIXES_LOAD_RUNTIME(2, symbol, Ptr##symbol)


class QTestLiteStaticInfoPrivate
{
public:
    QTestLiteStaticInfoPrivate()
        : use_xfixes(false)
        , xfixes_major(0)
        , xfixes_eventbase(0)
        , xfixes_errorbase(0)
    {
        QXlibScreen *screen = qobject_cast<QXlibScreen *> (QApplicationPrivate::platformIntegration()->screens().at(0));
        Q_ASSERT(screen);

        initializeAllAtoms(screen);
        initializeSupportedAtoms(screen);

        resolveXFixes(screen);
    }

    bool isSupportedByWM(Atom atom)
    {
        if (!m_supportedAtoms)
            return false;

        bool supported = false;
        int i = 0;
        while (m_supportedAtoms[i] != 0) {
            if (m_supportedAtoms[i++] == atom) {
                supported = true;
                break;
            }
        }

        return supported;
    }

    Atom atom(QXlibStatic::X11Atom atom)
    {
        return m_allAtoms[atom];
    }

    bool useXFixes() const { return use_xfixes; }

    int xFixesEventBase() const {return xfixes_eventbase; }

    PtrXFixesSelectSelectionInput  xFixesSelectSelectionInput() const
    {
        return ptrXFixesSelectSelectionInput;
    }

    QImage qimageFromXImage(XImage *xi)
    {
        QImage::Format format = QImage::Format_ARGB32_Premultiplied;
        if (xi->depth == 24)
            format = QImage::Format_RGB32;
        else if (xi->depth == 16)
            format = QImage::Format_RGB16;

        QImage image = QImage((uchar *)xi->data, xi->width, xi->height, xi->bytes_per_line, format).copy();

        // we may have to swap the byte order
        if ((QSysInfo::ByteOrder == QSysInfo::LittleEndian && xi->byte_order == MSBFirst)
            || (QSysInfo::ByteOrder == QSysInfo::BigEndian && xi->byte_order == LSBFirst))
        {
            for (int i=0; i < image.height(); i++) {
                if (xi->depth == 16) {
                    ushort *p = (ushort*)image.scanLine(i);
                    ushort *end = p + image.width();
                    while (p < end) {
                        *p = ((*p << 8) & 0xff00) | ((*p >> 8) & 0x00ff);
                        p++;
                    }
                } else {
                    uint *p = (uint*)image.scanLine(i);
                    uint *end = p + image.width();
                    while (p < end) {
                        *p = ((*p << 24) & 0xff000000) | ((*p << 8) & 0x00ff0000)
                             | ((*p >> 8) & 0x0000ff00) | ((*p >> 24) & 0x000000ff);
                        p++;
                    }
                }
            }
        }

        // fix-up alpha channel
        if (format == QImage::Format_RGB32) {
            QRgb *p = (QRgb *)image.bits();
            for (int y = 0; y < xi->height; ++y) {
                for (int x = 0; x < xi->width; ++x)
                    p[x] |= 0xff000000;
                p += xi->bytes_per_line / 4;
            }
        }

        return image;
    }


private:

    void initializeAllAtoms(QXlibScreen *screen) {
        const char *names[QXlibStatic::NAtoms];
        const char *ptr = x11_atomnames;

        int i = 0;
        while (*ptr) {
            names[i++] = ptr;
            while (*ptr)
                ++ptr;
            ++ptr;
        }

        Q_ASSERT(i == QXlibStatic::NPredefinedAtoms);

        QByteArray settings_atom_name("_QT_SETTINGS_TIMESTAMP_");
        settings_atom_name += XDisplayName(qPrintable(screen->display()->displayName()));
        names[i++] = settings_atom_name;

        Q_ASSERT(i == QXlibStatic::NAtoms);
    #if 0//defined(XlibSpecificationRelease) && (XlibSpecificationRelease >= 6)
        XInternAtoms(screen->display(), (char **)names, i, False, m_allAtoms);
    #else
        for (i = 0; i < QXlibStatic::NAtoms; ++i)
            m_allAtoms[i] = XInternAtom(screen->display()->nativeDisplay(), (char *)names[i], False);
    #endif
    }

    void initializeSupportedAtoms(QXlibScreen *screen)
    {
        Atom type;
        int format;
        long offset = 0;
        unsigned long nitems, after;
        unsigned char *data = 0;

        int e = XGetWindowProperty(screen->display()->nativeDisplay(), screen->rootWindow(),
                                   this->atom(QXlibStatic::_NET_SUPPORTED), 0, 0,
                                   False, XA_ATOM, &type, &format, &nitems, &after, &data);
        if (data)
            XFree(data);

        if (e == Success && type == XA_ATOM && format == 32) {
            QBuffer ts;
            ts.open(QIODevice::WriteOnly);

            while (after > 0) {
                XGetWindowProperty(screen->display()->nativeDisplay(), screen->rootWindow(),
                                   this->atom(QXlibStatic::_NET_SUPPORTED), offset, 1024,
                                   False, XA_ATOM, &type, &format, &nitems, &after, &data);

                if (type == XA_ATOM && format == 32) {
                    ts.write(reinterpret_cast<char *>(data), nitems * sizeof(long));
                    offset += nitems;
                } else
                    after = 0;
                if (data)
                    XFree(data);
            }

            // compute nitems
            QByteArray buffer(ts.buffer());
            nitems = buffer.size() / sizeof(Atom);
            m_supportedAtoms = new Atom[nitems + 1];
            Atom *a = (Atom *) buffer.data();
            uint i;
            for (i = 0; i < nitems; i++)
                m_supportedAtoms[i] = a[i];
            m_supportedAtoms[nitems] = 0;

        }
    }

    void resolveXFixes(QXlibScreen *screen)
    {
#ifndef QT_NO_XFIXES
        // See if Xfixes is supported on the connected display
        if (XQueryExtension(screen->display()->nativeDisplay(), "XFIXES", &xfixes_major,
                            &xfixes_eventbase, &xfixes_errorbase)) {
            ptrXFixesQueryExtension  = XFIXES_LOAD_V1(XFixesQueryExtension);
            ptrXFixesQueryVersion    = XFIXES_LOAD_V1(XFixesQueryVersion);
            ptrXFixesSetCursorName   = XFIXES_LOAD_V2(XFixesSetCursorName);
            ptrXFixesSelectSelectionInput = XFIXES_LOAD_V2(XFixesSelectSelectionInput);

            if(ptrXFixesQueryExtension && ptrXFixesQueryVersion
               && ptrXFixesQueryExtension(screen->display()->nativeDisplay(), &xfixes_eventbase,
                                               &xfixes_errorbase)) {
                // Xfixes is supported.
                // Note: the XFixes protocol version is negotiated using QueryVersion.
                // We supply the highest version we support, the X server replies with
                // the highest version it supports, but no higher than the version we
                // asked for. The version sent back is the protocol version the X server
                // will use to talk us. If this call is removed, the behavior of the
                // X server when it receives an XFixes request is undefined.
                int major = 3;
                int minor = 0;
                ptrXFixesQueryVersion(screen->display()->nativeDisplay(), &major, &minor);
                use_xfixes = (major >= 1);
                xfixes_major = major;
            }
        }
#endif // QT_NO_XFIXES

    }

    Atom *m_supportedAtoms;
    Atom m_allAtoms[QXlibStatic::NAtoms];

#ifndef QT_NO_XFIXES
    PtrXFixesQueryExtension ptrXFixesQueryExtension;
    PtrXFixesQueryVersion ptrXFixesQueryVersion;
    PtrXFixesSetCursorName ptrXFixesSetCursorName;
    PtrXFixesSelectSelectionInput ptrXFixesSelectSelectionInput;
#endif

    bool use_xfixes;
    int xfixes_major;
    int xfixes_eventbase;
    int xfixes_errorbase;

};
Q_GLOBAL_STATIC(QTestLiteStaticInfoPrivate, qTestLiteStaticInfoPrivate);


Atom QXlibStatic::atom(QXlibStatic::X11Atom atom)
{
    return qTestLiteStaticInfoPrivate()->atom(atom);
}

bool QXlibStatic::isSupportedByWM(Atom atom)
{
    return qTestLiteStaticInfoPrivate()->isSupportedByWM(atom);
}

bool QXlibStatic::useXFixes()
{
    return qTestLiteStaticInfoPrivate()->useXFixes();
}

int QXlibStatic::xFixesEventBase()
{
    return qTestLiteStaticInfoPrivate()->xFixesEventBase();
}

#ifndef QT_NO_XFIXES
PtrXFixesSelectSelectionInput QXlibStatic::xFixesSelectSelectionInput()
{
    qDebug() << qTestLiteStaticInfoPrivate()->useXFixes();
    if (!qTestLiteStaticInfoPrivate()->useXFixes())
        return 0;

    return qTestLiteStaticInfoPrivate()->xFixesSelectSelectionInput();
}

QImage QXlibStatic::qimageFromXImage(XImage *xi)
{
    return qTestLiteStaticInfoPrivate()->qimageFromXImage(xi);
}
#endif //QT_NO_XFIXES
