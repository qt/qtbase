// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

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
    void construction();
    void moveOperator();
    void operators();
    void qtbug_74575();
};

tst_QSqlError::tst_QSqlError()
{
}

tst_QSqlError::~tst_QSqlError()
{
}

void tst_QSqlError::construction()
{
   QSqlError obj1("drivertext", "databasetext", QSqlError::UnknownError, QStringLiteral("123"));
   QCOMPARE(obj1.driverText(), QString("drivertext"));
   QCOMPARE(obj1.databaseText(), QString("databasetext"));
   QCOMPARE(obj1.type(), QSqlError::UnknownError);
   QCOMPARE(obj1.nativeErrorCode(), QStringLiteral("123"));
   QVERIFY(obj1.isValid());

   QSqlError obj2(obj1);
   QCOMPARE(obj2.driverText(), obj1.driverText());
   QCOMPARE(obj2.databaseText(), obj1.databaseText());
   QCOMPARE(obj2.type(), obj1.type());
   QCOMPARE(obj2.nativeErrorCode(), obj1.nativeErrorCode());
   QVERIFY(obj2.isValid());

   QSqlError obj3 = obj2;
   QCOMPARE(obj3.driverText(), obj2.driverText());
   QCOMPARE(obj3.databaseText(), obj2.databaseText());
   QCOMPARE(obj3.type(), obj2.type());
   QCOMPARE(obj3.nativeErrorCode(), obj2.nativeErrorCode());
   QVERIFY(obj3.isValid());

   QSqlError obj4;
   QVERIFY(!obj4.isValid());
   QCOMPARE(obj4.driverText(), QString());
   QCOMPARE(obj4.databaseText(), QString());
   QCOMPARE(obj4.text(), QString());
   QCOMPARE(obj4.type(), QSqlError::NoError);
   QCOMPARE(obj4.nativeErrorCode(), QString());

   QSqlError obj5(QStringLiteral("drivertext"), QStringLiteral("databasetext"),
                  QSqlError::UnknownError, QStringLiteral("123"));
   QCOMPARE(obj5.driverText(), QString("drivertext"));
   QCOMPARE(obj5.databaseText(), QString("databasetext"));
   QCOMPARE(obj5.type(), QSqlError::UnknownError);
   QCOMPARE(obj5.nativeErrorCode(), QStringLiteral("123"));
   QVERIFY(obj5.isValid());

   QSqlError obj6(QStringLiteral("drivertext"), QStringLiteral("databasetext"),
                  QSqlError::UnknownError, QStringLiteral("Err123"));
   QCOMPARE(obj6.driverText(), QString("drivertext"));
   QCOMPARE(obj6.databaseText(), QString("databasetext"));
   QCOMPARE(obj6.type(), QSqlError::UnknownError);
   QCOMPARE(obj6.nativeErrorCode(), QStringLiteral("Err123"));
   QVERIFY(obj6.isValid());

   // Default constructed object
   QSqlError obj7;
   QVERIFY(!obj7.isValid());
   QCOMPARE(obj7.driverText(), QString());
   QCOMPARE(obj7.databaseText(), QString());
   QCOMPARE(obj7.type(), QSqlError::NoError);
   QCOMPARE(obj7.nativeErrorCode(), QString());

   // Move constructor
   QSqlError obj8(std::move(obj3));
   QCOMPARE(obj8.driverText(), obj2.driverText());
   QCOMPARE(obj8.databaseText(), obj2.databaseText());
   QCOMPARE(obj8.type(), obj2.type());
   QCOMPARE(obj8.nativeErrorCode(), obj2.nativeErrorCode());
   QVERIFY(obj8.isValid());
}

void tst_QSqlError::moveOperator()
{
   QSqlError obj1("drivertext", "databasetext", QSqlError::UnknownError, QStringLiteral("123")), obj2;
   obj2 = std::move(obj1);
   QCOMPARE(obj2.driverText(), QString("drivertext"));
   QCOMPARE(obj2.databaseText(), QString("databasetext"));
   QCOMPARE(obj2.type(), QSqlError::UnknownError);
   QCOMPARE(obj2.nativeErrorCode(), QStringLiteral("123"));
   QVERIFY(obj2.isValid());
}

void tst_QSqlError::operators()
{
   QSqlError error1(QStringLiteral("a"), QStringLiteral("b"), QSqlError::NoError, QStringLiteral("ec1"));
   QSqlError error2(QStringLiteral("c"), QStringLiteral("d"), QSqlError::NoError, QStringLiteral("ec1"));
   QSqlError error3(QString(), QString(), QSqlError::UnknownError, QStringLiteral("ec1"));
   QSqlError error4(QString(), QString(), QSqlError::NoError, QStringLiteral("ec2"));
   QSqlError error5(QString(), QString(), QSqlError::UnknownError, QStringLiteral("ec2"));

   QCOMPARE(error1, error2);
   QVERIFY(error1 != error3);
   QVERIFY(error1 != error4);
   QVERIFY(error4 != error5);
}

void tst_QSqlError::qtbug_74575()
{
   const QString driverText(QStringLiteral("drivertext"));
   const QString databaseText(QStringLiteral("databasetext"));
   const QString databaseTextNewline(QStringLiteral("databasetext\n"));

   QSqlError error1(driverText, databaseText,
                  QSqlError::UnknownError, QStringLiteral("123"));
   QCOMPARE(error1.text(), databaseText + QLatin1Char(' ') + driverText);

   QSqlError error2(QString(), databaseText,
                  QSqlError::UnknownError, QStringLiteral("123"));
   QCOMPARE(error2.text(), databaseText);

   QSqlError error3(driverText, QString(),
                  QSqlError::UnknownError, QStringLiteral("123"));
   QCOMPARE(error3.text(), driverText);

   QSqlError error4(driverText, databaseTextNewline,
                  QSqlError::UnknownError, QStringLiteral("123"));
   QCOMPARE(error4.text(), databaseTextNewline + driverText);
}

QTEST_MAIN(tst_QSqlError)
#include "tst_qsqlerror.moc"
