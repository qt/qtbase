/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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

int collectJson(const QStringList &jsonFiles, const QString &outputFile)
{
    qSetGlobalQHashSeed(0);

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
    if (jsonFiles.isEmpty()) {
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

    for (const QString &jsonFile: jsonFiles) {
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
