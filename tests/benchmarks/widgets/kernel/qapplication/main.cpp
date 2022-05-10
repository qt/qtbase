// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QApplication>

#include <qtest.h>


class tst_qapplication : public QObject
{
    Q_OBJECT
private slots:
    void ctor();
};

/*
    Test the performance of the QApplication constructor.

    Note: results from the second start on can be misleading,
    since all global statics are already initialized.
*/
void tst_qapplication::ctor()
{
    // simulate reasonable argc, argv
    int argc = 1;
    char *argv[] = { const_cast<char*>("tst_qapplication") };
    QBENCHMARK {
        QApplication app(argc, argv);
    }
}

QTEST_APPLESS_MAIN(tst_qapplication)

#include "main.moc"
