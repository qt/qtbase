/********************************************************************************
** Form generated from reading UI file 'cookies.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef COOKIES_H
#define COOKIES_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include "edittableview.h"
#include "searchlineedit.h"

QT_BEGIN_NAMESPACE

class Ui_CookiesDialog
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem;
    SearchLineEdit *search;
    EditTableView *cookiesTable;
    QHBoxLayout *hboxLayout;
    QPushButton *removeButton;
    QPushButton *removeAllButton;
    QSpacerItem *spacerItem1;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CookiesDialog)
    {
        if (CookiesDialog->objectName().isEmpty())
            CookiesDialog->setObjectName("CookiesDialog");
        CookiesDialog->resize(550, 370);
        gridLayout = new QGridLayout(CookiesDialog);
        gridLayout->setObjectName("gridLayout");
        spacerItem = new QSpacerItem(252, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 0, 0, 1, 1);

        search = new SearchLineEdit(CookiesDialog);
        search->setObjectName("search");

        gridLayout->addWidget(search, 0, 1, 1, 1);

        cookiesTable = new EditTableView(CookiesDialog);
        cookiesTable->setObjectName("cookiesTable");

        gridLayout->addWidget(cookiesTable, 1, 0, 1, 2);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        removeButton = new QPushButton(CookiesDialog);
        removeButton->setObjectName("removeButton");

        hboxLayout->addWidget(removeButton);

        removeAllButton = new QPushButton(CookiesDialog);
        removeAllButton->setObjectName("removeAllButton");

        hboxLayout->addWidget(removeAllButton);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);

        buttonBox = new QDialogButtonBox(CookiesDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        hboxLayout->addWidget(buttonBox);


        gridLayout->addLayout(hboxLayout, 2, 0, 1, 2);


        retranslateUi(CookiesDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, CookiesDialog, qOverload<>(&QDialog::accept));

        QMetaObject::connectSlotsByName(CookiesDialog);
    } // setupUi

    void retranslateUi(QDialog *CookiesDialog)
    {
        CookiesDialog->setWindowTitle(QCoreApplication::translate("CookiesDialog", "Cookies", nullptr));
        removeButton->setText(QCoreApplication::translate("CookiesDialog", "&Remove", nullptr));
        removeAllButton->setText(QCoreApplication::translate("CookiesDialog", "Remove &All Cookies", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CookiesDialog: public Ui_CookiesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // COOKIES_H
