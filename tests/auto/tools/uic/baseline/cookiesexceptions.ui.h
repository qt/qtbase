/********************************************************************************
** Form generated from reading UI file 'cookiesexceptions.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef COOKIESEXCEPTIONS_H
#define COOKIESEXCEPTIONS_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "edittableview.h"
#include "searchlineedit.h"

QT_BEGIN_NAMESPACE

class Ui_CookiesExceptionsDialog
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *newExceptionGroupBox;
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QLabel *label;
    QLineEdit *domainLineEdit;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem;
    QPushButton *blockButton;
    QPushButton *allowForSessionButton;
    QPushButton *allowButton;
    QGroupBox *ExceptionsGroupBox;
    QGridLayout *gridLayout1;
    QSpacerItem *spacerItem1;
    SearchLineEdit *search;
    EditTableView *exceptionTable;
    QPushButton *removeButton;
    QPushButton *removeAllButton;
    QSpacerItem *spacerItem2;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CookiesExceptionsDialog)
    {
        if (CookiesExceptionsDialog->objectName().isEmpty())
            CookiesExceptionsDialog->setObjectName("CookiesExceptionsDialog");
        CookiesExceptionsDialog->resize(466, 446);
        vboxLayout = new QVBoxLayout(CookiesExceptionsDialog);
        vboxLayout->setObjectName("vboxLayout");
        newExceptionGroupBox = new QGroupBox(CookiesExceptionsDialog);
        newExceptionGroupBox->setObjectName("newExceptionGroupBox");
        gridLayout = new QGridLayout(newExceptionGroupBox);
        gridLayout->setObjectName("gridLayout");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        label = new QLabel(newExceptionGroupBox);
        label->setObjectName("label");

        hboxLayout->addWidget(label);

        domainLineEdit = new QLineEdit(newExceptionGroupBox);
        domainLineEdit->setObjectName("domainLineEdit");

        hboxLayout->addWidget(domainLineEdit);


        gridLayout->addLayout(hboxLayout, 0, 0, 1, 1);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName("hboxLayout1");
        spacerItem = new QSpacerItem(81, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem);

        blockButton = new QPushButton(newExceptionGroupBox);
        blockButton->setObjectName("blockButton");
        blockButton->setEnabled(false);

        hboxLayout1->addWidget(blockButton);

        allowForSessionButton = new QPushButton(newExceptionGroupBox);
        allowForSessionButton->setObjectName("allowForSessionButton");
        allowForSessionButton->setEnabled(false);

        hboxLayout1->addWidget(allowForSessionButton);

        allowButton = new QPushButton(newExceptionGroupBox);
        allowButton->setObjectName("allowButton");
        allowButton->setEnabled(false);

        hboxLayout1->addWidget(allowButton);


        gridLayout->addLayout(hboxLayout1, 1, 0, 1, 1);


        vboxLayout->addWidget(newExceptionGroupBox);

        ExceptionsGroupBox = new QGroupBox(CookiesExceptionsDialog);
        ExceptionsGroupBox->setObjectName("ExceptionsGroupBox");
        gridLayout1 = new QGridLayout(ExceptionsGroupBox);
        gridLayout1->setObjectName("gridLayout1");
        spacerItem1 = new QSpacerItem(252, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem1, 0, 0, 1, 3);

        search = new SearchLineEdit(ExceptionsGroupBox);
        search->setObjectName("search");

        gridLayout1->addWidget(search, 0, 3, 1, 1);

        exceptionTable = new EditTableView(ExceptionsGroupBox);
        exceptionTable->setObjectName("exceptionTable");

        gridLayout1->addWidget(exceptionTable, 1, 0, 1, 4);

        removeButton = new QPushButton(ExceptionsGroupBox);
        removeButton->setObjectName("removeButton");

        gridLayout1->addWidget(removeButton, 2, 0, 1, 1);

        removeAllButton = new QPushButton(ExceptionsGroupBox);
        removeAllButton->setObjectName("removeAllButton");

        gridLayout1->addWidget(removeAllButton, 2, 1, 1, 1);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem2, 2, 2, 1, 2);


        vboxLayout->addWidget(ExceptionsGroupBox);

        buttonBox = new QDialogButtonBox(CookiesExceptionsDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(CookiesExceptionsDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, CookiesExceptionsDialog, qOverload<>(&QDialog::accept));

        QMetaObject::connectSlotsByName(CookiesExceptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *CookiesExceptionsDialog)
    {
        CookiesExceptionsDialog->setWindowTitle(QCoreApplication::translate("CookiesExceptionsDialog", "Cookie Exceptions", nullptr));
        newExceptionGroupBox->setTitle(QCoreApplication::translate("CookiesExceptionsDialog", "New Exception", nullptr));
        label->setText(QCoreApplication::translate("CookiesExceptionsDialog", "Domain:", nullptr));
        blockButton->setText(QCoreApplication::translate("CookiesExceptionsDialog", "Block", nullptr));
        allowForSessionButton->setText(QCoreApplication::translate("CookiesExceptionsDialog", "Allow For Session", nullptr));
        allowButton->setText(QCoreApplication::translate("CookiesExceptionsDialog", "Allow", nullptr));
        ExceptionsGroupBox->setTitle(QCoreApplication::translate("CookiesExceptionsDialog", "Exceptions", nullptr));
        removeButton->setText(QCoreApplication::translate("CookiesExceptionsDialog", "&Remove", nullptr));
        removeAllButton->setText(QCoreApplication::translate("CookiesExceptionsDialog", "Remove &All", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CookiesExceptionsDialog: public Ui_CookiesExceptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // COOKIESEXCEPTIONS_H
