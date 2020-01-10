/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
