// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_BenchlibWalltime: public QObject
{
    Q_OBJECT

private slots:
    void waitForOneThousand();
    void waitForFourThousand();
    void qbenchmark_once();
};

void tst_BenchlibWalltime::waitForOneThousand()
{
    QBENCHMARK {
        QTest::qWait(1000);
    }
}

void tst_BenchlibWalltime::waitForFourThousand()
{
    QBENCHMARK {
        QTest::qWait(4000);
    }
}

void tst_BenchlibWalltime::qbenchmark_once()
{
    int iterations = 0;
    QBENCHMARK_ONCE {
        ++iterations;
    }
    QCOMPARE(iterations, 1);
}


QTEST_MAIN(tst_BenchlibWalltime)

#include "tst_benchlibwalltime.moc"
