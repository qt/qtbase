/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
****************************************************************************/

#include "widget.h"
#include "ui_widget.h"

#include <QScrollBar>
#include <QFile>
#include <QDir>
#include <QTemporaryFile>

#ifndef QT_NO_DESKTOPSERVICES
#include <QDesktopServices>
#endif

#ifndef QT_NO_PRINTER
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#endif

// This manual test allows checking the QTextTable border logic (QTBUG-36152)

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    resize(1400, 800);

    connect(ui->docComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Widget::onDocumentSelected);
    connect(ui->printButton, &QPushButton::clicked, this, &Widget::onPrint);
    connect(ui->previewButton, &QPushButton::clicked, this, &Widget::onPreview);
    connect(ui->openBrowserButton, &QPushButton::clicked, this, &Widget::onOpenBrowser);

    connect(ui->sourceEdit, &QTextEdit::textChanged, this,
            [this]() {
                // make this a world class HTML IDE
                auto pos = ui->htmlEdit->verticalScrollBar()->value();
                ui->htmlEdit->setHtml(ui->sourceEdit->toPlainText());
                ui->htmlEdit->verticalScrollBar()->setValue(pos);
            });

    ui->docComboBox->addItem(tr("Table Border Test"), ":/table-border-test.html");
    ui->docComboBox->addItem(tr("Table Border Header Test"), ":/table-border-test-header.html");

    ui->docComboBox->setCurrentIndex(0);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onDocumentSelected()
{
    QString url = ui->docComboBox->itemData(ui->docComboBox->currentIndex()).toString();
    QFile f(url);
    if (f.open(QFile::ReadOnly)) {
        ui->sourceEdit->setPlainText(QString::fromUtf8(f.readAll()));
        // preview HTML is set via textChanged signal
    }
}

void Widget::onPrint()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    if (ui->htmlEdit->textCursor().hasSelection())
        dlg.addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg.setWindowTitle(tr("Print Document"));
    if (dlg.exec() == QDialog::Accepted) {
        ui->htmlEdit->print(&printer);
    }
#endif
}

void Widget::onPreview()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, ui->htmlEdit, &QTextEdit::print);
    preview.exec();
#endif
}

void Widget::onOpenBrowser()
{
    // write the current html to a temp file and open the system browser
#ifndef QT_NO_DESKTOPSERVICES
    auto tf = new QTemporaryFile(QDir::tempPath() + "/XXXXXX.html", this);
    if (tf->open()) {
        tf->write(ui->sourceEdit->toPlainText().toUtf8());
        tf->close();
        QDesktopServices::openUrl(QUrl::fromLocalFile(tf->fileName()));
    }
#endif
}
