/********************************************************************************
** Form generated from reading UI file 'sslerrors.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SSLERRORS_H
#define SSLERRORS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_SslErrors
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *label;
    QListWidget *sslErrorList;
    QHBoxLayout *hboxLayout;
    QPushButton *certificateChainButton;
    QSpacerItem *spacerItem;
    QPushButton *pushButton;
    QPushButton *pushButton_2;

    void setupUi(QDialog *SslErrors)
    {
        if (SslErrors->objectName().isEmpty())
            SslErrors->setObjectName(QString::fromUtf8("SslErrors"));
        SslErrors->resize(371, 216);
        vboxLayout = new QVBoxLayout(SslErrors);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        label = new QLabel(SslErrors);
        label->setObjectName(QString::fromUtf8("label"));
        label->setWordWrap(true);

        vboxLayout->addWidget(label);

        sslErrorList = new QListWidget(SslErrors);
        sslErrorList->setObjectName(QString::fromUtf8("sslErrorList"));

        vboxLayout->addWidget(sslErrorList);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        certificateChainButton = new QPushButton(SslErrors);
        certificateChainButton->setObjectName(QString::fromUtf8("certificateChainButton"));
        certificateChainButton->setAutoDefault(false);

        hboxLayout->addWidget(certificateChainButton);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        pushButton = new QPushButton(SslErrors);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));

        hboxLayout->addWidget(pushButton);

        pushButton_2 = new QPushButton(SslErrors);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));

        hboxLayout->addWidget(pushButton_2);


        vboxLayout->addLayout(hboxLayout);


        retranslateUi(SslErrors);
        QObject::connect(pushButton, SIGNAL(clicked()), SslErrors, SLOT(accept()));
        QObject::connect(pushButton_2, SIGNAL(clicked()), SslErrors, SLOT(reject()));

        QMetaObject::connectSlotsByName(SslErrors);
    } // setupUi

    void retranslateUi(QDialog *SslErrors)
    {
        SslErrors->setWindowTitle(QCoreApplication::translate("SslErrors", "Unable To Validate The Connection", nullptr));
        label->setText(QCoreApplication::translate("SslErrors", "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600; color:#ff0000;\">Warning</span><span style=\" color:#ff0000;\">:</span><span style=\" color:#000000;\"> One or more errors with this connection prevent validating the authenticity of the host you are connecting to. Please review the following list of errors, and click </span><span style=\" color:#000000;\">Ignore</span><span style=\" color:#000000;\"> to continue, or </span><span style=\" color:#000000;\">Cancel</span><span style=\" color:#000000;\"> to abort the connection.</span></p></body></html>", nullptr));
        certificateChainButton->setText(QCoreApplication::translate("SslErrors", "View Certificate Chain", nullptr));
        pushButton->setText(QCoreApplication::translate("SslErrors", "Ignore", nullptr));
        pushButton_2->setText(QCoreApplication::translate("SslErrors", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SslErrors: public Ui_SslErrors {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SSLERRORS_H
