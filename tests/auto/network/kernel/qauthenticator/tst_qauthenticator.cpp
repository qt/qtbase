// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QString>
#include <QTest>
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

    void sha256AndMd5Digest();

    void equalityOperators();

    void isMethodSupported();
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
    headers << qMakePair(QByteArray("WWW-Authenticate"), "Basic " + data.toUtf8());
    priv->parseHttpResponse(headers, /*isProxy = */ false, {});

    QCOMPARE(auth.realm(), realm);
    QCOMPARE(auth.option("realm").toString(), realm);

    auth.setUser(user);
    auth.setPassword(password);

    QCOMPARE(priv->phase, QAuthenticatorPrivate::Start);

    QCOMPARE(priv->calculateResponse("GET", "/", u"").constData(), QByteArray("Basic " + expectedReply).constData());
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
    headers << qMakePair(QByteArrayLiteral("WWW-Authenticate"), QByteArrayLiteral("NTLM"));
    priv->parseHttpResponse(headers, /*isProxy = */ false, {});
    if (sso)
        QVERIFY(priv->calculateResponse("GET", "/", u"").startsWith("NTLM "));
    else
        QCOMPARE(priv->calculateResponse("GET", "/", u"").constData(), "NTLM TlRMTVNTUAABAAAABYIIAAAAAAAAAAAAAAAAAAAAAAA=");

    // NTLM phase 2: challenge
    headers.clear();
    headers << qMakePair(QByteArray("WWW-Authenticate"), "NTLM " + data.toUtf8());
    priv->parseHttpResponse(headers, /*isProxy = */ false, {});

    QEXPECT_FAIL("with-realm", "NTLM authentication code doesn't extract the realm", Continue);
    QEXPECT_FAIL("with-realm-sso", "NTLM authentication code doesn't extract the realm", Continue);
    QCOMPARE(auth.realm(), realm);

    QVERIFY(priv->calculateResponse("GET", "/", u"").startsWith("NTLM "));
}

// We don't (currently) support SHA256. So, when presented with the option of MD5 or SHA256,
// we should always pick MD5.
void tst_QAuthenticator::sha256AndMd5Digest()
{
    QByteArray md5 = "Digest realm=\"\", nonce=\"\", algorithm=MD5, qop=\"auth\"";
    QByteArray sha256 = "Digest realm=\"\", nonce=\"\", algorithm=SHA-256, qop=\"auth\"";

    QAuthenticator auth;
    auth.setUser("unimportant");
    auth.setPassword("unimportant");

    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);
    QVERIFY(priv->isMethodSupported("digest")); // sanity check

    QCOMPARE(priv->phase, QAuthenticatorPrivate::Start);
    QList<QPair<QByteArray, QByteArray>> headers;
    // Put sha256 first, so that its parsed first...
    headers.emplace_back("WWW-Authenticate", sha256);
    headers.emplace_back("WWW-Authenticate", md5);
    priv->parseHttpResponse(headers, false, QString());

    QByteArray response = priv->calculateResponse("GET", "/index", {});
    QCOMPARE(priv->phase, QAuthenticatorPrivate::Done);

    QVERIFY(!response.isEmpty());
    QVERIFY(!response.contains("algorithm=SHA-256"));
    QVERIFY(response.contains("algorithm=MD5"));
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

void tst_QAuthenticator::isMethodSupported()
{
    QVERIFY(QAuthenticatorPrivate::isMethodSupported("basic"));
    QVERIFY(QAuthenticatorPrivate::isMethodSupported("Basic realm=\"Shadow\""));
    QVERIFY(QAuthenticatorPrivate::isMethodSupported("DIgesT"));
    QVERIFY(QAuthenticatorPrivate::isMethodSupported("NTLM"));
    QVERIFY(QAuthenticatorPrivate::isMethodSupported("ntlm"));
#if QT_CONFIG(sspi) || QT_CONFIG(gssapi)
    QVERIFY(QAuthenticatorPrivate::isMethodSupported("negotiate"));
#else
    QVERIFY(!QAuthenticatorPrivate::isMethodSupported("negotiate"));
#endif

    QVERIFY(!QAuthenticatorPrivate::isMethodSupported("Bearer"));
}

QTEST_MAIN(tst_QAuthenticator);

#include "tst_qauthenticator.moc"
