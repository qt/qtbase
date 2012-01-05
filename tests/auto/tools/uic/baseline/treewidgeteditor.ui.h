/*
*********************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'treewidgeteditor.ui'
**
** Created: Fri Sep 4 10:17:15 2009
**      by: Qt User Interface Compiler version 4.6.0
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
        qdesigner_internal__TreeWidgetEditor->setWindowTitle(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Edit Tree Widget", 0, QApplication::UnicodeUTF8));
        itemsBox->setTitle(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Items", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("qdesigner_internal::TreeWidgetEditor", "1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        treeWidget->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Items", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newItemButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Item", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        newItemButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "&New", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        newSubItemButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Subitem", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        newSubItemButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New &Subitem", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        deleteItemButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete Item", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        deleteItemButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "&Delete", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveItemLeftButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Left (before Parent Item)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveItemLeftButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "L", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveItemRightButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Right (as a First Subitem of the Next Sibling Item)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveItemRightButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "R", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveItemUpButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Up", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveItemUpButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "U", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveItemDownButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Item Down", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveItemDownButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "D", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Icon", 0, QApplication::UnicodeUTF8));
        columnsBox->setTitle(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Columns", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        listWidget->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Tree Columns", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New Column", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        newColumnButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "New", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        deleteColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete Column", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        deleteColumnButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Delete", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveColumnUpButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Column Up", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveColumnUpButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "U", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveColumnDownButton->setToolTip(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Move Column Down", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveColumnDownButton->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "D", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("qdesigner_internal::TreeWidgetEditor", "Icon", 0, QApplication::UnicodeUTF8));
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
