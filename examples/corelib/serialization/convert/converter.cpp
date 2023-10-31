// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "converter.h"

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
