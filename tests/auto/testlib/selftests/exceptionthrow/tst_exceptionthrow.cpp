// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

class tst_Exception: public QObject
{
    Q_OBJECT

private slots:
    void throwException() const;
};

/*!
 \internal

 We simply throw an exception to check that we get sane output/reporting.
 */
void tst_Exception::throwException() const
{
    /* When exceptions are disabled, some compilers, at least linux-g++, treat
     * exception clauses as hard errors. */
#ifndef QT_NO_EXCEPTIONS
    throw 3;
#endif
}

QTEST_MAIN(tst_Exception)

#include "tst_exceptionthrow.moc"
