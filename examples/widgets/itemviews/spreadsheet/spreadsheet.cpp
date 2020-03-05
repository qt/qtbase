/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "spreadsheet.h"
#include "spreadsheetdelegate.h"
#include "spreadsheetitem.h"
#include "printview.h"

#include <QtWidgets>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport>
#endif

SpreadSheet::SpreadSheet(int rows, int cols, QWidget *parent)
    : QMainWindow(parent),
      toolBar(new QToolBar(this)),
      cellLabel(new QLabel(toolBar)),
      table(new QTableWidget(rows, cols, this)),
      formulaInput(new QLineEdit(this))
{
    addToolBar(toolBar);

    cellLabel->setMinimumSize(80, 0);

    toolBar->addWidget(cellLabel);
    toolBar->addWidget(formulaInput);

    table->setSizeAdjustPolicy(QTableWidget::AdjustToContents);
    for (int c = 0; c < cols; ++c) {
        QString character(QChar('A' + c));
        table->setHorizontalHeaderItem(c, new QTableWidgetItem(character));
    }

    table->setItemPrototype(table->item(rows - 1, cols - 1));
    table->setItemDelegate(new SpreadSheetDelegate());

    createActions();
    updateColor(nullptr);
    setupMenuBar();
    setupContents();
    setupContextMenu();
    setCentralWidget(table);

    statusBar();
    connect(table, &QTableWidget::currentItemChanged,
            this, &SpreadSheet::updateStatus);
    connect(table, &QTableWidget::currentItemChanged,
            this, &SpreadSheet::updateColor);
    connect(table, &QTableWidget::currentItemChanged,
            this, &SpreadSheet::updateLineEdit);
    connect(table, &QTableWidget::itemChanged,
            this, &SpreadSheet::updateStatus);
    connect(formulaInput, &QLineEdit::returnPressed,
            this, &SpreadSheet::returnPressed);
    connect(table, &QTableWidget::itemChanged,
            this, &SpreadSheet::updateLineEdit);

    setWindowTitle(tr("Spreadsheet"));
}

void SpreadSheet::createActions()
{
    cell_sumAction = new QAction(tr("Sum"), this);
    connect(cell_sumAction, &QAction::triggered, this, &SpreadSheet::actionSum);

    cell_addAction = new QAction(tr("&Add"), this);
    cell_addAction->setShortcut(Qt::CTRL | Qt::Key_Plus);
    connect(cell_addAction, &QAction::triggered, this, &SpreadSheet::actionAdd);

    cell_subAction = new QAction(tr("&Subtract"), this);
    cell_subAction->setShortcut(Qt::CTRL | Qt::Key_Minus);
    connect(cell_subAction, &QAction::triggered, this, &SpreadSheet::actionSubtract);

    cell_mulAction = new QAction(tr("&Multiply"), this);
    cell_mulAction->setShortcut(Qt::CTRL | Qt::Key_multiply);
    connect(cell_mulAction, &QAction::triggered, this, &SpreadSheet::actionMultiply);

    cell_divAction = new QAction(tr("&Divide"), this);
    cell_divAction->setShortcut(Qt::CTRL | Qt::Key_division);
    connect(cell_divAction, &QAction::triggered, this, &SpreadSheet::actionDivide);

    fontAction = new QAction(tr("Font..."), this);
    fontAction->setShortcut(Qt::CTRL | Qt::Key_F);
    connect(fontAction, &QAction::triggered, this, &SpreadSheet::selectFont);

    colorAction = new QAction(QPixmap(16, 16), tr("Background &Color..."), this);
    connect(colorAction, &QAction::triggered, this, &SpreadSheet::selectColor);

    clearAction = new QAction(tr("Clear"), this);
    clearAction->setShortcut(Qt::Key_Delete);
    connect(clearAction, &QAction::triggered, this, &SpreadSheet::clear);

    aboutSpreadSheet = new QAction(tr("About Spreadsheet"), this);
    connect(aboutSpreadSheet, &QAction::triggered, this, &SpreadSheet::showAbout);

    exitAction = new QAction(tr("E&xit"), this);
    connect(exitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    printAction = new QAction(tr("&Print"), this);
    connect(printAction, &QAction::triggered, this, &SpreadSheet::print);

    firstSeparator = new QAction(this);
    firstSeparator->setSeparator(true);

    secondSeparator = new QAction(this);
    secondSeparator->setSeparator(true);
}

void SpreadSheet::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(printAction);
    fileMenu->addAction(exitAction);

    QMenu *cellMenu = menuBar()->addMenu(tr("&Cell"));
    cellMenu->addAction(cell_addAction);
    cellMenu->addAction(cell_subAction);
    cellMenu->addAction(cell_mulAction);
    cellMenu->addAction(cell_divAction);
    cellMenu->addAction(cell_sumAction);
    cellMenu->addSeparator();
    cellMenu->addAction(colorAction);
    cellMenu->addAction(fontAction);

    menuBar()->addSeparator();

    QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutSpreadSheet);
}

