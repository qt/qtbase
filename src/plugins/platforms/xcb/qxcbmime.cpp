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

#include "qxcbmime.h"

#include <QtCore/QTextCodec>
#include <QtGui/QImageWriter>
#include <QtCore/QBuffer>

#include <X11/Xutil.h>

#undef XA_STRING
#undef XA_PIXMAP
#undef XA_BITMAP

QXcbMime::QXcbMime()
    : QInternalMimeData()
{ }

QXcbMime::~QXcbMime()
{}



QString QXcbMime::mimeAtomToString(QXcbConnection *connection, xcb_atom_t a)
{
    if (a == XCB_NONE)
        return 0;

    // special cases for string type
    if (a == connection->atom(QXcbAtom::XA_STRING)
            || a == connection->atom(QXcbAtom::UTF8_STRING)
            || a == connection->atom(QXcbAtom::TEXT)
            || a == connection->atom(QXcbAtom::COMPOUND_TEXT))
        return QLatin1String("text/plain");

    // special case for images
    if (a == connection->atom(QXcbAtom::XA_PIXMAP))
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
         || a == connection->atom(QXcbAtom::XA_STRING)
         || a == connection->atom(QXcbAtom::TEXT)
         || a == connection->atom(QXcbAtom::COMPOUND_TEXT))
        && QInternalMimeData::hasFormatHelper(QLatin1String("text/plain"), mimeData)) {
        if (a == connection->atom(QXcbAtom::UTF8_STRING)){
            *data = QInternalMimeData::renderDataHelper(QLatin1String("text/plain"), mimeData);
            ret = true;
        } else if (a == connection->atom(QXcbAtom::XA_STRING)) {
            *data = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                        QLatin1String("text/plain"), mimeData)).toLocal8Bit();
            ret = true;
        } else if (a == connection->atom(QXcbAtom::TEXT)
                   || a == connection->atom(QXcbAtom::COMPOUND_TEXT)) {
            // the ICCCM states that TEXT and COMPOUND_TEXT are in the
            // encoding of choice, so we choose the encoding of the locale
            QByteArray strData = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                                 QLatin1String("text/plain"), mimeData)).toLocal8Bit();
            char *list[] = { strData.data(), NULL };

            XICCEncodingStyle style = (a == connection->atom(QXcbAtom::COMPOUND_TEXT))
                                    ? XCompoundTextStyle : XStdICCTextStyle;
            XTextProperty textprop;
            if (list[0] != NULL
                && XmbTextListToTextProperty(DISPLAY_FROM_XCB(connection), list, 1, style, &textprop) == Success) {
                *atomFormat = textprop.encoding;
                *dataFormat = textprop.format;
                *data = QByteArray((const char *) textprop.value, textprop.nitems * textprop.format / 8);
                ret = true;

                XFree(textprop.value);
            }
        }
        return ret;
    }

    QString atomName = mimeAtomToString(connection, a);
    if (QInternalMimeData::hasFormatHelper(atomName, mimeData)) {
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        if (atomName == QLatin1String("application/x-color"))
            *dataFormat = 16;
        ret = true;
    } else if (atomName == QLatin1String("text/x-moz-url") &&
               QInternalMimeData::hasFormatHelper(QLatin1String("text/uri-list"), mimeData)) {
        QByteArray uri = QInternalMimeData::renderDataHelper(
                         QLatin1String("text/uri-list"), mimeData).split('\n').first();
        QString mozUri = QString::fromLatin1(uri, uri.size());
        mozUri += QLatin1Char('\n');
        *data = QByteArray(reinterpret_cast<const char *>(mozUri.utf16()), mozUri.length() * 2);
        ret = true;
    } else if ((a == connection->atom(QXcbAtom::XA_PIXMAP) || a == connection->atom(QXcbAtom::XA_BITMAP)) && mimeData->hasImage()) {
        ret = true;
    }
    return ret;
}

