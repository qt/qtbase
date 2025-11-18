// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QProcess proc;

    QStringList args = app.arguments();
    args.takeFirst();
    if (args.isEmpty()) {
        printf("%s [normal|detached] <executable> [args...]\n",
               qPrintable(app.applicationFilePath()));
        return 0;
    }

    bool startDetached = (args.takeFirst() == "detached");
    proc.setProgram(args.takeFirst());
    proc.setArguments(args);    // the rest
    proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);

    // Don't let our child share our stdin. On Windows, when the child dies,
    // the stdin handle wakes up from WaitForSingleObject.
    proc.setStandardInputFile(QProcess::nullDevice());

    qint64 pid = 0;
    if (startDetached) {
        proc.startDetached(&pid);
    } else {
        proc.start();
        if (proc.waitForStarted())
            pid = proc.processId();
    }
    if (pid <= 0) {
        fprintf(stderr, "%s\n", qPrintable(proc.errorString()));
        return 1;
    }

    // report grandchild PID
    printf("%lld\n", pid);
    fflush(stdout);

    // wait for our parent
    if (char line[128]; fgets(line, sizeof(line), stdin))
        printf("from parent: %s\n", line);
    else
        printf("nothing from parent\n");
    fflush(stdout);

    if (!startDetached) {
        proc.waitForFinished(-1);   // wait forever
        QByteArray all = proc.readAllStandardOutput();
        printf("\nfrom child: %.*s", int(all.size()), all.constData());
        fflush(stdout);
    }
}
