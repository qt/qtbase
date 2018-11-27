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
** Form generated from reading UI file 'treewidgeteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TREEWIDGETEDITOR_H
#define TREEWIDGETEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeWidget>
#include "iconselector_p.h"

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_TreeWidgetEditor
{
public:
    QGridLayout *gridLayout_3;
    QGroupBox *itemsBox;
    QGridLayout *gridLayout;
    QTreeWidget *treeWidget;
    QHBoxLayout *horizontalLayout_4;
    QToolButton *newItemButton;
    QToolButton *newSubItemButton;
    QToolButton *deleteItemButton;
    QSpacerItem *spacerItem;
    QToolButton *moveItemLeftButton;
    QToolButton *moveItemRightButton;
    QToolButton *moveItemUpButton;
    QToolButton *moveItemDownButton;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    qdesigner_internal::IconSelector *itemIconSelector;
    QSpacerItem *horizontalSpacer;
    QGroupBox *columnsBox;
    QGridLayout *gridLayout_2;
    QListWidget *listWidget;
    QHBoxLayout *horizontalLayout_3;
    QToolButton *newColumnButton;
    QToolButton *deleteColumnButton;
    QSpacerItem *spacerItem1;
    QToolButton *moveColumnUpButton;
    QToolButton *moveColumnDownButton;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    qdesigner_internal::IconSelector *columnIconSelector;
    QSpacerItem *spacerItem2;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *qdesigner_internal__TreeWidgetEditor)
    {
        if (qdesigner_internal__TreeWidgetEditor->objectName().isEmpty())
            qdesigner_internal__TreeWidgetEditor->setObjectName(QString::fromUtf8("qdesigner_internal__TreeWidgetEditor"));
        qdesigner_internal__TreeWidgetEditor->resize(619, 321);
        gridLayout_3 = new QGridLayout(qdesigner_internal__TreeWidgetEditor);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        itemsBox = new QGroupBox(qdesigner_internal__TreeWidgetEditor);
        itemsBox->setObjectName(QString::fromUtf8("itemsBox"));
        gridLayout = new QGridLayout(itemsBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        treeWidget = new QTreeWidget(itemsBox);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout->addWidget(treeWidget, 0, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        newItemButton = new QToolButton(itemsBox);
        newItemButton->setObjectName(QString::fromUtf8("newItemButton"));

        horizontalLayout_4->addWidget(newItemButton);

        newSubItemButton = new QToolButton(itemsBox);
        newSubItemButton->setObjectName(QString::fromUtf8("newSubItemButton"));

        horizontalLayout_4->addWidget(newSubItemButton);

        deleteItemButton = new QToolButton(itemsBox);
        deleteItemButton->setObjectName(QString::fromUtf8("deleteItemButton"));

        horizontalLayout_4->addWidget(deleteItemButton);

        spacerItem = new QSpacerItem(28, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(spacerItem);

        moveItemLeftButton = new QToolButton(itemsBox);
        moveItemLeftButton->setObjectName(QString::fromUtf8("moveItemLeftButton"));

        horizontalLayout_4->addWidget(moveItemLeftButton);

        moveItemRightButton = new QToolButton(itemsBox);
        moveItemRightButton->setObjectName(QString::fromUtf8("moveItemRightButton"));

        horizontalLayout_4->addWidget(moveItemRightButton);

        moveItemUpButton = new QToolButton(itemsBox);
        moveItemUpButton->setObjectName(QString::fromUtf8("moveItemUpButton"));

        horizontalLayout_4->addWidget(moveItemUpButton);

        moveItemDownButton = new QToolButton(itemsBox);
        moveItemDownButton->setObjectName(QString::fromUtf8("moveItemDownButton"));

        horizontalLayout_4->addWidget(moveItemDownButton);


        gridLayout->addLayout(horizontalLayout_4, 1, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_2 = new QLabel(itemsBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_2->addWidget(label_2);

        itemIconSelector = new qdesigner_internal::IconSelector(itemsBox);
        itemIconSelector->setObjectName(QString::fromUtf8("itemIconSelector"));

        horizontalLayout_2->addWidget(itemIconSelector);

        horizontalSpacer = new QSpacerItem(288, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout_2, 2, 0, 1, 1);


        gridLayout_3->addWidget(itemsBox, 0, 0, 1, 1);

        columnsBox = new QGroupBox(qdesigner_internal__TreeWidgetEditor);
        columnsBox->setObjectName(QString::fromUtf8("columnsBox"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(columnsBox->sizePolicy().hasHeightForWidth());
        columnsBox->setSizePolicy(sizePolicy);
        gridLayout_2 = new QGridLayout(columnsBox);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        listWidget = new QListWidget(columnsBox);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(listWidget->sizePolicy().hasHeightForWidth());
        listWidget->setSizePolicy(sizePolicy1);
        listWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout_2->addWidget(listWidget, 0, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        newColumnButton = new QToolButton(columnsBox);
        newColumnButton->setObjectName(QString::fromUtf8("newColumnButton"));

        horizontalLayout_3->addWidget(newColumnButton);

        deleteColumnButton = new QToolButton(columnsBox);
        deleteColumnButton->setObjectName(QString::fromUtf8("deleteColumnButton"));

        horizontalLayout_3->addWidget(deleteColumnButton);

        spacerItem1 = new QSpacerItem(13, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(spacerItem1);

        moveColumnUpButton = new QToolButton(columnsBox);
        moveColumnUpButton->setObjectName(QString::fromUtf8("moveColumnUpButton"));

        horizontalLayout_3->addWidget(moveColumnUpButton);

        moveColumnDownButton = new QToolButton(columnsBox);
        moveColumnDownButton->setObjectName(QString::fromUtf8("moveColumnDownButton"));

        horizontalLayout_3->addWidget(moveColumnDownButton);


        gridLayout_2->addLayout(horizontalLayout_3, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(columnsBox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        columnIconSelector = new qdesigner_internal::IconSelector(columnsBox);
        columnIconSelector->setObjectName(QString::fromUtf8("columnIconSelector"));

        horizontalLayout->addWidget(columnIconSelector);

        spacerItem2 = new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacerItem2);


        gridLayout_2->addLayout(horizontalLayout, 2, 0, 1, 1);


        gridLayout_3->addWidget(columnsBox, 0, 1, 1, 1);

        buttonBox = new QDialogButtonBox(qdesigner_internal__TreeWidgetEditor);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout_3->addWidget(buttonBox, 1, 0, 1, 2);

        QWidget::setTabOrder(treeWidget, newItemButton);
        QWidget::setTabOrder(newItemButton, newSubItemButton);
        QWidget::setTabOrder(newSubItemButton, deleteItemButton);
        QWidget::setTabOrder(deleteItemButton, moveItemLeftButton);
        QWidget::setTabOrder(moveItemLeftButton, moveItemRightButton);
        QWidget::setTabOrder(moveItemRightButton, moveItemUpButton);
        QWidget::setTabOrder(moveItemUpButton, moveItemDownButton);
        QWidget::setTabOrder(moveItemDownButton, listWidget);
        QWidget::setTabOrder(listWidget, newColumnButton);
        QWidget::setTabOrder(newColumnButton, deleteColumnButton);
        QWidget::setTabOrder(deleteColumnButton, moveColumnUpButton);
        QWidget::setTabOrder(moveColumnUpButton, moveColumnDownButton);

        retranslateUi(qdesigner_internal__TreeWidgetEditor);
        QObject::connect(buttonBox, SIGNAL(accepted()), qdesigner_internal__TreeWidgetEditor, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), qdesigner_internal__TreeWidgetEditor, SLOT(reject()));

        QMetaObject::connectSlotsByName(qdesigner_internal__TreeWidgetEditor);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__TreeWidgetEditor)
    {
        qdesigner_internal__TreeWidgetEditor->setWindowTitle(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Edit Tree Widget", nullptr));
        itemsBox->setTitle(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Items", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "1", nullptr));
#if QT_CONFIG(tooltip)
        treeWidget->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Items", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        newItemButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Item", nullptr));
#endif // QT_CONFIG(tooltip)
        newItemButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "&New", nullptr));
#if QT_CONFIG(tooltip)
        newSubItemButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Subitem", nullptr));
#endif // QT_CONFIG(tooltip)
        newSubItemButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "New &Subitem", nullptr));
#if QT_CONFIG(tooltip)
        deleteItemButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete Item", nullptr));
#endif // QT_CONFIG(tooltip)
        deleteItemButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "&Delete", nullptr));
#if QT_CONFIG(tooltip)
        moveItemLeftButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Left (before Parent Item)", nullptr));
#endif // QT_CONFIG(tooltip)
        moveItemLeftButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "L", nullptr));
#if QT_CONFIG(tooltip)
        moveItemRightButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Right (as a First Subitem of the Next Sibling Item)", nullptr));
#endif // QT_CONFIG(tooltip)
        moveItemRightButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "R", nullptr));
