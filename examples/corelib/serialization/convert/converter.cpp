// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "converter.h"

//! [0]
Converter::Converter()
{
    converters().append(this);
}

Converter::~Converter()
{
    converters().removeAll(this);
}

QList<const Converter *> &Converter::converters()
{
    Q_CONSTINIT static QList<const Converter *> store;
    return store;
}

const QList<const Converter *> &Converter::allConverters()
{
    return converters();
}
//! [0]

// Some virtual methods that Converter classes needn't override, when not relevant:
Converter::Options Converter::outputOptions() const { return {}; }
const char *Converter::optionsHelp() const { return nullptr; }
bool Converter::probeFile(QIODevice *) const { return false; }

// The virtual method they should override if they claim to support In:
QVariant Converter::loadFile(QIODevice *, const Converter *&outputConverter) const
{
    Q_ASSERT(!directions().testFlag(Converter::Direction::In));
    // For those that don't, this should never be called.
    Q_UNIMPLEMENTED();
    // But every implementation should at least do this:
    if (!outputConverter)
        outputConverter = this;
    return QVariant();
}
