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
** Form generated from reading UI file 'trpreviewtool.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TRPREVIEWTOOL_H
#define TRPREVIEWTOOL_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QListView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TrPreviewToolClass
{
public:
    QAction *actionOpenForm;
    QAction *actionLoadTranslation;
    QAction *actionReloadTranslations;
    QAction *actionClose;
    QAction *actionAbout;
    QAction *actionAbout_Qt;
    QWidget *centralWidget;
    QMenuBar *menuBar;
    QMenu *menuView;
    QMenu *menuViewViews;
    QMenu *menuHelp;
    QMenu *menuFile;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    QDockWidget *dwForms;
    QWidget *dockWidgetContents;
    QVBoxLayout *vboxLayout;
    QListView *viewForms;

    void setupUi(QMainWindow *TrPreviewToolClass)
    {
        if (TrPreviewToolClass->objectName().isEmpty())
            TrPreviewToolClass->setObjectName(QString::fromUtf8("TrPreviewToolClass"));
        TrPreviewToolClass->resize(593, 466);
        actionOpenForm = new QAction(TrPreviewToolClass);
        actionOpenForm->setObjectName(QString::fromUtf8("actionOpenForm"));
        const QIcon icon = QIcon(QString::fromUtf8(":/images/open_form.png"));
        actionOpenForm->setIcon(icon);
        actionLoadTranslation = new QAction(TrPreviewToolClass);
        actionLoadTranslation->setObjectName(QString::fromUtf8("actionLoadTranslation"));
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/load_translation.png"));
        actionLoadTranslation->setIcon(icon1);
        actionReloadTranslations = new QAction(TrPreviewToolClass);
        actionReloadTranslations->setObjectName(QString::fromUtf8("actionReloadTranslations"));
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/reload_translations.png"));
        actionReloadTranslations->setIcon(icon2);
        actionClose = new QAction(TrPreviewToolClass);
        actionClose->setObjectName(QString::fromUtf8("actionClose"));
        actionAbout = new QAction(TrPreviewToolClass);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        actionAbout_Qt = new QAction(TrPreviewToolClass);
        actionAbout_Qt->setObjectName(QString::fromUtf8("actionAbout_Qt"));
        centralWidget = new QWidget(TrPreviewToolClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        TrPreviewToolClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(TrPreviewToolClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 593, 21));
        menuView = new QMenu(menuBar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        menuViewViews = new QMenu(menuView);
        menuViewViews->setObjectName(QString::fromUtf8("menuViewViews"));
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        TrPreviewToolClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(TrPreviewToolClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        mainToolBar->setOrientation(Qt::Horizontal);
        TrPreviewToolClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(TrPreviewToolClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        TrPreviewToolClass->setStatusBar(statusBar);
        dwForms = new QDockWidget(TrPreviewToolClass);
        dwForms->setObjectName(QString::fromUtf8("dwForms"));
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
        vboxLayout = new QVBoxLayout(dockWidgetContents);
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        viewForms = new QListView(dockWidgetContents);
        viewForms->setObjectName(QString::fromUtf8("viewForms"));
        viewForms->setEditTriggers(QAbstractItemView::NoEditTriggers);
        viewForms->setAlternatingRowColors(true);
        viewForms->setUniformItemSizes(true);

        vboxLayout->addWidget(viewForms);

        dwForms->setWidget(dockWidgetContents);
        TrPreviewToolClass->addDockWidget(Qt::LeftDockWidgetArea, dwForms);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuView->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuView->addAction(menuViewViews->menuAction());
        menuHelp->addAction(actionAbout);
        menuHelp->addAction(actionAbout_Qt);
        menuFile->addAction(actionOpenForm);
        menuFile->addAction(actionLoadTranslation);
        menuFile->addAction(actionReloadTranslations);
        menuFile->addSeparator();
        menuFile->addAction(actionClose);
        mainToolBar->addAction(actionOpenForm);
        mainToolBar->addAction(actionLoadTranslation);
        mainToolBar->addAction(actionReloadTranslations);

        retranslateUi(TrPreviewToolClass);

        QMetaObject::connectSlotsByName(TrPreviewToolClass);
    } // setupUi

    void retranslateUi(QMainWindow *TrPreviewToolClass)
    {
        TrPreviewToolClass->setWindowTitle(QCoreApplication::translate("TrPreviewToolClass", "Qt Translation Preview Tool", nullptr));
        actionOpenForm->setText(QCoreApplication::translate("TrPreviewToolClass", "&Open Form...", nullptr));
        actionLoadTranslation->setText(QCoreApplication::translate("TrPreviewToolClass", "&Load Translation...", nullptr));
        actionReloadTranslations->setText(QCoreApplication::translate("TrPreviewToolClass", "&Reload Translations", nullptr));
#if QT_CONFIG(shortcut)
        actionReloadTranslations->setShortcut(QCoreApplication::translate("TrPreviewToolClass", "F5", nullptr));
#endif // QT_CONFIG(shortcut)
        actionClose->setText(QCoreApplication::translate("TrPreviewToolClass", "&Close", nullptr));
        actionAbout->setText(QCoreApplication::translate("TrPreviewToolClass", "About", nullptr));
        actionAbout_Qt->setText(QCoreApplication::translate("TrPreviewToolClass", "About Qt", nullptr));
        menuView->setTitle(QCoreApplication::translate("TrPreviewToolClass", "&View", nullptr));
        menuViewViews->setTitle(QCoreApplication::translate("TrPreviewToolClass", "&Views", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("TrPreviewToolClass", "&Help", nullptr));
        menuFile->setTitle(QCoreApplication::translate("TrPreviewToolClass", "&File", nullptr));
        dwForms->setWindowTitle(QCoreApplication::translate("TrPreviewToolClass", "Forms", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TrPreviewToolClass: public Ui_TrPreviewToolClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TRPREVIEWTOOL_H
