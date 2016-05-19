/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the FOO module of the Qt Toolkit.
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

    void equalityOperators();
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
    QCOMPARE(priv->phase, QAuthenticatorPrivate::Start);

    QList<QPair<QByteArray, QByteArray> > headers;
    headers << qMakePair<QByteArray, QByteArray>(QByteArray("WWW-Authenticate"), "Basic " + data.toUtf8());
    priv->parseHttpResponse(headers, /*isProxy = */ false);

    QCOMPARE(auth.realm(), realm);
    QCOMPARE(auth.option("realm").toString(), realm);

    auth.setUser(user);
    auth.setPassword(password);

    QCOMPARE(priv->phase, QAuthenticatorPrivate::Start);

    QCOMPARE(priv->calculateResponse("GET", "/").constData(), QByteArray("Basic " + expectedReply).constData());
}

void tst_QAuthenticator::ntlmAuth_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QString>("realm");
    QTest::addColumn<bool>("sso");

    QTest::newRow("no-realm") << "TlRMTVNTUAACAAAAHAAcADAAAAAFAoEATFZ3OLRQADIAAAAAAAAAAJYAlgBMAAAAUQBUAC0AVABFAFMAVAAtAEQATwBNAEEASQBOAAIAHABRAFQALQBUAEUAUwBUAC0ARABPAE0AQQBJAE4AAQAcAFEAVAAtAFQARQBTAFQALQBTAEUAUgBWAEUAUgAEABYAcQB0AC0AdABlAHMAdAAtAG4AZQB0AAMANABxAHQALQB0AGUAcwB0AC0AcwBlAHIAdgBlAHIALgBxAHQALQB0AGUAcwB0AC0AbgBlAHQAAAAAAA==" << "" << false;
    QTest::newRow("with-realm") << "TlRMTVNTUAACAAAADAAMADgAAAAFAoECWCZkccFFAzwAAAAAAAAAAL4AvgBEAAAABQLODgAAAA9NAEcARABOAE8ASwACAAwATQBHAEQATgBPAEsAAQAcAE4ATwBLAC0AQQBNAFMAUwBTAEYARQAtADAAMQAEACAAbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQADAD4AbgBvAGsALQBhAG0AcwBzAHMAZgBlAC0AMAAxAC4AbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQAFACAAbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQAAAAAA" << "NOE" << false;
    QTest::newRow("no-realm-sso") << "TlRMTVNTUAACAAAAHAAcADAAAAAFAoEATFZ3OLRQADIAAAAAAAAAAJYAlgBMAAAAUQBUAC0AVABFAFMAVAAtAEQATwBNAEEASQBOAAIAHABRAFQALQBUAEUAUwBUAC0ARABPAE0AQQBJAE4AAQAcAFEAVAAtAFQARQBTAFQALQBTAEUAUgBWAEUAUgAEABYAcQB0AC0AdABlAHMAdAAtAG4AZQB0AAMANABxAHQALQB0AGUAcwB0AC0AcwBlAHIAdgBlAHIALgBxAHQALQB0AGUAcwB0AC0AbgBlAHQAAAAAAA==" << "" << true;
    QTest::newRow("with-realm-sso") << "TlRMTVNTUAACAAAADAAMADgAAAAFAoECWCZkccFFAzwAAAAAAAAAAL4AvgBEAAAABQLODgAAAA9NAEcARABOAE8ASwACAAwATQBHAEQATgBPAEsAAQAcAE4ATwBLAC0AQQBNAFMAUwBTAEYARQAtADAAMQAEACAAbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQADAD4AbgBvAGsALQBhAG0AcwBzAHMAZgBlAC0AMAAxAC4AbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQAFACAAbQBnAGQAbgBvAGsALgBuAG8AawBpAGEALgBjAG8AbQAAAAAA" << "NOE" << true;
}

void tst_QAuthenticator::ntlmAuth()
{
    QFETCH(QString, data);
    QFETCH(QString, realm);
    QFETCH(bool, sso);

    QAuthenticator auth;
    if (!sso) {
        auth.setUser("unimportant");
        auth.setPassword("unimportant");
    }

    auth.detach();
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);
    QCOMPARE(priv->phase, QAuthenticatorPrivate::Start);

    QList<QPair<QByteArray, QByteArray> > headers;

    // NTLM phase 1: negotiate
    // This phase of NTLM contains no information, other than what we're willing to negotiate
    // Current implementation uses flags:
    //  NTLMSSP_NEGOTIATE_UNICODE | NTLMSSP_NEGOTIATE_NTLM | NTLMSSP_REQUEST_TARGET
    headers << qMakePair<QByteArray, QByteArray>("WWW-Authenticate", "NTLM");
    priv->parseHttpResponse(headers, /*isProxy = */ false);
    if (sso)
        QVERIFY(priv->calculateResponse("GET", "/").startsWith("NTLM "));
    else
        QCOMPARE(priv->calculateResponse("GET", "/").constData(), "NTLM TlRMTVNTUAABAAAABYIIAAAAAAAAAAAAAAAAAAAAAAA=");

    // NTLM phase 2: challenge
    headers.clear();
    headers << qMakePair<QByteArray, QByteArray>(QByteArray("WWW-Authenticate"), "NTLM " + data.toUtf8());
    priv->parseHttpResponse(headers, /*isProxy = */ false);

    QEXPECT_FAIL("with-realm", "NTLM authentication code doesn't extract the realm", Continue);
    QEXPECT_FAIL("with-realm-sso", "NTLM authentication code doesn't extract the realm", Continue);
    QCOMPARE(auth.realm(), realm);

    QVERIFY(priv->calculateResponse("GET", "/").startsWith("NTLM "));
}

void tst_QAuthenticator::equalityOperators()
{
    QAuthenticator s1, s2;
    QVERIFY(s2 == s1);
    QVERIFY(s1 == s2);
    QVERIFY(!(s1 != s2));
    QVERIFY(!(s2 != s1));
    s1.setUser("User");
    QVERIFY(!(s2 == s1));
    QVERIFY(!(s1 == s2));
    QVERIFY(s1 != s2);
    QVERIFY(s2 != s1);
}

QTEST_MAIN(tst_QAuthenticator);

#include "tst_qauthenticator.moc"
