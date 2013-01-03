/********************************************************************************
** Form generated from reading UI file 'mydialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MYDIALOG_H
#define MYDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
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
            MyDialog->setObjectName(QStringLiteral("MyDialog"));
        MyDialog->resize(401, 70);
        vboxLayout = new QVBoxLayout(MyDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        aLabel = new QLabel(MyDialog);
        aLabel->setObjectName(QStringLiteral("aLabel"));

        vboxLayout->addWidget(aLabel);

        aButton = new QPushButton(MyDialog);
        aButton->setObjectName(QStringLiteral("aButton"));

        vboxLayout->addWidget(aButton);


        retranslateUi(MyDialog);

        QMetaObject::connectSlotsByName(MyDialog);
    } // setupUi

    void retranslateUi(QDialog *MyDialog)
    {
        MyDialog->setWindowTitle(QApplication::translate("MyDialog", "Mach 2!", 0));
        aLabel->setText(QApplication::translate("MyDialog", "Join the life in the fastlane; - PCH enable your project today! -", 0));
        aButton->setText(QApplication::translate("MyDialog", "&Quit", 0));
        aButton->setShortcut(QApplication::translate("MyDialog", "Alt+Q", 0));
    } // retranslateUi

};

namespace Ui {
    class MyDialog: public Ui_MyDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MYDIALOG_H
