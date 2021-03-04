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

#include "qtlsbackend_st_p.h"
#include "qtlskey_st_p.h"
#include "qx509_st_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTlsBackend, "qt.tlsbackend.securetransport");

QString QSecureTransportBackend::backendName() const
{
    return builtinBackendNames[nameIndexSecureTransport];
}

QTlsPrivate::TlsKey *QSecureTransportBackend::createKey() const
{
    return new QTlsPrivate::TlsKeySecureTransport;
}

QTlsPrivate::X509Certificate *QSecureTransportBackend::createCertificate() const
{
    return new QTlsPrivate::X509CertificateSecureTransport;
}

QList<QSsl::SslProtocol> QSecureTransportBackend::supportedProtocols() const
{
    QList<QSsl::SslProtocol> protocols;

    protocols << QSsl::AnyProtocol;
    protocols << QSsl::SecureProtocols;
    protocols << QSsl::TlsV1_0;
    protocols << QSsl::TlsV1_0OrLater;
    protocols << QSsl::TlsV1_1;
    protocols << QSsl::TlsV1_1OrLater;
    protocols << QSsl::TlsV1_2;
    protocols << QSsl::TlsV1_2OrLater;

    return protocols;
}

QList<QSsl::SupportedFeature> QSecureTransportBackend::supportedFeatures() const
{
    QList<QSsl::SupportedFeature> features;
    features << QSsl::SupportedFeature::ClientSideAlpn;

    return features;
}

QList<QSsl::ImplementedClass> QSecureTransportBackend::implementedClasses() const
{
    QList<QSsl::ImplementedClass> classes;
    classes << QSsl::ImplementedClass::Socket;
    classes << QSsl::ImplementedClass::Certificate;
    classes << QSsl::ImplementedClass::Key;

    return classes;
}

QTlsPrivate::X509PemReaderPtr QSecureTransportBackend::X509PemReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromPem;
}

QTlsPrivate::X509DerReaderPtr QSecureTransportBackend::X509DerReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromDer;
}

QT_END_NAMESPACE

