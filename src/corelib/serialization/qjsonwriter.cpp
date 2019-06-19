/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <cmath>
#include <qlocale.h>
#include "qjsonwriter_p.h"
#include "qjson_p.h"
#include "private/qutfcodec_p.h"
#include <private/qnumeric_p.h>
#include <private/qcborvalue_p.h>

QT_BEGIN_NAMESPACE

using namespace QJsonPrivate;

static void objectContentToJson(const QCborContainerPrivate *o, QByteArray &json, int indent, bool compact);
static void arrayContentToJson(const QCborContainerPrivate *a, QByteArray &json, int indent, bool compact);

static inline uchar hexdig(uint u)
{
    return (u < 0xa ? '0' + u : 'a' + u - 0xa);
}

static QByteArray escapedString(const QString &s)
{
    QByteArray ba(s.length(), Qt::Uninitialized);

    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(ba.constData()));
    const uchar *ba_end = cursor + ba.length();
    const ushort *src = reinterpret_cast<const ushort *>(s.constBegin());
    const ushort *const end = reinterpret_cast<const ushort *>(s.constEnd());

    while (src != end) {
        if (cursor >= ba_end - 6) {
            // ensure we have enough space
            int pos = cursor - (const uchar *)ba.constData();
            ba.resize(ba.size()*2);
            cursor = (uchar *)ba.data() + pos;
            ba_end = (const uchar *)ba.constData() + ba.length();
        }

        uint u = *src++;
        if (u < 0x80) {
            if (u < 0x20 || u == 0x22 || u == 0x5c) {
                *cursor++ = '\\';
                switch (u) {
                case 0x22:
                    *cursor++ = '"';
                    break;
                case 0x5c:
                    *cursor++ = '\\';
                    break;
                case 0x8:
                    *cursor++ = 'b';
                    break;
                case 0xc:
                    *cursor++ = 'f';
                    break;
                case 0xa:
                    *cursor++ = 'n';
                    break;
                case 0xd:
                    *cursor++ = 'r';
                    break;
                case 0x9:
                    *cursor++ = 't';
                    break;
                default:
                    *cursor++ = 'u';
                    *cursor++ = '0';
                    *cursor++ = '0';
                    *cursor++ = hexdig(u>>4);
                    *cursor++ = hexdig(u & 0xf);
               }
            } else {
                *cursor++ = (uchar)u;
            }
        } else if (QUtf8Functions::toUtf8<QUtf8BaseTraits>(u, cursor, src, end) < 0) {
            // failed to get valid utf8 use JSON escape sequence
            *cursor++ = '\\';
            *cursor++ = 'u';
            *cursor++ = hexdig(u>>12 & 0x0f);
            *cursor++ = hexdig(u>>8 & 0x0f);
            *cursor++ = hexdig(u>>4 & 0x0f);
            *cursor++ = hexdig(u & 0x0f);
        }
    }

    ba.resize(cursor - (const uchar *)ba.constData());
    return ba;
}

static void valueToJson(const QCborValue &v, QByteArray &json, int indent, bool compact)
{
    QCborValue::Type type = v.type();
    switch (type) {
    case QCborValue::True:
        json += "true";
        break;
    case QCborValue::False:
        json += "false";
        break;
    case QCborValue::Integer:
    case QCborValue::Double: {
        const double d = v.toDouble();
        if (qIsFinite(d)) {
            quint64 absInt;
            json += QByteArray::number(d, convertDoubleTo(std::abs(d), &absInt) ? 'f' : 'g',
                                       QLocale::FloatingPointShortest);
        } else {
            json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
        }
        break;
    }
    case QCborValue::String:
        json += '"';
        json += escapedString(v.toString());
        json += '"';
        break;
    case QCborValue::Array:
        json += compact ? "[" : "[\n";
        arrayContentToJson(
                QJsonPrivate::Value::container(v), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4*indent, ' ');
        json += ']';
        break;
    case QCborValue::Map:
        json += compact ? "{" : "{\n";
        objectContentToJson(
                QJsonPrivate::Value::container(v), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4*indent, ' ');
        json += '}';
        break;
    case QCborValue::Null:
    default:
        json += "null";
    }
}

static void arrayContentToJson(const QCborContainerPrivate *a, QByteArray &json, int indent, bool compact)
{
    if (!a || a->elements.empty())
        return;

    QByteArray indentString(4*indent, ' ');

    qsizetype i = 0;
    while (true) {
        json += indentString;
        valueToJson(a->valueAt(i), json, indent, compact);

        if (++i == a->elements.size()) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}


static void objectContentToJson(const QCborContainerPrivate *o, QByteArray &json, int indent, bool compact)
{
    if (!o || o->elements.empty())
        return;

    QByteArray indentString(4*indent, ' ');

    qsizetype i = 0;
    while (true) {
        QCborValue e = o->valueAt(i);
        json += indentString;
        json += '"';
        json += escapedString(o->valueAt(i).toString());
        json += compact ? "\":" : "\": ";
        valueToJson(o->valueAt(i + 1), json, indent, compact);

        if ((i += 2) == o->elements.size()) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}

void Writer::objectToJson(const QCborContainerPrivate *o, QByteArray &json, int indent, bool compact)
{
    json.reserve(json.size() + (o ? (int)o->elements.size() : 16));
    json += compact ? "{" : "{\n";
    objectContentToJson(o, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4*indent, ' ');
    json += compact ? "}" : "}\n";
}

void Writer::arrayToJson(const QCborContainerPrivate *a, QByteArray &json, int indent, bool compact)
{
    json.reserve(json.size() + (a ? (int)a->elements.size() : 16));
    json += compact ? "[" : "[\n";
    arrayContentToJson(a, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4*indent, ' ');
    json += compact ? "]" : "]\n";
}

QT_END_NAMESPACE
