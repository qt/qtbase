// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "textconverter.h"

#include <QFile>
#include <QTextStream>

using namespace Qt::StringLiterals;

static void dumpVariant(QTextStream &out, const QVariant &v)
{
    switch (v.userType()) {
    case QMetaType::QVariantList: {
        const QVariantList list = v.toList();
        for (const QVariant &item : list)
            dumpVariant(out, item);
        break;
    }

    case QMetaType::QString: {
        const QStringList list = v.toStringList();
        for (const QString &s : list)
            out << s << Qt::endl;
        break;
    }

    case QMetaType::QVariantMap: {
        const QVariantMap map = v.toMap();
        for (auto it = map.begin(); it != map.end(); ++it) {
            out << it.key() << " => ";
            dumpVariant(out, it.value());
        }
        break;
    }

    case QMetaType::Nullptr:
        out << "(null)" << Qt::endl;
        break;

    default:
        out << v.toString() << Qt::endl;
        break;
    }
}

QString TextConverter::name() const
{
    return "text"_L1;
}

Converter::Directions TextConverter::directions() const
{
    return Direction::InOut;
}

bool TextConverter::probeFile(QIODevice *f) const
{
    if (QFile *file = qobject_cast<QFile *>(f))
        return file->fileName().endsWith(".txt"_L1);
    return false;
}

QVariant TextConverter::loadFile(QIODevice *f, const Converter *&outputConverter) const
{
    if (!outputConverter)
        outputConverter = this;

    QVariantList list;
    QTextStream in(f);
    QString line;
    while (!in.atEnd()) {
        in.readLineInto(&line);
        bool ok;

        if (qint64 v = line.toLongLong(&ok); ok)
            list.append(v);
        else if (double d = line.toDouble(&ok); ok)
            list.append(d);
        else
            list.append(line);
    }

    return list;
}

void TextConverter::saveFile(QIODevice *f, const QVariant &contents,
                             const QStringList &options) const
{
    if (!options.isEmpty()) {
        qFatal("Unknown option '%s' to text output. This format has no options.",
               qPrintable(options.first()));
    }

    QTextStream out(f);
    dumpVariant(out, contents);
}

static TextConverter textConverter;
