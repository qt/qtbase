// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Make sure we get a real Q_ASSERT even in release builds
#ifdef QT_NO_DEBUG
# undef QT_NO_DEBUG
#endif

#include <QtCore/QCoreApplication>
#include <QTest>

class tst_Assert: public QObject
{
    Q_OBJECT

private slots:
    void testNumber1() const;
    void testNumber2() const;
    void testNumber3() const;
};

void tst_Assert::testNumber1() const
{
}

void tst_Assert::testNumber2() const
{
    Q_ASSERT(false);
}

void tst_Assert::testNumber3() const
{
}

QTEST_MAIN(tst_Assert)

#include "tst_assert.moc"