void SpreadSheet::updateStatus(QTableWidgetItem *item)
{
    if (item && item == table->currentItem()) {
        statusBar()->showMessage(item->data(Qt::StatusTipRole).toString(), 1000);
        cellLabel->setText(tr("Cell: (%1)").arg(encode_pos(table->row(item), table->column(item))));
    }
}

void SpreadSheet::updateColor(QTableWidgetItem *item)
{
    QPixmap pix(16, 16);
    QColor col;
    if (item)
        col = item->background().color();
    if (!col.isValid())
        col = palette().base().color();

    QPainter pt(&pix);
    pt.fillRect(0, 0, 16, 16, col);

    QColor lighter = col.lighter();
    pt.setPen(lighter);
    QPoint lightFrame[] = { QPoint(0, 15), QPoint(0, 0), QPoint(15, 0) };
    pt.drawPolyline(lightFrame, 3);

    pt.setPen(col.darker());
    QPoint darkFrame[] = { QPoint(1, 15), QPoint(15, 15), QPoint(15, 1) };
    pt.drawPolyline(darkFrame, 3);

    pt.end();

    colorAction->setIcon(pix);
}

void SpreadSheet::updateLineEdit(QTableWidgetItem *item)
{
    if (item != table->currentItem())
        return;
    if (item)
        formulaInput->setText(item->data(Qt::EditRole).toString());
    else
        formulaInput->clear();
}

void SpreadSheet::returnPressed()
{
    QString text = formulaInput->text();
    int row = table->currentRow();
    int col = table->currentColumn();
    QTableWidgetItem *item = table->item(row, col);
    if (!item)
        table->setItem(row, col, new SpreadSheetItem(text));
    else
        item->setData(Qt::EditRole, text);
    table->viewport()->update();
}

void SpreadSheet::selectColor()
{
    QTableWidgetItem *item = table->currentItem();
    QColor col = item ? item->background().color() : table->palette().base().color();
    col = QColorDialog::getColor(col, this);
    if (!col.isValid())
        return;

    const QList<QTableWidgetItem *> selected = table->selectedItems();
    if (selected.isEmpty())
        return;

    for (QTableWidgetItem *i : selected) {
        if (i)
            i->setBackground(col);
    }

    updateColor(table->currentItem());
}

void SpreadSheet::selectFont()
{
    const QList<QTableWidgetItem *> selected = table->selectedItems();
    if (selected.isEmpty())
        return;

    bool ok = false;
    QFont fnt = QFontDialog::getFont(&ok, font(), this);

    if (!ok)
        return;
    for (QTableWidgetItem *i : selected) {
        if (i)
            i->setFont(fnt);
    }
}

