// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <simple.h>

#include "test_file.h"
#include <qguiapplication.h>

int main( int argc, char **argv )
{
    QGuiApplication a( argc, argv );
    Simple s;
    SomeObject sc;
    return a.exec();
}
