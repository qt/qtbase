/********************************************************************************
** Form generated from reading UI file 'filternamedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef FILTERNAMEDIALOG_H
#define FILTERNAMEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_FilterNameDialogClass
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *lineEdit;
    QFrame *line;
    QSpacerItem *spacerItem;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *FilterNameDialogClass)
    {
        if (FilterNameDialogClass->objectName().isEmpty())
            FilterNameDialogClass->setObjectName("FilterNameDialogClass");
        FilterNameDialogClass->resize(312, 95);
        gridLayout = new QGridLayout(FilterNameDialogClass);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(9, 9, 9, 9);
        gridLayout->setObjectName("gridLayout");
        label = new QLabel(FilterNameDialogClass);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        lineEdit = new QLineEdit(FilterNameDialogClass);
        lineEdit->setObjectName("lineEdit");

        gridLayout->addWidget(lineEdit, 0, 1, 1, 2);

        line = new QFrame(FilterNameDialogClass);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout->addWidget(line, 1, 0, 1, 3);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(spacerItem, 2, 0, 1, 2);

        buttonBox = new QDialogButtonBox(FilterNameDialogClass);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 2, 2, 1, 1);


        retranslateUi(FilterNameDialogClass);

        QMetaObject::connectSlotsByName(FilterNameDialogClass);
    } // setupUi

    void retranslateUi(QDialog *FilterNameDialogClass)
    {
        FilterNameDialogClass->setWindowTitle(QCoreApplication::translate("FilterNameDialogClass", "FilterNameDialog", nullptr));
        label->setText(QCoreApplication::translate("FilterNameDialogClass", "Filter Name:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FilterNameDialogClass: public Ui_FilterNameDialogClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // FILTERNAMEDIALOG_H
