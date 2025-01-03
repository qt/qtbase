// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtlsbackend_cert_p.h"

#include "../shared/qx509_generic_p.h"

#include <qssl.h>

#include <qlist.h>

QT_BEGIN_NAMESPACE

QString QTlsBackendCertOnly::backendName() const
{
    return builtinBackendNames[nameIndexCertOnly];
}


QList<QSsl::SslProtocol> QTlsBackendCertOnly::supportedProtocols() const
{
    return {};
}

QList<QSsl::SupportedFeature> QTlsBackendCertOnly::supportedFeatures() const
{
    return {};
}

QList<QSsl::ImplementedClass> QTlsBackendCertOnly::implementedClasses() const
{
    QList<QSsl::ImplementedClass> classes;
    classes << QSsl::ImplementedClass::Certificate;

    return classes;
}

QTlsPrivate::X509Certificate *QTlsBackendCertOnly::createCertificate() const
{
    return new QTlsPrivate::X509CertificateGeneric;
}

QTlsPrivate::X509PemReaderPtr QTlsBackendCertOnly::X509PemReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromPem;
}

QTlsPrivate::X509DerReaderPtr QTlsBackendCertOnly::X509DerReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromDer;
}

QT_END_NAMESPACE

#include "moc_qtlsbackend_cert_p.cpp"

