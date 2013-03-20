/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

#include <QtWidgets>
#ifndef QT_NO_PRINTER
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#endif

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
   : QMainWindow(parent)
{
    setupUi(this);

    sampleSizes << 32 << 24 << 16 << 14 << 12 << 8 << 4 << 2 << 1;
    markedCount = 0;
    setupFontTree();

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(fontTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(showFont(QTreeWidgetItem*)));
    connect(fontTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(updateStyles(QTreeWidgetItem*,int)));

    fontTree->setItemSelected(fontTree->topLevelItem(0), true);
    showFont(fontTree->topLevelItem(0));
}

void MainWindow::setupFontTree()
{
    QFontDatabase database;
    fontTree->setColumnCount(1);
    fontTree->setHeaderLabels(QStringList() << tr("Font"));

    foreach (QString family, database.families()) {
        const QStringList styles = database.styles(family);
        if (styles.isEmpty())
            continue;

        QTreeWidgetItem *familyItem = new QTreeWidgetItem(fontTree);
        familyItem->setText(0, family);
        familyItem->setCheckState(0, Qt::Unchecked);

        foreach (QString style, styles) {
            QTreeWidgetItem *styleItem = new QTreeWidgetItem(familyItem);
            styleItem->setText(0, style);
            styleItem->setCheckState(0, Qt::Unchecked);
            styleItem->setData(0, Qt::UserRole, QVariant(database.weight(family, style)));
            styleItem->setData(0, Qt::UserRole + 1, QVariant(database.italic(family, style)));
        }
    }
}

void MainWindow::on_clearAction_triggered()
{
    QTreeWidgetItem *currentItem = fontTree->currentItem();
    foreach (QTreeWidgetItem *item, fontTree->selectedItems())
        fontTree->setItemSelected(item, false);
    fontTree->setItemSelected(currentItem, true);
}

void MainWindow::on_markAction_triggered()
{
    markUnmarkFonts(Qt::Checked);
}

void MainWindow::on_unmarkAction_triggered()
{
    markUnmarkFonts(Qt::Unchecked);
}

void MainWindow::markUnmarkFonts(Qt::CheckState state)
{
    QList<QTreeWidgetItem *> items = fontTree->selectedItems();
    foreach (QTreeWidgetItem *item, items) {
        if (item->checkState(0) != state)
            item->setCheckState(0, state);
    }
}

void MainWindow::showFont(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QString family;
    QString style;
    int weight;
    bool italic;

    if (item->parent()) {
        family = item->parent()->text(0);
        style = item->text(0);
        weight = item->data(0, Qt::UserRole).toInt();
        italic = item->data(0, Qt::UserRole + 1).toBool();
    } else {
        family = item->text(0);
        style = item->child(0)->text(0);
        weight = item->child(0)->data(0, Qt::UserRole).toInt();
        italic = item->child(0)->data(0, Qt::UserRole + 1).toBool();
    }

    QString oldText = textEdit->toPlainText().trimmed();
    bool modified = textEdit->document()->isModified();
    textEdit->clear();
    textEdit->document()->setDefaultFont(QFont(family, 32, weight, italic));

    QTextCursor cursor = textEdit->textCursor();
    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(Qt::AlignCenter);
    cursor.insertBlock(blockFormat);

    if (modified)
        cursor.insertText(QString(oldText));
    else
        cursor.insertText(QString("%1 %2").arg(family).arg(style));

    textEdit->document()->setModified(modified);
}

void MainWindow::updateStyles(QTreeWidgetItem *item, int column)
{
    if (!item || column != 0)
        return;

    Qt::CheckState state = item->checkState(0);
    QTreeWidgetItem *parent = item->parent();

    if (parent) {

        // Only count style items.
        if (state == Qt::Checked)
            ++markedCount;
        else
            --markedCount;

        if (state == Qt::Checked && parent->checkState(0) == Qt::Unchecked) {
            // Mark parent items when child items are checked.
            parent->setCheckState(0, Qt::Checked);

        } else if (state == Qt::Unchecked && parent->checkState(0) == Qt::Checked) {

            bool marked = false;
            for (int row = 0; row < parent->childCount(); ++row) {
                if (parent->child(row)->checkState(0) == Qt::Checked) {
                    marked = true;
                    break;
                }
            }
            // Unmark parent items when all child items are unchecked.
            if (!marked)
                parent->setCheckState(0, Qt::Unchecked);
        }
    } else {
        int row;
        int number = 0;
        for (row = 0; row < item->childCount(); ++row) {
            if (item->child(row)->checkState(0) == Qt::Checked)
                ++number;
        }

        // Mark/unmark all child items when marking/unmarking top-level
        // items.
        if (state == Qt::Checked && number == 0) {
            for (row = 0; row < item->childCount(); ++row) {
                if (item->child(row)->checkState(0) == Qt::Unchecked)
                    item->child(row)->setCheckState(0, Qt::Checked);
            }
        } else if (state == Qt::Unchecked && number > 0) {
            for (row = 0; row < item->childCount(); ++row) {
                if (item->child(row)->checkState(0) == Qt::Checked)
                    item->child(row)->setCheckState(0, Qt::Unchecked);
            }
        }
    }

    printAction->setEnabled(markedCount > 0);
    printPreviewAction->setEnabled(markedCount > 0);
}

