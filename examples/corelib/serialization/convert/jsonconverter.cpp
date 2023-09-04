// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "jsonconverter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace Qt::StringLiterals;

static JsonConverter jsonConverter;

static const char jsonOptionHelp[] = "compact=no|yes              Use compact JSON form.\n";

static QJsonDocument convertFromVariant(const QVariant &v)
{
    QJsonDocument doc = QJsonDocument::fromVariant(v);
    if (!doc.isObject() && !doc.isArray())
        qFatal("Could not convert contents to JSON.");
    return doc;
}

QString JsonConverter::name() const
{
    return "json"_L1;
}

Converter::Directions JsonConverter::directions() const
{
    return Direction::InOut;
}

const char *JsonConverter::optionsHelp() const
{
    return jsonOptionHelp;
}

bool JsonConverter::probeFile(QIODevice *f) const
{
    if (QFile *file = qobject_cast<QFile *>(f)) {
        if (file->fileName().endsWith(".json"_L1))
            return true;
    }

    if (f->isReadable()) {
        QByteArray ba = f->peek(1);
        return ba == "{" || ba == "[";
    }
    return false;
}

QVariant JsonConverter::loadFile(QIODevice *f, const Converter *&outputConverter) const
{
    if (!outputConverter)
        outputConverter = this;

    QJsonParseError error;
    QJsonDocument doc;
    if (auto file = qobject_cast<QFile *>(f)) {
        const char *ptr = reinterpret_cast<char *>(file->map(0, file->size()));
        if (ptr)
            doc = QJsonDocument::fromJson(QByteArray::fromRawData(ptr, file->size()), &error);
    }

    if (doc.isNull())
        doc = QJsonDocument::fromJson(f->readAll(), &error);
    if (error.error) {
        qFatal("Could not parse JSON content: offset %d: %s",
               error.offset, qPrintable(error.errorString()));
    }
    if (isNull(outputConverter))
        return QVariant();
    return doc.toVariant();
}

void JsonConverter::saveFile(QIODevice *f, const QVariant &contents,
                             const QStringList &options) const
{
    QJsonDocument::JsonFormat format = QJsonDocument::Indented;
    for (const QString &s : options) {
        if (s == "compact=no"_L1) {
            format = QJsonDocument::Indented;
        } else if (s == "compact=yes"_L1) {
            format = QJsonDocument::Compact;
        } else {
            qFatal("Unknown option '%s' to JSON output. Valid options are:\n%s",
                   qPrintable(s), jsonOptionHelp);
        }
    }

    f->write(convertFromVariant(contents).toJson(format));
}
