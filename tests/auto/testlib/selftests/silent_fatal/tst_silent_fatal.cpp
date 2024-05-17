// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QTest>
#include <private/qtestlog_p.h>

class tst_SilentFatal : public QObject
{
    Q_OBJECT

private slots:
    void fatalmessages();
};
void tst_SilentFatal::fatalmessages()
{
    qFatal("This is a fatal error message that should still appear in silent test output");
}

QTEST_MAIN_WRAPPER(tst_SilentFatal,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-silent");
    args.push_back("-nocrashhandler");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_silent_fatal.moc"
