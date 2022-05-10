// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

class tst_Nothing: public QObject
{
Q_OBJECT
private slots:
    void nothing() { QVERIFY(true); }
};

int main(int argc, char *argv[])
{
    tst_Nothing nada;
    for (int i = 0; i < 5; ++i)
        QTest::qExec(&nada, argc, argv);
    return 0;
}

#include "tst_multiexec.moc"