QMap<QString, StyleItems> MainWindow::currentPageMap()
{
    QMap<QString, StyleItems> pageMap;

    for (int row = 0; row < fontTree->topLevelItemCount(); ++row) {
        QTreeWidgetItem *familyItem = fontTree->topLevelItem(row);
        QString family;

        if (familyItem->checkState(0) == Qt::Checked) {
            family = familyItem->text(0);
            pageMap[family] = StyleItems();
        }

        for (int childRow = 0; childRow < familyItem->childCount(); ++childRow) {
            QTreeWidgetItem *styleItem = familyItem->child(childRow);
            if (styleItem->checkState(0) == Qt::Checked)
                pageMap[family].append(styleItem);
        }
    }

    return pageMap;
}

#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
void MainWindow::on_printAction_triggered()
{
    pageMap = currentPageMap();

    if (pageMap.count() == 0)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    int from = printer.fromPage();
    int to = printer.toPage();
    if (from <= 0 && to <= 0)
        printer.setFromTo(1, pageMap.keys().count());

    printDocument(&printer);
}

void MainWindow::printDocument(QPrinter *printer)
{
    printer->setFromTo(1, pageMap.count());

    QProgressDialog progress(tr("Preparing font samples..."), tr("&Cancel"),
                             0, pageMap.count(), this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setWindowTitle(tr("Font Sampler"));
    progress.setMinimum(printer->fromPage() - 1);
    progress.setMaximum(printer->toPage());

    QPainter painter;
    painter.begin(printer);
    bool firstPage = true;

    for (int page = printer->fromPage(); page <= printer->toPage(); ++page) {

        if (!firstPage)
            printer->newPage();

        qApp->processEvents();
        if (progress.wasCanceled())
            break;

        printPage(page - 1, &painter, printer);
        progress.setValue(page);
        firstPage = false;
    }

    painter.end();
}

void MainWindow::on_printPreviewAction_triggered()
{
    pageMap = currentPageMap();

    if (pageMap.count() == 0)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, SIGNAL(paintRequested(QPrinter*)),
            this, SLOT(printDocument(QPrinter*)));
    preview.exec();
}

void MainWindow::printPage(int index, QPainter *painter, QPrinter *printer)
{
    QString family = pageMap.keys()[index];
    StyleItems items = pageMap[family];

    // Find the dimensions of the text on each page.
    qreal width = 0.0;
    qreal height = 0.0;
    foreach (QTreeWidgetItem *item, items) {
        QString style = item->text(0);
        int weight = item->data(0, Qt::UserRole).toInt();
        bool italic = item->data(0, Qt::UserRole + 1).toBool();

        // Calculate the maximum width and total height of the text.
        foreach (int size, sampleSizes) {
            QFont font(family, size, weight, italic);
            font = QFont(font, painter->device());
            QFontMetricsF fontMetrics(font);
            QRectF rect = fontMetrics.boundingRect(
            QString("%1 %2").arg(family).arg(style));
            width = qMax(rect.width(), width);
            height += rect.height();
        }
    }

    qreal xScale = printer->pageRect().width() / width;
    qreal yScale = printer->pageRect().height() / height;
    qreal scale = qMin(xScale, yScale);

    qreal remainingHeight = printer->pageRect().height()/scale - height;
    qreal spaceHeight = (remainingHeight / 4.0) / (items.count() + 1);
    qreal interLineHeight = (remainingHeight / 4.0) / (sampleSizes.count() * items.count());

    painter->save();
    painter->translate(printer->pageRect().width() / 2.0, printer->pageRect().height() / 2.0);
    painter->scale(scale, scale);
    painter->setBrush(QBrush(Qt::black));

    qreal x = -width / 2.0;
    qreal y = -height / 2.0 - remainingHeight / 4.0 + spaceHeight;

    foreach (QTreeWidgetItem *item, items) {
        QString style = item->text(0);
        int weight = item->data(0, Qt::UserRole).toInt();
        bool italic = item->data(0, Qt::UserRole + 1).toBool();

        // Draw each line of text.
        foreach (int size, sampleSizes) {
            QFont font(family, size, weight, italic);
            font = QFont(font, painter->device());
            QFontMetricsF fontMetrics(font);
            QRectF rect = fontMetrics.boundingRect(QString("%1 %2").arg(
                          font.family()).arg(style));
            y += rect.height();
            painter->setFont(font);
            painter->drawText(QPointF(x, y), QString("%1 %2").arg(family).arg(style));
            y += interLineHeight;
        }
        y += spaceHeight;
    }

    painter->restore();
}
#endif // QT_NO_PRINTER
