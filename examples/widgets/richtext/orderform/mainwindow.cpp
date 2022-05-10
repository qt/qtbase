// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrinter>
#include <QPrintDialog>
#endif
#endif

#include "detailsdialog.h"
#include "mainwindow.h"

//! [0]
MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"), this);
    QAction *newAction = fileMenu->addAction(tr("&New..."));
    newAction->setShortcuts(QKeySequence::New);
    printAction = fileMenu->addAction(tr("&Print..."), this, &MainWindow::printFile);
    printAction->setShortcuts(QKeySequence::Print);
    printAction->setEnabled(false);
    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcuts(QKeySequence::Quit);
    menuBar()->addMenu(fileMenu);

    letters = new QTabWidget;

    connect(newAction, &QAction::triggered, this, &MainWindow::openDialog);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);

    setCentralWidget(letters);
    setWindowTitle(tr("Order Form"));
}
//! [0]

//! [1]
void MainWindow::createLetter(const QString &name, const QString &address,
                              QList<QPair<QString,int> > orderItems,
                              bool sendOffers)
{
    QTextEdit *editor = new QTextEdit;
    int tabIndex = letters->addTab(editor, name);
    letters->setCurrentIndex(tabIndex);
//! [1]

//! [2]
    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start);
//! [2] //! [3]
    QTextFrame *topFrame = cursor.currentFrame();
    QTextFrameFormat topFrameFormat = topFrame->frameFormat();
    topFrameFormat.setPadding(16);
    topFrame->setFrameFormat(topFrameFormat);

    QTextCharFormat textFormat;
    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);

    QTextFrameFormat referenceFrameFormat;
    referenceFrameFormat.setBorder(1);
    referenceFrameFormat.setPadding(8);
    referenceFrameFormat.setPosition(QTextFrameFormat::FloatRight);
    referenceFrameFormat.setWidth(QTextLength(QTextLength::PercentageLength, 40));
    cursor.insertFrame(referenceFrameFormat);

    cursor.insertText("A company", boldFormat);
    cursor.insertBlock();
    cursor.insertText("321 City Street");
    cursor.insertBlock();
    cursor.insertText("Industry Park");
    cursor.insertBlock();
    cursor.insertText("Another country");
//! [3]

//! [4]
    cursor.setPosition(topFrame->lastPosition());

    cursor.insertText(name, textFormat);
    const QStringList lines = address.split('\n');
    for (const QString &line : lines) {
        cursor.insertBlock();
        cursor.insertText(line);
    }
//! [4] //! [5]
    cursor.insertBlock();
    cursor.insertBlock();

    QDate date = QDate::currentDate();
    cursor.insertText(tr("Date: %1").arg(date.toString("d MMMM yyyy")),
                      textFormat);
    cursor.insertBlock();

    QTextFrameFormat bodyFrameFormat;
    bodyFrameFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    cursor.insertFrame(bodyFrameFormat);
//! [5]

//! [6]
    cursor.insertText(tr("I would like to place an order for the following "
                         "items:"), textFormat);
    cursor.insertBlock();
//! [6] //! [7]
    cursor.insertBlock();
//! [7]

//! [8]
    QTextTableFormat orderTableFormat;
    orderTableFormat.setAlignment(Qt::AlignHCenter);
    QTextTable *orderTable = cursor.insertTable(1, 2, orderTableFormat);

    QTextFrameFormat orderFrameFormat = cursor.currentFrame()->frameFormat();
    orderFrameFormat.setBorder(1);
    cursor.currentFrame()->setFrameFormat(orderFrameFormat);
//! [8]

//! [9]
    cursor = orderTable->cellAt(0, 0).firstCursorPosition();
    cursor.insertText(tr("Product"), boldFormat);
    cursor = orderTable->cellAt(0, 1).firstCursorPosition();
    cursor.insertText(tr("Quantity"), boldFormat);
//! [9]

//! [10]
    for (int i = 0; i < orderItems.count(); ++i) {
        QPair<QString,int> item = orderItems[i];
        int row = orderTable->rows();

        orderTable->insertRows(row, 1);
        cursor = orderTable->cellAt(row, 0).firstCursorPosition();
        cursor.insertText(item.first, textFormat);
        cursor = orderTable->cellAt(row, 1).firstCursorPosition();
        cursor.insertText(QString("%1").arg(item.second), textFormat);
    }
//! [10]

//! [11]
    cursor.setPosition(topFrame->lastPosition());

    cursor.insertBlock();
//! [11] //! [12]
    cursor.insertText(tr("Please update my records to take account of the "
                         "following privacy information:"));
    cursor.insertBlock();
//! [12]

//! [13]
    QTextTable *offersTable = cursor.insertTable(2, 2);

    cursor = offersTable->cellAt(0, 1).firstCursorPosition();
    cursor.insertText(tr("I want to receive more information about your "
                         "company's products and special offers."), textFormat);
    cursor = offersTable->cellAt(1, 1).firstCursorPosition();
    cursor.insertText(tr("I do not want to receive any promotional information "
                         "from your company."), textFormat);

    if (sendOffers)
        cursor = offersTable->cellAt(0, 0).firstCursorPosition();
    else
        cursor = offersTable->cellAt(1, 0).firstCursorPosition();

    cursor.insertText("X", boldFormat);
//! [13]

//! [14]
    cursor.setPosition(topFrame->lastPosition());
    cursor.insertBlock();
    cursor.insertText(tr("Sincerely,"), textFormat);
    cursor.insertBlock();
    cursor.insertBlock();
    cursor.insertBlock();
    cursor.insertText(name);

    printAction->setEnabled(true);
}
//! [14]

//! [15]
void MainWindow::createSample()
{
    DetailsDialog dialog("Dialog with default values", this);
    createLetter("Mr. Smith", "12 High Street\nSmall Town\nThis country",
                 dialog.orderItems(), true);
}
//! [15]

//! [16]
void MainWindow::openDialog()
{
    DetailsDialog dialog(tr("Enter Customer Details"), this);

    if (dialog.exec() == QDialog::Accepted) {
        createLetter(dialog.senderName(), dialog.senderAddress(),
                     dialog.orderItems(), dialog.sendOffers());
    }
}
//! [16]

//! [17]
void MainWindow::printFile()
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
    QTextEdit *editor = static_cast<QTextEdit*>(letters->currentWidget());
//! [18]
    QPrinter printer;

    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Document"));
    if (editor->textCursor().hasSelection())
        dialog.setOption(QAbstractPrintDialog::PrintSelection);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
//! [18]

    editor->print(&printer);
#endif
}
//! [17]
