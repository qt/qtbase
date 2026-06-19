// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QString>
#include <QTest>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QHttpHeaders>
#include <QRegularExpression>

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

    void testClear();

    void ntlmV2KnownVectors_data();
    void ntlmV2KnownVectors();
    void ntlmMalformedTargetInfo_data();
    void ntlmMalformedTargetInfo();

    void methodPinningRefusesDowngrade();
    void methodPinningAllowsNormalReauth();

    void methodSelectionPriority_data();
    void methodSelectionPriority();
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

    QHttpHeaders headers;
    headers.append(QByteArray("WWW-Authenticate"), "Basic " + data.toUtf8());
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

    QHttpHeaders headers;

    // NTLM phase 1: negotiate
    // This phase of NTLM contains no information, other than what we're willing to negotiate
    // Current implementation uses flags:
    //  NTLMSSP_NEGOTIATE_UNICODE | NTLMSSP_NEGOTIATE_NTLM | NTLMSSP_REQUEST_TARGET
    headers.append(QByteArrayLiteral("WWW-Authenticate"), QByteArrayLiteral("NTLM"));
    priv->parseHttpResponse(headers, /*isProxy = */ false, {});
    if (sso)
        QVERIFY(priv->calculateResponse("GET", "/", u"").startsWith("NTLM "));
    else
        QCOMPARE(priv->calculateResponse("GET", "/", u"").constData(), "NTLM TlRMTVNTUAABAAAABYIIAAAAAAAAAAAAAAAAAAAAAAA=");

    // NTLM phase 2: challenge
    headers.clear();
    headers.append(QByteArray("WWW-Authenticate"), "NTLM " + data.toUtf8());
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
    QHttpHeaders headers;
    // Put sha256 first, so that its parsed first...
    headers.append("WWW-Authenticate", sha256);
    headers.append("WWW-Authenticate", md5);
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

void tst_QAuthenticator::testClear()
{
    QAuthenticator qauth;
    QVERIFY(qauth.isNull());

    qauth.setUser("User");
    qauth.setPassword("Password");
    qauth.setRealm("Nether");

    QVERIFY(!qauth.isNull());
    QCOMPARE(qauth.user(), "User");
    QCOMPARE(qauth.password(), "Password");
    QCOMPARE(qauth.realm(), "Nether");

    qauth.clear();
    QVERIFY(!qauth.isNull());
    QCOMPARE(qauth.user(), QString());
    QCOMPARE(qauth.password(), QString());
    QCOMPARE(qauth.realm(), QString());
}

