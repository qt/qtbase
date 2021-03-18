/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbmime.h"

#include <QtCore/QTextCodec>
#include <QtGui/QImageWriter>
#include <QtCore/QBuffer>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

QXcbMime::QXcbMime()
    : QInternalMimeData()
{ }

QXcbMime::~QXcbMime()
{}



QString QXcbMime::mimeAtomToString(QXcbConnection *connection, xcb_atom_t a)
{
    if (a == XCB_NONE)
        return QString();

    // special cases for string type
    if (a == XCB_ATOM_STRING
        || a == connection->atom(QXcbAtom::UTF8_STRING)
        || a == connection->atom(QXcbAtom::TEXT))
        return QLatin1String("text/plain");

    // special case for images
    if (a == XCB_ATOM_PIXMAP)
        return QLatin1String("image/ppm");

    QByteArray atomName = connection->atomName(a);

    // special cases for uris
    if (atomName == "text/x-moz-url")
        atomName = "text/uri-list";

    return QString::fromLatin1(atomName.constData());
}

bool QXcbMime::mimeDataForAtom(QXcbConnection *connection, xcb_atom_t a, QMimeData *mimeData, QByteArray *data,
                               xcb_atom_t *atomFormat, int *dataFormat)
{
    if (!data)
        return false;

    bool ret = false;
    *atomFormat = a;
    *dataFormat = 8;

    if ((a == connection->atom(QXcbAtom::UTF8_STRING)
         || a == XCB_ATOM_STRING
         || a == connection->atom(QXcbAtom::TEXT))
        && QInternalMimeData::hasFormatHelper(QLatin1String("text/plain"), mimeData)) {
        if (a == connection->atom(QXcbAtom::UTF8_STRING)) {
            *data = QInternalMimeData::renderDataHelper(QLatin1String("text/plain"), mimeData);
            ret = true;
        } else if (a == XCB_ATOM_STRING ||
                   a == connection->atom(QXcbAtom::TEXT)) {
            // ICCCM says STRING is latin1
            *data = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                        QLatin1String("text/plain"), mimeData)).toLatin1();
            ret = true;
        }
        return ret;
    }

    QString atomName = mimeAtomToString(connection, a);
    if (QInternalMimeData::hasFormatHelper(atomName, mimeData)) {
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        // mimeAtomToString() converts "text/x-moz-url" to "text/uri-list",
        // so QXcbConnection::atomName() has to be used.
        if (atomName == QLatin1String("text/uri-list")
            && connection->atomName(a) == "text/x-moz-url") {
            const QString mozUri = QLatin1String(data->split('\n').constFirst()) + QLatin1Char('\n');
            *data = QByteArray(reinterpret_cast<const char *>(mozUri.utf16()),
                               mozUri.length() * 2);
        } else if (atomName == QLatin1String("application/x-color"))
            *dataFormat = 16;
        ret = true;
    } else if ((a == XCB_ATOM_PIXMAP || a == XCB_ATOM_BITMAP) && mimeData->hasImage()) {
        ret = true;
    } else if (atomName == QLatin1String("text/plain")
               && mimeData->hasFormat(QLatin1String("text/uri-list"))) {
        // Return URLs also as plain text.
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        ret = true;
    }
    return ret;
}

QVector<xcb_atom_t> QXcbMime::mimeAtomsForFormat(QXcbConnection *connection, const QString &format)
{
    QVector<xcb_atom_t> atoms;
    atoms.reserve(7);
    atoms.append(connection->internAtom(format.toLatin1()));

    // special cases for strings
    if (format == QLatin1String("text/plain")) {
        atoms.append(connection->atom(QXcbAtom::UTF8_STRING));
        atoms.append(XCB_ATOM_STRING);
        atoms.append(connection->atom(QXcbAtom::TEXT));
    }

    // special cases for uris
    if (format == QLatin1String("text/uri-list")) {
        atoms.append(connection->internAtom("text/x-moz-url"));
        atoms.append(connection->internAtom("text/plain"));
    }

    //special cases for images
    if (format == QLatin1String("image/ppm"))
        atoms.append(XCB_ATOM_PIXMAP);
    if (format == QLatin1String("image/pbm"))
        atoms.append(XCB_ATOM_BITMAP);

    return atoms;
}

