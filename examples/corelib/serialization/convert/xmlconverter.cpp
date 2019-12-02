/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "xmlconverter.h"

#include <QBitArray>
#include <QtCborCommon>
#include <QFile>
#include <QFloat16>
#include <QMetaType>
#include <QRegularExpression>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

static const char optionHelp[] =
        "compact=no|yes              Use compact XML form.\n";

static XmlConverter xmlConverter;

static QVariant variantFromXml(QXmlStreamReader &xml, Converter::Options options);

static QVariantList listFromXml(QXmlStreamReader &xml, Converter::Options options)
{
    QVariantList list;
    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == QLatin1String("list"))) {
        xml.readNext();
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            list << variantFromXml(xml, options);
            continue;

        case QXmlStreamReader::EndElement:
            continue;

        case QXmlStreamReader::Comment:
            // ignore comments
            continue;

        case QXmlStreamReader::Characters:
            // ignore whitespace
            if (xml.isWhitespace())
                continue;
            Q_FALLTHROUGH();

        default:
            break;
        }

        fprintf(stderr, "%lld:%lld: Invalid XML %s '%s'.\n",
                xml.lineNumber(), xml.columnNumber(),
                qPrintable(xml.tokenString()), qPrintable(xml.name().toString()));
        exit(EXIT_FAILURE);
    }

    xml.readNext();
    return list;
}

static VariantOrderedMap::value_type mapEntryFromXml(QXmlStreamReader &xml, Converter::Options options)
{
    QVariant key, value;
    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == QLatin1String("entry"))) {
        xml.readNext();
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (value.isValid())
                break;
            if (key.isValid())
                value = variantFromXml(xml, options);
            else
                key = variantFromXml(xml, options);
            continue;

        case QXmlStreamReader::EndElement:
            continue;

        case QXmlStreamReader::Comment:
            // ignore comments
            continue;

        case QXmlStreamReader::Characters:
            // ignore whitespace
            if (xml.isWhitespace())
                continue;
            Q_FALLTHROUGH();

        default:
            break;
        }

        fprintf(stderr, "%lld:%lld: Invalid XML %s '%s'.\n",
                xml.lineNumber(), xml.columnNumber(),
                qPrintable(xml.tokenString()), qPrintable(xml.name().toString()));
        exit(EXIT_FAILURE);
    }

    return { key, value };
}

static QVariant mapFromXml(QXmlStreamReader &xml, Converter::Options options)
{
    QVariantMap map1;
    VariantOrderedMap map2;

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == QLatin1String("map"))) {
        xml.readNext();
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (xml.name() == QLatin1String("entry")) {
                auto pair = mapEntryFromXml(xml, options);
                if (options & Converter::SupportsArbitraryMapKeys)
                    map2.append(pair);
                else
                    map1.insert(pair.first.toString(), pair.second);
                continue;
            }
            break;

        case QXmlStreamReader::EndElement:
            continue;

        case QXmlStreamReader::Comment:
            // ignore comments
            continue;

        case QXmlStreamReader::Characters:
            // ignore whitespace
            if (xml.isWhitespace())
                continue;
            Q_FALLTHROUGH();

        default:
            break;
        }

        fprintf(stderr, "%lld:%lld: Invalid XML %s '%s'.\n",
                xml.lineNumber(), xml.columnNumber(),
                qPrintable(xml.tokenString()), qPrintable(xml.name().toString()));
        exit(EXIT_FAILURE);
    }

    xml.readNext();
    if (options & Converter::SupportsArbitraryMapKeys)
        return QVariant::fromValue(map2);
    return map1;
}

