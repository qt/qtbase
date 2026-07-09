// Copyright (C) 2026 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A minimal find(1)-alike built on QDirListing: it recursively lists the
// directory given on the command line and prints each entry's file path, one
// per line, to stdout. It exists so tst_QDirListing can observe the native
// iteration path from inside a separate mount namespace (see bindMountDuplicateId).

#include <qcoreapplication.h>
#include <qdirlisting.h>

#include <cstdio>

using ItFlag = QDirListing::IteratorFlag;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst(); // argv[0]

    QDirListing::IteratorFlags flags = ItFlag::Recursive;
    if (!args.isEmpty() && args.constFirst() == QLatin1String("--follow")) {
        flags |= ItFlag::FollowDirSymlinks;
        args.removeFirst();
    }

    if (args.size() != 1) {
        std::fprintf(stderr, "usage: lister [--follow] <directory>\n");
        return 2;
    }

    // Cap the iteration so that a loop-detection regression manifests as a clear
    // failure here rather than hanging the parent test until its timeout.
    int guard = 100000;
    for (const auto &dirEntry : QDirListing(args.constFirst(), flags)) {
        std::printf("%s\n", qUtf8Printable(dirEntry.filePath()));
        if (--guard == 0) {
            std::fprintf(stderr, "lister: runaway iteration, aborting\n");
            return 3;
        }
    }
    return 0;
}
