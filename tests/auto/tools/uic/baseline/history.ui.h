/********************************************************************************
** Form generated from reading UI file 'history.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef HISTORY_H
#define HISTORY_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include "edittreeview.h"
#include "searchlineedit.h"

QT_BEGIN_NAMESPACE

class Ui_HistoryDialog
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem;
    SearchLineEdit *search;
    EditTreeView *tree;
    QHBoxLayout *hboxLayout;
    QPushButton *removeButton;
    QPushButton *removeAllButton;
    QSpacerItem *spacerItem1;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *HistoryDialog)
    {
        if (HistoryDialog->objectName().isEmpty())
            HistoryDialog->setObjectName(QStringLiteral("HistoryDialog"));
        HistoryDialog->resize(758, 450);
        gridLayout = new QGridLayout(HistoryDialog);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        spacerItem = new QSpacerItem(252, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 0, 0, 1, 1);

        search = new SearchLineEdit(HistoryDialog);
        search->setObjectName(QStringLiteral("search"));

        gridLayout->addWidget(search, 0, 1, 1, 1);

        tree = new EditTreeView(HistoryDialog);
        tree->setObjectName(QStringLiteral("tree"));

        gridLayout->addWidget(tree, 1, 0, 1, 2);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        removeButton = new QPushButton(HistoryDialog);
        removeButton->setObjectName(QStringLiteral("removeButton"));

        hboxLayout->addWidget(removeButton);

        removeAllButton = new QPushButton(HistoryDialog);
        removeAllButton->setObjectName(QStringLiteral("removeAllButton"));

        hboxLayout->addWidget(removeAllButton);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);

        buttonBox = new QDialogButtonBox(HistoryDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        hboxLayout->addWidget(buttonBox);


        gridLayout->addLayout(hboxLayout, 2, 0, 1, 2);


        retranslateUi(HistoryDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), HistoryDialog, SLOT(accept()));

        QMetaObject::connectSlotsByName(HistoryDialog);
    } // setupUi

    void retranslateUi(QDialog *HistoryDialog)
    {
        HistoryDialog->setWindowTitle(QApplication::translate("HistoryDialog", "History", nullptr));
        removeButton->setText(QApplication::translate("HistoryDialog", "&Remove", nullptr));
        removeAllButton->setText(QApplication::translate("HistoryDialog", "Remove &All", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HistoryDialog: public Ui_HistoryDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // HISTORY_H
