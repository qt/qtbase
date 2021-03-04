/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QTLS_OPENSSL_P_H
#define QTLS_OPENSSL_P_H

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

#include <private/qtnetworkglobal_p.h>

#include "qtlsbackend_openssl_p.h"
#include "qsslcontext_openssl_p.h"
#include "qsslcertificate.h"
#include "qocspresponse.h"
#include "qopenssl_p.h"

#include <QtCore/qsharedpointer.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class TlsCryptographOpenSSL : public TlsCryptograph
{
public:
    enum ExDataOffset {
        errorOffsetInExData = 1,
        socketOffsetInExData = 2
    };

    ~TlsCryptographOpenSSL();

    void init(QSslSocket *qObj, QSslSocketPrivate *dObj) override;
    void checkSettingSslContext(QSharedPointer<QSslContext> tlsContext) override;
    QSharedPointer<QSslContext> sslContext() const override;

    QList<QSslError> tlsErrors() const override;

    void startClientEncryption() override;
    void startServerEncryption() override;
    bool startHandshake();
    void enableHandshakeContinuation() override;
    void cancelCAFetch() override;
    void continueHandshake() override;
    void transmit() override;
    void disconnectFromHost() override;
    void disconnected() override;
    QSslCipher sessionCipher() const override;
    QSsl::SslProtocol sessionProtocol() const override;
    QList<QOcspResponse> ocsps() const override;

    bool checkSslErrors();
    int handleNewSessionTicket(SSL *connection);

    void alertMessageSent(int encoded);
    void alertMessageReceived(int encoded);

    int emitErrorFromCallback(X509_STORE_CTX *ctx);
    void trySendFatalAlert();

#if QT_CONFIG(ocsp)
    bool checkOcspStatus();
#endif

    QSslSocket *q = nullptr;
    QSslSocketPrivate *d = nullptr;

    void storePeerCertificates();

    unsigned pskClientTlsCallback(const char *hint, char *identity, unsigned max_identity_len,
                                 unsigned char *psk, unsigned max_psk_len);
    unsigned pskServerTlsCallback(const char *identity, unsigned char *psk,
                                  unsigned max_psk_len);

#ifdef Q_OS_WIN
    void fetchCaRootForCert(const QSslCertificate &cert);
    void caRootLoaded(QSslCertificate certificate, QSslCertificate trustedRoot);
#endif

    QByteArray ocspResponseDer;
private:
    // TLSTODO: names were preserved, to make comparison
    // easier (see qsslsocket_openssl.cpp, while it exists).
    bool initSslContext();
    void destroySslContext();

    QSharedPointer<QSslContext> sslContextPointer;
    SSL *ssl = nullptr; // TLSTODO: RAII.

    QList<QSslErrorEntry> errorList;
    QList<QSslError> sslErrors;

    BIO *readBio = nullptr;
    BIO *writeBio = nullptr;

    QList<QOcspResponse> ocspResponses;

    // This decription will go to setErrorAndEmit(SslHandshakeError, ocspErrorDescription)
    QString ocspErrorDescription;
    // These will go to sslErrors()
    QList<QSslError> ocspErrors;

    bool systemOrSslErrorDetected = false;
    bool handshakeInterrupted = false;

    bool fetchAuthorityInformation = false;
    QSslCertificate caToFetch;

    bool inSetAndEmitError = false;
    bool pendingFatalAlert = false;
    bool errorsReportedFromCallback = false;

    bool shutdown = false;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLS_OPENSSL_P_H

