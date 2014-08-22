/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QSSLSOCKET_MAC_P_H
#define QSSLSOCKET_MAC_P_H

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

#include <QtCore/private/qcore_mac_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>

#include "qabstractsocket.h"
#include "qsslsocket_p.h"

#include <Security/Security.h>
#include <Security/SecureTransport.h>

QT_BEGIN_NAMESPACE

class QSslSocketBackendPrivate : public QSslSocketPrivate
{
    Q_DECLARE_PUBLIC(QSslSocket)
public:
    QSslSocketBackendPrivate();
    virtual ~QSslSocketBackendPrivate();

    // Final-overriders (QSslSocketPrivate):
    void continueHandshake() Q_DECL_OVERRIDE;
    void disconnected() Q_DECL_OVERRIDE;
    void disconnectFromHost() Q_DECL_OVERRIDE;
    QSslCipher sessionCipher() const Q_DECL_OVERRIDE;
    QSsl::SslProtocol sessionProtocol() const Q_DECL_OVERRIDE;
    void startClientEncryption() Q_DECL_OVERRIDE;
    void startServerEncryption() Q_DECL_OVERRIDE;
    void transmit() Q_DECL_OVERRIDE;

    static QList<QSslError> (verify)(QList<QSslCertificate> certificateChain,
                                     const QString &hostName);

    static bool importPkcs12(QIODevice *device,
                             QSslKey *key, QSslCertificate *cert,
                             QList<QSslCertificate> *caCertificates,
                             const QByteArray &passPhrase);

    static QSslCipher QSslCipher_from_SSLCipherSuite(SSLCipherSuite cipher);

private:
    // SSL context management/properties:
    bool initSslContext();
    void destroySslContext();
    bool setSessionCertificate(QString &errorDescription,
                               QAbstractSocket::SocketError &errorCode);
    bool setSessionProtocol();
    // Aux. functions to do a verification during handshake phase:
    bool verifySessionProtocol() const;
    bool verifyPeerTrust();

    bool checkSslErrors();
    bool startHandshake();

    // Aux. function, sets:
    //1) socket error code,
    //2) error string (description)
    //3) emits a signal.
    void setError(const QString &errorString,
                  QAbstractSocket::SocketError errorCode);

    mutable QCFType<SSLContextRef> context;

    Q_DISABLE_COPY(QSslSocketBackendPrivate);
};

QT_END_NAMESPACE

#endif
