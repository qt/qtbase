// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "debugtextdumper.h"
#include "variantorderedmap.h"

#include <QDebug>
#include <QTextStream>

using namespace Qt::StringLiterals;

// Static instance is declared in datastreamconverter.cpp, since it uses it.

static QString dumpVariant(const QVariant &v, const QString &indent = "\n"_L1)
{
    QString result;
    QString indented = indent + "  "_L1;

    int type = v.userType();
    if (type == qMetaTypeId<VariantOrderedMap>() || type == QMetaType::QVariantMap) {
        const auto map = (type == QMetaType::QVariantMap) ? VariantOrderedMap(v.toMap())
                                                          : qvariant_cast<VariantOrderedMap>(v);

        result = "Map {"_L1;
        for (const auto &pair : map) {
            result += indented + dumpVariant(pair.first, indented);
            result.chop(1); // remove comma
            result += " => "_L1 + dumpVariant(pair.second, indented);
        }
        result.chop(1); // remove comma
        result += indent + "},"_L1;
    } else if (type == QMetaType::QVariantList) {
        const QVariantList list = v.toList();

        result = "List ["_L1;
        for (const auto &item : list)
            result += indented + dumpVariant(item, indented);
        result.chop(1); // remove comma
        result += indent + "],"_L1;
    } else {
        QDebug debug(&result);
        debug.nospace() << v << ',';
    }
    return result;
}

QString DebugTextDumper::name() const
{
    return "debugtext-dump"_L1;
}

Converter::Directions DebugTextDumper::directions() const
{
    return Direction::Out;
}

Converter::Options DebugTextDumper::outputOptions() const
{
    return SupportsArbitraryMapKeys;
}

void DebugTextDumper::saveFile(QIODevice *f, const QVariant &contents,
                               const QStringList &options) const
{
    if (!options.isEmpty()) {
        qFatal("Unknown option '%s' to debug text output. This format has no options.",
               qPrintable(options.first()));
    }
    QString s = dumpVariant(contents);
    s[s.size() - 1] = u'\n'; // replace the comma with newline

    QTextStream out(f);
    out << s;
}
