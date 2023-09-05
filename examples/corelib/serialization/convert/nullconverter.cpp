// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "nullconverter.h"

using namespace Qt::StringLiterals;

static NullConverter nullConverter;
Converter *Converter::null = &nullConverter;

QString NullConverter::name() const
{
    return "null"_L1;
}

Converter::Directions NullConverter::directions() const
{
    return Direction::Out;
}

Converter::Options NullConverter::outputOptions() const
{
    return SupportsArbitraryMapKeys;
}

const char *NullConverter::optionsHelp() const
{
    return nullptr;
}

bool NullConverter::probeFile(QIODevice *f) const
{
    Q_UNUSED(f);
    return false;
}

QVariant NullConverter::loadFile(QIODevice *f, const Converter *&outputConverter) const
{
    Q_UNUSED(f);
    Q_UNUSED(outputConverter);
    outputConverter = this;
    return QVariant();
}

void NullConverter::saveFile(QIODevice *f, const QVariant &contents,
                             const QStringList &options) const
{
    if (!options.isEmpty()) {
        fprintf(stderr, "Unknown option '%s' to null output. This format has no options.\n",
                qPrintable(options.first()));
        exit(EXIT_FAILURE);
    }

    Q_UNUSED(f);
    Q_UNUSED(contents);
}
