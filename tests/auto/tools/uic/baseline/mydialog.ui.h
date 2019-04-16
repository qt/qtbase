/********************************************************************************
** Form generated from reading UI file 'mydialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MYDIALOG_H
#define MYDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_MyDialog
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *aLabel;
    QPushButton *aButton;

    void setupUi(QDialog *MyDialog)
    {
        if (MyDialog->objectName().isEmpty())
            MyDialog->setObjectName(QString::fromUtf8("MyDialog"));
        MyDialog->resize(401, 70);
        vboxLayout = new QVBoxLayout(MyDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        aLabel = new QLabel(MyDialog);
        aLabel->setObjectName(QString::fromUtf8("aLabel"));

        vboxLayout->addWidget(aLabel);

        aButton = new QPushButton(MyDialog);
        aButton->setObjectName(QString::fromUtf8("aButton"));

        vboxLayout->addWidget(aButton);


        retranslateUi(MyDialog);

        QMetaObject::connectSlotsByName(MyDialog);
    } // setupUi

    void retranslateUi(QDialog *MyDialog)
    {
        MyDialog->setWindowTitle(QCoreApplication::translate("MyDialog", "Mach 2!", nullptr));
        aLabel->setText(QCoreApplication::translate("MyDialog", "Join the life in the fastlane; - PCH enable your project today! -", nullptr));
        aButton->setText(QCoreApplication::translate("MyDialog", "&Quit", nullptr));
#if QT_CONFIG(shortcut)
        aButton->setShortcut(QCoreApplication::translate("MyDialog", "Alt+Q", nullptr));
#endif // QT_CONFIG(shortcut)
    } // retranslateUi

};

namespace Ui {
    class MyDialog: public Ui_MyDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MYDIALOG_H
