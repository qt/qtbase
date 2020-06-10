/********************************************************************************
** Form generated from reading UI file 'idbased.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef IDBASED_H
#define IDBASED_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QVBoxLayout *verticalLayout;
    QPushButton *pushButton;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName("Form");
        Form->resize(400, 300);
        verticalLayout = new QVBoxLayout(Form);
        verticalLayout->setObjectName("verticalLayout");
        pushButton = new QPushButton(Form);
        pushButton->setObjectName("pushButton");

        verticalLayout->addWidget(pushButton);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(qtTrId("windowTitleId"));
#if QT_CONFIG(tooltip)
        pushButton->setToolTip(qtTrId("buttonToolTipId"));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        pushButton->setStatusTip(qtTrId("buttonStatusTipId"));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(whatsthis)
        pushButton->setWhatsThis(qtTrId("buttonWhatsThisId"));
#endif // QT_CONFIG(whatsthis)
        pushButton->setText(qtTrId("buttonTextId"));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // IDBASED_H