bool SpreadSheet::runInputDialog(const QString &title,
                                 const QString &c1Text,
                                 const QString &c2Text,
                                 const QString &opText,
                                 const QString &outText,
                                 QString *cell1, QString *cell2, QString *outCell)
{
    QStringList rows, cols;
    for (int c = 0; c < table->columnCount(); ++c)
        cols << QChar('A' + c);
    for (int r = 0; r < table->rowCount(); ++r)
        rows << QString::number(1 + r);

    QDialog addDialog(this);
    addDialog.setWindowTitle(title);

    QGroupBox group(title, &addDialog);
    group.setMinimumSize(250, 100);

    QLabel cell1Label(c1Text, &group);
    QComboBox cell1RowInput(&group);
    int c1Row, c1Col;
    decode_pos(*cell1, &c1Row, &c1Col);
    cell1RowInput.addItems(rows);
    cell1RowInput.setCurrentIndex(c1Row);

    QComboBox cell1ColInput(&group);
    cell1ColInput.addItems(cols);
    cell1ColInput.setCurrentIndex(c1Col);

    QLabel operatorLabel(opText, &group);
    operatorLabel.setAlignment(Qt::AlignHCenter);

    QLabel cell2Label(c2Text, &group);
    QComboBox cell2RowInput(&group);
    int c2Row, c2Col;
    decode_pos(*cell2, &c2Row, &c2Col);
    cell2RowInput.addItems(rows);
    cell2RowInput.setCurrentIndex(c2Row);
    QComboBox cell2ColInput(&group);
    cell2ColInput.addItems(cols);
    cell2ColInput.setCurrentIndex(c2Col);

    QLabel equalsLabel("=", &group);
    equalsLabel.setAlignment(Qt::AlignHCenter);

    QLabel outLabel(outText, &group);
    QComboBox outRowInput(&group);
    int outRow, outCol;
    decode_pos(*outCell, &outRow, &outCol);
    outRowInput.addItems(rows);
    outRowInput.setCurrentIndex(outRow);
    QComboBox outColInput(&group);
    outColInput.addItems(cols);
    outColInput.setCurrentIndex(outCol);

    QPushButton cancelButton(tr("Cancel"), &addDialog);
    connect(&cancelButton, &QAbstractButton::clicked, &addDialog, &QDialog::reject);

    QPushButton okButton(tr("OK"), &addDialog);
    okButton.setDefault(true);
    connect(&okButton, &QAbstractButton::clicked, &addDialog, &QDialog::accept);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(&okButton);
    buttonsLayout->addSpacing(10);
    buttonsLayout->addWidget(&cancelButton);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&addDialog);
    dialogLayout->addWidget(&group);
    dialogLayout->addStretch(1);
    dialogLayout->addItem(buttonsLayout);

    QHBoxLayout *cell1Layout = new QHBoxLayout;
    cell1Layout->addWidget(&cell1Label);
    cell1Layout->addSpacing(10);
    cell1Layout->addWidget(&cell1ColInput);
    cell1Layout->addSpacing(10);
    cell1Layout->addWidget(&cell1RowInput);

    QHBoxLayout *cell2Layout = new QHBoxLayout;
    cell2Layout->addWidget(&cell2Label);
    cell2Layout->addSpacing(10);
    cell2Layout->addWidget(&cell2ColInput);
    cell2Layout->addSpacing(10);
    cell2Layout->addWidget(&cell2RowInput);

    QHBoxLayout *outLayout = new QHBoxLayout;
    outLayout->addWidget(&outLabel);
    outLayout->addSpacing(10);
    outLayout->addWidget(&outColInput);
    outLayout->addSpacing(10);
    outLayout->addWidget(&outRowInput);

    QVBoxLayout *vLayout = new QVBoxLayout(&group);
    vLayout->addItem(cell1Layout);
    vLayout->addWidget(&operatorLabel);
    vLayout->addItem(cell2Layout);
    vLayout->addWidget(&equalsLabel);
    vLayout->addStretch(1);
    vLayout->addItem(outLayout);

    if (addDialog.exec()) {
        *cell1 = cell1ColInput.currentText() + cell1RowInput.currentText();
        *cell2 = cell2ColInput.currentText() + cell2RowInput.currentText();
        *outCell = outColInput.currentText() + outRowInput.currentText();
        return true;
    }

    return false;
}

void SpreadSheet::actionSum()
{
    int row_first = 0;
    int row_last = 0;
    int row_cur = 0;

    int col_first = 0;
    int col_last = 0;
    int col_cur = 0;

    const QList<QTableWidgetItem *> selected = table->selectedItems();

    if (!selected.isEmpty()) {
        QTableWidgetItem *first = selected.first();
        QTableWidgetItem *last = selected.last();
        row_first = table->row(first);
        row_last = table->row(last);
        col_first = table->column(first);
        col_last = table->column(last);
    }

    const QTableWidgetItem *current = table->currentItem();

    if (current) {
        row_cur = table->row(current);
        col_cur = table->column(current);
    }

    QString cell1 = encode_pos(row_first, col_first);
    QString cell2 = encode_pos(row_last, col_last);
    QString out = encode_pos(row_cur, col_cur);

    if (runInputDialog(tr("Sum cells"), tr("First cell:"), tr("Last cell:"),
                       QString("%1").arg(QChar(0x03a3)), tr("Output to:"),
                       &cell1, &cell2, &out)) {
        int row;
        int col;
        decode_pos(out, &row, &col);
        table->item(row, col)->setText(tr("sum %1 %2").arg(cell1, cell2));
    }
}

