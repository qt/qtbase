/********************************************************************************
** Form generated from reading UI file 'templateconnection.ui'
**
** Created by: Qt User Interface Compiler version 6.13.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TEMPLATECONNECTION_H
#define TEMPLATECONNECTION_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "customplot.h"

QT_BEGIN_NAMESPACE

class Ui_TemplateConnectionForm
{
public:
    QVBoxLayout *verticalLayout;
    CustomPlot *source;
    CustomPlot *target;

    void setupUi(QWidget *TemplateConnectionForm)
    {
        if (TemplateConnectionForm->objectName().isEmpty())
            TemplateConnectionForm->setObjectName("TemplateConnectionForm");
        verticalLayout = new QVBoxLayout(TemplateConnectionForm);
        verticalLayout->setObjectName("verticalLayout");
        source = new CustomPlot(TemplateConnectionForm);
        source->setObjectName("source");

        verticalLayout->addWidget(source);

        target = new CustomPlot(TemplateConnectionForm);
        target->setObjectName("target");

        verticalLayout->addWidget(target);


        retranslateUi(TemplateConnectionForm);
        QObject::connect(source, SIGNAL(valuesChanged(QList<double>)), target, SLOT(setValues(QList<double>)));
        QObject::connect(source, SIGNAL(rangesChanged(QMap<QString,int>)), target, SLOT(setRanges(QMap<QString,int>)));

        QMetaObject::connectSlotsByName(TemplateConnectionForm);
    } // setupUi

    void retranslateUi(QWidget *TemplateConnectionForm)
    {
        (void)TemplateConnectionForm;
    } // retranslateUi

};

namespace Ui {
    class TemplateConnectionForm: public Ui_TemplateConnectionForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TEMPLATECONNECTION_H
