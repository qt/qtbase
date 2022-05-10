// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONVERTER_H
#define CONVERTER_H

#include <QIODevice>
#include <QPair>
#include <QVariant>
#include <QVariantMap>
#include <QList>

class VariantOrderedMap : public QList<QPair<QVariant, QVariant>>
{
public:
    VariantOrderedMap() = default;
    VariantOrderedMap(const QVariantMap &map)
    {
        reserve(map.size());
        for (auto it = map.begin(); it != map.end(); ++it)
            append({it.key(), it.value()});
    }
};
using Map = VariantOrderedMap;
Q_DECLARE_METATYPE(Map)

class Converter
{
protected:
    Converter();

public:
    static Converter *null;

    enum Direction {
        In = 1, Out = 2, InOut = 3
    };

    enum Option {
        SupportsArbitraryMapKeys = 0x01
    };
    Q_DECLARE_FLAGS(Options, Option)

    virtual ~Converter() = 0;

    virtual QString name() = 0;
    virtual Direction directions() = 0;
    virtual Options outputOptions() = 0;
    virtual const char *optionsHelp() = 0;
    virtual bool probeFile(QIODevice *f) = 0;
    virtual QVariant loadFile(QIODevice *f, Converter *&outputConverter) = 0;
    virtual void saveFile(QIODevice *f, const QVariant &contents, const QStringList &options) = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Converter::Options)

#endif // CONVERTER_H
