/********************************************************************************
** Form generated from reading UI file 'sslclient.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SSLCLIENT_H
#define SSLCLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
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
            Form->setObjectName(QStringLiteral("Form"));
        Form->resize(343, 320);
        vboxLayout = new QVBoxLayout(Form);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        hostNameLabel = new QLabel(Form);
        hostNameLabel->setObjectName(QStringLiteral("hostNameLabel"));

        gridLayout->addWidget(hostNameLabel, 0, 0, 1, 1);

        hostNameEdit = new QLineEdit(Form);
        hostNameEdit->setObjectName(QStringLiteral("hostNameEdit"));

        gridLayout->addWidget(hostNameEdit, 0, 1, 1, 1);

        portLabel = new QLabel(Form);
        portLabel->setObjectName(QStringLiteral("portLabel"));

        gridLayout->addWidget(portLabel, 1, 0, 1, 1);

        portBox = new QSpinBox(Form);
        portBox->setObjectName(QStringLiteral("portBox"));
        portBox->setMinimum(1);
        portBox->setMaximum(65535);
        portBox->setValue(993);

        gridLayout->addWidget(portBox, 1, 1, 1, 1);


        vboxLayout->addLayout(gridLayout);

        connectButton = new QPushButton(Form);
        connectButton->setObjectName(QStringLiteral("connectButton"));
        connectButton->setEnabled(true);

        vboxLayout->addWidget(connectButton);

        sessionBox = new QGroupBox(Form);
        sessionBox->setObjectName(QStringLiteral("sessionBox"));
        sessionBox->setEnabled(false);
        vboxLayout1 = new QVBoxLayout(sessionBox);
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        cipherText = new QLabel(sessionBox);
        cipherText->setObjectName(QStringLiteral("cipherText"));

        hboxLayout->addWidget(cipherText);

        cipherLabel = new QLabel(sessionBox);
        cipherLabel->setObjectName(QStringLiteral("cipherLabel"));
        cipherLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        hboxLayout->addWidget(cipherLabel);


        vboxLayout1->addLayout(hboxLayout);

        sessionOutput = new QTextEdit(sessionBox);
        sessionOutput->setObjectName(QStringLiteral("sessionOutput"));
        sessionOutput->setEnabled(false);
        sessionOutput->setFocusPolicy(Qt::NoFocus);
        sessionOutput->setReadOnly(true);

        vboxLayout1->addWidget(sessionOutput);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        sessionInputLabel = new QLabel(sessionBox);
        sessionInputLabel->setObjectName(QStringLiteral("sessionInputLabel"));

        hboxLayout1->addWidget(sessionInputLabel);

        sessionInput = new QLineEdit(sessionBox);
        sessionInput->setObjectName(QStringLiteral("sessionInput"));
        sessionInput->setEnabled(false);

        hboxLayout1->addWidget(sessionInput);

        sendButton = new QPushButton(sessionBox);
        sendButton->setObjectName(QStringLiteral("sendButton"));
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
        Form->setWindowTitle(QApplication::translate("Form", "Secure Socket Client", Q_NULLPTR));
        hostNameLabel->setText(QApplication::translate("Form", "Host name:", Q_NULLPTR));
        hostNameEdit->setText(QApplication::translate("Form", "imap.example.com", Q_NULLPTR));
        portLabel->setText(QApplication::translate("Form", "Port:", Q_NULLPTR));
        connectButton->setText(QApplication::translate("Form", "Connect to host", Q_NULLPTR));
        sessionBox->setTitle(QApplication::translate("Form", "Active session", Q_NULLPTR));
        cipherText->setText(QApplication::translate("Form", "Cryptographic Cipher:", Q_NULLPTR));
        cipherLabel->setText(QApplication::translate("Form", "<none>", Q_NULLPTR));
        sessionOutput->setHtml(QApplication::translate("Form", "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p></body></html>", Q_NULLPTR));
        sessionInputLabel->setText(QApplication::translate("Form", "Input:", Q_NULLPTR));
        sendButton->setText(QApplication::translate("Form", "&Send", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SSLCLIENT_H