static QVariant variantFromXml(QXmlStreamReader &xml, Converter::Options options)
{
    QStringRef name = xml.name();
    if (name == QLatin1String("list"))
        return listFromXml(xml, options);
    if (name == QLatin1String("map"))
        return mapFromXml(xml, options);
    if (name != QLatin1String("value")) {
        fprintf(stderr, "%lld:%lld: Invalid XML key '%s'.\n",
                xml.lineNumber(), xml.columnNumber(), qPrintable(name.toString()));
        exit(EXIT_FAILURE);
    }

    QXmlStreamAttributes attrs = xml.attributes();
    QStringRef type = attrs.value(QLatin1String("type"));

    forever {
        xml.readNext();
        if (xml.isComment())
            continue;
        if (xml.isCDATA() || xml.isCharacters() || xml.isEndElement())
            break;

        fprintf(stderr, "%lld:%lld: Invalid XML %s '%s'.\n",
                xml.lineNumber(), xml.columnNumber(),
                qPrintable(xml.tokenString()), qPrintable(name.toString()));
        exit(EXIT_FAILURE);
    }

    QStringRef text = xml.text();
    if (!xml.isCDATA())
        text = text.trimmed();

    QVariant result;
    bool ok;
    if (type.isEmpty()) {
        // ok
    } else if (type == QLatin1String("number")) {
        // try integer first
        qint64 v = text.toLongLong(&ok);
        if (ok) {
            result = v;
        } else {
            // let's see floating point
            double d = text.toDouble(&ok);
            result = d;
            if (!ok) {
                fprintf(stderr, "%lld:%lld: Invalid XML: could not interpret '%s' as a number.\n",
                        xml.lineNumber(), xml.columnNumber(), qPrintable(text.toString()));
                exit(EXIT_FAILURE);
            }
        }
    } else if (type == QLatin1String("bytes")) {
        QByteArray data = text.toLatin1();
        QStringRef encoding = attrs.value("encoding");
        if (encoding == QLatin1String("base64url")) {
            result = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding);
        } else if (encoding == QLatin1String("hex")) {
            result = QByteArray::fromHex(data);
        } else if (encoding.isEmpty() || encoding == QLatin1String("base64")) {
            result = QByteArray::fromBase64(data);
        } else {
            fprintf(stderr, "%lld:%lld: Invalid XML: unknown encoding '%s' for bytes.\n",
                    xml.lineNumber(), xml.columnNumber(), qPrintable(encoding.toString()));
            exit(EXIT_FAILURE);
        }
    } else if (type == QLatin1String("string")) {
        result = text.toString();
    } else if (type == QLatin1String("null")) {
        result = QVariant::fromValue(nullptr);
    } else if (type == QLatin1String("CBOR simple type")) {
        result = QVariant::fromValue(QCborSimpleType(text.toShort()));
    } else if (type == QLatin1String("bits")) {
        QBitArray ba;
        ba.resize(text.size());
        qsizetype n = 0;
        for (qsizetype i = 0; i < text.size(); ++i) {
            QChar c = text.at(i);
            if (c == '1') {
                ba.setBit(n++);
            } else if (c == '0') {
                ++n;
            } else if (!c.isSpace()) {
                fprintf(stderr, "%lld:%lld: Invalid XML: invalid bit string '%s'.\n",
                        xml.lineNumber(), xml.columnNumber(), qPrintable(text.toString()));
                exit(EXIT_FAILURE);
            }
        }
        ba.resize(n);
        result = ba;
    } else {
        int id = QMetaType::UnknownType;
        if (type == QLatin1String("datetime"))
            id = QMetaType::QDateTime;
        else if (type == QLatin1String("url"))
            id = QMetaType::QUrl;
        else if (type == QLatin1String("uuid"))
            id = QMetaType::QUuid;
        else if (type == QLatin1String("regex"))
            id = QMetaType::QRegularExpression;
        else
            id = QMetaType::type(type.toLatin1());
        if (id == QMetaType::UnknownType) {
            fprintf(stderr, "%lld:%lld: Invalid XML: unknown type '%s'.\n",
                    xml.lineNumber(), xml.columnNumber(), qPrintable(type.toString()));
            exit(EXIT_FAILURE);
        }

        result = text.toString();
        if (!result.convert(id)) {
            fprintf(stderr, "%lld:%lld: Invalid XML: could not parse content as type '%s'.\n",
                    xml.lineNumber(), xml.columnNumber(), qPrintable(type.toString()));
            exit(EXIT_FAILURE);
        }
    }

    do {
        xml.readNext();
    } while (xml.isComment() || xml.isWhitespace());

    if (!xml.isEndElement()) {
        fprintf(stderr, "%lld:%lld: Invalid XML %s '%s'.\n",
                xml.lineNumber(), xml.columnNumber(),
                qPrintable(xml.tokenString()), qPrintable(name.toString()));
        exit(EXIT_FAILURE);
    }

    xml.readNext();
    return result;
}

