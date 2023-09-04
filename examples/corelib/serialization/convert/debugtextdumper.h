// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DEBUGTEXTDUMPER_H
#define DEBUGTEXTDUMPER_H

#include "converter.h"

class DebugTextDumper : public Converter
{
    // Converter interface
public:
    QString name() const override;
    Directions directions() const override;
    Options outputOptions() const override;
    void saveFile(QIODevice *f, const QVariant &contents,
                  const QStringList &options) const override;
};

#endif // DEBUGTEXTDUMPER_H
