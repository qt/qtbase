// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbmime.h"

#include <QtGui/QImageWriter>
#include <QtCore/QBuffer>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
        || a == connection->atom(QXcbAtom::AtomUTF8_STRING)
        || a == connection->atom(QXcbAtom::AtomTEXT))
        return "text/plain"_L1;

    // special case for images
    if (a == XCB_ATOM_PIXMAP)
        return "image/ppm"_L1;

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

    if ((a == connection->atom(QXcbAtom::AtomUTF8_STRING)
         || a == XCB_ATOM_STRING
         || a == connection->atom(QXcbAtom::AtomTEXT))
        && QInternalMimeData::hasFormatHelper("text/plain"_L1, mimeData)) {
        if (a == connection->atom(QXcbAtom::AtomUTF8_STRING)) {
            *data = QInternalMimeData::renderDataHelper("text/plain"_L1, mimeData);
            ret = true;
        } else if (a == XCB_ATOM_STRING ||
                   a == connection->atom(QXcbAtom::AtomTEXT)) {
            // ICCCM says STRING is latin1
            *data = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                        "text/plain"_L1, mimeData)).toLatin1();
            ret = true;
        }
        return ret;
    }

    QString atomName = mimeAtomToString(connection, a);
    if (QInternalMimeData::hasFormatHelper(atomName, mimeData)) {
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        // mimeAtomToString() converts "text/x-moz-url" to "text/uri-list",
        // so QXcbConnection::atomName() has to be used.
        if (atomName == "text/uri-list"_L1
            && connection->atomName(a) == "text/x-moz-url") {
            const QString mozUri = QLatin1StringView(data->split('\n').constFirst()) + u'\n';
            data->assign({reinterpret_cast<const char *>(mozUri.data()), mozUri.size() * 2});
        } else if (atomName == "application/x-color"_L1)
            *dataFormat = 16;
        ret = true;
    } else if ((a == XCB_ATOM_PIXMAP || a == XCB_ATOM_BITMAP) && mimeData->hasImage()) {
        ret = true;
    } else if (atomName == "text/plain"_L1 && mimeData->hasFormat("text/uri-list"_L1)) {
        // Return URLs also as plain text.
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        ret = true;
    }
    return ret;
}

QList<xcb_atom_t> QXcbMime::mimeAtomsForFormat(QXcbConnection *connection, const QString &format)
{
    QList<xcb_atom_t> atoms;
    atoms.reserve(7);
    atoms.append(connection->internAtom(format.toLatin1()));

    // special cases for strings
    if (format == "text/plain"_L1) {
        atoms.append(connection->atom(QXcbAtom::AtomUTF8_STRING));
        atoms.append(XCB_ATOM_STRING);
        atoms.append(connection->atom(QXcbAtom::AtomTEXT));
    }

    // special cases for uris
    if (format == "text/uri-list"_L1) {
        atoms.append(connection->internAtom("text/x-moz-url"));
        atoms.append(connection->internAtom("text/plain"));
    }

    //special cases for images
    if (format == "image/ppm"_L1)
        atoms.append(XCB_ATOM_PIXMAP);
    if (format == "image/pbm"_L1)
        atoms.append(XCB_ATOM_BITMAP);

    return atoms;
}

QVariant QXcbMime::mimeConvertToFormat(QXcbConnection *connection, xcb_atom_t a, const QByteArray &d, const QString &format,
                                       QMetaType requestedType, bool hasUtf8)
{
    QByteArray data = d;
    QString atomName = mimeAtomToString(connection, a);
//    qDebug() << "mimeConvertDataToFormat" << format << atomName << data;

    if (hasUtf8 && atomName == format + ";charset=utf-8"_L1) {
        if (requestedType.id() == QMetaType::QString)
            return QString::fromUtf8(data);
        return data;
    }

    // special cases for string types
    if (format == "text/plain"_L1) {
        if (data.endsWith('\0'))
            data.chop(1);
        if (a == connection->atom(QXcbAtom::AtomUTF8_STRING)) {
            return QString::fromUtf8(data);
        }
        if (a == XCB_ATOM_STRING ||
            a == connection->atom(QXcbAtom::AtomTEXT))
            return QString::fromLatin1(data);
    }
    // If data contains UTF16 text, convert it to a string.
    // Firefox uses UTF16 without BOM for text/x-moz-url, "text/html",
    // Google Chrome uses UTF16 without BOM for "text/x-moz-url",
    // UTF16 with BOM for "text/html".
    if ((format == "text/html"_L1 || format == "text/uri-list"_L1)
        && data.size() > 1) {
        const quint8 byte0 = data.at(0);
        const quint8 byte1 = data.at(1);
        if ((byte0 == 0xff && byte1 == 0xfe) || (byte0 == 0xfe && byte1 == 0xff)
            || (byte0 != 0 && byte1 == 0) || (byte0 == 0 && byte1 != 0)) {
            const QString str = QString::fromUtf16(
                  reinterpret_cast<const char16_t *>(data.constData()), data.size() / 2);
            if (!str.isNull()) {
                if (format == "text/uri-list"_L1) {
                    const auto urls = QStringView{str}.split(u'\n');
                    QList<QVariant> list;
                    list.reserve(urls.size());
                    for (const QStringView &s : urls) {
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
    if (format == "image/ppm"_L1) {
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

xcb_atom_t QXcbMime::mimeAtomForFormat(QXcbConnection *connection, const QString &format, QMetaType requestedType,
                                 const QList<xcb_atom_t> &atoms, bool *hasUtf8)
{
    *hasUtf8 = false;

    // find matches for string types
    if (format == "text/plain"_L1) {
        if (atoms.contains(connection->atom(QXcbAtom::AtomUTF8_STRING)))
            return connection->atom(QXcbAtom::AtomUTF8_STRING);
        if (atoms.contains(XCB_ATOM_STRING))
            return XCB_ATOM_STRING;
        if (atoms.contains(connection->atom(QXcbAtom::AtomTEXT)))
            return connection->atom(QXcbAtom::AtomTEXT);
    }

    // find matches for uri types
    if (format == "text/uri-list"_L1) {
        xcb_atom_t a = connection->internAtom(format.toLatin1());
        if (a && atoms.contains(a))
            return a;
        a = connection->internAtom("text/x-moz-url");
        if (a && atoms.contains(a))
            return a;
    }

    // find match for image
    if (format == "image/ppm"_L1) {
        if (atoms.contains(XCB_ATOM_PIXMAP))
            return XCB_ATOM_PIXMAP;
    }

    // for string/text requests try to use a format with a well-defined charset
    // first to avoid encoding problems
    if (requestedType.id() == QMetaType::QString
        && format.startsWith("text/"_L1)
        && !format.contains("charset="_L1)) {

        QString formatWithCharset = format;
        formatWithCharset.append(";charset=utf-8"_L1);

        xcb_atom_t a = connection->internAtom(std::move(formatWithCharset).toLatin1());
        if (a && atoms.contains(a)) {
            *hasUtf8 = true;
            return a;
        }
    }

    xcb_atom_t a = connection->internAtom(format.toLatin1());
    if (a && atoms.contains(a))
        return a;

    return 0;
}

QT_END_NAMESPACE

#include "moc_qxcbmime.cpp"
