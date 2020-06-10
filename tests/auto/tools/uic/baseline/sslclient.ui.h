/********************************************************************************
** Form generated from reading UI file 'sslclient.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SSLCLIENT_H
#define SSLCLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QVBoxLayout *vboxLayout;
    QGridLayout *gridLayout;
    QLabel *hostNameLabel;
    QLineEdit *hostNameEdit;
    QLabel *portLabel;
    QSpinBox *portBox;
    QPushButton *connectButton;
    QGroupBox *sessionBox;
    QVBoxLayout *vboxLayout1;
    QHBoxLayout *hboxLayout;
    QLabel *cipherText;
    QLabel *cipherLabel;
    QTextEdit *sessionOutput;
    QHBoxLayout *hboxLayout1;
    QLabel *sessionInputLabel;
    QLineEdit *sessionInput;
    QPushButton *sendButton;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName("Form");
        Form->resize(343, 320);
        vboxLayout = new QVBoxLayout(Form);
        vboxLayout->setObjectName("vboxLayout");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        hostNameLabel = new QLabel(Form);
        hostNameLabel->setObjectName("hostNameLabel");

        gridLayout->addWidget(hostNameLabel, 0, 0, 1, 1);

        hostNameEdit = new QLineEdit(Form);
        hostNameEdit->setObjectName("hostNameEdit");

        gridLayout->addWidget(hostNameEdit, 0, 1, 1, 1);

        portLabel = new QLabel(Form);
        portLabel->setObjectName("portLabel");

        gridLayout->addWidget(portLabel, 1, 0, 1, 1);

        portBox = new QSpinBox(Form);
        portBox->setObjectName("portBox");
        portBox->setMinimum(1);
        portBox->setMaximum(65535);
        portBox->setValue(993);

        gridLayout->addWidget(portBox, 1, 1, 1, 1);


        vboxLayout->addLayout(gridLayout);

        connectButton = new QPushButton(Form);
        connectButton->setObjectName("connectButton");
        connectButton->setEnabled(true);

        vboxLayout->addWidget(connectButton);

        sessionBox = new QGroupBox(Form);
        sessionBox->setObjectName("sessionBox");
        sessionBox->setEnabled(false);
        vboxLayout1 = new QVBoxLayout(sessionBox);
        vboxLayout1->setObjectName("vboxLayout1");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        cipherText = new QLabel(sessionBox);
        cipherText->setObjectName("cipherText");

        hboxLayout->addWidget(cipherText);

        cipherLabel = new QLabel(sessionBox);
        cipherLabel->setObjectName("cipherLabel");
        cipherLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        hboxLayout->addWidget(cipherLabel);


        vboxLayout1->addLayout(hboxLayout);

        sessionOutput = new QTextEdit(sessionBox);
        sessionOutput->setObjectName("sessionOutput");
        sessionOutput->setEnabled(false);
        sessionOutput->setFocusPolicy(Qt::NoFocus);
        sessionOutput->setReadOnly(true);

        vboxLayout1->addWidget(sessionOutput);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName("hboxLayout1");
        sessionInputLabel = new QLabel(sessionBox);
        sessionInputLabel->setObjectName("sessionInputLabel");

        hboxLayout1->addWidget(sessionInputLabel);

        sessionInput = new QLineEdit(sessionBox);
        sessionInput->setObjectName("sessionInput");
        sessionInput->setEnabled(false);

        hboxLayout1->addWidget(sessionInput);

        sendButton = new QPushButton(sessionBox);
        sendButton->setObjectName("sendButton");
        sendButton->setEnabled(false);
        sendButton->setFocusPolicy(Qt::TabFocus);

        hboxLayout1->addWidget(sendButton);


        vboxLayout1->addLayout(hboxLayout1);


        vboxLayout->addWidget(sessionBox);


        retranslateUi(Form);
        QObject::connect(hostNameEdit, &QLineEdit::returnPressed, connectButton, qOverload<>(&QPushButton::animateClick));
        QObject::connect(sessionInput, &QLineEdit::returnPressed, sendButton, qOverload<>(&QPushButton::animateClick));

        connectButton->setDefault(true);
        sendButton->setDefault(true);


        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QCoreApplication::translate("Form", "Secure Socket Client", nullptr));
        hostNameLabel->setText(QCoreApplication::translate("Form", "Host name:", nullptr));
        hostNameEdit->setText(QCoreApplication::translate("Form", "imap.example.com", nullptr));
        portLabel->setText(QCoreApplication::translate("Form", "Port:", nullptr));
        connectButton->setText(QCoreApplication::translate("Form", "Connect to host", nullptr));
        sessionBox->setTitle(QCoreApplication::translate("Form", "Active session", nullptr));
        cipherText->setText(QCoreApplication::translate("Form", "Cryptographic Cipher:", nullptr));
        cipherLabel->setText(QCoreApplication::translate("Form", "<none>", nullptr));
        sessionOutput->setHtml(QCoreApplication::translate("Form", "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p></body></html>", nullptr));
        sessionInputLabel->setText(QCoreApplication::translate("Form", "Input:", nullptr));
        sendButton->setText(QCoreApplication::translate("Form", "&Send", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SSLCLIENT_H
