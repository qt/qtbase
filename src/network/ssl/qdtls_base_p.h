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

#ifndef QDTLS_BASE_P_H
#define QDTLS_BASE_P_H

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

QT_REQUIRE_CONFIG(dtls);

#include "qsslconfiguration.h"
#include "qtlsbackend_p.h"
#include "qsslcipher.h"
#include "qsslsocket.h"
#include "qssl.h"

#include <QtNetwork/qhostaddress.h>

#include <QtCore/qcryptographichash.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

// This class exists to re-implement the shared error/cookie handling
// for both QDtls and QDtlsClientVerifier classes. Use it if/when
// you need it. Backend neutral.
class QDtlsBasePrivate : virtual public QTlsPrivate::DtlsBase
{
public:
    QDtlsBasePrivate(QSslSocket::SslMode m, const QByteArray &s) : mode(m), secret(s) {}
    void setDtlsError(QDtlsError code, const QString &description) override;
    QDtlsError error() const override;
    QString errorString() const override;
    void clearDtlsError() override;

    void setConfiguration(const QSslConfiguration &configuration) override;
    QSslConfiguration configuration() const override;

    bool setCookieGeneratorParameters(const GenParams &) override;
    GenParams cookieGeneratorParameters() const override;

    static bool isDtlsProtocol(QSsl::SslProtocol protocol);

    QHostAddress remoteAddress;
    quint16 remotePort = 0;
    quint16 mtuHint = 0;

    QDtlsError errorCode = QDtlsError::NoError;
    QString errorDescription;
    QSslConfiguration dtlsConfiguration;
    QSslSocket::SslMode mode = QSslSocket::SslClientMode;
    QSslCipher sessionCipher;
    QSsl::SslProtocol sessionProtocol = QSsl::UnknownProtocol;
    QString peerVfyName;
    QByteArray secret;

#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha1;
#else
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha256;
#endif
};

QT_END_NAMESPACE

#endif // QDTLS_BASE_P_H
