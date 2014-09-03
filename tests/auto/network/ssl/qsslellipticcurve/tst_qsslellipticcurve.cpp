/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
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
#include <QSslEllipticCurve>
#include <QSslSocket>

class tst_QSslEllipticCurve : public QObject
{
    Q_OBJECT

#ifndef QT_NO_SSL
private Q_SLOTS:
    void constExpr();
    void construction();
    void fromShortName_data();
    void fromShortName();
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
    Q_FOREACH (QSslEllipticCurve ec, QSslSocket::supportedEllipticCurves()) {
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

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslEllipticCurve)
#include "tst_qsslellipticcurve.moc"
