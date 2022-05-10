// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qx509_base_p.h"

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

QByteArray X509CertificateBase::subjectInfoToString(QSslCertificate::SubjectInfo info)
{
    QByteArray str;
    switch (info) {
    case QSslCertificate::Organization: str = QByteArray("O"); break;
    case QSslCertificate::CommonName: str = QByteArray("CN"); break;
    case QSslCertificate::LocalityName: str = QByteArray("L"); break;
    case QSslCertificate::OrganizationalUnitName: str = QByteArray("OU"); break;
    case QSslCertificate::CountryName: str = QByteArray("C"); break;
    case QSslCertificate::StateOrProvinceName: str = QByteArray("ST"); break;
    case QSslCertificate::DistinguishedNameQualifier: str = QByteArray("dnQualifier"); break;
    case QSslCertificate::SerialNumber: str = QByteArray("serialNumber"); break;
    case QSslCertificate::EmailAddress: str = QByteArray("emailAddress"); break;
    }

    return str;
}

bool X509CertificateBase::matchLineFeed(const QByteArray &pem, int *offset)
{
    Q_ASSERT(offset);

    char ch = 0;
    // ignore extra whitespace at the end of the line
    while (*offset < pem.size() && (ch = pem.at(*offset)) == ' ')
        ++*offset;

    if (ch == '\n') {
        *offset += 1;
        return true;
    }

    if (ch == '\r' && pem.size() > (*offset + 1) && pem.at(*offset + 1) == '\n') {
        *offset += 2;
        return true;
    }

    return false;
}

bool X509CertificateBase::isNull() const
{
    return null;
}

QByteArray X509CertificateBase::version() const
{
    return versionString;
}

QByteArray X509CertificateBase::serialNumber() const
{
    return serialNumberString;
}

QStringList X509CertificateBase::issuerInfo(QSslCertificate::SubjectInfo info) const
{
    return issuerInfo(subjectInfoToString(info));
}

QStringList X509CertificateBase::issuerInfo(const QByteArray &attribute) const
{
    return issuerInfoEntries.values(attribute);
}

QStringList X509CertificateBase::subjectInfo(QSslCertificate::SubjectInfo info) const
{
    return subjectInfo(subjectInfoToString(info));
}

QStringList X509CertificateBase::subjectInfo(const QByteArray &attribute) const
{
    return subjectInfoEntries.values(attribute);
}

QList<QByteArray> X509CertificateBase::subjectInfoAttributes() const
{
    return subjectInfoEntries.uniqueKeys();
}

QList<QByteArray> X509CertificateBase::issuerInfoAttributes() const
{
    return issuerInfoEntries.uniqueKeys();
}

QDateTime X509CertificateBase::effectiveDate() const
{
    return notValidBefore;
}

QDateTime X509CertificateBase::expiryDate() const
{
    return notValidAfter;
}

qsizetype X509CertificateBase::numberOfExtensions() const
{
    return extensions.size();
}

QString X509CertificateBase::oidForExtension(qsizetype index) const
{
    Q_ASSERT(validIndex(index));
    return extensions[index].oid;
}

QString X509CertificateBase::nameForExtension(qsizetype index) const
{
    Q_ASSERT(validIndex(index));
    return extensions[index].name;
}

QVariant X509CertificateBase::valueForExtension(qsizetype index) const
{
    Q_ASSERT(validIndex(index));
    return extensions[index].value;
}

bool X509CertificateBase::isExtensionCritical(qsizetype index) const
{
    Q_ASSERT(validIndex(index));
    return extensions[index].critical;
}

bool X509CertificateBase::isExtensionSupported(qsizetype index) const
{
    Q_ASSERT(validIndex(index));
    return extensions[index].supported;
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
