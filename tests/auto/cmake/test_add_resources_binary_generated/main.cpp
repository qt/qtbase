// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QFile>
#include <QResource>

int main(int, char **)
{
    if (!QResource::registerResource(RESOURCE1_FULL_PATH)
            || !QFile::exists(":/resource1.txt")
            || !QResource::registerResource(RESOURCE2_FULL_PATH)
            || !QFile::exists(":/resource2.txt")) {
        return -1;
    }

    // Avoid leaks
    QResource::unregisterResource(RESOURCE1_FULL_PATH);
    QResource::unregisterResource(RESOURCE2_FULL_PATH);
    return 0;
}
