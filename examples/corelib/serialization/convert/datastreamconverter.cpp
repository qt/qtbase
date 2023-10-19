// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "datastreamconverter.h"
#include "debugtextdumper.h"
#include "variantorderedmap.h"

#include <QDataStream>

using namespace Qt::StringLiterals;

static const char dataStreamOptionHelp[] =
        "byteorder=host|big|little      Byte order to use.\n"
        "version=<n>                    QDataStream version (default: Qt 6.0).\n"
        ;

static const char signature[] = "qds";

static DataStreamConverter dataStreamConverter;
static DebugTextDumper debugTextDumper;

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

DataStreamConverter::DataStreamConverter()
{
    qRegisterMetaType<VariantOrderedMap>();
}

QString DataStreamConverter::name() const
{
    return "datastream"_L1;
}

Converter::Directions DataStreamConverter::directions() const
{
    return Direction::InOut;
}

Converter::Options DataStreamConverter::outputOptions() const
{
    return SupportsArbitraryMapKeys;
}

const char *DataStreamConverter::optionsHelp() const
{
    return dataStreamOptionHelp;
}

bool DataStreamConverter::probeFile(QIODevice *f) const
{
    return f->isReadable() && f->peek(sizeof(signature) - 1) == signature;
}

QVariant DataStreamConverter::loadFile(QIODevice *f, const Converter *&outputConverter) const
{
    if (!outputConverter)
        outputConverter = &debugTextDumper;

    char c;
    if (f->read(sizeof(signature) - 1) != signature || !f->getChar(&c) || (c != 'l' && c != 'B'))
        qFatal("Could not load QDataStream file: invalid signature.");

    QDataStream ds(f);
    ds.setByteOrder(c == 'l' ? QDataStream::LittleEndian : QDataStream::BigEndian);

    std::underlying_type<QDataStream::Version>::type version;
    ds >> version;
    ds.setVersion(QDataStream::Version(version));

    QVariant result;
    ds >> result;
    return result;
}

void DataStreamConverter::saveFile(QIODevice *f, const QVariant &contents,
                                   const QStringList &options) const
{
    QDataStream::Version version = QDataStream::Qt_6_0;
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

                qFatal("Invalid version number '%s': must be a number from 1 to %d.",
                       qPrintable(pair.last()), QDataStream::Qt_DefaultCompiledVersion);
            }
        }

        qFatal("Unknown QDataStream formatting option '%s'. Available options are:\n%s",
                qPrintable(option), dataStreamOptionHelp);
    }

    char c = order == QDataStream::LittleEndian ? 'l' : 'B';
    f->write(signature);
    f->write(&c, 1);

    QDataStream ds(f);
    ds.setVersion(version);
    ds.setByteOrder(order);
    ds << std::underlying_type<decltype(version)>::type(version);
    ds << contents;
}
