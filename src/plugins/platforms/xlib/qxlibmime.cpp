/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qxlibmime.h"

#include "qxlibstatic.h"
#include "qxlibscreen.h"

#include <QtCore/QTextCodec>
#include <QtGui/QImageWriter>
#include <QtCore/QBuffer>

QXlibMime::QXlibMime()
    : QInternalMimeData()
{ }

QXlibMime::~QXlibMime()
{}





QString QXlibMime::mimeAtomToString(Display *display, Atom a)
{
    if (!a) return 0;

    if (a == XA_STRING || a == QXlibStatic::atom(QXlibStatic::UTF8_STRING)) {
        return "text/plain"; // some Xdnd clients are dumb
    }
    char *atom = XGetAtomName(display, a);
    QString result = QString::fromLatin1(atom);
    XFree(atom);
    return result;
}

Atom QXlibMime::mimeStringToAtom(Display *display, const QString &mimeType)
{
    if (mimeType.isEmpty())
        return 0;
    return XInternAtom(display, mimeType.toLatin1().constData(), False);
}

QStringList QXlibMime::mimeFormatsForAtom(Display *display, Atom a)
{
    QStringList formats;
    if (a) {
        QString atomName = mimeAtomToString(display, a);
        formats.append(atomName);

        // special cases for string type
        if (a == QXlibStatic::atom(QXlibStatic::UTF8_STRING)
                || a == XA_STRING
                || a == QXlibStatic::atom(QXlibStatic::TEXT)
                || a == QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT))
            formats.append(QLatin1String("text/plain"));

        // special cases for uris
        if (atomName == QLatin1String("text/x-moz-url"))
            formats.append(QLatin1String("text/uri-list"));

        // special case for images
        if (a == XA_PIXMAP)
            formats.append(QLatin1String("image/ppm"));
    }
    return formats;
}

bool QXlibMime::mimeDataForAtom(Display *display, Atom a, QMimeData *mimeData, QByteArray *data, Atom *atomFormat, int *dataFormat)
{
    bool ret = false;
    *atomFormat = a;
    *dataFormat = 8;
    QString atomName = mimeAtomToString(display, a);
    if (QInternalMimeData::hasFormatHelper(atomName, mimeData)) {
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        if (atomName == QLatin1String("application/x-color"))
            *dataFormat = 16;
        ret = true;
    } else {
        if ((a == QXlibStatic::atom(QXlibStatic::UTF8_STRING)
             || a == XA_STRING
             || a == QXlibStatic::atom(QXlibStatic::TEXT)
             || a == QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT))
            && QInternalMimeData::hasFormatHelper(QLatin1String("text/plain"), mimeData)) {
            if (a == QXlibStatic::atom(QXlibStatic::UTF8_STRING)){
                *data = QInternalMimeData::renderDataHelper(QLatin1String("text/plain"), mimeData);
                ret = true;
            } else if (a == XA_STRING) {
                *data = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                        QLatin1String("text/plain"), mimeData)).toLocal8Bit();
                ret = true;
            } else if (a == QXlibStatic::atom(QXlibStatic::TEXT)
                       || a == QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT)) {
                // the ICCCM states that TEXT and COMPOUND_TEXT are in the
                // encoding of choice, so we choose the encoding of the locale
                QByteArray strData = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                                     QLatin1String("text/plain"), mimeData)).toLocal8Bit();
                char *list[] = { strData.data(), NULL };

                XICCEncodingStyle style = (a == QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT))
                                        ? XCompoundTextStyle : XStdICCTextStyle;
                XTextProperty textprop;
                if (list[0] != NULL
                    && XmbTextListToTextProperty(display, list, 1, style,
                                                 &textprop) == Success) {
                    *atomFormat = textprop.encoding;
                    *dataFormat = textprop.format;
                    *data = QByteArray((const char *) textprop.value, textprop.nitems * textprop.format / 8);
                    ret = true;

                    XFree(textprop.value);
                }
            }
        } else if (atomName == QLatin1String("text/x-moz-url") &&
                   QInternalMimeData::hasFormatHelper(QLatin1String("text/uri-list"), mimeData)) {
            QByteArray uri = QInternalMimeData::renderDataHelper(
                             QLatin1String("text/uri-list"), mimeData).split('\n').first();
            QString mozUri = QString::fromLatin1(uri, uri.size());
            mozUri += QLatin1Char('\n');
            *data = QByteArray(reinterpret_cast<const char *>(mozUri.utf16()), mozUri.length() * 2);
            ret = true;
        } else if ((a == XA_PIXMAP || a == XA_BITMAP) && mimeData->hasImage()) {
            ret = true;
        }
    }
    return ret && data != 0;
}

