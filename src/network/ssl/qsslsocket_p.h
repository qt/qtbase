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


#ifndef QSSLSOCKET_P_H
#define QSSLSOCKET_P_H

#include "qsslsocket.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <private/qtcpsocket_p.h>

#include "qocspresponse.h"
#include "qsslconfiguration_p.h"
#include "qsslkey.h"
#include "qtlsbackend_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstringlist.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QSslContext;
class QTlsBackend;

class QSslSocketPrivate : public QTcpSocketPrivate
{
    Q_DECLARE_PUBLIC(QSslSocket)
public:
    QSslSocketPrivate();
    virtual ~QSslSocketPrivate();

    void init();
    bool verifyProtocolSupported(const char *where);
    bool initialized;

    QSslSocket::SslMode mode;
    bool autoStartHandshake;
    bool connectionEncrypted;
    bool ignoreAllSslErrors;
    QList<QSslError> ignoreErrorsList;
    bool* readyReadEmittedPointer;

    QSslConfigurationPrivate configuration;

    // if set, this hostname is used for certificate validation instead of the hostname
    // that was used for connecting to.
    QString verificationPeerName;

    bool allowRootCertOnDemandLoading;

    static bool s_loadRootCertsOnDemand;

    static bool supportsSsl();
    static void ensureInitialized();

    static QList<QSslCipher> defaultCiphers();
    static QList<QSslCipher> defaultDtlsCiphers();
    static QList<QSslCipher> supportedCiphers();
    static void setDefaultCiphers(const QList<QSslCipher> &ciphers);
    static void setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers);
    static void setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers);

    static QList<QSslEllipticCurve> supportedEllipticCurves();
    static void setDefaultSupportedEllipticCurves(const QList<QSslEllipticCurve> &curves);
    static void resetDefaultEllipticCurves();

    static QList<QSslCertificate> defaultCaCertificates();
    static QList<QSslCertificate> systemCaCertificates();
    static void setDefaultCaCertificates(const QList<QSslCertificate> &certs);
    static void addDefaultCaCertificate(const QSslCertificate &cert);
    static void addDefaultCaCertificates(const QList<QSslCertificate> &certs);
    Q_AUTOTEST_EXPORT static bool isMatchingHostname(const QSslCertificate &cert, const QString &peerName);
    Q_AUTOTEST_EXPORT static bool isMatchingHostname(const QString &cn, const QString &hostname);

    // The socket itself, including private slots.
    QTcpSocket *plainSocket = nullptr;
    void createPlainSocket(QIODevice::OpenMode openMode);
    Q_NETWORK_EXPORT static void pauseSocketNotifiers(QSslSocket*);
    Q_NETWORK_EXPORT static void resumeSocketNotifiers(QSslSocket*);
    // ### The 2 methods below should be made member methods once the QSslContext class is made public
    static void checkSettingSslContext(QSslSocket*, QSharedPointer<QSslContext>);
    static QSharedPointer<QSslContext> sslContext(QSslSocket *socket);
    Q_NETWORK_EXPORT bool isPaused() const;
    Q_NETWORK_EXPORT void setPaused(bool p);
    bool bind(const QHostAddress &address, quint16, QAbstractSocket::BindMode) override;
    void _q_connectedSlot();
    void _q_hostFoundSlot();
    void _q_disconnectedSlot();
    void _q_stateChangedSlot(QAbstractSocket::SocketState);
    void _q_errorSlot(QAbstractSocket::SocketError);
    void _q_readyReadSlot();
    void _q_channelReadyReadSlot(int);
    void _q_bytesWrittenSlot(qint64);
    void _q_channelBytesWrittenSlot(int, qint64);
    void _q_readChannelFinishedSlot();
    void _q_flushWriteBuffer();
    void _q_flushReadBuffer();
    void _q_resumeImplementation();

    Q_NETWORK_PRIVATE_EXPORT static QList<QByteArray> unixRootCertDirectories(); // used also by QSslContext

    qint64 peek(char *data, qint64 maxSize) override;
    QByteArray peek(qint64 maxSize) override;
    bool flush() override;

    void startClientEncryption();
    void startServerEncryption();
    void transmit();
    void disconnectFromHost();
    void disconnected();
    QSslCipher sessionCipher() const;
    QSsl::SslProtocol sessionProtocol() const;
    void continueHandshake();

    Q_NETWORK_PRIVATE_EXPORT static bool rootCertOnDemandLoadingSupported();
    Q_NETWORK_PRIVATE_EXPORT static void setRootCertOnDemandLoadingSupported(bool supported);

    static QTlsBackend *tlsBackendInUse();
    static void registerAdHocFactory();

    // Needed by TlsCryptograph:
    Q_NETWORK_PRIVATE_EXPORT QSslSocket::SslMode tlsMode() const;
    Q_NETWORK_PRIVATE_EXPORT bool isRootsOnDemandAllowed() const;
    Q_NETWORK_PRIVATE_EXPORT QString verificationName() const;
    Q_NETWORK_PRIVATE_EXPORT QString tlsHostName() const;
    Q_NETWORK_PRIVATE_EXPORT QTcpSocket *plainTcpSocket() const;
    Q_NETWORK_PRIVATE_EXPORT bool verifyErrorsHaveBeenIgnored();
    Q_NETWORK_PRIVATE_EXPORT bool isAutoStartingHandshake() const;
    Q_NETWORK_PRIVATE_EXPORT bool isPendingClose() const;
    Q_NETWORK_PRIVATE_EXPORT void setPendingClose(bool pc);
    Q_NETWORK_PRIVATE_EXPORT qint64 maxReadBufferSize() const;
    Q_NETWORK_PRIVATE_EXPORT void setMaxReadBufferSize(qint64 maxSize);
    Q_NETWORK_PRIVATE_EXPORT void setEncrypted(bool enc);
    Q_NETWORK_PRIVATE_EXPORT QRingBufferRef &tlsWriteBuffer();
    Q_NETWORK_PRIVATE_EXPORT QRingBufferRef &tlsBuffer();
    Q_NETWORK_PRIVATE_EXPORT bool &tlsEmittedBytesWritten();
    Q_NETWORK_PRIVATE_EXPORT bool *readyReadPointer();

protected:

    bool hasUndecryptedData() const;
    bool paused;
    bool flushTriggered;

    static inline QMutex backendMutex;
    static inline QString activeBackendName;
    static inline QTlsBackend *tlsBackend = nullptr;

    std::unique_ptr<QTlsPrivate::TlsCryptograph> backend;
};

QT_END_NAMESPACE

#endif
