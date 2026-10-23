/********************************************************************************
** Form generated from reading UI file 'normalizedconnection.ui'
**
** Created by: Qt User Interface Compiler version 6.13.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef NORMALIZEDCONNECTION_H
#define NORMALIZEDCONNECTION_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NormalizedConnectionForm
{
public:
    QVBoxLayout *verticalLayout;
    QLineEdit *line_edit;
    QLabel *status_label;

    void setupUi(QWidget *NormalizedConnectionForm)
    {
        if (NormalizedConnectionForm->objectName().isEmpty())
            NormalizedConnectionForm->setObjectName("NormalizedConnectionForm");
        verticalLayout = new QVBoxLayout(NormalizedConnectionForm);
        verticalLayout->setObjectName("verticalLayout");
        line_edit = new QLineEdit(NormalizedConnectionForm);
        line_edit->setObjectName("line_edit");

        verticalLayout->addWidget(line_edit);

        status_label = new QLabel(NormalizedConnectionForm);
        status_label->setObjectName("status_label");

        verticalLayout->addWidget(status_label);


        retranslateUi(NormalizedConnectionForm);
        QObject::connect(line_edit, &QLineEdit::textChanged, status_label, &QLabel::setText);

        QMetaObject::connectSlotsByName(NormalizedConnectionForm);
    } // setupUi

    void retranslateUi(QWidget *NormalizedConnectionForm)
    {
        (void)NormalizedConnectionForm;
    } // retranslateUi

};

namespace Ui {
    class NormalizedConnectionForm: public Ui_NormalizedConnectionForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // NORMALIZEDCONNECTION_H
