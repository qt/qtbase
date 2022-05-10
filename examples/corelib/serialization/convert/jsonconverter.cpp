// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "jsonconverter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

static JsonConverter jsonConverter;

static const char optionHelp[] =
        "compact=no|yes              Use compact JSON form.\n";

static QJsonDocument convertFromVariant(const QVariant &v)
{
    QJsonDocument doc = QJsonDocument::fromVariant(v);
    if (!doc.isObject() && !doc.isArray()) {
        fprintf(stderr, "Could not convert contents to JSON.\n");
        exit(EXIT_FAILURE);
    }
    return doc;
}

JsonConverter::JsonConverter()
{
}

QString JsonConverter::name()
{
    return "json";
}

Converter::Direction JsonConverter::directions()
{
    return InOut;
}

Converter::Options JsonConverter::outputOptions()
{
    return {};
}

const char *JsonConverter::optionsHelp()
{
    return optionHelp;
}

bool JsonConverter::probeFile(QIODevice *f)
{
    if (QFile *file = qobject_cast<QFile *>(f)) {
        if (file->fileName().endsWith(QLatin1String(".json")))
            return true;
    }

    if (f->isReadable()) {
        QByteArray ba = f->peek(1);
        return ba == "{" || ba == "[";
    }
    return false;
}

QVariant JsonConverter::loadFile(QIODevice *f, Converter *&outputConverter)
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
        fprintf(stderr, "Could not parse JSON content: offset %d: %s",
                error.offset, qPrintable(error.errorString()));
        exit(EXIT_FAILURE);
    }
    if (outputConverter == null)
        return QVariant();
    return doc.toVariant();
}

void JsonConverter::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    QJsonDocument::JsonFormat format = QJsonDocument::Indented;
    for (const QString &s : options) {
        if (s == QLatin1String("compact=no")) {
            format = QJsonDocument::Indented;
        } else if (s == QLatin1String("compact=yes")) {
            format = QJsonDocument::Compact;
        } else {
            fprintf(stderr, "Unknown option '%s' to JSON output. Valid options are:\n%s", qPrintable(s), optionHelp);
            exit(EXIT_FAILURE);
        }
    }

    f->write(convertFromVariant(contents).toJson(format));
}
