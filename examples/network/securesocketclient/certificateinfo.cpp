/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "certificateinfo.h"
#include "ui_certificateinfo.h"

CertificateInfo::CertificateInfo(QWidget *parent)
    : QDialog(parent)
{
    form = new Ui_CertificateInfo;
    form->setupUi(this);

    connect(form->certificationPathView, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CertificateInfo::updateCertificateInfo);
}

CertificateInfo::~CertificateInfo()
{
    delete form;
}

void CertificateInfo::setCertificateChain(const QList<QSslCertificate> &chain)
{
    certificateChain = chain;

    form->certificationPathView->clear();
    for (int i = 0; i < certificateChain.size(); ++i) {
        const QSslCertificate &cert = certificateChain.at(i);
        form->certificationPathView->addItem(tr("%1%2 (%3)").arg(!i ? QString() : tr("Issued by: "))
                                             .arg(cert.subjectInfo(QSslCertificate::Organization).join(QLatin1Char(' ')))
                                             .arg(cert.subjectInfo(QSslCertificate::CommonName).join(QLatin1Char(' '))));
    }
    form->certificationPathView->setCurrentIndex(0);
}

void CertificateInfo::updateCertificateInfo(int index)
{
    form->certificateInfoView->clear();
    if (index >= 0 && index < certificateChain.size()) {
        const QSslCertificate &cert = certificateChain.at(index);
        QStringList lines;
        lines << tr("Organization: %1").arg(cert.subjectInfo(QSslCertificate::Organization).join(QLatin1Char(' ')))
              << tr("Subunit: %1").arg(cert.subjectInfo(QSslCertificate::OrganizationalUnitName).join(QLatin1Char(' ')))
              << tr("Country: %1").arg(cert.subjectInfo(QSslCertificate::CountryName).join(QLatin1Char(' ')))
              << tr("Locality: %1").arg(cert.subjectInfo(QSslCertificate::LocalityName).join(QLatin1Char(' ')))
              << tr("State/Province: %1").arg(cert.subjectInfo(QSslCertificate::StateOrProvinceName).join(QLatin1Char(' ')))
              << tr("Common Name: %1").arg(cert.subjectInfo(QSslCertificate::CommonName).join(QLatin1Char(' ')))
              << QString()
              << tr("Issuer Organization: %1").arg(cert.issuerInfo(QSslCertificate::Organization).join(QLatin1Char(' ')))
              << tr("Issuer Unit Name: %1").arg(cert.issuerInfo(QSslCertificate::OrganizationalUnitName).join(QLatin1Char(' ')))
              << tr("Issuer Country: %1").arg(cert.issuerInfo(QSslCertificate::CountryName).join(QLatin1Char(' ')))
              << tr("Issuer Locality: %1").arg(cert.issuerInfo(QSslCertificate::LocalityName).join(QLatin1Char(' ')))
              << tr("Issuer State/Province: %1").arg(cert.issuerInfo(QSslCertificate::StateOrProvinceName).join(QLatin1Char(' ')))
              << tr("Issuer Common Name: %1").arg(cert.issuerInfo(QSslCertificate::CommonName).join(QLatin1Char(' ')));
        for (const auto &line : lines)
            form->certificateInfoView->addItem(line);
    }
}
