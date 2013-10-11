/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>


#ifndef QT_NO_EXCEPTIONS
#  include <stdexcept>
#endif


#ifndef QT_NO_EXCEPTIONS

class MyBaseException
{

};

class MyDerivedException: public MyBaseException, public std::domain_error
{
public:
    MyDerivedException(): std::domain_error("MyDerivedException") {}
};


#endif // !QT_NO_EXCEPTIONS


class tst_VerifyExceptionThrown: public QObject
{
    Q_OBJECT
private:
    void doSomething() const {}

private slots:
// Remove all test cases if exceptions are not available
#ifndef QT_NO_EXCEPTIONS
    void testCorrectStdTypes() const;
    void testCorrectStdExceptions() const;
    void testCorrectMyExceptions() const;

    void testFailInt() const;
    void testFailStdString() const;
    void testFailStdRuntimeError() const;
    void testFailMyException() const;
    void testFailMyDerivedException() const;

    void testFailNoException() const;
#endif // !QT_NO_EXCEPTIONS
};



#ifndef QT_NO_EXCEPTIONS

void tst_VerifyExceptionThrown::testCorrectStdTypes() const
{
    QVERIFY_EXCEPTION_THROWN(throw int(5), int);
    QVERIFY_EXCEPTION_THROWN(throw float(9.8), float);
    QVERIFY_EXCEPTION_THROWN(throw bool(true), bool);
    QVERIFY_EXCEPTION_THROWN(throw std::string("some string"), std::string);
}

void tst_VerifyExceptionThrown::testCorrectStdExceptions() const
{
    // same type
    QVERIFY_EXCEPTION_THROWN(throw std::exception(), std::exception);
    QVERIFY_EXCEPTION_THROWN(throw std::runtime_error("runtime error"), std::runtime_error);
    QVERIFY_EXCEPTION_THROWN(throw std::overflow_error("overflow error"), std::overflow_error);

    // inheritance
    QVERIFY_EXCEPTION_THROWN(throw std::overflow_error("overflow error"), std::runtime_error);
    QVERIFY_EXCEPTION_THROWN(throw std::overflow_error("overflow error"), std::exception);
}

void tst_VerifyExceptionThrown::testCorrectMyExceptions() const
{
    // same type
    QVERIFY_EXCEPTION_THROWN(throw MyBaseException(), MyBaseException);
    QVERIFY_EXCEPTION_THROWN(throw MyDerivedException(), MyDerivedException);

    // inheritance
    QVERIFY_EXCEPTION_THROWN(throw MyDerivedException(), MyBaseException);
    QVERIFY_EXCEPTION_THROWN(throw MyDerivedException(), std::domain_error);
}

void tst_VerifyExceptionThrown::testFailInt() const
{
    QVERIFY_EXCEPTION_THROWN(throw int(5), double);
}

void tst_VerifyExceptionThrown::testFailStdString() const
{
    QVERIFY_EXCEPTION_THROWN(throw std::string("some string"), char*);
}

void tst_VerifyExceptionThrown::testFailStdRuntimeError() const
{
    QVERIFY_EXCEPTION_THROWN(throw std::logic_error("logic error"), std::runtime_error);
}

void tst_VerifyExceptionThrown::testFailMyException() const
{
    QVERIFY_EXCEPTION_THROWN(throw std::logic_error("logic error"), MyBaseException);
}

void tst_VerifyExceptionThrown::testFailMyDerivedException() const
{
    QVERIFY_EXCEPTION_THROWN(throw MyDerivedException(), std::runtime_error);
}

void tst_VerifyExceptionThrown::testFailNoException() const
{
    QVERIFY_EXCEPTION_THROWN(doSomething(), std::exception);
}

#endif // !QT_NO_EXCEPTIONS



QTEST_MAIN(tst_VerifyExceptionThrown)

#include "tst_verifyexceptionthrown.moc"
