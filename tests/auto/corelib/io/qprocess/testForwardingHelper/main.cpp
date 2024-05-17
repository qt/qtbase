// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <fstream>
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        puts("Usage: testForwardingHelper <doneFilePath>");
        return 1;
    }
    fputs("out data", stdout);
    fflush(stdout);
    fputs("err data", stderr);
    fflush(stderr);
    std::ofstream out(argv[1]);
    out << "That's all folks!";
    return 0;
}
