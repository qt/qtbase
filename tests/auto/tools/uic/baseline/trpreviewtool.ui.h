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
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TRPREVIEWTOOL_H
#define TRPREVIEWTOOL_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHeaderView>
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
            TrPreviewToolClass->setObjectName(QStringLiteral("TrPreviewToolClass"));
        TrPreviewToolClass->resize(593, 466);
        actionOpenForm = new QAction(TrPreviewToolClass);
        actionOpenForm->setObjectName(QStringLiteral("actionOpenForm"));
        const QIcon icon = QIcon(QString::fromUtf8(":/images/open_form.png"));
        actionOpenForm->setIcon(icon);
        actionLoadTranslation = new QAction(TrPreviewToolClass);
        actionLoadTranslation->setObjectName(QStringLiteral("actionLoadTranslation"));
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/load_translation.png"));
        actionLoadTranslation->setIcon(icon1);
        actionReloadTranslations = new QAction(TrPreviewToolClass);
        actionReloadTranslations->setObjectName(QStringLiteral("actionReloadTranslations"));
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/reload_translations.png"));
        actionReloadTranslations->setIcon(icon2);
        actionClose = new QAction(TrPreviewToolClass);
        actionClose->setObjectName(QStringLiteral("actionClose"));
        actionAbout = new QAction(TrPreviewToolClass);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionAbout_Qt = new QAction(TrPreviewToolClass);
        actionAbout_Qt->setObjectName(QStringLiteral("actionAbout_Qt"));
        centralWidget = new QWidget(TrPreviewToolClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        TrPreviewToolClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(TrPreviewToolClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 593, 21));
        menuView = new QMenu(menuBar);
        menuView->setObjectName(QStringLiteral("menuView"));
        menuViewViews = new QMenu(menuView);
        menuViewViews->setObjectName(QStringLiteral("menuViewViews"));
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QStringLiteral("menuHelp"));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        TrPreviewToolClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(TrPreviewToolClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        mainToolBar->setOrientation(Qt::Horizontal);
        TrPreviewToolClass->addToolBar(static_cast<Qt::ToolBarArea>(4), mainToolBar);
        statusBar = new QStatusBar(TrPreviewToolClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        TrPreviewToolClass->setStatusBar(statusBar);
        dwForms = new QDockWidget(TrPreviewToolClass);
        dwForms->setObjectName(QStringLiteral("dwForms"));
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));
        vboxLayout = new QVBoxLayout(dockWidgetContents);
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        viewForms = new QListView(dockWidgetContents);
        viewForms->setObjectName(QStringLiteral("viewForms"));
        viewForms->setEditTriggers(QAbstractItemView::NoEditTriggers);
        viewForms->setAlternatingRowColors(true);
        viewForms->setUniformItemSizes(true);

        vboxLayout->addWidget(viewForms);

        dwForms->setWidget(dockWidgetContents);
        TrPreviewToolClass->addDockWidget(static_cast<Qt::DockWidgetArea>(1), dwForms);

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
        TrPreviewToolClass->setWindowTitle(QApplication::translate("TrPreviewToolClass", "Qt Translation Preview Tool", 0));
        actionOpenForm->setText(QApplication::translate("TrPreviewToolClass", "&Open Form...", 0));
        actionLoadTranslation->setText(QApplication::translate("TrPreviewToolClass", "&Load Translation...", 0));
        actionReloadTranslations->setText(QApplication::translate("TrPreviewToolClass", "&Reload Translations", 0));
        actionReloadTranslations->setShortcut(QApplication::translate("TrPreviewToolClass", "F5", 0));
        actionClose->setText(QApplication::translate("TrPreviewToolClass", "&Close", 0));
        actionAbout->setText(QApplication::translate("TrPreviewToolClass", "About", 0));
        actionAbout_Qt->setText(QApplication::translate("TrPreviewToolClass", "About Qt", 0));
        menuView->setTitle(QApplication::translate("TrPreviewToolClass", "&View", 0));
        menuViewViews->setTitle(QApplication::translate("TrPreviewToolClass", "&Views", 0));
        menuHelp->setTitle(QApplication::translate("TrPreviewToolClass", "&Help", 0));
        menuFile->setTitle(QApplication::translate("TrPreviewToolClass", "&File", 0));
        dwForms->setWindowTitle(QApplication::translate("TrPreviewToolClass", "Forms", 0));
    } // retranslateUi

};

namespace Ui {
    class TrPreviewToolClass: public Ui_TrPreviewToolClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TRPREVIEWTOOL_H
