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
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TREEWIDGETEDITOR_H
#define TREEWIDGETEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
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
            qdesigner_internal__TreeWidgetEditor->setObjectName(QStringLiteral("qdesigner_internal__TreeWidgetEditor"));
        qdesigner_internal__TreeWidgetEditor->resize(619, 321);
        gridLayout_3 = new QGridLayout(qdesigner_internal__TreeWidgetEditor);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        itemsBox = new QGroupBox(qdesigner_internal__TreeWidgetEditor);
        itemsBox->setObjectName(QStringLiteral("itemsBox"));
        gridLayout = new QGridLayout(itemsBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        treeWidget = new QTreeWidget(itemsBox);
        treeWidget->setObjectName(QStringLiteral("treeWidget"));
        treeWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout->addWidget(treeWidget, 0, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        newItemButton = new QToolButton(itemsBox);
        newItemButton->setObjectName(QStringLiteral("newItemButton"));

        horizontalLayout_4->addWidget(newItemButton);

        newSubItemButton = new QToolButton(itemsBox);
        newSubItemButton->setObjectName(QStringLiteral("newSubItemButton"));

        horizontalLayout_4->addWidget(newSubItemButton);

        deleteItemButton = new QToolButton(itemsBox);
        deleteItemButton->setObjectName(QStringLiteral("deleteItemButton"));

        horizontalLayout_4->addWidget(deleteItemButton);

        spacerItem = new QSpacerItem(28, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(spacerItem);

        moveItemLeftButton = new QToolButton(itemsBox);
        moveItemLeftButton->setObjectName(QStringLiteral("moveItemLeftButton"));

        horizontalLayout_4->addWidget(moveItemLeftButton);

        moveItemRightButton = new QToolButton(itemsBox);
        moveItemRightButton->setObjectName(QStringLiteral("moveItemRightButton"));

        horizontalLayout_4->addWidget(moveItemRightButton);

        moveItemUpButton = new QToolButton(itemsBox);
        moveItemUpButton->setObjectName(QStringLiteral("moveItemUpButton"));

        horizontalLayout_4->addWidget(moveItemUpButton);

        moveItemDownButton = new QToolButton(itemsBox);
        moveItemDownButton->setObjectName(QStringLiteral("moveItemDownButton"));

        horizontalLayout_4->addWidget(moveItemDownButton);


        gridLayout->addLayout(horizontalLayout_4, 1, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_2 = new QLabel(itemsBox);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout_2->addWidget(label_2);

        itemIconSelector = new qdesigner_internal::IconSelector(itemsBox);
        itemIconSelector->setObjectName(QStringLiteral("itemIconSelector"));

        horizontalLayout_2->addWidget(itemIconSelector);

        horizontalSpacer = new QSpacerItem(288, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout_2, 2, 0, 1, 1);


        gridLayout_3->addWidget(itemsBox, 0, 0, 1, 1);

        columnsBox = new QGroupBox(qdesigner_internal__TreeWidgetEditor);
        columnsBox->setObjectName(QStringLiteral("columnsBox"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(columnsBox->sizePolicy().hasHeightForWidth());
        columnsBox->setSizePolicy(sizePolicy);
        gridLayout_2 = new QGridLayout(columnsBox);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        listWidget = new QListWidget(columnsBox);
        listWidget->setObjectName(QStringLiteral("listWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(listWidget->sizePolicy().hasHeightForWidth());
        listWidget->setSizePolicy(sizePolicy1);
        listWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout_2->addWidget(listWidget, 0, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        newColumnButton = new QToolButton(columnsBox);
        newColumnButton->setObjectName(QStringLiteral("newColumnButton"));

        horizontalLayout_3->addWidget(newColumnButton);

        deleteColumnButton = new QToolButton(columnsBox);
        deleteColumnButton->setObjectName(QStringLiteral("deleteColumnButton"));

        horizontalLayout_3->addWidget(deleteColumnButton);

        spacerItem1 = new QSpacerItem(13, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(spacerItem1);

        moveColumnUpButton = new QToolButton(columnsBox);
        moveColumnUpButton->setObjectName(QStringLiteral("moveColumnUpButton"));

        horizontalLayout_3->addWidget(moveColumnUpButton);

        moveColumnDownButton = new QToolButton(columnsBox);
        moveColumnDownButton->setObjectName(QStringLiteral("moveColumnDownButton"));

        horizontalLayout_3->addWidget(moveColumnDownButton);


        gridLayout_2->addLayout(horizontalLayout_3, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(columnsBox);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        columnIconSelector = new qdesigner_internal::IconSelector(columnsBox);
        columnIconSelector->setObjectName(QStringLiteral("columnIconSelector"));

        horizontalLayout->addWidget(columnIconSelector);

        spacerItem2 = new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacerItem2);


        gridLayout_2->addLayout(horizontalLayout, 2, 0, 1, 1);


        gridLayout_3->addWidget(columnsBox, 0, 1, 1, 1);

        buttonBox = new QDialogButtonBox(qdesigner_internal__TreeWidgetEditor);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
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
        qdesigner_internal__TreeWidgetEditor->setWindowTitle(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Edit Tree Widget", Q_NULLPTR));
        itemsBox->setTitle(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Items", Q_NULLPTR));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("qdesigner_internal::TreeWidgetEditor", "1", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        treeWidget->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Items", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newItemButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Item", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        newItemButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "&New", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        newSubItemButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Subitem", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        newSubItemButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New &Subitem", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        deleteItemButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete Item", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        deleteItemButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "&Delete", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        moveItemLeftButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Left (before Parent Item)", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        moveItemLeftButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "L", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        moveItemRightButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Right (as a First Subitem of the Next Sibling Item)", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        moveItemRightButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "R", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        moveItemUpButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Up", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        moveItemUpButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "U", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        moveItemDownButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Down", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        moveItemDownButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "D", Q_NULLPTR));
        label_2->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Icon", Q_NULLPTR));
        columnsBox->setTitle(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Columns", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        listWidget->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Columns", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Column", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        newColumnButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        deleteColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete Column", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        deleteColumnButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        moveColumnUpButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Column Up", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        moveColumnUpButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "U", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        moveColumnDownButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Column Down", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        moveColumnDownButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "D", Q_NULLPTR));
        label->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Icon", Q_NULLPTR));
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
