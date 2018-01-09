/********************************************************************************
** Form generated from reading UI file 'cookiesexceptions.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef COOKIESEXCEPTIONS_H
#define COOKIESEXCEPTIONS_H

#include <QtCore/QVariant>
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
            CookiesExceptionsDialog->setObjectName(QStringLiteral("CookiesExceptionsDialog"));
        CookiesExceptionsDialog->resize(466, 446);
        vboxLayout = new QVBoxLayout(CookiesExceptionsDialog);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        newExceptionGroupBox = new QGroupBox(CookiesExceptionsDialog);
        newExceptionGroupBox->setObjectName(QStringLiteral("newExceptionGroupBox"));
        gridLayout = new QGridLayout(newExceptionGroupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        label = new QLabel(newExceptionGroupBox);
        label->setObjectName(QStringLiteral("label"));

        hboxLayout->addWidget(label);

        domainLineEdit = new QLineEdit(newExceptionGroupBox);
        domainLineEdit->setObjectName(QStringLiteral("domainLineEdit"));

        hboxLayout->addWidget(domainLineEdit);


        gridLayout->addLayout(hboxLayout, 0, 0, 1, 1);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        spacerItem = new QSpacerItem(81, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem);

        blockButton = new QPushButton(newExceptionGroupBox);
        blockButton->setObjectName(QStringLiteral("blockButton"));
        blockButton->setEnabled(false);

        hboxLayout1->addWidget(blockButton);

        allowForSessionButton = new QPushButton(newExceptionGroupBox);
        allowForSessionButton->setObjectName(QStringLiteral("allowForSessionButton"));
        allowForSessionButton->setEnabled(false);

        hboxLayout1->addWidget(allowForSessionButton);

        allowButton = new QPushButton(newExceptionGroupBox);
        allowButton->setObjectName(QStringLiteral("allowButton"));
        allowButton->setEnabled(false);

        hboxLayout1->addWidget(allowButton);


        gridLayout->addLayout(hboxLayout1, 1, 0, 1, 1);


        vboxLayout->addWidget(newExceptionGroupBox);

        ExceptionsGroupBox = new QGroupBox(CookiesExceptionsDialog);
        ExceptionsGroupBox->setObjectName(QStringLiteral("ExceptionsGroupBox"));
        gridLayout1 = new QGridLayout(ExceptionsGroupBox);
        gridLayout1->setObjectName(QStringLiteral("gridLayout1"));
        spacerItem1 = new QSpacerItem(252, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem1, 0, 0, 1, 3);

        search = new SearchLineEdit(ExceptionsGroupBox);
        search->setObjectName(QStringLiteral("search"));

        gridLayout1->addWidget(search, 0, 3, 1, 1);

        exceptionTable = new EditTableView(ExceptionsGroupBox);
        exceptionTable->setObjectName(QStringLiteral("exceptionTable"));

        gridLayout1->addWidget(exceptionTable, 1, 0, 1, 4);

        removeButton = new QPushButton(ExceptionsGroupBox);
        removeButton->setObjectName(QStringLiteral("removeButton"));

        gridLayout1->addWidget(removeButton, 2, 0, 1, 1);

        removeAllButton = new QPushButton(ExceptionsGroupBox);
        removeAllButton->setObjectName(QStringLiteral("removeAllButton"));

        gridLayout1->addWidget(removeAllButton, 2, 1, 1, 1);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem2, 2, 2, 1, 2);


        vboxLayout->addWidget(ExceptionsGroupBox);

        buttonBox = new QDialogButtonBox(CookiesExceptionsDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(CookiesExceptionsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), CookiesExceptionsDialog, SLOT(accept()));

        QMetaObject::connectSlotsByName(CookiesExceptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *CookiesExceptionsDialog)
    {
        CookiesExceptionsDialog->setWindowTitle(QApplication::translate("CookiesExceptionsDialog", "Cookie Exceptions", nullptr));
        newExceptionGroupBox->setTitle(QApplication::translate("CookiesExceptionsDialog", "New Exception", nullptr));
        label->setText(QApplication::translate("CookiesExceptionsDialog", "Domain:", nullptr));
        blockButton->setText(QApplication::translate("CookiesExceptionsDialog", "Block", nullptr));
        allowForSessionButton->setText(QApplication::translate("CookiesExceptionsDialog", "Allow For Session", nullptr));
        allowButton->setText(QApplication::translate("CookiesExceptionsDialog", "Allow", nullptr));
        ExceptionsGroupBox->setTitle(QApplication::translate("CookiesExceptionsDialog", "Exceptions", nullptr));
        removeButton->setText(QApplication::translate("CookiesExceptionsDialog", "&Remove", nullptr));
        removeAllButton->setText(QApplication::translate("CookiesExceptionsDialog", "Remove &All", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CookiesExceptionsDialog: public Ui_CookiesExceptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // COOKIESEXCEPTIONS_H
