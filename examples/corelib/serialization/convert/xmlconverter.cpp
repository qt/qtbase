// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "xmlconverter.h"
#include "variantorderedmap.h"

#include <QBitArray>
#include <QtCborCommon>
#include <QFile>
#include <QFloat16>
#include <QMetaType>
#include <QRegularExpression>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace Qt::StringLiterals;

static const char xmlOptionHelp[] = "compact=no|yes              Use compact XML form.\n";

static XmlConverter xmlConverter;

static QVariant variantFromXml(QXmlStreamReader &xml, Converter::Options options);

static QVariantList listFromXml(QXmlStreamReader &xml, Converter::Options options)
{
    QVariantList list;
    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "list"_L1)) {
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

        qFatal("%lld:%lld: Invalid XML %s '%s'.", xml.lineNumber(), xml.columnNumber(),
               qPrintable(xml.tokenString()), qPrintable(xml.name().toString()));
    }

    xml.readNext();
    return list;
}

static VariantOrderedMap::value_type mapEntryFromXml(QXmlStreamReader &xml,
                                                     Converter::Options options)
{
    QVariant key, value;
    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "entry"_L1)) {
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

        qFatal("%lld:%lld: Invalid XML %s '%s'.", xml.lineNumber(), xml.columnNumber(),
               qPrintable(xml.tokenString()), qPrintable(xml.name().toString()));
    }

    return { key, value };
}

static QVariant mapFromXml(QXmlStreamReader &xml, Converter::Options options)
{
    QVariantMap map1;
    VariantOrderedMap map2;

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "map"_L1)) {
        xml.readNext();
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (xml.name() == "entry"_L1) {
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

        qFatal("%lld:%lld: Invalid XML %s '%s'.", xml.lineNumber(), xml.columnNumber(),
               qPrintable(xml.tokenString()), qPrintable(xml.name().toString()));
    }

    xml.readNext();
    if (options & Converter::SupportsArbitraryMapKeys)
        return QVariant::fromValue(map2);
    return map1;
}

