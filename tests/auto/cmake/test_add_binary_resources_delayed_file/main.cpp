// Copyright (C) 2015 André Klitzing <aklitzing@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QFile>
#include <QResource>

int main(int argc, char **argv)
{
    if (QResource::registerResource("rcc_file.rcc") &&
        QFile::exists("://resource_file.txt") && QFile::exists("://resource_file_two.txt"))
    {
        QResource::unregisterResource("rcc_file.rcc");          // avoid leaks
        return 0;
    }

    return -1;
}
