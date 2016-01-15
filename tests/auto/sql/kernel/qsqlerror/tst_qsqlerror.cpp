/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qsqlerror.h>

class tst_QSqlError : public QObject
{
Q_OBJECT

public:
    tst_QSqlError();
    virtual ~tst_QSqlError();

private slots:
    void getSetCheck();
    void construction();
    void operators();
};

tst_QSqlError::tst_QSqlError()
{
}

tst_QSqlError::~tst_QSqlError()
{
}

// Testing get/set functions
void tst_QSqlError::getSetCheck()
{
    QSqlError obj1;
    // ErrorType QSqlError::type()
    // void QSqlError::setType(ErrorType)
    obj1.setType(QSqlError::ErrorType(QSqlError::NoError));
    QCOMPARE(QSqlError::ErrorType(QSqlError::NoError), obj1.type());
    obj1.setType(QSqlError::ErrorType(QSqlError::ConnectionError));
    QCOMPARE(QSqlError::ErrorType(QSqlError::ConnectionError), obj1.type());
    obj1.setType(QSqlError::ErrorType(QSqlError::StatementError));
    QCOMPARE(QSqlError::ErrorType(QSqlError::StatementError), obj1.type());
    obj1.setType(QSqlError::ErrorType(QSqlError::TransactionError));
    QCOMPARE(QSqlError::ErrorType(QSqlError::TransactionError), obj1.type());
    obj1.setType(QSqlError::ErrorType(QSqlError::UnknownError));
    QCOMPARE(QSqlError::ErrorType(QSqlError::UnknownError), obj1.type());

    // int QSqlError::number()
    // void QSqlError::setNumber(int)
    obj1.setNumber(0);
    QCOMPARE(0, obj1.number());
    obj1.setNumber(INT_MIN);
    QCOMPARE(INT_MIN, obj1.number());
    obj1.setNumber(INT_MAX);
    QCOMPARE(INT_MAX, obj1.number());
}

void tst_QSqlError::construction()
{
   QSqlError obj1("drivertext", "databasetext", QSqlError::UnknownError, 123);
   QCOMPARE(obj1.driverText(), QString("drivertext"));
   QCOMPARE(obj1.databaseText(), QString("databasetext"));
   QCOMPARE(obj1.type(), QSqlError::UnknownError);
   QCOMPARE(obj1.number(), 123);
   QCOMPARE(obj1.nativeErrorCode(), QStringLiteral("123"));
   QVERIFY(obj1.isValid());

   QSqlError obj2(obj1);
   QCOMPARE(obj2.driverText(), obj1.driverText());
   QCOMPARE(obj2.databaseText(), obj1.databaseText());
   QCOMPARE(obj2.type(), obj1.type());
   QCOMPARE(obj2.number(), obj1.number());
   QCOMPARE(obj2.nativeErrorCode(), obj1.nativeErrorCode());
   QVERIFY(obj2.isValid());

   QSqlError obj3 = obj2;
   QCOMPARE(obj3.driverText(), obj2.driverText());
   QCOMPARE(obj3.databaseText(), obj2.databaseText());
   QCOMPARE(obj3.type(), obj2.type());
   QCOMPARE(obj3.number(), obj2.number());
   QCOMPARE(obj3.nativeErrorCode(), obj2.nativeErrorCode());
   QVERIFY(obj3.isValid());

   QSqlError obj4;
   QVERIFY(!obj4.isValid());
   QCOMPARE(obj4.driverText(), QString());
   QCOMPARE(obj4.databaseText(), QString());
   QCOMPARE(obj4.type(), QSqlError::NoError);
   QCOMPARE(obj4.number(), -1);
   QCOMPARE(obj4.nativeErrorCode(), QString());

   QSqlError obj5(QStringLiteral("drivertext"), QStringLiteral("databasetext"),
                  QSqlError::UnknownError, QStringLiteral("123"));
   QCOMPARE(obj5.driverText(), QString("drivertext"));
   QCOMPARE(obj5.databaseText(), QString("databasetext"));
   QCOMPARE(obj5.type(), QSqlError::UnknownError);
   QCOMPARE(obj5.number(), 123);
   QCOMPARE(obj5.nativeErrorCode(), QStringLiteral("123"));
   QVERIFY(obj5.isValid());

   QSqlError obj6(QStringLiteral("drivertext"), QStringLiteral("databasetext"),
                  QSqlError::UnknownError, QStringLiteral("Err123"));
   QCOMPARE(obj6.driverText(), QString("drivertext"));
   QCOMPARE(obj6.databaseText(), QString("databasetext"));
   QCOMPARE(obj6.type(), QSqlError::UnknownError);
   QCOMPARE(obj6.number(), 0);
   QCOMPARE(obj6.nativeErrorCode(), QStringLiteral("Err123"));
   QVERIFY(obj6.isValid());

   // Default constructed object as constructed before Qt 5.3
   QSqlError obj7(QString(), QString(), QSqlError::NoError, -1);
   QVERIFY(!obj7.isValid());
   QCOMPARE(obj7.driverText(), QString());
   QCOMPARE(obj7.databaseText(), QString());
   QCOMPARE(obj7.type(), QSqlError::NoError);
   QCOMPARE(obj7.number(), -1);
   QCOMPARE(obj7.nativeErrorCode(), QString());

}

void tst_QSqlError::operators()
{
   QSqlError error1;
   QSqlError error2;
   QSqlError error3;

   error1.setType(QSqlError::NoError);
   error2.setType(QSqlError::NoError);
   error3.setType(QSqlError::UnknownError);

   QCOMPARE(error1, error2);
   QVERIFY(error1 != error3);
}


QTEST_MAIN(tst_QSqlError)
#include "tst_qsqlerror.moc"
