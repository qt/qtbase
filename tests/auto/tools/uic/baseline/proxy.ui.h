/********************************************************************************
** Form generated from reading UI file 'proxy.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PROXY_H
#define PROXY_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

QT_BEGIN_NAMESPACE

class Ui_ProxyDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *iconLabel;
    QLabel *introLabel;
    QLabel *usernameLabel;
    QLineEdit *userNameLineEdit;
    QLabel *passwordLabel;
    QLineEdit *passwordLineEdit;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ProxyDialog)
    {
        if (ProxyDialog->objectName().isEmpty())
            ProxyDialog->setObjectName("ProxyDialog");
        ProxyDialog->resize(369, 144);
        gridLayout = new QGridLayout(ProxyDialog);
        gridLayout->setObjectName("gridLayout");
        iconLabel = new QLabel(ProxyDialog);
        iconLabel->setObjectName("iconLabel");

        gridLayout->addWidget(iconLabel, 0, 0, 1, 1);

        introLabel = new QLabel(ProxyDialog);
        introLabel->setObjectName("introLabel");
        introLabel->setWordWrap(true);

        gridLayout->addWidget(introLabel, 0, 1, 1, 2);

        usernameLabel = new QLabel(ProxyDialog);
        usernameLabel->setObjectName("usernameLabel");

        gridLayout->addWidget(usernameLabel, 1, 0, 1, 2);

        userNameLineEdit = new QLineEdit(ProxyDialog);
        userNameLineEdit->setObjectName("userNameLineEdit");

        gridLayout->addWidget(userNameLineEdit, 1, 2, 1, 1);

        passwordLabel = new QLabel(ProxyDialog);
        passwordLabel->setObjectName("passwordLabel");

        gridLayout->addWidget(passwordLabel, 2, 0, 1, 2);

        passwordLineEdit = new QLineEdit(ProxyDialog);
        passwordLineEdit->setObjectName("passwordLineEdit");
        passwordLineEdit->setEchoMode(QLineEdit::Password);

        gridLayout->addWidget(passwordLineEdit, 2, 2, 1, 1);

        buttonBox = new QDialogButtonBox(ProxyDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 3, 0, 1, 3);


        retranslateUi(ProxyDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, ProxyDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, ProxyDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(ProxyDialog);
    } // setupUi

    void retranslateUi(QDialog *ProxyDialog)
    {
        ProxyDialog->setWindowTitle(QCoreApplication::translate("ProxyDialog", "Proxy Authentication", nullptr));
        iconLabel->setText(QCoreApplication::translate("ProxyDialog", "ICON", nullptr));
        introLabel->setText(QCoreApplication::translate("ProxyDialog", "Connect to proxy", nullptr));
        usernameLabel->setText(QCoreApplication::translate("ProxyDialog", "Username:", nullptr));
        passwordLabel->setText(QCoreApplication::translate("ProxyDialog", "Password:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ProxyDialog: public Ui_ProxyDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PROXY_H