static QVariant variantFromXml(QXmlStreamReader &xml, Converter::Options options)
{
    QStringView name = xml.name();
    if (name == "list"_L1)
        return listFromXml(xml, options);
    if (name == "map"_L1)
        return mapFromXml(xml, options);
    if (name != "value"_L1) {
        qFatal("%lld:%lld: Invalid XML key '%s'.",
               xml.lineNumber(), xml.columnNumber(), qPrintable(name.toString()));
    }

    QXmlStreamAttributes attrs = xml.attributes();
    QStringView type = attrs.value("type"_L1);

    forever {
        xml.readNext();
        if (xml.isComment())
            continue;
        if (xml.isCDATA() || xml.isCharacters() || xml.isEndElement())
            break;

        qFatal("%lld:%lld: Invalid XML %s '%s'.", xml.lineNumber(), xml.columnNumber(),
               qPrintable(xml.tokenString()), qPrintable(name.toString()));
    }

    QStringView text = xml.text();
    if (!xml.isCDATA())
        text = text.trimmed();

    QVariant result;
    if (type.isEmpty()) {
        // ok
    } else if (type == "number"_L1) {
        // try integer first
        bool ok;
        qint64 v = text.toLongLong(&ok);
        if (ok) {
            result = v;
        } else {
            // let's see floating point
            double d = text.toDouble(&ok);
            if (!ok) {
                qFatal("%lld:%lld: Invalid XML: could not interpret '%s' as a number.",
                       xml.lineNumber(), xml.columnNumber(), qPrintable(text.toString()));
            }
            result = d;
        }
    } else if (type == "bytes"_L1) {
        QByteArray data = text.toLatin1();
        QStringView encoding = attrs.value("encoding");
        if (encoding == "base64url"_L1) {
            result = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding);
        } else if (encoding == "hex"_L1) {
            result = QByteArray::fromHex(data);
        } else if (encoding.isEmpty() || encoding == "base64"_L1) {
            result = QByteArray::fromBase64(data);
        } else {
            qFatal("%lld:%lld: Invalid XML: unknown encoding '%s' for bytes.",
                   xml.lineNumber(), xml.columnNumber(), qPrintable(encoding.toString()));
        }
    } else if (type == "string"_L1) {
        result = text.toString();
    } else if (type == "null"_L1) {
        result = QVariant::fromValue(nullptr);
    } else if (type == "CBOR simple type"_L1) {
        result = QVariant::fromValue(QCborSimpleType(text.toShort()));
    } else if (type == "bits"_L1) {
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
                qFatal("%lld:%lld: Invalid XML: invalid bit string '%s'.",
                       xml.lineNumber(), xml.columnNumber(), qPrintable(text.toString()));
            }
        }
        ba.resize(n);
        result = ba;
    } else {
        int id = QMetaType::UnknownType;
        if (type == "datetime"_L1)
            id = QMetaType::QDateTime;
        else if (type == "url"_L1)
            id = QMetaType::QUrl;
        else if (type == "uuid"_L1)
            id = QMetaType::QUuid;
        else if (type == "regex"_L1)
            id = QMetaType::QRegularExpression;
        else
            id = QMetaType::fromName(type.toLatin1()).id();
        if (id == QMetaType::UnknownType) {
            qFatal("%lld:%lld: Invalid XML: unknown type '%s'.",
                   xml.lineNumber(), xml.columnNumber(), qPrintable(type.toString()));
        }

        result = text.toString();
        if (!result.convert(QMetaType(id))) {
            qFatal("%lld:%lld: Invalid XML: could not parse content as type '%s'.",
                   xml.lineNumber(), xml.columnNumber(), qPrintable(type.toString()));
        }
    }

    do {
        xml.readNext();
    } while (xml.isComment() || xml.isWhitespace());

    if (!xml.isEndElement()) {
        qFatal("%lld:%lld: Invalid XML %s '%s'.", xml.lineNumber(), xml.columnNumber(),
               qPrintable(xml.tokenString()), qPrintable(name.toString()));
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
        const VariantOrderedMap map = (type == QMetaType::QVariantMap)
                ? VariantOrderedMap(v.toMap())
                : qvariant_cast<VariantOrderedMap>(v);

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
        QString typeString = "type"_L1;
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
                if (copy.convert(QMetaType(QMetaType::QString))) {
                    xml.writeAttribute(typeString, QString::fromLatin1(typeName));
                    xml.writeCharacters(copy.toString());
                } else {
                    qFatal("XML: don't know how to serialize type '%s'.", typeName);
                }
            }
        }
        xml.writeEndElement();
    }
}

QString XmlConverter::name() const
{
    return "xml"_L1;
}

Converter::Directions XmlConverter::directions() const
{
    return Direction::InOut;
}

Converter::Options XmlConverter::outputOptions() const
{
    return SupportsArbitraryMapKeys;
}

const char *XmlConverter::optionsHelp() const
{
    return xmlOptionHelp;
}

bool XmlConverter::probeFile(QIODevice *f) const
{
    if (QFile *file = qobject_cast<QFile *>(f)) {
        if (file->fileName().endsWith(".xml"_L1))
            return true;
    }

    return f->isReadable() && f->peek(5) == "<?xml";
}

QVariant XmlConverter::loadFile(QIODevice *f, const Converter *&outputConverter) const
{
    if (!outputConverter)
        outputConverter = this;

    QXmlStreamReader xml(f);
    xml.readNextStartElement();
    QVariant v = variantFromXml(xml, outputConverter->outputOptions());
    if (xml.hasError())
        qFatal("XML error: %s", qPrintable(xml.errorString()));

    return v;
}

void XmlConverter::saveFile(QIODevice *f, const QVariant &contents,
                            const QStringList &options) const
{
    bool compact = false;
    for (const QString &s : options) {
        if (s == "compact=no"_L1) {
            compact = false;
        } else if (s == "compact=yes"_L1) {
            compact = true;
        } else {
            qFatal("Unknown option '%s' to XML output. Valid options are:\n%s",
                   qPrintable(s), xmlOptionHelp);
        }
    }

    QXmlStreamWriter xml(f);
    xml.setAutoFormatting(!compact);
    xml.writeStartDocument();
    variantToXml(xml, contents);
    xml.writeEndDocument();
}