QVariant QXcbMime::mimeConvertToFormat(QXcbConnection *connection, xcb_atom_t a, const QByteArray &d, const QString &format,
                                       QMetaType::Type requestedType, const QByteArray &encoding)
{
    QByteArray data = d;
    QString atomName = mimeAtomToString(connection, a);
//    qDebug() << "mimeConvertDataToFormat" << format << atomName << data;

    if (!encoding.isEmpty()
        && atomName == format + QLatin1String(";charset=") + QLatin1String(encoding)) {

#if QT_CONFIG(textcodec)
        if (requestedType == QMetaType::QString) {
            QTextCodec *codec = QTextCodec::codecForName(encoding);
            if (codec)
                return codec->toUnicode(data);
        }
#endif

        return data;
    }

    // special cases for string types
    if (format == QLatin1String("text/plain")) {
        if (data.endsWith('\0'))
            data.chop(1);
        if (a == connection->atom(QXcbAtom::UTF8_STRING)) {
            return QString::fromUtf8(data);
        }
        if (a == XCB_ATOM_STRING ||
            a == connection->atom(QXcbAtom::TEXT))
            return QString::fromLatin1(data);
    }
    // If data contains UTF16 text, convert it to a string.
    // Firefox uses UTF16 without BOM for text/x-moz-url, "text/html",
    // Google Chrome uses UTF16 without BOM for "text/x-moz-url",
    // UTF16 with BOM for "text/html".
    if ((format == QLatin1String("text/html") || format == QLatin1String("text/uri-list"))
        && data.size() > 1) {
        const quint8 byte0 = data.at(0);
        const quint8 byte1 = data.at(1);
        if ((byte0 == 0xff && byte1 == 0xfe) || (byte0 == 0xfe && byte1 == 0xff)
            || (byte0 != 0 && byte1 == 0) || (byte0 == 0 && byte1 != 0)) {
            const QString str = QString::fromUtf16(
                  reinterpret_cast<const ushort *>(data.constData()), data.size() / 2);
            if (!str.isNull()) {
                if (format == QLatin1String("text/uri-list")) {
                    const auto urls = str.splitRef(QLatin1Char('\n'));
                    QList<QVariant> list;
                    list.reserve(urls.size());
                    for (const QStringRef &s : urls) {
                        const QUrl url(s.trimmed().toString());
                        if (url.isValid())
                            list.append(url);
                    }
                    // We expect "text/x-moz-url" as <url><space><title>.
                    // The atomName variable is not used because mimeAtomToString()
                    // converts "text/x-moz-url" to "text/uri-list".
                    if (!list.isEmpty() && connection->atomName(a) == "text/x-moz-url")
                        return list.constFirst();
                    return list;
                } else {
                    return str;
                }
            }
        }
        // 8 byte encoding, remove a possible 0 at the end
        if (data.endsWith('\0'))
            data.chop(1);
    }

    if (atomName == format)
        return data;

#if 0 // ###
    // special case for images
    if (format == QLatin1String("image/ppm")) {
        if (a == XCB_ATOM_PIXMAP && data.size() == sizeof(Pixmap)) {
            Pixmap xpm = *((Pixmap*)data.data());
            if (!xpm)
                return QByteArray();
            Window root;
            int x;
            int y;
            uint width;
            uint height;
            uint border_width;
            uint depth;

            XGetGeometry(display, xpm, &root, &x, &y, &width, &height, &border_width, &depth);
            XImage *ximg = XGetImage(display,xpm,x,y,width,height,AllPlanes,depth==1 ? XYPixmap : ZPixmap);
            QImage qimg = QXlibStatic::qimageFromXImage(ximg);
            XDestroyImage(ximg);

            QImageWriter imageWriter;
            imageWriter.setFormat("PPMRAW");
            QBuffer buf;
            buf.open(QIODevice::WriteOnly);
            imageWriter.setDevice(&buf);
            imageWriter.write(qimg);
            return buf.buffer();
        }
    }
#endif
    return QVariant();
}

xcb_atom_t QXcbMime::mimeAtomForFormat(QXcbConnection *connection, const QString &format, QMetaType::Type requestedType,
                                 const QVector<xcb_atom_t> &atoms, QByteArray *requestedEncoding)
{
    requestedEncoding->clear();

    // find matches for string types
    if (format == QLatin1String("text/plain")) {
        if (atoms.contains(connection->atom(QXcbAtom::UTF8_STRING)))
            return connection->atom(QXcbAtom::UTF8_STRING);
        if (atoms.contains(XCB_ATOM_STRING))
            return XCB_ATOM_STRING;
        if (atoms.contains(connection->atom(QXcbAtom::TEXT)))
            return connection->atom(QXcbAtom::TEXT);
    }

    // find matches for uri types
    if (format == QLatin1String("text/uri-list")) {
        xcb_atom_t a = connection->internAtom(format.toLatin1());
        if (a && atoms.contains(a))
            return a;
        a = connection->internAtom("text/x-moz-url");
        if (a && atoms.contains(a))
            return a;
    }

    // find match for image
    if (format == QLatin1String("image/ppm")) {
        if (atoms.contains(XCB_ATOM_PIXMAP))
            return XCB_ATOM_PIXMAP;
    }

    // for string/text requests try to use a format with a well-defined charset
    // first to avoid encoding problems
    if (requestedType == QMetaType::QString
        && format.startsWith(QLatin1String("text/"))
        && !format.contains(QLatin1String("charset="))) {

        QString formatWithCharset = format;
        formatWithCharset.append(QLatin1String(";charset=utf-8"));

        xcb_atom_t a = connection->internAtom(std::move(formatWithCharset).toLatin1());
        if (a && atoms.contains(a)) {
            *requestedEncoding = "utf-8";
            return a;
        }
    }

    xcb_atom_t a = connection->internAtom(format.toLatin1());
    if (a && atoms.contains(a))
        return a;

    return 0;
}

QT_END_NAMESPACE
