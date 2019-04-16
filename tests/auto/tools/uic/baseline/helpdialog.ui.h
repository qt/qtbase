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
** Form generated from reading UI file 'helpdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
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
            HelpDialog->setObjectName(QString::fromUtf8("HelpDialog"));
        HelpDialog->resize(274, 417);
        vboxLayout = new QVBoxLayout(HelpDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        tabWidget = new QTabWidget(HelpDialog);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        contentPage = new QWidget();
        contentPage->setObjectName(QString::fromUtf8("contentPage"));
        vboxLayout1 = new QVBoxLayout(contentPage);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
        vboxLayout1->setContentsMargins(5, 5, 5, 5);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        listContents = new QTreeWidget(contentPage);
        listContents->setObjectName(QString::fromUtf8("listContents"));
        listContents->setContextMenuPolicy(Qt::CustomContextMenu);
        listContents->setRootIsDecorated(true);
        listContents->setUniformRowHeights(true);

        vboxLayout1->addWidget(listContents);

        QIcon icon(QIcon::fromTheme(QString::fromUtf8("edit-copy")));
        tabWidget->addTab(contentPage, icon, QString());
        indexPage = new QWidget();
        indexPage->setObjectName(QString::fromUtf8("indexPage"));
        vboxLayout2 = new QVBoxLayout(indexPage);
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(5, 5, 5, 5);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        TextLabel1 = new QLabel(indexPage);
        TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));

        vboxLayout2->addWidget(TextLabel1);

        editIndex = new QLineEdit(indexPage);
        editIndex->setObjectName(QString::fromUtf8("editIndex"));

        vboxLayout2->addWidget(editIndex);

        listIndex = new QListView(indexPage);
        listIndex->setObjectName(QString::fromUtf8("listIndex"));
        listIndex->setContextMenuPolicy(Qt::CustomContextMenu);

        vboxLayout2->addWidget(listIndex);

        tabWidget->addTab(indexPage, QString());
        bookmarkPage = new QWidget();
        bookmarkPage->setObjectName(QString::fromUtf8("bookmarkPage"));
        vboxLayout3 = new QVBoxLayout(bookmarkPage);
#ifndef Q_OS_MAC
        vboxLayout3->setSpacing(6);
#endif
        vboxLayout3->setContentsMargins(5, 5, 5, 5);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        listBookmarks = new QTreeWidget(bookmarkPage);
        listBookmarks->setObjectName(QString::fromUtf8("listBookmarks"));
        listBookmarks->setContextMenuPolicy(Qt::CustomContextMenu);
        listBookmarks->setUniformRowHeights(true);

        vboxLayout3->addWidget(listBookmarks);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        buttonAdd = new QPushButton(bookmarkPage);
        buttonAdd->setObjectName(QString::fromUtf8("buttonAdd"));

        hboxLayout->addWidget(buttonAdd);

        buttonRemove = new QPushButton(bookmarkPage);
        buttonRemove->setObjectName(QString::fromUtf8("buttonRemove"));

        hboxLayout->addWidget(buttonRemove);


        vboxLayout3->addLayout(hboxLayout);

        tabWidget->addTab(bookmarkPage, QString());
        searchPage = new QWidget();
        searchPage->setObjectName(QString::fromUtf8("searchPage"));
        gridLayout = new QGridLayout(searchPage);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        spacerItem1 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem1, 3, 0, 1, 1);

        TextLabel1_2 = new QLabel(searchPage);
        TextLabel1_2->setObjectName(QString::fromUtf8("TextLabel1_2"));

        gridLayout->addWidget(TextLabel1_2, 0, 0, 1, 1);

        termsEdit = new QLineEdit(searchPage);
        termsEdit->setObjectName(QString::fromUtf8("termsEdit"));

        gridLayout->addWidget(termsEdit, 1, 0, 1, 1);

        resultBox = new QListWidget(searchPage);
        resultBox->setObjectName(QString::fromUtf8("resultBox"));
        resultBox->setContextMenuPolicy(Qt::CustomContextMenu);

        gridLayout->addWidget(resultBox, 5, 0, 1, 1);

        TextLabel2 = new QLabel(searchPage);
        TextLabel2->setObjectName(QString::fromUtf8("TextLabel2"));

        gridLayout->addWidget(TextLabel2, 4, 0, 1, 1);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(1, 1, 1, 1);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        helpButton = new QPushButton(searchPage);
        helpButton->setObjectName(QString::fromUtf8("helpButton"));

        hboxLayout1->addWidget(helpButton);

        spacerItem2 = new QSpacerItem(61, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem2);

        searchButton = new QPushButton(searchPage);
        searchButton->setObjectName(QString::fromUtf8("searchButton"));
        searchButton->setEnabled(false);

        hboxLayout1->addWidget(searchButton);


        gridLayout->addLayout(hboxLayout1, 2, 0, 1, 1);

        tabWidget->addTab(searchPage, QString());

        vboxLayout->addWidget(tabWidget);

        framePrepare = new QFrame(HelpDialog);
        framePrepare->setObjectName(QString::fromUtf8("framePrepare"));
        framePrepare->setFrameShape(QFrame::StyledPanel);
        framePrepare->setFrameShadow(QFrame::Raised);
        hboxLayout2 = new QHBoxLayout(framePrepare);
