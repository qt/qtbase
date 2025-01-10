// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "nullconverter.h"

using namespace Qt::StringLiterals;

static NullConverter nullConverter;
bool Converter::isNull(const Converter *converter)
{
    return converter == &nullConverter;
}

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

void NullConverter::saveFile(QIODevice *f, const QVariant &contents,
                             const QStringList &options) const
{
    if (!options.isEmpty()) {
        qFatal("Unknown option '%s' to null output. This format has no options.",
               qPrintable(options.first()));
    }

    Q_UNUSED(f);
    Q_UNUSED(contents);
}
