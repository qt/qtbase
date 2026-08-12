// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QAUTHENTICATOR_P_H
#define QAUTHENTICATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <qhash.h>
#include <qbytearray.h>
#include <qstring.h>
#include <qauthenticator.h>
#include <qvariant.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QHttpResponseHeader;
class QHttpHeaders;
#if QT_CONFIG(sspi) // SSPI
class QSSPIWindowsHandles;
#elif QT_CONFIG(gssapi) // GSSAPI
class QGssApiHandles;
#endif

class Q_NETWORK_EXPORT QAuthenticatorPrivate
{
public:
    enum Method { None, Basic, Negotiate, Ntlm, DigestMd5, };
    QAuthenticatorPrivate();

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QAuthenticatorPrivate)
    void swap(QAuthenticatorPrivate &other) noexcept
    {
        user.swap(other.user);
        extractedUser.swap(other.extractedUser);
        password.swap(other.password);
        options.swap(other.options);
        std::swap(method, other.method);
        realm.swap(other.realm);
        challenge.swap(other.challenge);
#if QT_CONFIG(sspi) // SSPI
        sspiWindowsHandles.swap(other.sspiWindowsHandles);
#elif QT_CONFIG(gssapi) // GSSAPI
        gssApiHandles.swap(other.gssApiHandles);
#endif
        std::swap(hasFailed, other.hasFailed);
        std::swap(phase, other.phase);
        cnonce.swap(other.cnonce);
        std::swap(nonceCount, other.nonceCount);
        workstation.swap(other.workstation);
        userDomain.swap(other.userDomain);
    }

    ~QAuthenticatorPrivate();

    QString user;
    QString extractedUser;
    QString password;
    QVariantHash options;
    Method method;
    QString realm;
    QByteArray challenge;
#if QT_CONFIG(sspi) // SSPI
    std::unique_ptr<QSSPIWindowsHandles> sspiWindowsHandles;
#elif QT_CONFIG(gssapi) // GSSAPI
    std::unique_ptr<QGssApiHandles> gssApiHandles;
#endif
    bool hasFailed; //credentials have been tried but rejected by server.

    enum Phase {
        Start,
        Phase1,
        Phase2,
        Done,
        Invalid
    };
    Phase phase;

    // digest specific
    QByteArray cnonce;
    int nonceCount;

    // ntlm specific
    QString workstation;
    QString userDomain;

    QByteArray calculateResponse(QByteArrayView method, QByteArrayView path, QStringView host);

    inline static QAuthenticatorPrivate *getPrivate(QAuthenticator &auth) { return auth.d; }
    inline static const QAuthenticatorPrivate *getPrivate(const QAuthenticator &auth) { return auth.d; }

    QByteArray digestMd5Response(QByteArrayView challenge, QByteArrayView method,
                                 QByteArrayView path);
    static QHash<QByteArray, QByteArray>
    parseDigestAuthenticationChallenge(QByteArrayView challenge);

    void parseHttpResponse(const QHttpHeaders &headers, bool isProxy);
    void updateCredentials();

    static bool isMethodSupported(QByteArrayView method);
};


QT_END_NAMESPACE

#endif
