// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qfile.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qhashfunctions.h>
#include <qstringlist.h>
#include <cstdlib>

static bool readFromDevice(QIODevice *device, QJsonArray *allMetaObjects)
{
    const QByteArray contents = device->readAll();
    if (contents.isEmpty())
        return true;

    QJsonParseError error {};
    QJsonDocument metaObjects = QJsonDocument::fromJson(contents, &error);
    if (error.error != QJsonParseError::NoError) {
        fprintf(stderr, "%s at %d\n", error.errorString().toUtf8().constData(), error.offset);
        return false;
    }

    allMetaObjects->append(metaObjects.object());
    return true;
}

int collectJson(const QStringList &jsonFiles, const QString &outputFile, bool skipStdIn)
{
    QHashSeed::setDeterministicGlobalSeed();

    QFile output;
    if (outputFile.isEmpty()) {
        if (!output.open(stdout, QIODevice::WriteOnly)) {
            fprintf(stderr, "Error opening stdout for writing\n");
            return EXIT_FAILURE;
        }
    } else {
        output.setFileName(outputFile);
        if (!output.open(QIODevice::WriteOnly)) {
            fprintf(stderr, "Error opening %s for writing\n", qPrintable(outputFile));
            return EXIT_FAILURE;
        }
    }

    QJsonArray allMetaObjects;
    if (jsonFiles.isEmpty() && !skipStdIn) {
        QFile f;
        if (!f.open(stdin, QIODevice::ReadOnly)) {
            fprintf(stderr, "Error opening stdin for reading\n");
            return EXIT_FAILURE;
        }

        if (!readFromDevice(&f, &allMetaObjects)) {
            fprintf(stderr, "Error parsing data from stdin\n");
            return EXIT_FAILURE;
        }
    }

    QStringList jsonFilesSorted = jsonFiles;
    jsonFilesSorted.sort();
    for (const QString &jsonFile : std::as_const(jsonFilesSorted)) {
        QFile f(jsonFile);
        if (!f.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Error opening %s for reading\n", qPrintable(jsonFile));
            return EXIT_FAILURE;
        }

        if (!readFromDevice(&f, &allMetaObjects)) {
            fprintf(stderr, "Error parsing %s\n", qPrintable(jsonFile));
            return EXIT_FAILURE;
        }
    }

    QJsonDocument doc(allMetaObjects);
    output.write(doc.toJson());

    return EXIT_SUCCESS;
}
