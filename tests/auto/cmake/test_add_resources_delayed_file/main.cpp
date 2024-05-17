// Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qglobal.h>

int main(int argc, char **argv)
{
    // Compile error if the resource file was not created.
    Q_INIT_RESOURCE(test_add_resources_delayed_file);
    return 0;
}
