// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "basica11ywidget.h"

BasicA11yWidget::BasicA11yWidget()  :
                                     m_toolBar (new QToolBar()),
                                     m_layout(new QVBoxLayout),
                                     m_tabWidget(new QTabWidget)
{
    createActions();
    createMenus();
    createToolBar();
    m_lblDateTime =new QLabel("Select Chrono Menu for todays date and time.");
    m_layout->addWidget(m_lblDateTime);
    m_tabWidget->addTab(new GeneralTab(), ("General Widget"));
    m_editView =new EditViewTab();
    m_tabWidget->addTab(m_editView, ("Edit Widget"));
    m_layout->addWidget(m_tabWidget);

    m_layout->addStretch();

    connect(m_editView, &EditViewTab::connectToToolBar, this,&BasicA11yWidget::connectToolBar);
    setLayout(m_layout);

}
void BasicA11yWidget::handleButton() {

    QDialog *asmSmplDlg = new QDialog(this);
    QVBoxLayout *vlayout = new QVBoxLayout(asmSmplDlg);
    asmSmplDlg->setWindowTitle("WebAssembly Dialog box ");
    QLabel *label = new QLabel("Accessibility Demo sample application developed in Qt.");
    QAbstractButton *bExit = new QPushButton("Exit");
    vlayout->addWidget(label);
    vlayout->addWidget(bExit);
    asmSmplDlg->setLayout(vlayout);
    auto p = asmSmplDlg->palette();
    p.setColor( asmSmplDlg->backgroundRole(), Qt::gray);
    asmSmplDlg->setPalette(p);
    asmSmplDlg->show();
    asmSmplDlg->connect(bExit, SIGNAL(clicked()), asmSmplDlg, SLOT(close()));
}

void BasicA11yWidget::createToolBar()
{
    m_copyAct = new QAction(tr("&Copy"), this);
    m_copyAct->setShortcuts(QKeySequence::Copy);

    m_pasteAct = new QAction(tr("&Paste"), this);
    m_pasteAct->setStatusTip(tr("To paste selected text"));
    m_pasteAct->setShortcuts(QKeySequence::Paste);

    m_cutAct = new QAction(tr("C&ut"), this);
    m_cutAct->setShortcuts(QKeySequence::Cut);

    m_toolBar->addAction(m_copyAct);
    m_toolBar->addAction(m_cutAct);
    m_toolBar->addAction(m_pasteAct);
    m_layout->addWidget(m_toolBar);

}
void BasicA11yWidget::connectToolBar()
{
  connect(m_copyAct, &QAction::triggered, m_editView->getTextEdit(), &QPlainTextEdit::copy);
  connect(m_pasteAct, &QAction::triggered, m_editView->getTextEdit(), &QPlainTextEdit::paste);
  connect(m_cutAct, &QAction::triggered, m_editView->getTextEdit(), &QPlainTextEdit::cut);
}
void BasicA11yWidget::createActions()
{
    m_DateAct = new QAction( tr("&Date"), this);
    m_DateAct->setStatusTip(tr("To tell you todays date."));
    connect(m_DateAct, &QAction::triggered, this, &BasicA11yWidget::todaysDate);

    m_TimeAct = new QAction(tr("&Time"), this);
    m_TimeAct->setStatusTip(tr("To tell you current time."));
    connect(m_TimeAct, &QAction::triggered, this, &BasicA11yWidget::currentTime);

}
void BasicA11yWidget::createMenus()
{
    m_menuBar = new QMenuBar();

    m_TodayMenu = m_menuBar->addMenu(tr("&Chrono"));
    m_TodayMenu->addAction(m_DateAct);
    m_TodayMenu->addAction(m_TimeAct);

    m_aboutAct = new QAction(tr("&About"), this);
    m_aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAct, &QAction::triggered, this, &BasicA11yWidget::about);

    m_helpMenu = m_menuBar->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAct);

    m_layout->setMenuBar(m_menuBar);
}

void BasicA11yWidget::todaysDate()
{
    QDateTime dt=QDateTime::currentDateTime();
    QString str = "Today's Date:"+ dt.date().toString();
    m_lblDateTime->setText(str);
}

void BasicA11yWidget::currentTime()
{
    QDateTime dt=QDateTime::currentDateTime();
    QString str = "Current Time:"+ dt.time().toString();
    m_lblDateTime->setText(str);
}

void BasicA11yWidget::about()
{
    handleButton();
}
