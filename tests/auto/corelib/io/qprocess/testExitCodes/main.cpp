// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <stdlib.h>
int main(int argc, char **argv)
{
    return argc >= 2 ? atoi(argv[1]) : -1;
}

