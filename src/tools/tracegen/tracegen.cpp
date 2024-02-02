// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "provider.h"
#include "ctf.h"
#include "lttng.h"
#include "etw.h"
#include "panic.h"

#include <qstring.h>
#include <qfile.h>

enum class Target
{
    LTTNG,
    ETW,
    CTF,
};

static inline void usage(int status)
{
    printf("Usage: tracegen <lttng|etw|ctf> <input file> <output file>\n");
    exit(status);
}

static void parseArgs(int argc, char *argv[], Target *target, QString *inFile, QString *outFile)
{
    if (argc == 1)
        usage(EXIT_SUCCESS);
    if (argc != 4)
        usage(EXIT_FAILURE);

    const char *targetString = argv[1];

    if (qstrcmp(targetString, "lttng") == 0) {
        *target = Target::LTTNG;
    } else if (qstrcmp(targetString, "etw") == 0) {
        *target = Target::ETW;
    } else if (qstrcmp(targetString, "ctf") == 0) {
        *target = Target::CTF;
    } else {
        fprintf(stderr, "Invalid target: %s\n", targetString);
        usage(EXIT_FAILURE);
    }

    *inFile = QLatin1StringView(argv[2]);
    *outFile = QLatin1StringView(argv[3]);
}

int main(int argc, char *argv[])
{
    Target target = Target::LTTNG;
    QString inFile;
    QString outFile;

    parseArgs(argc, argv, &target, &inFile, &outFile);

    Provider p = parseProvider(inFile);

    QFile out(outFile);

    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        panic("Cannot open '%s' for writing: %s",
                qPrintable(outFile), qPrintable(out.errorString()));
    }

    switch (target) {
    case Target::CTF:
        writeCtf(out, p);
        break;
    case Target::LTTNG:
        writeLttng(out, p);
        break;
    case Target::ETW:
        writeEtw(out, p);
        break;
    }

    return 0;
}
