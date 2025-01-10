/********************************************************************************
** Form generated from reading UI file 'certificateinfo.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CERTIFICATEINFO_H
#define CERTIFICATEINFO_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_CertificateInfo
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QHBoxLayout *hboxLayout;
    QListWidget *certificationPathView;
    QGroupBox *groupBox_2;
    QHBoxLayout *hboxLayout1;
    QListWidget *certificateInfoView;
    QHBoxLayout *hboxLayout2;
    QSpacerItem *spacerItem;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CertificateInfo)
    {
        if (CertificateInfo->objectName().isEmpty())
            CertificateInfo->setObjectName("CertificateInfo");
        CertificateInfo->resize(400, 397);
        vboxLayout = new QVBoxLayout(CertificateInfo);
        vboxLayout->setObjectName("vboxLayout");
        groupBox = new QGroupBox(CertificateInfo);
        groupBox->setObjectName("groupBox");
        hboxLayout = new QHBoxLayout(groupBox);
        hboxLayout->setObjectName("hboxLayout");
        certificationPathView = new QListWidget(groupBox);
        certificationPathView->setObjectName("certificationPathView");

        hboxLayout->addWidget(certificationPathView);


        vboxLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(CertificateInfo);
        groupBox_2->setObjectName("groupBox_2");
        hboxLayout1 = new QHBoxLayout(groupBox_2);
        hboxLayout1->setObjectName("hboxLayout1");
        certificateInfoView = new QListWidget(groupBox_2);
        certificateInfoView->setObjectName("certificateInfoView");

        hboxLayout1->addWidget(certificateInfoView);


        vboxLayout->addWidget(groupBox_2);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName("hboxLayout2");
        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout2->addItem(spacerItem);

        buttonBox = new QDialogButtonBox(CertificateInfo);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        hboxLayout2->addWidget(buttonBox);


        vboxLayout->addLayout(hboxLayout2);


        retranslateUi(CertificateInfo);
        QObject::connect(buttonBox, &QDialogButtonBox::clicked, CertificateInfo, qOverload<>(&QDialog::accept));

        QMetaObject::connectSlotsByName(CertificateInfo);
    } // setupUi

    void retranslateUi(QDialog *CertificateInfo)
    {
        CertificateInfo->setWindowTitle(QCoreApplication::translate("CertificateInfo", "Display Certificate Information", nullptr));
        groupBox->setTitle(QCoreApplication::translate("CertificateInfo", "Certification Path", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("CertificateInfo", "Certificate Information", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CertificateInfo: public Ui_CertificateInfo {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CERTIFICATEINFO_H
