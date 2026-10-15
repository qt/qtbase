/********************************************************************************
** Form generated from reading UI file 'templatewidget.ui'
**
** Created by: Qt User Interface Compiler version 6.13.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TEMPLATEWIDGET_H
#define TEMPLATEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "itemviewsized.h"

QT_BEGIN_NAMESPACE

class Ui_TemplateWidget
{
public:
    QVBoxLayout *verticalLayout;
    ItemViewSized<QListView> *listView;

    void setupUi(QWidget *TemplateWidget)
    {
        if (TemplateWidget->objectName().isEmpty())
            TemplateWidget->setObjectName("TemplateWidget");
        verticalLayout = new QVBoxLayout(TemplateWidget);
        verticalLayout->setObjectName("verticalLayout");
        listView = new ItemViewSized<QListView>(TemplateWidget);
        listView->setObjectName("listView");

        verticalLayout->addWidget(listView);


        retranslateUi(TemplateWidget);

        QMetaObject::connectSlotsByName(TemplateWidget);
    } // setupUi

    void retranslateUi(QWidget *TemplateWidget)
    {
        (void)TemplateWidget;
    } // retranslateUi

};

namespace Ui {
    class TemplateWidget: public Ui_TemplateWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TEMPLATEWIDGET_H
