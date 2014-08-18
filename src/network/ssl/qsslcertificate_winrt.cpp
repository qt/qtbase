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

QT_BEGIN_NAMESPACE

bool QSslCertificate::operator==(const QSslCertificate &other) const
{
    if (d == other.d)
        return true;
    return false;
}

bool QSslCertificate::isNull() const
{
    Q_UNIMPLEMENTED();
    return true;
}

bool QSslCertificate::isSelfSigned() const
{
    Q_UNIMPLEMENTED();
    return true;
}

QByteArray QSslCertificate::version() const
{
    Q_UNIMPLEMENTED();
    return QByteArray();
}

QByteArray QSslCertificate::serialNumber() const
{
    Q_UNIMPLEMENTED();
    return QByteArray();
}

QStringList QSslCertificate::issuerInfo(SubjectInfo info) const
{
    Q_UNIMPLEMENTED();
    return QStringList();
}

QStringList QSslCertificate::issuerInfo(const QByteArray &attribute) const
{
    Q_UNIMPLEMENTED();
    return QStringList();
}

QStringList QSslCertificate::subjectInfo(SubjectInfo info) const
{
    Q_UNIMPLEMENTED();
    return QStringList();
}

QStringList QSslCertificate::subjectInfo(const QByteArray &attribute) const
{
    Q_UNIMPLEMENTED();
    return QStringList();
}

QList<QByteArray> QSslCertificate::subjectInfoAttributes() const
{
    Q_UNIMPLEMENTED();
    return QList<QByteArray>();
}

QList<QByteArray> QSslCertificate::issuerInfoAttributes() const
{
    Q_UNIMPLEMENTED();
    return QList<QByteArray>();
}

QMultiMap<QSsl::AlternativeNameEntryType, QString> QSslCertificate::subjectAlternativeNames() const
{
    Q_UNIMPLEMENTED();
    return QMultiMap<QSsl::AlternativeNameEntryType, QString>();
}

QDateTime QSslCertificate::effectiveDate() const
{
    Q_UNIMPLEMENTED();
    return QDateTime();
}

QDateTime QSslCertificate::expiryDate() const
{
    Q_UNIMPLEMENTED();
    return QDateTime();
}

Qt::HANDLE QSslCertificate::handle() const
{
    Q_UNIMPLEMENTED();
    return 0;
}

QSslKey QSslCertificate::publicKey() const
{
    Q_UNIMPLEMENTED();
    return QSslKey();
}

QList<QSslCertificateExtension> QSslCertificate::extensions() const
{
    Q_UNIMPLEMENTED();
    return QList<QSslCertificateExtension>();
}

QByteArray QSslCertificate::toPem() const
{
    Q_UNIMPLEMENTED();
    return QByteArray();
}

QByteArray QSslCertificate::toDer() const
{
    Q_UNIMPLEMENTED();
    return QByteArray();
}

QString QSslCertificate::toText() const
{
    Q_UNIMPLEMENTED();
    return QString();
}

void QSslCertificatePrivate::init(const QByteArray &data, QSsl::EncodingFormat format)
{
    Q_UNIMPLEMENTED();
}

QList<QSslCertificate> QSslCertificatePrivate::certificatesFromPem(const QByteArray &pem, int count)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(pem)
    Q_UNUSED(count)
    return QList<QSslCertificate>();
}

QList<QSslCertificate> QSslCertificatePrivate::certificatesFromDer(const QByteArray &der, int count)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(der)
    Q_UNUSED(count)
    return QList<QSslCertificate>();
}

QT_END_NAMESPACE
