/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the FOO module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QAuthenticator>

#include <private/qauthenticator_p.h>

class tst_QAuthenticator : public QObject
{
    Q_OBJECT

public:
    tst_QAuthenticator();

private Q_SLOTS:
    void basicAuth();
    void basicAuth_data();

    void ntlmAuth_data();
    void ntlmAuth();
};

tst_QAuthenticator::tst_QAuthenticator()
{
}

void tst_QAuthenticator::basicAuth_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QString>("realm");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QByteArray>("expectedReply");

    QTest::newRow("just-user") << "" << "" << "foo" << "" << QByteArray("foo:").toBase64();
    QTest::newRow("user-password") << "" << "" << "foo" << "bar" << QByteArray("foo:bar").toBase64();
    QTest::newRow("user-password-realm") << "realm=\"secure area\"" << "secure area" << "foo" << "bar" << QByteArray("foo:bar").toBase64();
}

void tst_QAuthenticator::basicAuth()
{
    QFETCH(QString, data);
    QFETCH(QString, realm);
    QFETCH(QString, user);
    QFETCH(QString, password);
    QFETCH(QByteArray, expectedReply);

    QAuthenticator auth;
    auth.detach();
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);
    QVERIFY(priv->phase == QAuthenticatorPrivate::Start);

    QList<QPair<QByteArray, QByteArray> > headers;
    headers << qMakePair(QByteArray("WWW-Authenticate"), "Basic " + data.toUtf8());
    priv->parseHttpResponse(headers, /*isProxy = */ false);

    QCOMPARE(auth.realm(), realm);
    QCOMPARE(auth.option("realm").toString(), realm);

    auth.setUser(user);
    auth.setPassword(password);

    QVERIFY(priv->phase == QAuthenticatorPrivate::Start);

    QCOMPARE(priv->calculateResponse("GET", "/").constData(), ("Basic " + expectedReply).constData());
}

void tst_QAuthenticator::ntlmAuth_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QString>("realm");

    QTest::newRow("no-realm") << "TlRMTVNTUAACAAAAHAAcADAAAAAFAoEATFZ3OLRQADIAAAAAAAAAAJYAlgBMAAAAUQBUAC0AVABFAFMAVAAtAEQATwBNAEEASQBOAAIAHABRAFQALQBUAEUAUwBUAC0ARABPAE0AQQBJAE4AAQAcAFEAVAAtAFQARQBTAFQALQBTAEUAUgBWAEUAUgAEABYAcQB0AC0AdABlAHMAdAAtAG4AZQB0AAMANABxAHQALQB0AGUAcwB0AC0AcwBlAHIAdgBlAHIALgBxAHQALQB0AGUAcwB0AC0AbgBlAHQAAAAAAA==" << "";
    QTest::newRow("with-realm") << "TlRMTVNTUAACAAAADAAMADgAAAAFAoECWCZkccFFAzwAAAAAAAAAAL4AvgBEAAAABQLODgAAAA9NAEcARABOAE8ASwACAAwATQBHAEQATgBPAEsAAQAcAE4ATwBLAC0AQQBNAFMAUwBTAEYARQAtADAAMQAEACAAbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQADAD4AbgBvAGsALQBhAG0AcwBzAHMAZgBlAC0AMAAxAC4AbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQAFACAAbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQAAAAAA" << "NOE";
}

void tst_QAuthenticator::ntlmAuth()
{
    QFETCH(QString, data);
    QFETCH(QString, realm);

    QAuthenticator auth;
    auth.detach();
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);
    QVERIFY(priv->phase == QAuthenticatorPrivate::Start);

    QList<QPair<QByteArray, QByteArray> > headers;

    // NTLM phase 1: negotiate
    // This phase of NTLM contains no information, other than what we're willing to negotiate
    // Current implementation uses flags:
    //  NTLMSSP_NEGOTIATE_UNICODE | NTLMSSP_NEGOTIATE_NTLM | NTLMSSP_REQUEST_TARGET
    headers << qMakePair<QByteArray, QByteArray>("WWW-Authenticate", "NTLM");
    priv->parseHttpResponse(headers, /*isProxy = */ false);
    QCOMPARE(priv->calculateResponse("GET", "/").constData(), "NTLM TlRMTVNTUAABAAAABQIAAAAAAAAAAAAAAAAAAAAAAAA=");

    // NTLM phase 2: challenge
    headers.clear();
    headers << qMakePair(QByteArray("WWW-Authenticate"), "NTLM " + data.toUtf8());
    priv->parseHttpResponse(headers, /*isProxy = */ false);

    QEXPECT_FAIL("with-realm", "NTLM authentication code doesn't extract the realm", Continue);
    QCOMPARE(auth.realm(), realm);

    auth.setUser("unimportant");
    auth.setPassword("unimportant");

    QVERIFY(!priv->calculateResponse("GET", "/").isEmpty());
}

QTEST_MAIN(tst_QAuthenticator);

#include "tst_qauthenticator.moc"
