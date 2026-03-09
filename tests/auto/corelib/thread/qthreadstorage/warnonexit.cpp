// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QGlobalStatic>
#include <QThreadStorage>

struct S
{
    ~S() {}
    int i = 0;
};

Q_GLOBAL_STATIC(QThreadStorage<S>, object1)
Q_GLOBAL_STATIC(QThreadStorage<S>, object2)

int main(int argc, char **argv)
{
    // IMPORTANT: do not create any QObjects prior to QCoreApplication
    qputenv("QT_FORCE_STDERR_LOGGING", "1");
    qputenv("QT_FATAL_WARNINGS", "1");

    // one object before and one after QCoreApplication
    object1->localData().i = 1;
    QCoreApplication app(argc, argv);
    object2->localData().i = 2;

    // exit without destroying the QCoreApplication
    exit(0);
}