QList<xcb_atom_t> QXcbMime::mimeAtomsForFormat(QXcbConnection *connection, const QString &format)
{
    QList<xcb_atom_t> atoms;
    atoms.append(connection->internAtom(format.toLatin1()));

    // special cases for strings
    if (format == QLatin1String("text/plain")) {
        atoms.append(connection->atom(QXcbAtom::UTF8_STRING));
        atoms.append(connection->atom(QXcbAtom::XA_STRING));
        atoms.append(connection->atom(QXcbAtom::TEXT));
        atoms.append(connection->atom(QXcbAtom::COMPOUND_TEXT));
    }

    // special cases for uris
    if (format == QLatin1String("text/uri-list"))
        atoms.append(connection->internAtom("text/x-moz-url"));

    //special cases for images
    if (format == QLatin1String("image/ppm"))
        atoms.append(connection->atom(QXcbAtom::XA_PIXMAP));
    if (format == QLatin1String("image/pbm"))
        atoms.append(connection->atom(QXcbAtom::XA_BITMAP));

    return atoms;
}

QVariant QXcbMime::mimeConvertToFormat(QXcbConnection *connection, xcb_atom_t a, const QByteArray &data, const QString &format,
                                       QVariant::Type requestedType, const QByteArray &encoding)
{
    QString atomName = mimeAtomToString(connection, a);
    if (atomName == format)
        return data;

    if (!encoding.isEmpty()
        && atomName == format + QLatin1String(";charset=") + QString::fromLatin1(encoding)) {

        if (requestedType == QVariant::String) {
            QTextCodec *codec = QTextCodec::codecForName(encoding);
            if (codec)
                return codec->toUnicode(data);
        }

        return data;
    }

    // special cases for string types
    if (format == QLatin1String("text/plain")) {
        if (a == connection->atom(QXcbAtom::UTF8_STRING))
            return QString::fromUtf8(data);
        if (a == connection->atom(QXcbAtom::XA_STRING))
            return QString::fromLatin1(data);
        if (a == connection->atom(QXcbAtom::TEXT)
                || a == connection->atom(QXcbAtom::COMPOUND_TEXT))
            // #### might be wrong for COMPOUND_TEXT
            return QString::fromLocal8Bit(data, data.size());
    }

    // special case for uri types
    if (format == QLatin1String("text/uri-list")) {
        if (atomName == QLatin1String("text/x-moz-url")) {
            // we expect this as utf16 <url><space><title>
            // the first part is a url that should only contain ascci char
            // so it should be safe to check that the second char is 0
            // to verify that it is utf16
            if (data.size() > 1 && data.at(1) == 0)
                return QString::fromRawData((const QChar *)data.constData(),
                                data.size() / 2).split(QLatin1Char('\n')).first().toLatin1();
        }
    }

#if 0 // ###
    // special case for images
    if (format == QLatin1String("image/ppm")) {
        if (a == XA_PIXMAP && data.size() == sizeof(Pixmap)) {
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

xcb_atom_t QXcbMime::mimeAtomForFormat(QXcbConnection *connection, const QString &format, QVariant::Type requestedType,
                                 const QList<xcb_atom_t> &atoms, QByteArray *requestedEncoding)
{
    requestedEncoding->clear();

    // find matches for string types
    if (format == QLatin1String("text/plain")) {
        if (atoms.contains(connection->atom(QXcbAtom::UTF8_STRING)))
            return connection->atom(QXcbAtom::UTF8_STRING);
        if (atoms.contains(connection->atom(QXcbAtom::COMPOUND_TEXT)))
            return connection->atom(QXcbAtom::COMPOUND_TEXT);
        if (atoms.contains(connection->atom(QXcbAtom::TEXT)))
            return connection->atom(QXcbAtom::TEXT);
        if (atoms.contains(connection->atom(QXcbAtom::XA_STRING)))
            return connection->atom(QXcbAtom::XA_STRING);
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
        if (atoms.contains(connection->atom(QXcbAtom::XA_PIXMAP)))
            return connection->atom(QXcbAtom::XA_PIXMAP);
    }

    // for string/text requests try to use a format with a well-defined charset
    // first to avoid encoding problems
    if (requestedType == QVariant::String
        && format.startsWith(QLatin1String("text/"))
        && !format.contains(QLatin1String("charset="))) {

        QString formatWithCharset = format;
        formatWithCharset.append(QLatin1String(";charset=utf-8"));

        xcb_atom_t a = connection->internAtom(formatWithCharset.toLatin1());
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
