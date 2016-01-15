/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
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
#include <QSslEllipticCurve>
#include <QSslConfiguration>

class tst_QSslEllipticCurve : public QObject
{
    Q_OBJECT

#ifndef QT_NO_SSL
private Q_SLOTS:
    void constExpr();
    void construction();
    void fromShortName_data();
    void fromShortName();
    void fromLongName_data();
    void fromLongName();
#endif
};

#ifndef QT_NO_SSL

void tst_QSslEllipticCurve::constExpr()
{
#ifdef Q_COMPILER_CONSTEXPR
    // check that default ctor and op ==/!= are constexpr:
    char array1[QSslEllipticCurve() == QSslEllipticCurve() ?  1 : -1];
    char array2[QSslEllipticCurve() != QSslEllipticCurve() ? -1 :  1];
    Q_UNUSED(array1);
    Q_UNUSED(array2);
#else
    QSKIP("This test requires C++11 generalized constant expression support enabled in the compiler.");
#endif
}

void tst_QSslEllipticCurve::construction()
{
    QSslEllipticCurve curve;
    QCOMPARE(curve.isValid(), false);
    QCOMPARE(curve.shortName(), QString());
    QCOMPARE(curve.longName(), QString());
    QCOMPARE(curve.isTlsNamedCurve(), false);
}

void tst_QSslEllipticCurve::fromShortName_data()
{
    QTest::addColumn<QString>("shortName");
    QTest::addColumn<QSslEllipticCurve>("curve");
    QTest::addColumn<bool>("valid");

    QTest::newRow("QString()") << QString() << QSslEllipticCurve() << false;
    QTest::newRow("\"\"") << QString("") << QSslEllipticCurve() << false;
    QTest::newRow("does-not-exist") << QStringLiteral("does-not-exist") << QSslEllipticCurve() << false;
    Q_FOREACH (QSslEllipticCurve ec, QSslConfiguration::supportedEllipticCurves()) {
        const QString sN = ec.shortName();
        QTest::newRow(qPrintable("supported EC \"" + sN + '"')) << sN << ec << true;
        // At least in the OpenSSL impl, the short name is case-sensitive. That feels odd.
        //const QString SN = sN.toUpper();
        //QTest::newRow(qPrintable("supported EC \"" + SN + '"')) << SN << ec << true;
        //const QString sn = sN.toLower();
        //QTest::newRow(qPrintable("supported EC \"" + sn + '"')) << sn << ec << true;
    }
}

void tst_QSslEllipticCurve::fromShortName()
{
    QFETCH(QString, shortName);
    QFETCH(QSslEllipticCurve, curve);
    QFETCH(bool, valid);

    const QSslEllipticCurve result = QSslEllipticCurve::fromShortName(shortName);
    QCOMPARE(result, curve);
    QCOMPARE(result.isValid(), valid);
    QCOMPARE(result.shortName(), curve.shortName());
    QCOMPARE(result.shortName(), valid ? shortName : QString());
}

void tst_QSslEllipticCurve::fromLongName_data()
{
    QTest::addColumn<QString>("longName");
    QTest::addColumn<QSslEllipticCurve>("curve");
    QTest::addColumn<bool>("valid");

    QTest::newRow("QString()") << QString() << QSslEllipticCurve() << false;
    QTest::newRow("\"\"") << QString("") << QSslEllipticCurve() << false;
    QTest::newRow("does-not-exist") << QStringLiteral("does-not-exist") << QSslEllipticCurve() << false;
    Q_FOREACH (QSslEllipticCurve ec, QSslConfiguration::supportedEllipticCurves()) {
        const QString lN = ec.longName();
        QTest::newRow(qPrintable("supported EC \"" + lN + '"')) << lN << ec << true;
    }
}

void tst_QSslEllipticCurve::fromLongName()
{
    QFETCH(QString, longName);
    QFETCH(QSslEllipticCurve, curve);
    QFETCH(bool, valid);

    const QSslEllipticCurve result = QSslEllipticCurve::fromLongName(longName);
    QCOMPARE(result, curve);
    QCOMPARE(result.isValid(), valid);
    QCOMPARE(result.longName(), curve.longName());
    QCOMPARE(result.longName(), valid ? longName : QString());
}

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslEllipticCurve)
#include "tst_qsslellipticcurve.moc"
