// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>

#include "tracepointgen.h"
#include "parser.h"

static void usage(int status)
{
    printf("Generates a tracepoint file for tracegen tool from input files.\n\n");
    printf("Usage: tracepointgen <output file> <input files> \n");
    exit(status);
}

static void parseArgs(int argc, char *argv[], QString &provider, QString &outFile, QList<QString> &inputFiles)
{
    if (argc == 1)
        usage(0);
    if (argc < 4)
        usage(-1);

    provider = QLatin1StringView(argv[1]);
    outFile = QLatin1StringView(argv[2]);
    for (int i = 3; i < argc; i++)
        inputFiles.append(QLatin1StringView(argv[i]));
}

int main(int argc, char *argv[])
{
    QString provider;
    QList<QString> inputFiles;
    QString outFile;

    parseArgs(argc, argv, provider, outFile, inputFiles);

    Parser parser(provider);

    for (const QString &inputFile : inputFiles) {
        if (inputFile.startsWith(QLatin1Char('I'))) {
            QStringList includeDirs = inputFile.right(inputFile.length() - 1).split(QLatin1Char(';'));
            parser.addIncludeDirs(includeDirs);
            continue;
        }
        QFile in(inputFile);
        if (!in.open(QIODevice::ReadOnly | QIODevice::Text)) {
            panic("Cannot open '%s' for reading: %s\n",
                    qPrintable(inputFile), qPrintable(in.errorString()));
        }
        DEBUGPRINTF(printf("tracepointgen: parse %s\n", qPrintable(inputFile)));
        parser.parse(in, inputFile);
    }
    if (parser.isEmpty())
        panic("empty provider %s\n", qPrintable(provider));

    QFile out(outFile);

    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        panic("Cannot open '%s' for writing: %s\n",
                qPrintable(outFile), qPrintable(out.errorString()));
    }

    parser.write(out);
    out.close();

    return 0;
}
