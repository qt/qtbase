// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <cmath>
#include <qlocale.h>
#include "qjsonwriter_p.h"
#include "qjson_p.h"
#include "private/qstringconverter_p.h"
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

static QByteArray escapedString(QStringView s)
{
    // give it a minimum size to ensure the resize() below always adds enough space
    QByteArray ba(qMax(s.size(), 16), Qt::Uninitialized);

    auto ba_const_start = [&]() { return reinterpret_cast<const uchar *>(ba.constData()); };
    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(ba.constData()));
    const uchar *ba_end = cursor + ba.size();
    const char16_t *src = s.utf16();
    const char16_t *const end = s.utf16() + s.size();

    while (src != end) {
        if (cursor >= ba_end - 6) {
            // ensure we have enough space
            qptrdiff pos = cursor - ba_const_start();
            ba.resize(ba.size()*2);
            cursor = reinterpret_cast<uchar *>(ba.data()) + pos;
            ba_end = ba_const_start() + ba.size();
        }

        char16_t u = *src++;
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

    ba.resize(cursor - ba_const_start());
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
        json += QByteArray::number(v.toInteger());
        break;
    case QCborValue::Double: {
        const double d = v.toDouble();
        if (qIsFinite(d))
            json += QByteArray::number(d, 'g', QLocale::FloatingPointShortest);
        else
            json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
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
