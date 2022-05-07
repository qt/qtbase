/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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

