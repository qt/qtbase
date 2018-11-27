/********************************************************************************
** Form generated from reading UI file 'sslclient.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
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
            Form->setObjectName(QString::fromUtf8("Form"));
        Form->resize(343, 320);
        vboxLayout = new QVBoxLayout(Form);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        hostNameLabel = new QLabel(Form);
        hostNameLabel->setObjectName(QString::fromUtf8("hostNameLabel"));

        gridLayout->addWidget(hostNameLabel, 0, 0, 1, 1);

        hostNameEdit = new QLineEdit(Form);
        hostNameEdit->setObjectName(QString::fromUtf8("hostNameEdit"));

        gridLayout->addWidget(hostNameEdit, 0, 1, 1, 1);

        portLabel = new QLabel(Form);
        portLabel->setObjectName(QString::fromUtf8("portLabel"));

        gridLayout->addWidget(portLabel, 1, 0, 1, 1);

        portBox = new QSpinBox(Form);
        portBox->setObjectName(QString::fromUtf8("portBox"));
        portBox->setMinimum(1);
        portBox->setMaximum(65535);
        portBox->setValue(993);

        gridLayout->addWidget(portBox, 1, 1, 1, 1);


        vboxLayout->addLayout(gridLayout);

        connectButton = new QPushButton(Form);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));
        connectButton->setEnabled(true);

        vboxLayout->addWidget(connectButton);

        sessionBox = new QGroupBox(Form);
        sessionBox->setObjectName(QString::fromUtf8("sessionBox"));
        sessionBox->setEnabled(false);
        vboxLayout1 = new QVBoxLayout(sessionBox);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        cipherText = new QLabel(sessionBox);
        cipherText->setObjectName(QString::fromUtf8("cipherText"));

        hboxLayout->addWidget(cipherText);

        cipherLabel = new QLabel(sessionBox);
        cipherLabel->setObjectName(QString::fromUtf8("cipherLabel"));
        cipherLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        hboxLayout->addWidget(cipherLabel);


        vboxLayout1->addLayout(hboxLayout);

        sessionOutput = new QTextEdit(sessionBox);
        sessionOutput->setObjectName(QString::fromUtf8("sessionOutput"));
        sessionOutput->setEnabled(false);
        sessionOutput->setFocusPolicy(Qt::NoFocus);
        sessionOutput->setReadOnly(true);

        vboxLayout1->addWidget(sessionOutput);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        sessionInputLabel = new QLabel(sessionBox);
        sessionInputLabel->setObjectName(QString::fromUtf8("sessionInputLabel"));

        hboxLayout1->addWidget(sessionInputLabel);

        sessionInput = new QLineEdit(sessionBox);
        sessionInput->setObjectName(QString::fromUtf8("sessionInput"));
        sessionInput->setEnabled(false);

        hboxLayout1->addWidget(sessionInput);

        sendButton = new QPushButton(sessionBox);
        sendButton->setObjectName(QString::fromUtf8("sendButton"));
        sendButton->setEnabled(false);
        sendButton->setFocusPolicy(Qt::TabFocus);

        hboxLayout1->addWidget(sendButton);


        vboxLayout1->addLayout(hboxLayout1);


        vboxLayout->addWidget(sessionBox);


        retranslateUi(Form);
        QObject::connect(hostNameEdit, SIGNAL(returnPressed()), connectButton, SLOT(animateClick()));
        QObject::connect(sessionInput, SIGNAL(returnPressed()), sendButton, SLOT(animateClick()));

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
