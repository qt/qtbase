// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

class tst_TestA : public QObject
{
    Q_OBJECT

private slots:
    void slotName() const
    {
        QVERIFY(true);
    }

    void aDifferentSlot() const
    {
        QVERIFY(false);
    }
};

class tst_TestB : public QObject
{
    Q_OBJECT

private slots:
    void slotName() const
    {
        QVERIFY(true);
    }

    void aSecondDifferentSlot() const
    {
        QVERIFY(false);
    }
};

int main()
{
    char *argv[] = { const_cast<char *>("appName"), const_cast<char *>("slotName") };
    int argc = 2;

    tst_TestA testA;
    QTest::qExec(&testA, argc, argv);
    QTest::qExec(&testA, argc, argv);

    tst_TestB testB;
    QTest::qExec(&testB, argc, argv);

    return 0;
}

#include "tst_differentexec.moc"