#ifndef Q_OS_MAC
        hboxLayout2->setSpacing(6);
#endif
        hboxLayout2->setContentsMargins(3, 3, 3, 3);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        labelPrepare = new QLabel(framePrepare);
        labelPrepare->setObjectName(QString::fromUtf8("labelPrepare"));

        hboxLayout2->addWidget(labelPrepare);

        progressPrepare = new QProgressBar(framePrepare);
        progressPrepare->setObjectName(QString::fromUtf8("progressPrepare"));

        hboxLayout2->addWidget(progressPrepare);


        vboxLayout->addWidget(framePrepare);

#if QT_CONFIG(shortcut)
        TextLabel1->setBuddy(editIndex);
        TextLabel1_2->setBuddy(termsEdit);
        TextLabel2->setBuddy(resultBox);
#endif // QT_CONFIG(shortcut)
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
        HelpDialog->setWindowTitle(QCoreApplication::translate("HelpDialog", "Help", nullptr));
#if QT_CONFIG(whatsthis)
        HelpDialog->setWhatsThis(QCoreApplication::translate("HelpDialog", "<b>Help</b><p>Choose the topic you want help on from the contents list, or search the index for keywords.</p>", nullptr));
#endif // QT_CONFIG(whatsthis)
#if QT_CONFIG(whatsthis)
        tabWidget->setWhatsThis(QCoreApplication::translate("HelpDialog", "Displays help topics organized by category, index or bookmarks. Another tab inherits the full text search.", nullptr));
#endif // QT_CONFIG(whatsthis)
        QTreeWidgetItem *___qtreewidgetitem = listContents->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("HelpDialog", "column 1", nullptr));
#if QT_CONFIG(whatsthis)
        listContents->setWhatsThis(QCoreApplication::translate("HelpDialog", "<b>Help topics organized by category.</b><p>Double-click an item to see the topics in that category. To view a topic, just double-click it.</p>", nullptr));
#endif // QT_CONFIG(whatsthis)
        tabWidget->setTabText(tabWidget->indexOf(contentPage), QCoreApplication::translate("HelpDialog", "Con&tents", nullptr));
        TextLabel1->setText(QCoreApplication::translate("HelpDialog", "&Look For:", nullptr));
