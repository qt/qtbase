// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QProcess>
#include <qt_windows.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//! [0]
    QProcess process;
    process.setCreateProcessArgumentsModifier([] (QProcess::CreateProcessArguments *args)
    {
        args->flags |= CREATE_NEW_CONSOLE;
        args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
        args->startupInfo->dwFlags |= STARTF_USEFILLATTRIBUTE;
        args->startupInfo->dwFillAttribute = BACKGROUND_BLUE | FOREGROUND_RED
                                           | FOREGROUND_INTENSITY;
    });
    process.start("C:\\Windows\\System32\\cmd.exe", QStringList() << "/k" << "title" << "The Child Process");
//! [0]

    return app.exec();
}