#if QT_CONFIG(tooltip)
        moveItemUpButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Up", nullptr));
#endif // QT_CONFIG(tooltip)
        moveItemUpButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "U", nullptr));
#if QT_CONFIG(tooltip)
        moveItemDownButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Down", nullptr));
#endif // QT_CONFIG(tooltip)
        moveItemDownButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "D", nullptr));
        label_2->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Icon", nullptr));
        columnsBox->setTitle(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Columns", nullptr));
#if QT_CONFIG(tooltip)
        listWidget->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Columns", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        newColumnButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Column", nullptr));
#endif // QT_CONFIG(tooltip)
        newColumnButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "New", nullptr));
#if QT_CONFIG(tooltip)
        deleteColumnButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete Column", nullptr));
#endif // QT_CONFIG(tooltip)
        deleteColumnButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete", nullptr));
#if QT_CONFIG(tooltip)
        moveColumnUpButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Column Up", nullptr));
#endif // QT_CONFIG(tooltip)
        moveColumnUpButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "U", nullptr));
#if QT_CONFIG(tooltip)
        moveColumnDownButton->setToolTip(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Column Down", nullptr));
#endif // QT_CONFIG(tooltip)
        moveColumnDownButton->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "D", nullptr));
        label->setText(QCoreApplication::translate("qdesigner_internal::TreeWidgetEditor", "Icon", nullptr));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class TreeWidgetEditor: public Ui_TreeWidgetEditor {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // TREEWIDGETEDITOR_H