#if QT_CONFIG(tooltip)
        editIndex->setToolTip(QCoreApplication::translate("HelpDialog", "Enter keyword", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        editIndex->setWhatsThis(QCoreApplication::translate("HelpDialog", "<b>Enter a keyword.</b><p>The list will select an item that matches the entered string best.</p>", nullptr));
#endif // QT_CONFIG(whatsthis)
#if QT_CONFIG(whatsthis)
        listIndex->setWhatsThis(QCoreApplication::translate("HelpDialog", "<b>List of available help topics.</b><p>Double-click on an item to open its help page. If more than one is found, you must specify which page you want.</p>", nullptr));
#endif // QT_CONFIG(whatsthis)
        tabWidget->setTabText(tabWidget->indexOf(indexPage), QCoreApplication::translate("HelpDialog", "&Index", nullptr));
        QTreeWidgetItem *___qtreewidgetitem1 = listBookmarks->headerItem();
        ___qtreewidgetitem1->setText(0, QCoreApplication::translate("HelpDialog", "column 1", nullptr));
#if QT_CONFIG(whatsthis)
        listBookmarks->setWhatsThis(QCoreApplication::translate("HelpDialog", "Displays the list of bookmarks.", nullptr));
#endif // QT_CONFIG(whatsthis)
#if QT_CONFIG(tooltip)
        buttonAdd->setToolTip(QCoreApplication::translate("HelpDialog", "Add new bookmark", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        buttonAdd->setWhatsThis(QCoreApplication::translate("HelpDialog", "Add the currently displayed page as a new bookmark.", nullptr));
#endif // QT_CONFIG(whatsthis)
        buttonAdd->setText(QCoreApplication::translate("HelpDialog", "&New", nullptr));
#if QT_CONFIG(tooltip)
        buttonRemove->setToolTip(QCoreApplication::translate("HelpDialog", "Delete bookmark", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        buttonRemove->setWhatsThis(QCoreApplication::translate("HelpDialog", "Delete the selected bookmark.", nullptr));
#endif // QT_CONFIG(whatsthis)
        buttonRemove->setText(QCoreApplication::translate("HelpDialog", "&Delete", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(bookmarkPage), QCoreApplication::translate("HelpDialog", "&Bookmarks", nullptr));
        TextLabel1_2->setText(QCoreApplication::translate("HelpDialog", "Searching f&or:", nullptr));
#if QT_CONFIG(tooltip)
        termsEdit->setToolTip(QCoreApplication::translate("HelpDialog", "Enter searchword(s).", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        termsEdit->setWhatsThis(QCoreApplication::translate("HelpDialog", "<b>Enter search word(s).</b><p>Enter here the word(s) you are looking for. The words may contain wildcards (*). For a sequence of words quote them.</p>", nullptr));
#endif // QT_CONFIG(whatsthis)
#if QT_CONFIG(whatsthis)
        resultBox->setWhatsThis(QCoreApplication::translate("HelpDialog", "<b>Found documents</b><p>This list contains all found documents from the last search. The documents are ordered, i.e. the first document has the most matches.</p>", nullptr));
#endif // QT_CONFIG(whatsthis)
        TextLabel2->setText(QCoreApplication::translate("HelpDialog", "Found &Documents:", nullptr));
#if QT_CONFIG(tooltip)
        helpButton->setToolTip(QCoreApplication::translate("HelpDialog", "Display the help page.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        helpButton->setWhatsThis(QCoreApplication::translate("HelpDialog", "Display the help page for the full text search.", nullptr));
#endif // QT_CONFIG(whatsthis)
        helpButton->setText(QCoreApplication::translate("HelpDialog", "He&lp", nullptr));
#if QT_CONFIG(tooltip)
        searchButton->setToolTip(QCoreApplication::translate("HelpDialog", "Start searching.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        searchButton->setWhatsThis(QCoreApplication::translate("HelpDialog", "Pressing this button starts the search.", nullptr));
#endif // QT_CONFIG(whatsthis)
        searchButton->setText(QCoreApplication::translate("HelpDialog", "&Search", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(searchPage), QCoreApplication::translate("HelpDialog", "&Search", nullptr));
        labelPrepare->setText(QCoreApplication::translate("HelpDialog", "Preparing...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HelpDialog: public Ui_HelpDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // HELPDIALOG_H
