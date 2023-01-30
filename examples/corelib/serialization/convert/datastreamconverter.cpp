// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "datastreamconverter.h"

#include <QDataStream>
#include <QDebug>
#include <QTextStream>

static const char dataStreamOptionHelp[] =
        "byteorder=host|big|little      Byte order to use.\n"
        "version=<n>                    QDataStream version (default: Qt 5.0).\n"
        ;

static const char signature[] = "qds";

static DataStreamDumper dataStreamDumper;
static DataStreamConverter DataStreamConverter;

QDataStream &operator<<(QDataStream &ds, const VariantOrderedMap &map)
{
    ds << qint64(map.size());
    for (const auto &pair : map)
        ds << pair.first << pair.second;
    return ds;
}

QDataStream &operator>>(QDataStream &ds, VariantOrderedMap &map)
{
    map.clear();

    qint64 size;
    ds >> size;
    map.reserve(size);

    while (size-- > 0) {
        VariantOrderedMap::value_type pair;
        ds >> pair.first >> pair.second;
        map.append(pair);
    }

    return ds;
}


static QString dumpVariant(const QVariant &v, const QString &indent = QLatin1String("\n"))
{
    QString result;
    QString indented = indent + QLatin1String("  ");

    int type = v.userType();
    if (type == qMetaTypeId<VariantOrderedMap>() || type == QMetaType::QVariantMap) {
        const auto map = (type == QMetaType::QVariantMap) ?
                    VariantOrderedMap(v.toMap()) : qvariant_cast<VariantOrderedMap>(v);

        result = QLatin1String("Map {");
        for (const auto &pair : map) {
            result += indented + dumpVariant(pair.first, indented);
            result.chop(1);         // remove comma
            result += QLatin1String(" => ") + dumpVariant(pair.second, indented);

        }
        result.chop(1);             // remove comma
        result += indent + QLatin1String("},");
    } else if (type == QMetaType::QVariantList) {
        const QVariantList list = v.toList();

        result = QLatin1String("List [");
        for (const auto &item : list)
            result += indented + dumpVariant(item, indented);
        result.chop(1);             // remove comma
        result += indent + QLatin1String("],");
    } else {
        QDebug debug(&result);
        debug.nospace() << v << ',';
    }
    return result;
}

QString DataStreamDumper::name()
{
    return QStringLiteral("datastream-dump");
}

Converter::Direction DataStreamDumper::directions()
{
    return Out;
}

Converter::Options DataStreamDumper::outputOptions()
{
    return SupportsArbitraryMapKeys;
}

const char *DataStreamDumper::optionsHelp()
{
    return nullptr;
}

bool DataStreamDumper::probeFile(QIODevice *f)
{
    Q_UNUSED(f);
    return false;
}

QVariant DataStreamDumper::loadFile(QIODevice *f, Converter *&outputConverter)
{
    Q_UNREACHABLE();
    Q_UNUSED(f);
    Q_UNUSED(outputConverter);
    return QVariant();
}

void DataStreamDumper::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    Q_UNUSED(options);
    QString s = dumpVariant(contents);
    s[s.size() - 1] = QLatin1Char('\n');    // replace the comma with newline

    QTextStream out(f);
    out << s;
}

DataStreamConverter::DataStreamConverter()
{
    qRegisterMetaType<VariantOrderedMap>();
}

QString DataStreamConverter::name()
{
    return QStringLiteral("datastream");
}

Converter::Direction DataStreamConverter::directions()
{
    return InOut;
}

Converter::Options DataStreamConverter::outputOptions()
{
    return SupportsArbitraryMapKeys;
}

const char *DataStreamConverter::optionsHelp()
{
    return dataStreamOptionHelp;
}

bool DataStreamConverter::probeFile(QIODevice *f)
{
    return f->isReadable() && f->peek(sizeof(signature) - 1) == signature;
}

QVariant DataStreamConverter::loadFile(QIODevice *f, Converter *&outputConverter)
{
    if (!outputConverter)
        outputConverter = &dataStreamDumper;

    char c;
    if (f->read(sizeof(signature) -1) != signature ||
            !f->getChar(&c) || (c != 'l' && c != 'B')) {
        fprintf(stderr, "Could not load QDataStream file: invalid signature.\n");
        exit(EXIT_FAILURE);
    }

    QDataStream ds(f);
    ds.setByteOrder(c == 'l' ? QDataStream::LittleEndian : QDataStream::BigEndian);

    std::underlying_type<QDataStream::Version>::type version;
    ds >> version;
    ds.setVersion(QDataStream::Version(version));

    QVariant result;
    ds >> result;
    return result;
}

void DataStreamConverter::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    QDataStream::Version version = QDataStream::Qt_5_0;
    auto order = QDataStream::ByteOrder(QSysInfo::ByteOrder);
    for (const QString &option : options) {
        const QStringList pair = option.split('=');
        if (pair.size() == 2) {
            if (pair.first() == "byteorder") {
                if (pair.last() == "little") {
                    order = QDataStream::LittleEndian;
                    continue;
                } else if (pair.last() == "big") {
                    order = QDataStream::BigEndian;
                    continue;
                } else if (pair.last() == "host") {
                    order = QDataStream::ByteOrder(QSysInfo::ByteOrder);
                    continue;
                }
            }
            if (pair.first() == "version") {
                bool ok;
                int n = pair.last().toInt(&ok);
                if (ok) {
                    version = QDataStream::Version(n);
                    continue;
                }

                fprintf(stderr, "Invalid version number '%s': must be a number from 1 to %d.\n",
                        qPrintable(pair.last()), QDataStream::Qt_DefaultCompiledVersion);
                exit(EXIT_FAILURE);
            }
        }

        fprintf(stderr, "Unknown QDataStream formatting option '%s'. Available options are:\n%s",
                qPrintable(option), dataStreamOptionHelp);
        exit(EXIT_FAILURE);
    }

    char c = order == QDataStream::LittleEndian ? 'l'  : 'B';
    f->write(signature);
    f->write(&c, 1);

    QDataStream ds(f);
    ds.setVersion(version);
    ds.setByteOrder(order);
    ds << std::underlying_type<decltype(version)>::type(version);
    ds << contents;
}