static void variantToXml(QXmlStreamWriter &xml, const QVariant &v)
{
    int type = v.userType();
    if (type == QMetaType::QVariantList) {
        QVariantList list = v.toList();
        xml.writeStartElement("list");
        for (const QVariant &v : list)
            variantToXml(xml, v);
        xml.writeEndElement();
    } else if (type == QMetaType::QVariantMap || type == qMetaTypeId<VariantOrderedMap>()) {
        const VariantOrderedMap map = (type == QMetaType::QVariantMap) ?
                    VariantOrderedMap(v.toMap()) :
                    qvariant_cast<VariantOrderedMap>(v);

        xml.writeStartElement("map");
        for (const auto &pair : map) {
            xml.writeStartElement("entry");
            variantToXml(xml, pair.first);
            variantToXml(xml, pair.second);
            xml.writeEndElement();
        }
        xml.writeEndElement();
    } else {
        xml.writeStartElement("value");
        QString typeString = QStringLiteral("type");
        switch (type) {
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Long:
        case QMetaType::ULong:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Float:
        case QMetaType::Double:
            xml.writeAttribute(typeString, "number");
            xml.writeCharacters(v.toString());
            break;

        case QMetaType::QByteArray:
            xml.writeAttribute(typeString, "bytes");
            xml.writeAttribute("encoding", "base64");
            xml.writeCharacters(QString::fromLatin1(v.toByteArray().toBase64()));
            break;

        case QMetaType::QString:
            xml.writeAttribute(typeString, "string");
            xml.writeCDATA(v.toString());
            break;

        case QMetaType::Bool:
            xml.writeAttribute(typeString, "bool");
            xml.writeCharacters(v.toString());
            break;

        case QMetaType::Nullptr:
            xml.writeAttribute(typeString, "null");
            break;

        case QMetaType::UnknownType:
            break;

        case QMetaType::QDate:
        case QMetaType::QTime:
        case QMetaType::QDateTime:
            xml.writeAttribute(typeString, "dateime");
            xml.writeCharacters(v.toString());
            break;

        case QMetaType::QUrl:
            xml.writeAttribute(typeString, "url");
            xml.writeCharacters(v.toUrl().toString(QUrl::FullyEncoded));
            break;

        case QMetaType::QUuid:
            xml.writeAttribute(typeString, "uuid");
            xml.writeCharacters(v.toString());
            break;

        case QMetaType::QBitArray:
            xml.writeAttribute(typeString, "bits");
            xml.writeCharacters([](const QBitArray &ba) {
                QString result;
                for (qsizetype i = 0; i < ba.size(); ++i) {
                    if (i && i % 72 == 0)
                        result += '\n';
                    result += QLatin1Char(ba.testBit(i) ? '1' : '0');
                }
                return result;
            }(v.toBitArray()));
            break;

        case QMetaType::QRegularExpression:
            xml.writeAttribute(typeString, "regex");
            xml.writeCharacters(v.toRegularExpression().pattern());
            break;

        default:
            if (type == qMetaTypeId<qfloat16>()) {
                xml.writeAttribute(typeString, "number");
                xml.writeCharacters(QString::number(float(qvariant_cast<qfloat16>(v))));
            } else if (type == qMetaTypeId<QCborSimpleType>()) {
                xml.writeAttribute(typeString, "CBOR simple type");
                xml.writeCharacters(QString::number(int(qvariant_cast<QCborSimpleType>(v))));
            } else {
                // does this convert to string?
                const char *typeName = v.typeName();
                QVariant copy = v;
                if (copy.convert(QMetaType::QString)) {
                    xml.writeAttribute(typeString, QString::fromLatin1(typeName));
                    xml.writeCharacters(copy.toString());
                } else {
                    fprintf(stderr, "XML: don't know how to serialize type '%s'.\n", typeName);
                    exit(EXIT_FAILURE);
                }
            }
        }
        xml.writeEndElement();
    }
}

QString XmlConverter::name()
{
    return QStringLiteral("xml");
}

Converter::Direction XmlConverter::directions()
{
    return InOut;
}

Converter::Options XmlConverter::outputOptions()
{
    return SupportsArbitraryMapKeys;
}

const char *XmlConverter::optionsHelp()
{
    return optionHelp;
}

bool XmlConverter::probeFile(QIODevice *f)
{
    if (QFile *file = qobject_cast<QFile *>(f)) {
        if (file->fileName().endsWith(QLatin1String(".xml")))
            return true;
    }

    return f->isReadable() && f->peek(5) == "<?xml";
}

QVariant XmlConverter::loadFile(QIODevice *f, Converter *&outputConverter)
{
    if (!outputConverter)
        outputConverter = this;

    QXmlStreamReader xml(f);
    xml.readNextStartElement();
    QVariant v = variantFromXml(xml, outputConverter->outputOptions());
    if (xml.hasError()) {
        fprintf(stderr, "XML error: %s", qPrintable(xml.errorString()));
        exit(EXIT_FAILURE);
    }

    return v;
}

void XmlConverter::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    bool compact = false;
    for (const QString &s : options) {
        if (s == QLatin1String("compact=no")) {
            compact = false;
        } else if (s == QLatin1String("compact=yes")) {
            compact = true;
        } else {
            fprintf(stderr, "Unknown option '%s' to XML output. Valid options are:\n%s", qPrintable(s), optionHelp);
            exit(EXIT_FAILURE);
        }
    }

    QXmlStreamWriter xml(f);
    xml.setAutoFormatting(!compact);
    xml.writeStartDocument();
    variantToXml(xml, contents);
    xml.writeEndDocument();
}
