/********************************************************************************
** Form generated from reading UI file 'mydialog.ui'
**
** Created: Fri Sep 4 10:17:14 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MYDIALOG_H
#define MYDIALOG_H

#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

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
        MyDialog->setWindowTitle(QApplication::translate("MyDialog", "Mach 2!", 0, QApplication::UnicodeUTF8));
        aLabel->setText(QApplication::translate("MyDialog", "Join the life in the fastlane; - PCH enable your project today! -", 0, QApplication::UnicodeUTF8));
        aButton->setText(QApplication::translate("MyDialog", "&Quit", 0, QApplication::UnicodeUTF8));
        aButton->setShortcut(QApplication::translate("MyDialog", "Alt+Q", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MyDialog: public Ui_MyDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MYDIALOG_H
