/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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

    connect(form->certificationPathView, SIGNAL(currentRowChanged(int)),
            this, SLOT(updateCertificateInfo(int)));
}

CertificateInfo::~CertificateInfo()
{
    delete form;
}

void CertificateInfo::setCertificateChain(const QList<QSslCertificate> &chain)
{
    this->chain = chain;

    form->certificationPathView->clear();

    for (int i = 0; i < chain.size(); ++i) {
        const QSslCertificate &cert = chain.at(i);
        form->certificationPathView->addItem(tr("%1%2 (%3)").arg(!i ? QString() : tr("Issued by: "))
                                             .arg(cert.subjectInfo(QSslCertificate::Organization))
                                             .arg(cert.subjectInfo(QSslCertificate::CommonName)));
    }

    form->certificationPathView->setCurrentRow(0);
}

void CertificateInfo::updateCertificateInfo(int index)
{
    form->certificateInfoView->clear();
    if (index >= 0 && index < chain.size()) {
        const QSslCertificate &cert = chain.at(index);
        QStringList lines;
        lines << tr("Organization: %1").arg(cert.subjectInfo(QSslCertificate::Organization))
              << tr("Subunit: %1").arg(cert.subjectInfo(QSslCertificate::OrganizationalUnitName))
              << tr("Country: %1").arg(cert.subjectInfo(QSslCertificate::CountryName))
              << tr("Locality: %1").arg(cert.subjectInfo(QSslCertificate::LocalityName))
              << tr("State/Province: %1").arg(cert.subjectInfo(QSslCertificate::StateOrProvinceName))
              << tr("Common Name: %1").arg(cert.subjectInfo(QSslCertificate::CommonName))
              << QString()
              << tr("Issuer Organization: %1").arg(cert.issuerInfo(QSslCertificate::Organization))
              << tr("Issuer Unit Name: %1").arg(cert.issuerInfo(QSslCertificate::OrganizationalUnitName))
              << tr("Issuer Country: %1").arg(cert.issuerInfo(QSslCertificate::CountryName))
              << tr("Issuer Locality: %1").arg(cert.issuerInfo(QSslCertificate::LocalityName))
              << tr("Issuer State/Province: %1").arg(cert.issuerInfo(QSslCertificate::StateOrProvinceName))
              << tr("Issuer Common Name: %1").arg(cert.issuerInfo(QSslCertificate::CommonName));
        foreach (QString line, lines)
            form->certificateInfoView->addItem(line);
    } else {
        form->certificateInfoView->clear();
    }
}
