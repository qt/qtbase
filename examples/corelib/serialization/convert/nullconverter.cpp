// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "nullconverter.h"

static NullConverter nullConverter;
Converter* Converter::null = &nullConverter;

QString NullConverter::name()
{
    return QLatin1String("null");
}

Converter::Direction NullConverter::directions()
{
    return Out;
}

Converter::Options NullConverter::outputOptions()
{
    return SupportsArbitraryMapKeys;
}

const char *NullConverter::optionsHelp()
{
    return nullptr;
}

bool NullConverter::probeFile(QIODevice *f)
{
    Q_UNUSED(f);
    return false;
}

QVariant NullConverter::loadFile(QIODevice *f, Converter *&outputConverter)
{
    Q_UNUSED(f);
    Q_UNUSED(outputConverter);
    outputConverter = this;
    return QVariant();
}

void NullConverter::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    if (!options.isEmpty()) {
        fprintf(stderr, "Unknown option '%s' to null output. This format has no options.\n", qPrintable(options.first()));
        exit(EXIT_FAILURE);
    }

    Q_UNUSED(f);
    Q_UNUSED(contents);
}
