/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

//#define QSSLSOCKET_DEBUG
//#define QT_DECRYPT_SSL_TRAFFIC

#include "qsslsocket_winrt_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"

QT_BEGIN_NAMESPACE

bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
{
    ensureInitialized();
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
}

void QSslSocketPrivate::deinitialize()
{
    Q_UNIMPLEMENTED();
}

bool QSslSocketPrivate::supportsSsl()
{
    return true;
}

bool QSslSocketPrivate::ensureLibraryLoaded()
{
    return true;
}

void QSslSocketPrivate::ensureCiphersAndCertsLoaded()
{
    Q_UNIMPLEMENTED();
}

void QSslSocketPrivate::ensureInitialized()
{
}

long QSslSocketPrivate::sslLibraryVersionNumber()
{
    Q_UNIMPLEMENTED();
    return 0;
}


QString QSslSocketPrivate::sslLibraryVersionString()
{
    Q_UNIMPLEMENTED();
    return QString::number(sslLibraryVersionNumber());
}

long QSslSocketPrivate::sslLibraryBuildVersionNumber()
{
    Q_UNIMPLEMENTED();
    return 0;
}

QString QSslSocketPrivate::sslLibraryBuildVersionString()
{
    Q_UNIMPLEMENTED();
    return QString::number(sslLibraryBuildVersionNumber());
}

void QSslSocketPrivate::resetDefaultCiphers()
{
    Q_UNIMPLEMENTED();
}

QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    Q_UNIMPLEMENTED();
    ensureInitialized();
    QList<QSslCertificate> systemCerts;
    return systemCerts;
}

void QSslSocketBackendPrivate::startClientEncryption()
{
    Q_UNIMPLEMENTED();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    Q_UNIMPLEMENTED();
}

void QSslSocketBackendPrivate::transmit()
{
    Q_UNIMPLEMENTED();
}

void QSslSocketBackendPrivate::disconnectFromHost()
{
    Q_UNIMPLEMENTED();
}

void QSslSocketBackendPrivate::disconnected()
{
    Q_UNIMPLEMENTED();
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    Q_UNIMPLEMENTED();
    return QSslCipher();
}

QSsl::SslProtocol QSslSocketBackendPrivate::sessionProtocol() const
{
    Q_UNIMPLEMENTED();
    return QSsl::UnknownProtocol;
}
void QSslSocketBackendPrivate::continueHandshake()
{
    Q_UNIMPLEMENTED();
}

QList<QSslError> QSslSocketBackendPrivate::verify(QList<QSslCertificate> certificateChain, const QString &hostName)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(certificateChain)
    Q_UNUSED(hostName)
    QList<QSslError> errors;

    return errors;
}

bool QSslSocketBackendPrivate::importPKCS12(QIODevice *device,
                         QSslKey *key, QSslCertificate *cert,
                         QList<QSslCertificate> *caCertificates,
                         const QByteArray &passPhrase)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(device)
    Q_UNUSED(key)
    Q_UNUSED(cert)
    Q_UNUSED(caCertificates)
    Q_UNUSED(passPhrase)
    return false;
}

QT_END_NAMESPACE
