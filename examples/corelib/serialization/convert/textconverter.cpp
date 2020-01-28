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

#include "textconverter.h"

#include <QFile>
#include <QTextStream>

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

QString TextConverter::name()
{
    return QStringLiteral("text");
}

Converter::Direction TextConverter::directions()
{
    return InOut;
}

Converter::Options TextConverter::outputOptions()
{
    return {};
}

const char *TextConverter::optionsHelp()
{
    return nullptr;
}

bool TextConverter::probeFile(QIODevice *f)
{
    if (QFile *file = qobject_cast<QFile *>(f))
        return file->fileName().endsWith(QLatin1String(".txt"));
    return false;
}

QVariant TextConverter::loadFile(QIODevice *f, Converter *&outputConverter)
{
    if (!outputConverter)
        outputConverter = this;

    QVariantList list;
    QTextStream in(f);
    QString line ;
    while (!in.atEnd()) {
        in.readLineInto(&line);

        bool ok;
        qint64 v = line.toLongLong(&ok);
        if (ok) {
            list.append(v);
            continue;
        }

        double d = line.toDouble(&ok);
        if (ok) {
            list.append(d);
            continue;
        }

        list.append(line);
    }

    return list;
}

void TextConverter::saveFile(QIODevice *f, const QVariant &contents, const QStringList &options)
{
    if (!options.isEmpty()) {
        fprintf(stderr, "Unknown option '%s' to text output. This format has no options.\n", qPrintable(options.first()));
        exit(EXIT_FAILURE);
    }

    QTextStream out(f);
    dumpVariant(out, contents);
}

static TextConverter textConverter;
