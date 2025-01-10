// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONVERTER_H
#define CONVERTER_H

#include <QIODevice>
#include <QList>
#include <QStringList>
#include <QVariant>

//! [0]
class Converter
{
    static QList<const Converter *> &converters();
protected:
    Converter();
    static bool isNull(const Converter *converter); // in nullconverter.cpp

public:
    static const QList<const Converter *> &allConverters();

    enum class Direction { In = 1, Out = 2, InOut = In | Out };
    Q_DECLARE_FLAGS(Directions, Direction)

    enum Option { SupportsArbitraryMapKeys = 0x01 };
    Q_DECLARE_FLAGS(Options, Option)

    virtual ~Converter() = 0;

    virtual QString name() const = 0;
    virtual Directions directions() const = 0;
    virtual Options outputOptions() const;
    virtual const char *optionsHelp() const;
    virtual bool probeFile(QIODevice *f) const;
    virtual QVariant loadFile(QIODevice *f, const Converter *&outputConverter) const;
    virtual void saveFile(QIODevice *f, const QVariant &contents,
                          const QStringList &options) const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Converter::Directions)
Q_DECLARE_OPERATORS_FOR_FLAGS(Converter::Options)
//! [0]

#endif // CONVERTER_H
