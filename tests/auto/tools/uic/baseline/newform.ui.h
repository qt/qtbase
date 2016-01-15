/*
*********************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'newform.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef NEWFORM_H
#define NEWFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
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
            NewForm->setObjectName(QStringLiteral("NewForm"));
        NewForm->resize(495, 319);
        vboxLayout = new QVBoxLayout(NewForm);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        treeWidget = new QTreeWidget(NewForm);
        treeWidget->setObjectName(QStringLiteral("treeWidget"));
        treeWidget->setIconSize(QSize(128, 128));
        treeWidget->setRootIsDecorated(false);
        treeWidget->setColumnCount(1);

        hboxLayout->addWidget(treeWidget);

        lblPreview = new QLabel(NewForm);
        lblPreview->setObjectName(QStringLiteral("lblPreview"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(7), static_cast<QSizePolicy::Policy>(5));
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
        horizontalLine->setObjectName(QStringLiteral("horizontalLine"));
        horizontalLine->setFrameShape(QFrame::HLine);
        horizontalLine->setFrameShadow(QFrame::Sunken);

        vboxLayout->addWidget(horizontalLine);

        chkShowOnStartup = new QCheckBox(NewForm);
        chkShowOnStartup->setObjectName(QStringLiteral("chkShowOnStartup"));

        vboxLayout->addWidget(chkShowOnStartup);

        buttonBox = new QDialogButtonBox(NewForm);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));

        vboxLayout->addWidget(buttonBox);


        retranslateUi(NewForm);

        QMetaObject::connectSlotsByName(NewForm);
    } // setupUi

    void retranslateUi(QDialog *NewForm)
    {
        NewForm->setWindowTitle(QApplication::translate("NewForm", "New Form", 0));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("NewForm", "0", 0));
        lblPreview->setText(QApplication::translate("NewForm", "Choose a template for a preview", 0));
        chkShowOnStartup->setText(QApplication::translate("NewForm", "Show this Dialog on Startup", 0));
    } // retranslateUi

};

namespace Ui {
    class NewForm: public Ui_NewForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // NEWFORM_H
