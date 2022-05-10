// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>

void startProcess()
{
    {
//! [0]
QProcess process;
QStringList env = QProcess::systemEnvironment();
env << "TMPDIR=C:\\MyApp\\temp"; // Add an environment variable
process.setEnvironment(env);
process.start("myapp");
//! [0]
    }

    {
//! [1]
QProcess process;
QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
env.insert("TMPDIR", "C:\\MyApp\\temp"); // Add an environment variable
process.setProcessEnvironment(env);
process.start("myapp");
//! [1]
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    startProcess();
    return app.exec();
}
