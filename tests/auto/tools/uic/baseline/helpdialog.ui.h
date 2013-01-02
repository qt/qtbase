/*
*********************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'helpdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_HelpDialog
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *tabWidget;
    QWidget *contentPage;
    QVBoxLayout *vboxLayout1;
    QTreeWidget *listContents;
    QWidget *indexPage;
    QVBoxLayout *vboxLayout2;
    QLabel *TextLabel1;
    QLineEdit *editIndex;
    QListView *listIndex;
    QWidget *bookmarkPage;
    QVBoxLayout *vboxLayout3;
    QTreeWidget *listBookmarks;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QPushButton *buttonAdd;
    QPushButton *buttonRemove;
    QWidget *searchPage;
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem1;
    QLabel *TextLabel1_2;
    QLineEdit *termsEdit;
    QListWidget *resultBox;
    QLabel *TextLabel2;
    QHBoxLayout *hboxLayout1;
    QPushButton *helpButton;
    QSpacerItem *spacerItem2;
    QPushButton *searchButton;
    QFrame *framePrepare;
    QHBoxLayout *hboxLayout2;
    QLabel *labelPrepare;
    QProgressBar *progressPrepare;

    void setupUi(QWidget *HelpDialog)
    {
        if (HelpDialog->objectName().isEmpty())
            HelpDialog->setObjectName(QStringLiteral("HelpDialog"));
        HelpDialog->resize(274, 417);
        vboxLayout = new QVBoxLayout(HelpDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        tabWidget = new QTabWidget(HelpDialog);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        contentPage = new QWidget();
        contentPage->setObjectName(QStringLiteral("contentPage"));
        vboxLayout1 = new QVBoxLayout(contentPage);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
        vboxLayout1->setContentsMargins(5, 5, 5, 5);
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        listContents = new QTreeWidget(contentPage);
        listContents->setObjectName(QStringLiteral("listContents"));
        listContents->setContextMenuPolicy(Qt::CustomContextMenu);
        listContents->setRootIsDecorated(true);
        listContents->setUniformRowHeights(true);

        vboxLayout1->addWidget(listContents);

        tabWidget->addTab(contentPage, QString());
        indexPage = new QWidget();
        indexPage->setObjectName(QStringLiteral("indexPage"));
        vboxLayout2 = new QVBoxLayout(indexPage);
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(5, 5, 5, 5);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        TextLabel1 = new QLabel(indexPage);
        TextLabel1->setObjectName(QStringLiteral("TextLabel1"));

        vboxLayout2->addWidget(TextLabel1);

        editIndex = new QLineEdit(indexPage);
        editIndex->setObjectName(QStringLiteral("editIndex"));

        vboxLayout2->addWidget(editIndex);

        listIndex = new QListView(indexPage);
        listIndex->setObjectName(QStringLiteral("listIndex"));
        listIndex->setContextMenuPolicy(Qt::CustomContextMenu);

        vboxLayout2->addWidget(listIndex);

        tabWidget->addTab(indexPage, QString());
        bookmarkPage = new QWidget();
        bookmarkPage->setObjectName(QStringLiteral("bookmarkPage"));
        vboxLayout3 = new QVBoxLayout(bookmarkPage);
#ifndef Q_OS_MAC
        vboxLayout3->setSpacing(6);
#endif
        vboxLayout3->setContentsMargins(5, 5, 5, 5);
        vboxLayout3->setObjectName(QStringLiteral("vboxLayout3"));
        listBookmarks = new QTreeWidget(bookmarkPage);
        listBookmarks->setObjectName(QStringLiteral("listBookmarks"));
        listBookmarks->setContextMenuPolicy(Qt::CustomContextMenu);
        listBookmarks->setUniformRowHeights(true);

        vboxLayout3->addWidget(listBookmarks);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        buttonAdd = new QPushButton(bookmarkPage);
        buttonAdd->setObjectName(QStringLiteral("buttonAdd"));

        hboxLayout->addWidget(buttonAdd);

        buttonRemove = new QPushButton(bookmarkPage);
        buttonRemove->setObjectName(QStringLiteral("buttonRemove"));

        hboxLayout->addWidget(buttonRemove);


        vboxLayout3->addLayout(hboxLayout);

        tabWidget->addTab(bookmarkPage, QString());
        searchPage = new QWidget();
        searchPage->setObjectName(QStringLiteral("searchPage"));
        gridLayout = new QGridLayout(searchPage);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        spacerItem1 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem1, 3, 0, 1, 1);

        TextLabel1_2 = new QLabel(searchPage);
        TextLabel1_2->setObjectName(QStringLiteral("TextLabel1_2"));

        gridLayout->addWidget(TextLabel1_2, 0, 0, 1, 1);

        termsEdit = new QLineEdit(searchPage);
        termsEdit->setObjectName(QStringLiteral("termsEdit"));

        gridLayout->addWidget(termsEdit, 1, 0, 1, 1);

        resultBox = new QListWidget(searchPage);
        resultBox->setObjectName(QStringLiteral("resultBox"));
        resultBox->setContextMenuPolicy(Qt::CustomContextMenu);

        gridLayout->addWidget(resultBox, 5, 0, 1, 1);

        TextLabel2 = new QLabel(searchPage);
        TextLabel2->setObjectName(QStringLiteral("TextLabel2"));

        gridLayout->addWidget(TextLabel2, 4, 0, 1, 1);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(1, 1, 1, 1);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        helpButton = new QPushButton(searchPage);
        helpButton->setObjectName(QStringLiteral("helpButton"));

        hboxLayout1->addWidget(helpButton);

        spacerItem2 = new QSpacerItem(61, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem2);

        searchButton = new QPushButton(searchPage);
        searchButton->setObjectName(QStringLiteral("searchButton"));
        searchButton->setEnabled(false);

        hboxLayout1->addWidget(searchButton);


        gridLayout->addLayout(hboxLayout1, 2, 0, 1, 1);

        tabWidget->addTab(searchPage, QString());

        vboxLayout->addWidget(tabWidget);

        framePrepare = new QFrame(HelpDialog);
        framePrepare->setObjectName(QStringLiteral("framePrepare"));
        framePrepare->setFrameShape(QFrame::StyledPanel);
        framePrepare->setFrameShadow(QFrame::Raised);
        hboxLayout2 = new QHBoxLayout(framePrepare);
#ifndef Q_OS_MAC
        hboxLayout2->setSpacing(6);
#endif
        hboxLayout2->setContentsMargins(3, 3, 3, 3);
        hboxLayout2->setObjectName(QStringLiteral("hboxLayout2"));
        labelPrepare = new QLabel(framePrepare);
        labelPrepare->setObjectName(QStringLiteral("labelPrepare"));

        hboxLayout2->addWidget(labelPrepare);

        progressPrepare = new QProgressBar(framePrepare);
        progressPrepare->setObjectName(QStringLiteral("progressPrepare"));

        hboxLayout2->addWidget(progressPrepare);


        vboxLayout->addWidget(framePrepare);

#ifndef QT_NO_SHORTCUT
        TextLabel1->setBuddy(editIndex);
        TextLabel1_2->setBuddy(termsEdit);
        TextLabel2->setBuddy(resultBox);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(tabWidget, listContents);
        QWidget::setTabOrder(listContents, editIndex);
        QWidget::setTabOrder(editIndex, listIndex);
        QWidget::setTabOrder(listIndex, listBookmarks);
        QWidget::setTabOrder(listBookmarks, buttonAdd);
        QWidget::setTabOrder(buttonAdd, buttonRemove);
        QWidget::setTabOrder(buttonRemove, termsEdit);
        QWidget::setTabOrder(termsEdit, searchButton);
        QWidget::setTabOrder(searchButton, helpButton);
        QWidget::setTabOrder(helpButton, resultBox);

        retranslateUi(HelpDialog);

        QMetaObject::connectSlotsByName(HelpDialog);
    } // setupUi

    void retranslateUi(QWidget *HelpDialog)
    {
        HelpDialog->setWindowTitle(QApplication::translate("HelpDialog", "Help", 0));
#ifndef QT_NO_WHATSTHIS
        HelpDialog->setWhatsThis(QApplication::translate("HelpDialog", "<b>Help</b><p>Choose the topic you want help on from the contents list, or search the index for keywords.</p>", 0));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        tabWidget->setWhatsThis(QApplication::translate("HelpDialog", "Displays help topics organized by category, index or bookmarks. Another tab inherits the full text search.", 0));
#endif // QT_NO_WHATSTHIS
        QTreeWidgetItem *___qtreewidgetitem = listContents->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("HelpDialog", "column 1", 0));
#ifndef QT_NO_WHATSTHIS
        listContents->setWhatsThis(QApplication::translate("HelpDialog", "<b>Help topics organized by category.</b><p>Double-click an item to see the topics in that category. To view a topic, just double-click it.</p>", 0));
#endif // QT_NO_WHATSTHIS
        tabWidget->setTabText(tabWidget->indexOf(contentPage), QApplication::translate("HelpDialog", "Con&tents", 0));
        TextLabel1->setText(QApplication::translate("HelpDialog", "&Look For:", 0));
#ifndef QT_NO_TOOLTIP
        editIndex->setToolTip(QApplication::translate("HelpDialog", "Enter keyword", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        editIndex->setWhatsThis(QApplication::translate("HelpDialog", "<b>Enter a keyword.</b><p>The list will select an item that matches the entered string best.</p>", 0));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        listIndex->setWhatsThis(QApplication::translate("HelpDialog", "<b>List of available help topics.</b><p>Double-click on an item to open its help page. If more than one is found, you must specify which page you want.</p>", 0));
#endif // QT_NO_WHATSTHIS
        tabWidget->setTabText(tabWidget->indexOf(indexPage), QApplication::translate("HelpDialog", "&Index", 0));
        QTreeWidgetItem *___qtreewidgetitem1 = listBookmarks->headerItem();
        ___qtreewidgetitem1->setText(0, QApplication::translate("HelpDialog", "column 1", 0));
#ifndef QT_NO_WHATSTHIS
        listBookmarks->setWhatsThis(QApplication::translate("HelpDialog", "Displays the list of bookmarks.", 0));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        buttonAdd->setToolTip(QApplication::translate("HelpDialog", "Add new bookmark", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        buttonAdd->setWhatsThis(QApplication::translate("HelpDialog", "Add the currently displayed page as a new bookmark.", 0));
#endif // QT_NO_WHATSTHIS
        buttonAdd->setText(QApplication::translate("HelpDialog", "&New", 0));
#ifndef QT_NO_TOOLTIP
        buttonRemove->setToolTip(QApplication::translate("HelpDialog", "Delete bookmark", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        buttonRemove->setWhatsThis(QApplication::translate("HelpDialog", "Delete the selected bookmark.", 0));
#endif // QT_NO_WHATSTHIS
        buttonRemove->setText(QApplication::translate("HelpDialog", "&Delete", 0));
        tabWidget->setTabText(tabWidget->indexOf(bookmarkPage), QApplication::translate("HelpDialog", "&Bookmarks", 0));
        TextLabel1_2->setText(QApplication::translate("HelpDialog", "Searching f&or:", 0));
#ifndef QT_NO_TOOLTIP
        termsEdit->setToolTip(QApplication::translate("HelpDialog", "Enter searchword(s).", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        termsEdit->setWhatsThis(QApplication::translate("HelpDialog", "<b>Enter search word(s).</b><p>Enter here the word(s) you are looking for. The words may contain wildcards (*). For a sequence of words quote them.</p>", 0));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        resultBox->setWhatsThis(QApplication::translate("HelpDialog", "<b>Found documents</b><p>This list contains all found documents from the last search. The documents are ordered, i.e. the first document has the most matches.</p>", 0));
#endif // QT_NO_WHATSTHIS
        TextLabel2->setText(QApplication::translate("HelpDialog", "Found &Documents:", 0));
#ifndef QT_NO_TOOLTIP
        helpButton->setToolTip(QApplication::translate("HelpDialog", "Display the help page.", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        helpButton->setWhatsThis(QApplication::translate("HelpDialog", "Display the help page for the full text search.", 0));
#endif // QT_NO_WHATSTHIS
        helpButton->setText(QApplication::translate("HelpDialog", "He&lp", 0));
#ifndef QT_NO_TOOLTIP
        searchButton->setToolTip(QApplication::translate("HelpDialog", "Start searching.", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        searchButton->setWhatsThis(QApplication::translate("HelpDialog", "Pressing this button starts the search.", 0));
#endif // QT_NO_WHATSTHIS
        searchButton->setText(QApplication::translate("HelpDialog", "&Search", 0));
        tabWidget->setTabText(tabWidget->indexOf(searchPage), QApplication::translate("HelpDialog", "&Search", 0));
        labelPrepare->setText(QApplication::translate("HelpDialog", "Preparing...", 0));
    } // retranslateUi

};

namespace Ui {
    class HelpDialog: public Ui_HelpDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // HELPDIALOG_H
