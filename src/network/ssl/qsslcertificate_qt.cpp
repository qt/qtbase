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



#include "qsslcertificate.h"
#include "qsslcertificate_p.h"
#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qsslcertificateextension.h"
#include "qsslcertificateextension_p.h"

QT_BEGIN_NAMESPACE

bool QSslCertificate::operator==(const QSslCertificate &other) const
{
    if (d == other.d)
        return true;
    if (d->null && other.d->null)
        return true;
    return d->derData == other.d->derData;
}

bool QSslCertificate::isNull() const
{
    return d->null;
}

bool QSslCertificate::isSelfSigned() const
{
    if (d->null)
        return false;

    qWarning("QSslCertificate::isSelfSigned: This function does not check, whether the certificate \
             is actually signed. It just checks whether issuer and subject are identical");
    return d->subjectMatchesIssuer;
}

QByteArray QSslCertificate::version() const
{
    return d->versionString;
}

QByteArray QSslCertificate::serialNumber() const
{
    return d->serialNumberString;
}

QStringList QSslCertificate::issuerInfo(SubjectInfo info) const
{
    return issuerInfo(QSslCertificatePrivate::subjectInfoToString(info));
}

QStringList QSslCertificate::issuerInfo(const QByteArray &attribute) const
{
    return d->issuerInfo.values(attribute);
}

QStringList QSslCertificate::subjectInfo(SubjectInfo info) const
{
    return subjectInfo(QSslCertificatePrivate::subjectInfoToString(info));
}

QStringList QSslCertificate::subjectInfo(const QByteArray &attribute) const
{
    return d->subjectInfo.values(attribute);
}

QList<QByteArray> QSslCertificate::subjectInfoAttributes() const
{
    return d->subjectInfo.uniqueKeys();
}

QList<QByteArray> QSslCertificate::issuerInfoAttributes() const
{
    return d->issuerInfo.uniqueKeys();
}

QMultiMap<QSsl::AlternativeNameEntryType, QString> QSslCertificate::subjectAlternativeNames() const
{
    return d->subjectAlternativeNames;
}

QDateTime QSslCertificate::effectiveDate() const
{
    return d->notValidBefore;
}

QDateTime QSslCertificate::expiryDate() const
{
    return d->notValidAfter;
}

Qt::HANDLE QSslCertificate::handle() const
{
    Q_UNIMPLEMENTED();
    return 0;
}

QSslKey QSslCertificate::publicKey() const
{
    QSslKey key;
    key.d->type = QSsl::PublicKey;
    if (d->publicKeyAlgorithm != QSsl::Opaque) {
    key.d->algorithm = d->publicKeyAlgorithm;
    key.d->decodeDer(d->publicKeyDerData, QByteArray());
    }
    return key;
}

QList<QSslCertificateExtension> QSslCertificate::extensions() const
{
    Q_UNIMPLEMENTED();
    return QList<QSslCertificateExtension>();
}

#define BEGINCERTSTRING "-----BEGIN CERTIFICATE-----"
#define ENDCERTSTRING "-----END CERTIFICATE-----"

QByteArray QSslCertificate::toPem() const
{
    QByteArray array = toDer();

    // Convert to Base64 - wrap at 64 characters.
    array = array.toBase64();
    QByteArray tmp;
    for (int i = 0; i <= array.size() - 64; i += 64) {
        tmp += QByteArray::fromRawData(array.data() + i, 64);
        tmp += '\n';
    }
    if (int remainder = array.size() % 64) {
        tmp += QByteArray::fromRawData(array.data() + array.size() - remainder, remainder);
        tmp += '\n';
    }

    return BEGINCERTSTRING "\n" + tmp + ENDCERTSTRING "\n";
}

QByteArray QSslCertificate::toDer() const
{
    return d->derData;
}

QString QSslCertificate::toText() const
{
    Q_UNIMPLEMENTED();
    return QString();
}

void QSslCertificatePrivate::init(const QByteArray &data, QSsl::EncodingFormat format)
{
    if (!data.isEmpty()) {
        QList<QSslCertificate> certs = (format == QSsl::Pem)
            ? certificatesFromPem(data, 1)
            : certificatesFromDer(data, 1);
        if (!certs.isEmpty()) {
            *this = *certs.first().d;
        }
    }
}

static bool matchLineFeed(const QByteArray &pem, int *offset)
{
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

QList<QSslCertificate> QSslCertificatePrivate::certificatesFromPem(const QByteArray &pem, int count)
{
    QList<QSslCertificate> certificates;
    int offset = 0;
    while (count == -1 || certificates.size() < count) {
        int startPos = pem.indexOf(BEGINCERTSTRING, offset);
        if (startPos == -1)
            break;
        startPos += sizeof(BEGINCERTSTRING) - 1;
        if (!matchLineFeed(pem, &startPos))
            break;

        int endPos = pem.indexOf(ENDCERTSTRING, startPos);
        if (endPos == -1)
            break;

        offset = endPos + sizeof(ENDCERTSTRING) - 1;
        if (offset < pem.size() && !matchLineFeed(pem, &offset))
            break;

        QByteArray decoded = QByteArray::fromBase64(
            QByteArray::fromRawData(pem.data() + startPos, endPos - startPos));
        certificates << certificatesFromDer(decoded, 1);;
    }

    return certificates;
}

QList<QSslCertificate> QSslCertificatePrivate::certificatesFromDer(const QByteArray &der, int count)
{
    QList<QSslCertificate> certificates;

    QByteArray data = der;
    while (count == -1 || certificates.size() < count) {
        QSslCertificate cert;
        if (!cert.d->parse(data))
            break;

        certificates << cert;
        data.remove(0, cert.d->derData.size());
    }

    return certificates;
}

QT_END_NAMESPACE