void SpreadSheet::actionMath_helper(const QString &title, const QString &op)
{
    QString cell1 = "C1";
    QString cell2 = "C2";
    QString out = "C3";

    const QTableWidgetItem *current = table->currentItem();
    if (current)
        out = encode_pos(table->currentRow(), table->currentColumn());

    if (runInputDialog(title, tr("Cell 1"), tr("Cell 2"), op, tr("Output to:"),
                       &cell1, &cell2, &out)) {
        int row, col;
        decode_pos(out, &row, &col);
        table->item(row, col)->setText(tr("%1 %2 %3").arg(op, cell1, cell2));
    }
}

void SpreadSheet::actionAdd()
{
    actionMath_helper(tr("Addition"), "+");
}

void SpreadSheet::actionSubtract()
{
    actionMath_helper(tr("Subtraction"), "-");
}

void SpreadSheet::actionMultiply()
{
    actionMath_helper(tr("Multiplication"), "*");
}
void SpreadSheet::actionDivide()
{
    actionMath_helper(tr("Division"), "/");
}

void SpreadSheet::clear()
{
    const QList<QTableWidgetItem *> selectedItems = table->selectedItems();
    for (QTableWidgetItem *i : selectedItems)
        i->setText(QString());
}

void SpreadSheet::setupContextMenu()
{
    addAction(cell_addAction);
    addAction(cell_subAction);
    addAction(cell_mulAction);
    addAction(cell_divAction);
    addAction(cell_sumAction);
    addAction(firstSeparator);
    addAction(colorAction);
    addAction(fontAction);
    addAction(secondSeparator);
    addAction(clearAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

void SpreadSheet::setupContents()
{
    QBrush titleBackground(Qt::lightGray);
    QFont titleFont = table->font();
    titleFont.setBold(true);

    // column 0
    table->setItem(0, 0, new SpreadSheetItem("Item"));
    table->item(0, 0)->setBackground(titleBackground);
    table->item(0, 0)->setToolTip("This column shows the purchased item/service");
    table->item(0, 0)->setFont(titleFont);

    table->setItem(1, 0, new SpreadSheetItem("AirportBus"));
    table->setItem(2, 0, new SpreadSheetItem("Flight (Munich)"));
    table->setItem(3, 0, new SpreadSheetItem("Lunch"));
    table->setItem(4, 0, new SpreadSheetItem("Flight (LA)"));
    table->setItem(5, 0, new SpreadSheetItem("Taxi"));
    table->setItem(6, 0, new SpreadSheetItem("Dinner"));
    table->setItem(7, 0, new SpreadSheetItem("Hotel"));
    table->setItem(8, 0, new SpreadSheetItem("Flight (Oslo)"));
    table->setItem(9, 0, new SpreadSheetItem("Total:"));

    table->item(9, 0)->setFont(titleFont);
    table->item(9, 0)->setBackground(titleBackground);

    // column 1
    table->setItem(0, 1, new SpreadSheetItem("Date"));
    table->item(0, 1)->setBackground(titleBackground);
    table->item(0, 1)->setToolTip("This column shows the purchase date, double click to change");
    table->item(0, 1)->setFont(titleFont);

    table->setItem(1, 1, new SpreadSheetItem("15/6/2006"));
    table->setItem(2, 1, new SpreadSheetItem("15/6/2006"));
    table->setItem(3, 1, new SpreadSheetItem("15/6/2006"));
    table->setItem(4, 1, new SpreadSheetItem("21/5/2006"));
    table->setItem(5, 1, new SpreadSheetItem("16/6/2006"));
    table->setItem(6, 1, new SpreadSheetItem("16/6/2006"));
    table->setItem(7, 1, new SpreadSheetItem("16/6/2006"));
    table->setItem(8, 1, new SpreadSheetItem("18/6/2006"));

    table->setItem(9, 1, new SpreadSheetItem());
    table->item(9, 1)->setBackground(titleBackground);

    // column 2
    table->setItem(0, 2, new SpreadSheetItem("Price"));
    table->item(0, 2)->setBackground(titleBackground);
    table->item(0, 2)->setToolTip("This column shows the price of the purchase");
    table->item(0, 2)->setFont(titleFont);

    table->setItem(1, 2, new SpreadSheetItem("150"));
    table->setItem(2, 2, new SpreadSheetItem("2350"));
    table->setItem(3, 2, new SpreadSheetItem("-14"));
    table->setItem(4, 2, new SpreadSheetItem("980"));
    table->setItem(5, 2, new SpreadSheetItem("5"));
    table->setItem(6, 2, new SpreadSheetItem("120"));
    table->setItem(7, 2, new SpreadSheetItem("300"));
    table->setItem(8, 2, new SpreadSheetItem("1240"));

    table->setItem(9, 2, new SpreadSheetItem());
    table->item(9, 2)->setBackground(Qt::lightGray);

    // column 3
    table->setItem(0, 3, new SpreadSheetItem("Currency"));
    table->item(0, 3)->setBackground(titleBackground);
    table->item(0, 3)->setToolTip("This column shows the currency");
    table->item(0, 3)->setFont(titleFont);

    table->setItem(1, 3, new SpreadSheetItem("NOK"));
    table->setItem(2, 3, new SpreadSheetItem("NOK"));
    table->setItem(3, 3, new SpreadSheetItem("EUR"));
    table->setItem(4, 3, new SpreadSheetItem("EUR"));
    table->setItem(5, 3, new SpreadSheetItem("USD"));
    table->setItem(6, 3, new SpreadSheetItem("USD"));
    table->setItem(7, 3, new SpreadSheetItem("USD"));
    table->setItem(8, 3, new SpreadSheetItem("USD"));

    table->setItem(9, 3, new SpreadSheetItem());
    table->item(9, 3)->setBackground(Qt::lightGray);

    // column 4
    table->setItem(0, 4, new SpreadSheetItem("Ex. Rate"));
    table->item(0, 4)->setBackground(titleBackground);
    table->item(0, 4)->setToolTip("This column shows the exchange rate to NOK");
    table->item(0, 4)->setFont(titleFont);

    table->setItem(1, 4, new SpreadSheetItem("1"));
    table->setItem(2, 4, new SpreadSheetItem("1"));
    table->setItem(3, 4, new SpreadSheetItem("8"));
    table->setItem(4, 4, new SpreadSheetItem("8"));
    table->setItem(5, 4, new SpreadSheetItem("7"));
    table->setItem(6, 4, new SpreadSheetItem("7"));
    table->setItem(7, 4, new SpreadSheetItem("7"));
    table->setItem(8, 4, new SpreadSheetItem("7"));

    table->setItem(9, 4, new SpreadSheetItem());
    table->item(9, 4)->setBackground(titleBackground);

    // column 5
    table->setItem(0, 5, new SpreadSheetItem("NOK"));
    table->item(0, 5)->setBackground(titleBackground);
    table->item(0, 5)->setToolTip("This column shows the expenses in NOK");
    table->item(0, 5)->setFont(titleFont);

    table->setItem(1, 5, new SpreadSheetItem("* C2 E2"));
    table->setItem(2, 5, new SpreadSheetItem("* C3 E3"));
    table->setItem(3, 5, new SpreadSheetItem("* C4 E4"));
    table->setItem(4, 5, new SpreadSheetItem("* C5 E5"));
    table->setItem(5, 5, new SpreadSheetItem("* C6 E6"));
    table->setItem(6, 5, new SpreadSheetItem("* C7 E7"));
    table->setItem(7, 5, new SpreadSheetItem("* C8 E8"));
    table->setItem(8, 5, new SpreadSheetItem("* C9 E9"));

    table->setItem(9, 5, new SpreadSheetItem("sum F2 F9"));
    table->item(9, 5)->setBackground(titleBackground);
}

const char *htmlText =
"<HTML>"
"<p><b>This demo shows use of <c>QTableWidget</c> with custom handling for"
" individual cells.</b></p>"
"<p>Using a customized table item we make it possible to have dynamic"
" output in different cells. The content that is implemented for this"
" particular demo is:"
"<ul>"
"<li>Adding two cells.</li>"
"<li>Subtracting one cell from another.</li>"
"<li>Multiplying two cells.</li>"
"<li>Dividing one cell with another.</li>"
"<li>Summing the contents of an arbitrary number of cells.</li>"
"</HTML>";

void SpreadSheet::showAbout()
{
    QMessageBox::about(this, "About Spreadsheet", htmlText);
}

void decode_pos(const QString &pos, int *row, int *col)
{
    if (pos.isEmpty()) {
        *col = -1;
        *row = -1;
    } else {
        *col = pos.at(0).toLatin1() - 'A';
        *row = pos.right(pos.size() - 1).toInt() - 1;
    }
}

QString encode_pos(int row, int col)
{
    return QString(col + 'A') + QString::number(row + 1);
}


void SpreadSheet::print()
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printpreviewdialog)
    QPrinter printer(QPrinter::ScreenResolution);
    QPrintPreviewDialog dlg(&printer);
    PrintView view;
    view.setModel(table->model());
    connect(&dlg, &QPrintPreviewDialog::paintRequested, &view, &PrintView::print);
    dlg.exec();
#endif
}