void tst_QAuthenticator::ntlmV2KnownVectors_data()
{
    // Verifies the NTLMv2 and LMv2 crypto against the reference test vectors
    // from the Microsoft MS-NLMP specification, Section 4.2.4.
    //
    // All inputs and expected outputs are taken directly from the spec:
    //   User:             "User"
    //   Domain:           "Domain"
    //   Password:         "Password"
    //   Server challenge: 01 23 45 67 89 ab cd ef
    //   Client challenge: aa aa aa aa aa aa aa aa  (fixed via cnonce below)
    //
    // Two test rows are provided:
    //   ms-nlmp-sec-4.2.4         — blob with targetInfo + timestamp, verifies NTProofStr
    //   ms-nlmp-sec-4.2.4-no-ti   — blob without targetInfo, verifies LMv2 response
    //
    // The Phase 2 blobs are derived from the Chromium NTLM test suite
    // (net/ntlm/ntlm_test_data.h), which is itself derived from MS-NLMP
    // Section 4.2.4.3. Using independently verified blobs eliminates the risk
    // of a hand-constructed blob causing a false test failure.

    QTest::addColumn<QByteArray>("phase2blob");
    QTest::addColumn<QByteArray>("expectedNtProofStr");
    QTest::addColumn<QByteArray>("expectedLmv2Prefix");

    // --- Row 1: blob with targetInfo + timestamp ---
    // Based on kChallengeMsgFromSpecV2 from Chromium net/ntlm/ntlm_test_data.h
    // (MS-NLMP Section 4.2.4.3), with MsvAvTimestamp=0 added to targetInfo.
    // The timestamp must be present so qExtractServerTime() returns it instead
    // of falling back to QDateTime::currentSecsSinceEpoch(), which would make
    // the output non-deterministic. The spec uses kServerTimestamp=0.
    // When targetInfo is present, LMv2 is intentionally omitted per spec —
    // so expectedLmv2Prefix is empty for this row.
    QByteArray phase2WithTi = QByteArray::fromHex(
        "4e544c4d53535000"               // signature "NTLMSSP\0"
        "02000000"                       // type = 2
        "0c000c0038000000"               // targetName: len=12, maxLen=12, offset=56
        "33828ae2"                       // flags
        "0123456789abcdef"               // server challenge
        "0000000000000000"               // context (reserved)
        "3000300044000000"               // targetInfo: len=48, maxLen=48, offset=68
        "060070170000000f"               // OS version (ignored by parser)
        "530065007200760065007200"         // targetName = "Server" UCS-2LE
        "02000c0044006f006d00610069006e00" // AvId=2 (MsvAvNbDomainName) "Domain"
        "01000c00530065007200760065007200" // AvId=1 (MsvAvNbComputerName) "Server"
        "070008000000000000000000"         // AvId=7 (MsvAvTimestamp) = 0
        "00000000"                         // AvId=0 MsvAvEOL
    );

    // NTProofStr recalculated with MsvAvTimestamp=0 in targetInfo.
    // Verified independently with Python/pycryptodome.
    QByteArray ntProofStrWithTi =
        QByteArray::fromHex("fb4ec58d662a1ebc3b5524971d77207e");

    QTest::newRow("ms-nlmp-sec-4.2.4")
        << phase2WithTi << ntProofStrWithTi << QByteArray();

    // --- Row 2: blob without targetInfo ---
    // kChallengeMsgV1 from Chromium net/ntlm/ntlm_test_data.h
    // (MS-NLMP Section 4.2.3.3). No targetInfo means qNtlmPhase3 will compute
    // the LMv2 response instead of omitting it. NTProofStr is not verifiable
    // for this blob because there is no timestamp in targetInfo, so the code
    // falls back to QDateTime::currentSecsSinceEpoch() which is non-deterministic.
    QByteArray phase2NoTi = QByteArray::fromHex(
        "4e544c4d53535000"               // signature "NTLMSSP\0"
        "02000000"                       // type = 2
        "0c000c0038000000"               // targetName: len=12, maxLen=12, offset=56
        "33820a82"                       // flags (no NTLMSSP_NEGOTIATE_TARGET_INFO)
        "0123456789abcdef"               // server challenge
        "0000000000000000"               // context (reserved)
        "0000000000000000"               // targetInfo: len=0 (absent)
        "060070170000000f"               // OS version (ignored by parser)
        "530065007200760065007200"         // targetName = "Server" UCS-2LE
    );

    // Expected LMv2 response prefix (MS-NLMP Section 4.2.4.2.1):
    // HMAC-MD5(v2Hash, server_challenge + client_challenge).
    // Verified independently with Python/pycryptodome.
    QByteArray lmv2Prefix =
        QByteArray::fromHex("86c35097ac9cec102554764a57cccc19");

    QTest::newRow("ms-nlmp-sec-4.2.4-no-ti")
        << phase2NoTi << QByteArray() << lmv2Prefix;
}

