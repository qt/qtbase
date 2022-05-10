// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>


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
#ifndef QT_NO_EXCEPTIONS
    void throwSomething() const { throw std::logic_error("This line doesn't throw"); }
#endif

private slots:
// Remove all test cases if exceptions are not available
#ifndef QT_NO_EXCEPTIONS
    void testCorrectStdTypes() const;
    void testCorrectStdExceptions() const;
    void testCorrectMyExceptions() const;
    void testCorrectNoException() const;

    void testFailInt() const;
    void testFailStdString() const;
    void testFailStdRuntimeError() const;
    void testFailMyException() const;
    void testFailMyDerivedException() const;

    void testFailNoException() const;
    void testFailNoException2() const;
#endif // !QT_NO_EXCEPTIONS
};



#ifndef QT_NO_EXCEPTIONS

void tst_VerifyExceptionThrown::testCorrectStdTypes() const
{
    QVERIFY_THROWS_EXCEPTION(int, throw int(5));
    QVERIFY_THROWS_EXCEPTION(float, throw float(9.8));
    QVERIFY_THROWS_EXCEPTION(bool, throw bool(true));
    QVERIFY_THROWS_EXCEPTION(std::string, throw std::string("some string"));
#if QT_DEPRECATED_SINCE(6, 3)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QVERIFY_EXCEPTION_THROWN(throw int(5), int);
    QVERIFY_EXCEPTION_THROWN(throw float(9.8), float);
    QVERIFY_EXCEPTION_THROWN(throw bool(true), bool);
    QVERIFY_EXCEPTION_THROWN(throw std::string("some string"), std::string);
    QT_WARNING_POP
#endif
}

void tst_VerifyExceptionThrown::testCorrectStdExceptions() const
{
    // same type
    QVERIFY_THROWS_EXCEPTION(std::exception, throw std::exception());
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, throw std::runtime_error("runtime error"));
    QVERIFY_THROWS_EXCEPTION(std::overflow_error, throw std::overflow_error("overflow error"));
#if QT_DEPRECATED_SINCE(6, 3)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QVERIFY_EXCEPTION_THROWN(throw std::exception(), std::exception);
    QVERIFY_EXCEPTION_THROWN(throw std::runtime_error("runtime error"), std::runtime_error);
    QVERIFY_EXCEPTION_THROWN(throw std::overflow_error("overflow error"), std::overflow_error);
    QT_WARNING_POP
#endif

    // inheritance
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, throw std::overflow_error("overflow error"));
    QVERIFY_THROWS_EXCEPTION(std::exception, throw std::overflow_error("overflow error"));
#if QT_DEPRECATED_SINCE(6, 3)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QVERIFY_EXCEPTION_THROWN(throw std::overflow_error("overflow error"), std::runtime_error);
    QVERIFY_EXCEPTION_THROWN(throw std::overflow_error("overflow error"), std::exception);
    QT_WARNING_POP
#endif
}

void tst_VerifyExceptionThrown::testCorrectMyExceptions() const
{
    // same type
    QVERIFY_THROWS_EXCEPTION(MyBaseException, throw MyBaseException());
    QVERIFY_THROWS_EXCEPTION(MyDerivedException, throw MyDerivedException());
#if QT_DEPRECATED_SINCE(6, 3)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QVERIFY_EXCEPTION_THROWN(throw MyBaseException(), MyBaseException);
    QVERIFY_EXCEPTION_THROWN(throw MyDerivedException(), MyDerivedException);
    QT_WARNING_POP
#endif

    // inheritance
    QVERIFY_THROWS_EXCEPTION(MyBaseException, throw MyDerivedException());
    QVERIFY_THROWS_EXCEPTION(std::domain_error, throw MyDerivedException());
#if QT_DEPRECATED_SINCE(6, 3)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QVERIFY_EXCEPTION_THROWN(throw MyDerivedException(), MyBaseException);
    QVERIFY_EXCEPTION_THROWN(throw MyDerivedException(), std::domain_error);
    QT_WARNING_POP
#endif
}

void tst_VerifyExceptionThrown::testCorrectNoException() const
{
    QVERIFY_THROWS_NO_EXCEPTION(doSomething());
}

void tst_VerifyExceptionThrown::testFailInt() const
{
    try {
        QVERIFY_THROWS_EXCEPTION(double, throw int(5));
    } catch (int) {}
}

void tst_VerifyExceptionThrown::testFailStdString() const
{
    try {
        QVERIFY_THROWS_EXCEPTION(char*, throw std::string("some string"));
    } catch (const std::string &) {}
}

void tst_VerifyExceptionThrown::testFailStdRuntimeError() const
{
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, throw std::logic_error("logic error"));
}

void tst_VerifyExceptionThrown::testFailMyException() const
{
    QVERIFY_THROWS_EXCEPTION(MyBaseException, throw std::logic_error("logic error"));
}

void tst_VerifyExceptionThrown::testFailMyDerivedException() const
{
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, throw MyDerivedException());
}

void tst_VerifyExceptionThrown::testFailNoException() const
{
    QVERIFY_THROWS_EXCEPTION(std::exception, doSomething());
}

void tst_VerifyExceptionThrown::testFailNoException2() const
{
    QVERIFY_THROWS_NO_EXCEPTION(throwSomething());
}

#endif // !QT_NO_EXCEPTIONS



QTEST_MAIN(tst_VerifyExceptionThrown)

#include "tst_verifyexceptionthrown.moc"
