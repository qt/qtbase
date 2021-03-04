/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTLS_ST_P_H
#define QTLS_ST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QtNetwork library.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qtlsbackend_st_p.h"

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>

#include "qabstractsocket.h"
#include "qsslsocket_p.h"

#include <Security/Security.h>
#include <Security/SecureTransport.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class QSecureTransportContext
{
public:
    explicit QSecureTransportContext(SSLContextRef context);
    ~QSecureTransportContext();

    operator SSLContextRef () const;
    void reset(SSLContextRef newContext);
private:
    SSLContextRef context;

    Q_DISABLE_COPY_MOVE(QSecureTransportContext)
};

class TlsCryptographSecureTransport : public TlsCryptograph
{
public:
    TlsCryptographSecureTransport();
    ~TlsCryptographSecureTransport() override;

    void init(QSslSocket *qObj, QSslSocketPrivate *dObj) override;
    void continueHandshake() override;
    void disconnected() override;
    void disconnectFromHost() override;
    QSslCipher sessionCipher() const override;
    QSsl::SslProtocol sessionProtocol() const override;
    void startClientEncryption() override;
    void startServerEncryption() override;
    void transmit() override;
    QList<QSslError> tlsErrors() const override;

    SSLCipherSuite SSLCipherSuite_from_QSslCipher(const QSslCipher &ciph);

private:
    // SSL context management/properties:
    bool initSslContext();
    void destroySslContext();
    bool setSessionCertificate(QString &errorDescription,
                               QAbstractSocket::SocketError &errorCode);
    bool setSessionProtocol();
    // Aux. functions to do a verification during handshake phase:
    bool canIgnoreTrustVerificationFailure() const;
    bool verifySessionProtocol() const;
    bool verifyPeerTrust();

    bool checkSslErrors();
    bool startHandshake();

    bool isHandshakeComplete() const;

    // IO callbacks:
    static OSStatus ReadCallback(TlsCryptographSecureTransport *socket, char *data, size_t *dataLength);
    static OSStatus WriteCallback(TlsCryptographSecureTransport *plainSocket, const char *data, size_t *dataLength);

    QSecureTransportContext context;
    bool renegotiating = false;
    QSslSocket *q = nullptr;
    QSslSocketPrivate *d = nullptr;
    bool shutdown = false;
    QList<QSslError> sslErrors;

    Q_DISABLE_COPY_MOVE(TlsCryptographSecureTransport)
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLS_ST_P_H