void tst_QAuthenticator::ntlmV2KnownVectors()
{
    QFETCH(QByteArray, phase2blob);
    QFETCH(QByteArray, expectedNtProofStr);
    QFETCH(QByteArray, expectedLmv2Prefix);

    QAuthenticator auth;
    auth.setUser("Domain\\User");   // triggers domain/user split in updateCredentials()
    auth.setPassword("Password");

    auth.detach();
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);

    // Fix the cnonce so the output is deterministic and matches the spec.
    // clientChallenge() takes the last 8 bytes of cnonce directly as raw bytes,
    // so we set them to 0xaa (the MS-NLMP spec client challenge value).
    // The leading padding satisfies the Q_ASSERT(cnonce.size() >= 8) check.
    priv->cnonce = QByteArray(32, 'x') + QByteArray("\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 8);

    // --- Phase 1: negotiate ---
    QHttpHeaders headers;
    headers.append(QByteArrayLiteral("WWW-Authenticate"), QByteArrayLiteral("NTLM"));
    priv->parseHttpResponse(headers, /*isProxy=*/false, {});
    priv->calculateResponse("GET", "/", u""); // discard — only needed to advance state

    // --- Phase 2: challenge ---
    headers.clear();
    headers.append(QByteArray("WWW-Authenticate"),
                   "NTLM " + phase2blob.toBase64());
    priv->parseHttpResponse(headers, /*isProxy=*/false, {});

    // --- Phase 3: authenticate ---
    QByteArray response = priv->calculateResponse("GET", "/", u"");

    // Basic sanity — if the blob was rejected by the parser this will fail,
    // telling us immediately that the problem is in the test setup, not the crypto.
    QVERIFY2(response.startsWith("NTLM "), "Phase 2 blob was rejected by qNtlmDecodePhase2");

    // Decode the Phase 3 AUTHENTICATE_MESSAGE from base64
    QByteArray phase3 = QByteArray::fromBase64(response.mid(5));
    QVERIFY(phase3.size() >= 64); // minimum AUTHENTICATE_MESSAGE header size

    // Read the buffer descriptors from the AUTHENTICATE_MESSAGE header.
    // Layout (all little-endian):
    //   offset  0: signature (8 bytes)
    //   offset  8: type (4 bytes)
    //   offset 12: LmChallengeResponse descriptor (len u16, maxLen u16, offset u32)
    //   offset 20: NtChallengeResponse descriptor  (len u16, maxLen u16, offset u32)
    QDataStream ds(phase3);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.skipRawData(12); // skip signature + type

    quint16 lmLen, lmMaxLen; quint32 lmOffset;
    ds >> lmLen >> lmMaxLen >> lmOffset;

    quint16 ntLen, ntMaxLen; quint32 ntOffset;
    ds >> ntLen >> ntMaxLen >> ntOffset;

    // --- Verify LMv2 response ---
    // When targetInfo is present the LMv2 response is intentionally omitted
    // per MS-NLMP spec — expectedLmv2Prefix is empty for those rows.
    if (!expectedLmv2Prefix.isEmpty()) {
        QVERIFY(qsizetype(lmOffset + lmLen) <= phase3.size());
        QByteArray lmResponse = phase3.mid(lmOffset, lmLen);
        QCOMPARE(lmResponse.size(), 24); // 16 bytes HMAC + 8 bytes client challenge
        QCOMPARE(lmResponse.left(16), expectedLmv2Prefix);
        QCOMPARE(lmResponse.right(8), QByteArray("\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 8));
    }

    // --- Verify NTLMv2 response ---
    // When there is no timestamp in targetInfo the code falls back to
    // currentSecsSinceEpoch() making NTProofStr non-deterministic —
    // expectedNtProofStr is empty for those rows.
    if (!expectedNtProofStr.isEmpty()) {
        QVERIFY(qsizetype(ntOffset + ntLen) <= phase3.size());
        QByteArray ntResponse = phase3.mid(ntOffset, ntLen);
        QVERIFY(ntResponse.size() >= 16); // NTProofStr is always the first 16 bytes
        QCOMPARE(ntResponse.left(16), expectedNtProofStr);
    }
}

void tst_QAuthenticator::ntlmMalformedTargetInfo_data()
{
    QTest::addColumn<QByteArray>("targetInfo");

    // Reusable well-formed AV_PAIR components
    QByteArray avDomain = QByteArray::fromHex(
        "02000c0044006f006d00610069006e00");   // AvId=2, len=12, "Domain" UCS-2LE
    QByteArray avServer = QByteArray::fromHex(
        "01000c00530065007200760065007200");   // AvId=1, len=12, "Server" UCS-2LE
    QByteArray avEol = QByteArray::fromHex("00000000");

    // AVTIMESTAMP with wrong avLen (4 instead of 8)
    QTest::newRow("timestamp-wrong-avlen")
        << avDomain + avServer
           + QByteArray::fromHex("07000400" "00000000")
           + avEol;

    // Buffer truncated mid-AV_PAIR: timestamp header present but data cut short
    {
        QByteArray full = avDomain + avServer
            + QByteArray::fromHex("070008000000000000000000") + avEol;
        QTest::newRow("truncated-mid-avpair")
            << full.left(full.size() - 6);
    }

    // Too short for even one AV_PAIR header (only 2 bytes)
    QTest::newRow("too-short-for-header")
        << QByteArray::fromHex("0700");

    // avLen exceeds remaining buffer
    QTest::newRow("avlen-exceeds-buffer")
        << QByteArray::fromHex("0200FF00")
           + QByteArray::fromHex("44006f006d00610069006e00")
           + avEol;

    // Empty targetInfo
    QTest::newRow("empty-targetinfo")
        << QByteArray();
}

void tst_QAuthenticator::ntlmMalformedTargetInfo()
{
    QFETCH(QByteArray, targetInfo);

    // Build a Phase 2 challenge blob wrapping the test targetInfo
    QByteArray targetName = QByteArray::fromHex(
        "530065007200760065007200");            // "Server" UCS-2LE
    quint16 targetNameLen = targetName.size();
    quint16 targetInfoLen = targetInfo.size();
    quint32 targetNameOffset = 56;              // 48 (min header) + 8 (version)
    quint32 targetInfoOffset = targetNameOffset + targetNameLen;

    QByteArray phase2;
    QDataStream ds(&phase2, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    ds.writeRawData("NTLMSSP\0", 8);                              // signature
    ds << quint32(2);                                              // type
    ds << targetNameLen << targetNameLen << targetNameOffset;       // targetName descriptor
    ds << quint32(0xe28a8233);                                     // flags
    ds.writeRawData("\x01\x23\x45\x67\x89\xab\xcd\xef", 8);      // server challenge
    ds << quint32(0) << quint32(0);                                // context
    ds << targetInfoLen << targetInfoLen << targetInfoOffset;       // targetInfo descriptor
    ds.writeRawData("\x06\x00\x70\x17\x00\x00\x00\x0f", 8);      // version
    ds.writeRawData(targetName.constData(), targetName.size());
    if (!targetInfo.isEmpty())
        ds.writeRawData(targetInfo.constData(), targetInfo.size());

    // Run the full NTLM auth flow
    QAuthenticator auth;
    auth.setUser("Domain\\User");
    auth.setPassword("Password");
    auth.detach();

    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);

    // Phase 1: negotiate
    QHttpHeaders headers;
    headers.append(QByteArrayLiteral("WWW-Authenticate"), QByteArrayLiteral("NTLM"));
    priv->parseHttpResponse(headers, false, {});
    priv->calculateResponse("GET", "/", u"");

    // Phase 2: challenge with malformed targetInfo
    headers.clear();
    headers.append(QByteArray("WWW-Authenticate"), "NTLM " + phase2.toBase64());
    priv->parseHttpResponse(headers, false, {});

    // Phase 3: authenticate — must not crash, must produce a response
    QByteArray response = priv->calculateResponse("GET", "/", u"");

    QVERIFY2(response.startsWith("NTLM "),
             "Auth flow should complete despite malformed targetInfo");
    QByteArray phase3 = QByteArray::fromBase64(response.mid(5));
    QVERIFY(phase3.size() >= 64); // minimum AUTHENTICATE_MESSAGE header size
}

void tst_QAuthenticator::methodPinningRefusesDowngrade()
{
    // Simulate a man-in-the-middle that replaces an NTLM Phase 2 challenge
    // with a Basic-only response, attempting to force a credential downgrade.
    QAuthenticator auth;
    auth.setUser("user");
    auth.setPassword("pass");
    auth.detach();

    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);

    // Phase 1: server offers NTLM, we select it
    QHttpHeaders headers;
    headers.append(QByteArrayLiteral("WWW-Authenticate"), QByteArrayLiteral("NTLM"));
    priv->parseHttpResponse(headers, /*isProxy=*/false, {});
    QCOMPARE(priv->method, QAuthenticatorPrivate::Ntlm);

    // calculateResponse sends the negotiate token and advances to Phase2
    QByteArray response = priv->calculateResponse("GET", "/", u"");
    QVERIFY(response.startsWith("NTLM "));
    QCOMPARE(priv->phase, QAuthenticatorPrivate::Phase2);

    // MitM replaces the Phase 2 response with Basic only
    headers.clear();
    headers.append(QByteArrayLiteral("WWW-Authenticate"),
                   QByteArrayLiteral("Basic realm=\"evil\""));

    QTest::ignoreMessage(QtWarningMsg,
        QRegularExpression(".*downgrade from NTLM to Basic refused.*"));

    priv->parseHttpResponse(headers, /*isProxy=*/false, {});

    // Authentication must be aborted, not downgraded
    QCOMPARE(priv->method, QAuthenticatorPrivate::None);
    QCOMPARE(priv->phase, QAuthenticatorPrivate::Done);
    QVERIFY(priv->hasFailed);
    QVERIFY(priv->challenge.isEmpty());
}

void tst_QAuthenticator::methodPinningAllowsNormalReauth()
{
    // After a completed (Done) Digest-MD5 exchange, the server may
    // legitimately re-challenge with a different method set.  This must
    // not be blocked by method pinning.
    QAuthenticator auth;
    auth.setUser("user");
    auth.setPassword("pass");
    auth.detach();

    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);

    // Server offers Digest-MD5, we complete the exchange
    QHttpHeaders headers;
    headers.append(QByteArrayLiteral("WWW-Authenticate"),
        QByteArrayLiteral("Digest realm=\"test\", nonce=\"abc123\", "
                          "algorithm=MD5, qop=\"auth\""));
    priv->parseHttpResponse(headers, /*isProxy=*/false, {});
    QCOMPARE(priv->method, QAuthenticatorPrivate::DigestMd5);

    priv->calculateResponse("GET", "/", u"");
    QCOMPARE(priv->phase, QAuthenticatorPrivate::Done);

    // Server rejects credentials and re-challenges with Basic only.
    // Phase is Done (not Phase2), so pinning must not interfere.
    headers.clear();
    headers.append(QByteArrayLiteral("WWW-Authenticate"),
                   QByteArrayLiteral("Basic realm=\"test\""));
    priv->parseHttpResponse(headers, /*isProxy=*/false, {});

    QCOMPARE(priv->method, QAuthenticatorPrivate::Basic);
    QVERIFY(!priv->hasFailed);
}

