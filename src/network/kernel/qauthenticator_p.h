/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
#include <qscopedpointer.h>
#include <qstring.h>
#include <qauthenticator.h>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

class QHttpResponseHeader;
#if QT_CONFIG(sspi) // SSPI
class QSSPIWindowsHandles;
#elif QT_CONFIG(gssapi) // GSSAPI
class QGssApiHandles;
#endif

class Q_AUTOTEST_EXPORT QAuthenticatorPrivate
{
public:
    enum Method { None, Basic, Negotiate, Ntlm, DigestMd5, };
    QAuthenticatorPrivate();
    ~QAuthenticatorPrivate();

    QString user;
    QString extractedUser;
    QString password;
    QVariantHash options;
    Method method;
    QString realm;
    QByteArray challenge;
#if QT_CONFIG(sspi) // SSPI
    QScopedPointer<QSSPIWindowsHandles> sspiWindowsHandles;
#elif QT_CONFIG(gssapi) // GSSAPI
    QScopedPointer<QGssApiHandles> gssApiHandles;
#endif
    bool hasFailed; //credentials have been tried but rejected by server.

    enum Phase {
        Start,
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

    QByteArray calculateResponse(const QByteArray &method, const QByteArray &path, const QString& host);

    inline static QAuthenticatorPrivate *getPrivate(QAuthenticator &auth) { return auth.d; }
    inline static const QAuthenticatorPrivate *getPrivate(const QAuthenticator &auth) { return auth.d; }

    QByteArray digestMd5Response(const QByteArray &challenge, const QByteArray &method, const QByteArray &path);
    static QHash<QByteArray, QByteArray> parseDigestAuthenticationChallenge(const QByteArray &challenge);

    void parseHttpResponse(const QList<QPair<QByteArray, QByteArray> >&, bool isProxy, const QString &host);
    void updateCredentials();
};


QT_END_NAMESPACE

#endif
