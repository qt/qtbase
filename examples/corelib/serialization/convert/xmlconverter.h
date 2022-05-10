// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef XMLCONVERTER_H
#define XMLCONVERTER_H

#include "converter.h"

class XmlConverter : public Converter
{
    // Converter interface
public:
    QString name() override;
    Direction directions() override;
    Options outputOptions() override;
    const char *optionsHelp() override;
    bool probeFile(QIODevice *f) override;
    QVariant loadFile(QIODevice *f, Converter *&outputConverter) override;
    void saveFile(QIODevice *f, const QVariant &contents, const QStringList &options) override;
};

#endif // XMLCONVERTER_H