void tst_QAuthenticator::methodSelectionPriority_data()
{
    QTest::addColumn<QList<QByteArray>>("headers");
    QTest::addColumn<int>("expectedMethod");

    const QByteArray basic = "Basic realm=\"test\"";
    const QByteArray digest =
        "Digest realm=\"test\", nonce=\"abc\", algorithm=MD5, qop=\"auth\"";
    const QByteArray ntlm = "NTLM";

    // NTLM must beat Digest-MD5 regardless of header order
    QTest::newRow("digest-then-ntlm")
        << QList<QByteArray>{digest, ntlm}
        << int(QAuthenticatorPrivate::Ntlm);

    QTest::newRow("ntlm-then-digest")
        << QList<QByteArray>{ntlm, digest}
        << int(QAuthenticatorPrivate::Ntlm);

    // NTLM must beat Basic
    QTest::newRow("basic-then-ntlm")
        << QList<QByteArray>{basic, ntlm}
        << int(QAuthenticatorPrivate::Ntlm);

    // Digest-MD5 must beat Basic
    QTest::newRow("basic-then-digest")
        << QList<QByteArray>{basic, digest}
        << int(QAuthenticatorPrivate::DigestMd5);

    // All three offered — NTLM must win
    QTest::newRow("all-three")
        << QList<QByteArray>{basic, digest, ntlm}
        << int(QAuthenticatorPrivate::Ntlm);

    // All three offered, reverse order — NTLM must still win
    QTest::newRow("all-three-reversed")
        << QList<QByteArray>{ntlm, digest, basic}
        << int(QAuthenticatorPrivate::Ntlm);

    // Only Basic offered — must accept it
    QTest::newRow("basic-only")
        << QList<QByteArray>{basic}
        << int(QAuthenticatorPrivate::Basic);
}

void tst_QAuthenticator::methodSelectionPriority()
{
    QFETCH(QList<QByteArray>, headers);
    QFETCH(int, expectedMethod);

    QAuthenticator auth;
    auth.setUser("user");
    auth.setPassword("pass");
    auth.detach();

    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(auth);

    QHttpHeaders httpHeaders;
    for (const QByteArray &header : headers)
        httpHeaders.append(QByteArrayLiteral("WWW-Authenticate"), header);

    priv->parseHttpResponse(httpHeaders, /*isProxy=*/false, {});
    QCOMPARE(int(priv->method), expectedMethod);
}


QTEST_MAIN(tst_QAuthenticator);

#include "tst_qauthenticator.moc"
