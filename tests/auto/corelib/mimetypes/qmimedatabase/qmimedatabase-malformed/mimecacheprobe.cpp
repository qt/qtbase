// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Victim process for tst_qmimedatabase_malformed
// Usage: mimecacheprobe <case> [dataFilePath]

#include "mimecacheprobe.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMimeDatabase>
#include <QtCore/QMimeType>
#include <QtCore/QStringList>

#include <cstdio>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList args = QCoreApplication::arguments();
    if (args.size() < 2) {
        fprintf(stderr, "mimecacheprobe: missing <case> argument\n");
        return 2;
    }
    const QString caseName = args.at(1);
    const QString dataFile = args.size() > 2 ? args.at(2) : QString();

    if ((caseName == QueryCase::Content || caseName == QueryCase::FileName)
        && dataFile.isEmpty()) {
        fprintf(stderr, "mimecacheprobe: case '%s' needs a data file argument\n",
                qPrintable(caseName));
        return 2;
    }

    QMimeDatabase db;
    if (caseName == QueryCase::Content) {
        const QMimeType t = db.mimeTypeForFile(dataFile, QMimeDatabase::MatchContent);
        printf("ok=%s\n", qPrintable(t.name()));
    } else if (caseName == QueryCase::FileName) {
        const QMimeType t = db.mimeTypeForFile(dataFile, QMimeDatabase::MatchDefault);
        printf("ok=%s\n", qPrintable(t.name()));
    } else if (caseName == QueryCase::Name) {
        const QMimeType t = db.mimeTypeForName(QLatin1StringView(ProbedMimeTypeName));
        printf("ok=%s parents=%s aliases=%s icon=%s genericIcon=%s\n",
               qPrintable(t.name()),
               qPrintable(t.parentMimeTypes().join(u',')),
               qPrintable(t.aliases().join(u',')),
               qPrintable(t.iconName()),
               qPrintable(t.genericIconName()));
    } else if (caseName == QueryCase::Ancestors) {
        // Usage: mimecacheprobe ancestors <mimetype> [<inheritsTarget>...]
        if (dataFile.isEmpty()) {
            fprintf(stderr, "mimecacheprobe: case 'ancestors' needs a mimetype argument\n");
            return 2;
        }
        const QStringList targets = args.mid(3);
        const QMimeType t = db.mimeTypeForName(dataFile);
        QString line = u"%1 parents=%2 ancestors=%3"_s.arg(
                t.name(), t.parentMimeTypes().join(u','), t.allAncestors().join(u','));
        for (const QString &target : targets)
            line += u" inherits(%1)=%2"_s.arg(target).arg(int(t.inherits(target)));
        printf("ok=%s\n", qPrintable(line));
    } else {
        fprintf(stderr, "mimecacheprobe: unknown case '%s'\n", qPrintable(caseName));
        return 2;
    }
    return 0;
}