QList<Atom> QXlibMime::mimeAtomsForFormat(Display *display, const QString &format)
{
    QList<Atom> atoms;
    atoms.append(mimeStringToAtom(display, format));

    // special cases for strings
    if (format == QLatin1String("text/plain")) {
        atoms.append(QXlibStatic::atom(QXlibStatic::UTF8_STRING));
        atoms.append(XA_STRING);
        atoms.append(QXlibStatic::atom(QXlibStatic::TEXT));
        atoms.append(QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT));
    }

    // special cases for uris
    if (format == QLatin1String("text/uri-list")) {
        atoms.append(mimeStringToAtom(display,QLatin1String("text/x-moz-url")));
    }

    //special cases for images
    if (format == QLatin1String("image/ppm"))
        atoms.append(XA_PIXMAP);
    if (format == QLatin1String("image/pbm"))
        atoms.append(XA_BITMAP);

    return atoms;
}

QVariant QXlibMime::mimeConvertToFormat(Display *display, Atom a, const QByteArray &data, const QString &format, QVariant::Type requestedType, const QByteArray &encoding)
{
    QString atomName = mimeAtomToString(display,a);
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
        if (a == QXlibStatic::atom(QXlibStatic::UTF8_STRING))
            return QString::fromUtf8(data);
        if (a == XA_STRING)
            return QString::fromLatin1(data);
        if (a == QXlibStatic::atom(QXlibStatic::TEXT)
                || a == QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT))
            // #### might be wrong for COMPUND_TEXT
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

    // special cas for images
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
    return QVariant();
}

Atom QXlibMime::mimeAtomForFormat(Display *display, const QString &format, QVariant::Type requestedType, const QList<Atom> &atoms, QByteArray *requestedEncoding)
{
    requestedEncoding->clear();

    // find matches for string types
    if (format == QLatin1String("text/plain")) {
        if (atoms.contains(QXlibStatic::atom(QXlibStatic::UTF8_STRING)))
            return QXlibStatic::atom(QXlibStatic::UTF8_STRING);
        if (atoms.contains(QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT)))
            return QXlibStatic::atom(QXlibStatic::COMPOUND_TEXT);
        if (atoms.contains(QXlibStatic::atom(QXlibStatic::TEXT)))
            return QXlibStatic::atom(QXlibStatic::TEXT);
        if (atoms.contains(XA_STRING))
            return XA_STRING;
    }

    // find matches for uri types
    if (format == QLatin1String("text/uri-list")) {
        Atom a = mimeStringToAtom(display,format);
        if (a && atoms.contains(a))
            return a;
        a = mimeStringToAtom(display,QLatin1String("text/x-moz-url"));
        if (a && atoms.contains(a))
            return a;
    }

    // find match for image
    if (format == QLatin1String("image/ppm")) {
        if (atoms.contains(XA_PIXMAP))
            return XA_PIXMAP;
    }

    // for string/text requests try to use a format with a well-defined charset
    // first to avoid encoding problems
    if (requestedType == QVariant::String
        && format.startsWith(QLatin1String("text/"))
        && !format.contains(QLatin1String("charset="))) {

        QString formatWithCharset = format;
        formatWithCharset.append(QLatin1String(";charset=utf-8"));

        Atom a = mimeStringToAtom(display,formatWithCharset);
        if (a && atoms.contains(a)) {
            *requestedEncoding = "utf-8";
            return a;
        }
    }

    Atom a = mimeStringToAtom(display,format);
    if (a && atoms.contains(a))
        return a;

    return 0;
}
