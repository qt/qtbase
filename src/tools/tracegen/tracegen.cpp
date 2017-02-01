/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "provider.h"
#include "lttng.h"
#include "etw.h"
#include "panic.h"

#include <qstring.h>
#include <qfile.h>

enum class Target
{
    LTTNG,
    ETW
};

static inline void usage(int status)
{
    printf("Usage: tracegen <lttng|etw> <input file> <output file>\n");
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
    } else {
        fprintf(stderr, "Invalid target: %s\n", targetString);
        usage(EXIT_FAILURE);
    }

    *inFile = QLatin1String(argv[2]);
    *outFile = QLatin1String(argv[3]);
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
    case Target::LTTNG:
        writeLttng(out, p);
        break;
    case Target::ETW:
        writeEtw(out, p);
        break;
    }

    return 0;
}
