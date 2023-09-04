// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef NULLCONVERTER_H
#define NULLCONVERTER_H

#include "converter.h"

class NullConverter : public Converter
{
    // Converter interface
public:
    QString name() const override;
    Directions directions() const override;
    Options outputOptions() const override;
    void saveFile(QIODevice *f, const QVariant &contents,
                  const QStringList &options) const override;
};

#endif // NULLCONVERTER_H
