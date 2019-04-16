/********************************************************************************
** Form generated from reading UI file 'pixmapfunction.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PIXMAPFUNCTION_H
#define PIXMAPFUNCTION_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QPushButton *pushButton;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QString::fromUtf8("Form"));
        Form->resize(149, 112);
        verticalLayout = new QVBoxLayout(Form);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label = new QLabel(Form);
        label->setObjectName(QString::fromUtf8("label"));
        label->setPixmap(QPixmap(pixmapFunction("labelPixmap")));

        verticalLayout->addWidget(label);

        pushButton = new QPushButton(Form);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        QIcon icon;
        icon.addPixmap(QPixmap(pixmapFunction("buttonIconNormalOff")), QIcon::Normal, QIcon::Off);
        icon.addPixmap(QPixmap(pixmapFunction("buttonIconNormalOn")), QIcon::Normal, QIcon::On);
        icon.addPixmap(QPixmap(pixmapFunction("buttonIconDisabledOff")), QIcon::Disabled, QIcon::Off);
        icon.addPixmap(QPixmap(pixmapFunction("buttonIconDisabledOn")), QIcon::Disabled, QIcon::On);
        pushButton->setIcon(icon);

        verticalLayout->addWidget(pushButton);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QCoreApplication::translate("Form", "Form", nullptr));
        label->setText(QString());
        pushButton->setText(QCoreApplication::translate("Form", "PushButton", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PIXMAPFUNCTION_H
