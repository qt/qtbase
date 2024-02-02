/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

*/

/********************************************************************************
** Form generated from reading UI file 'newform.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef NEWFORM_H
#define NEWFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_NewForm
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QTreeWidget *treeWidget;
    QLabel *lblPreview;
    QFrame *horizontalLine;
    QCheckBox *chkShowOnStartup;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *NewForm)
    {
        if (NewForm->objectName().isEmpty())
            NewForm->setObjectName("NewForm");
        NewForm->resize(495, 319);
        vboxLayout = new QVBoxLayout(NewForm);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName("vboxLayout");
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName("hboxLayout");
        treeWidget = new QTreeWidget(NewForm);
        treeWidget->setObjectName("treeWidget");
        treeWidget->setIconSize(QSize(128, 128));
        treeWidget->setRootIsDecorated(false);
        treeWidget->setColumnCount(1);

        hboxLayout->addWidget(treeWidget);

        lblPreview = new QLabel(NewForm);
        lblPreview->setObjectName("lblPreview");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lblPreview->sizePolicy().hasHeightForWidth());
        lblPreview->setSizePolicy(sizePolicy);
        lblPreview->setLineWidth(1);
        lblPreview->setAlignment(Qt::AlignCenter);
        lblPreview->setMargin(5);

        hboxLayout->addWidget(lblPreview);


        vboxLayout->addLayout(hboxLayout);

        horizontalLine = new QFrame(NewForm);
        horizontalLine->setObjectName("horizontalLine");
        horizontalLine->setFrameShape(QFrame::HLine);
        horizontalLine->setFrameShadow(QFrame::Sunken);

        vboxLayout->addWidget(horizontalLine);

        chkShowOnStartup = new QCheckBox(NewForm);
        chkShowOnStartup->setObjectName("chkShowOnStartup");

        vboxLayout->addWidget(chkShowOnStartup);

        buttonBox = new QDialogButtonBox(NewForm);
        buttonBox->setObjectName("buttonBox");

        vboxLayout->addWidget(buttonBox);


        retranslateUi(NewForm);

        QMetaObject::connectSlotsByName(NewForm);
    } // setupUi

    void retranslateUi(QDialog *NewForm)
    {
        NewForm->setWindowTitle(QCoreApplication::translate("NewForm", "New Form", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("NewForm", "0", nullptr));
        lblPreview->setText(QCoreApplication::translate("NewForm", "Choose a template for a preview", nullptr));
        chkShowOnStartup->setText(QCoreApplication::translate("NewForm", "Show this Dialog on Startup", nullptr));
    } // retranslateUi

};

namespace Ui {
    class NewForm: public Ui_NewForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // NEWFORM_H
