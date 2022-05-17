/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

*/

/********************************************************************************
** Form generated from reading UI file 'orderdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ORDERDIALOG_H
#define ORDERDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_OrderDialog
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QHBoxLayout *hboxLayout;
    QListWidget *pageList;
    QVBoxLayout *vboxLayout1;
    QToolButton *upButton;
    QToolButton *downButton;
    QSpacerItem *spacerItem;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *qdesigner_internal__OrderDialog)
    {
        if (qdesigner_internal__OrderDialog->objectName().isEmpty())
            qdesigner_internal__OrderDialog->setObjectName("qdesigner_internal__OrderDialog");
        qdesigner_internal__OrderDialog->resize(467, 310);
        vboxLayout = new QVBoxLayout(qdesigner_internal__OrderDialog);
        vboxLayout->setObjectName("vboxLayout");
        groupBox = new QGroupBox(qdesigner_internal__OrderDialog);
        groupBox->setObjectName("groupBox");
        hboxLayout = new QHBoxLayout(groupBox);
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName("hboxLayout");
        hboxLayout->setContentsMargins(9, 9, 9, 9);
        pageList = new QListWidget(groupBox);
        pageList->setObjectName("pageList");
        pageList->setMinimumSize(QSize(344, 0));
        pageList->setDragDropMode(QAbstractItemView::InternalMove);
        pageList->setSelectionMode(QAbstractItemView::ContiguousSelection);
        pageList->setMovement(QListView::Snap);

        hboxLayout->addWidget(pageList);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setObjectName("vboxLayout1");
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
        upButton = new QToolButton(groupBox);
        upButton->setObjectName("upButton");

        vboxLayout1->addWidget(upButton);

        downButton = new QToolButton(groupBox);
        downButton->setObjectName("downButton");

        vboxLayout1->addWidget(downButton);

        spacerItem = new QSpacerItem(20, 99, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacerItem);


        hboxLayout->addLayout(vboxLayout1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(qdesigner_internal__OrderDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::Reset);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(qdesigner_internal__OrderDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, qdesigner_internal__OrderDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, qdesigner_internal__OrderDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(qdesigner_internal__OrderDialog);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__OrderDialog)
    {
        qdesigner_internal__OrderDialog->setWindowTitle(QCoreApplication::translate("qdesigner_internal::OrderDialog", "Change Page Order", nullptr));
        groupBox->setTitle(QCoreApplication::translate("qdesigner_internal::OrderDialog", "Page Order", nullptr));
#if QT_CONFIG(tooltip)
        upButton->setToolTip(QCoreApplication::translate("qdesigner_internal::OrderDialog", "Move page up", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        downButton->setToolTip(QCoreApplication::translate("qdesigner_internal::OrderDialog", "Move page down", nullptr));
#endif // QT_CONFIG(tooltip)
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class OrderDialog: public Ui_OrderDialog {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // ORDERDIALOG_H
