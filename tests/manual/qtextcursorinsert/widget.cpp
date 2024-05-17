// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "widget.h"
#include "./ui_widget.h"

#include <QBuffer>
#include <QShortcut>
#include <QTextBlock>
#include <QTextDocumentWriter>
#include <QTextList>

using namespace Qt::StringLiterals;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    m_texts.insert(u"0-numbered html list"_s, u"<ol start=\"0\">\n<li>eggs</li>\n<li>maple syrup</li>\n</ol>"_s);
    m_texts.insert(u"0-numbered markdown list"_s, u"0) eggs\n1) maple syrup\n"_s);
    m_texts.insert(u"lorem ipsum markdown"_s,
                   u"Lorem ipsum dolor sit amet quod scimus quomodo legere et scribere Markdown, non solum applicationes interrete."_s);
    m_texts.insert(u"markdown checkboxes"_s,
                   u"- [ ] kürbis-kernöl\n- [ ] mon cheri (große schachtel)\n- [ ] bergkäse\n- [ ] mannerwaffln\n"_s);
    m_texts.insert(u"markdown checkboxes (3 hidden list styles)"_s,
                   u"- [ ] kürbis-kernöl\n- [ ] mon cheri (große schachtel)\n+ [ ] bergkäse\n* [ ] mannerwaffln\n"_s);
    m_texts.insert(u"numbered html list"_s, u"<ol>\n<li>eggs</li>\n<li>maple syrup</li>\n</ol>"_s);
    m_texts.insert(u"numbered markdown list"_s, u"1. bread\n1. milk\n"_s);

    for (auto it = m_texts.constBegin(); it != m_texts.constEnd(); ++it) {
        ui->richTextCB->addItem(it.key(), it.value());
        ui->plainTextCB->addItem(it.key(), it.value());
    }

    ui->richTextCB->setCurrentIndex(m_texts.count() - 1);
    ui->plainTextCB->setCurrentIndex(1);

    m_fileDialog.setWindowTitle(tr("Save rich text"));
    m_fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    m_fileDialog.setMimeTypeFilters({"text/markdown", "text/html", "text/plain",
                                     "application/vnd.oasis.opendocument.text"});
    connect(&m_fileDialog, &QFileDialog::fileSelected, this, &Widget::onSave);

    connect(new QShortcut(QKeySequence::Save, this), &QShortcut::activated, [this]() { m_fileDialog.open(); });
    connect(new QShortcut(QKeySequence::Quit, this), &QShortcut::activated, [this]() { qApp->quit(); });
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_insertMarkdownButton_clicked()
{
    if (ui->newBlockBeforeCB->isChecked()) {
        if (ui->moveToBeginningRB->isChecked())
            ui->textEdit->moveCursor(QTextCursor::StartOfBlock);
        if (ui->moveToEndRB->isChecked())
            ui->textEdit->moveCursor(QTextCursor::EndOfBlock);
        if (ui->defaultBlockFormatCB->isChecked())
            ui->textEdit->textCursor().insertBlock(QTextBlockFormat());
        else
            ui->textEdit->textCursor().insertBlock();
    }
    ui->textEdit->textCursor().insertMarkdown(ui->plainTextEdit->toPlainText());
    if (ui->newBlockBeforeCB->isChecked()) {
        if (ui->defaultBlockFormatCB->isChecked())
            ui->textEdit->textCursor().insertBlock(QTextBlockFormat());
        else
            ui->textEdit->textCursor().insertBlock();
    }
}

void Widget::on_insertHtmlButton_clicked()
{
    if (ui->newBlockBeforeCB->isChecked()) {
        if (ui->moveToBeginningRB->isChecked())
            ui->textEdit->moveCursor(QTextCursor::StartOfBlock);
        if (ui->moveToEndRB->isChecked())
            ui->textEdit->moveCursor(QTextCursor::EndOfBlock);
        if (ui->defaultBlockFormatCB->isChecked())
            ui->textEdit->textCursor().insertBlock(QTextBlockFormat());
        else
            ui->textEdit->textCursor().insertBlock();
    }
    ui->textEdit->insertHtml(ui->plainTextEdit->toPlainText());
    if (ui->newBlockBeforeCB->isChecked()) {
        if (ui->defaultBlockFormatCB->isChecked())
            ui->textEdit->textCursor().insertBlock(QTextBlockFormat());
        else
            ui->textEdit->textCursor().insertBlock();
    }
}

void Widget::on_insertPlainButton_clicked()
{
    if (ui->newBlockBeforeCB->isChecked()) {
        if (ui->moveToBeginningRB->isChecked())
            ui->textEdit->moveCursor(QTextCursor::StartOfBlock);
        if (ui->moveToEndRB->isChecked())
            ui->textEdit->moveCursor(QTextCursor::EndOfBlock);
        if (ui->defaultBlockFormatCB->isChecked())
            ui->textEdit->textCursor().insertBlock(QTextBlockFormat());
        else
            ui->textEdit->textCursor().insertBlock();
    }
    ui->textEdit->insertPlainText(ui->plainTextEdit->toPlainText());
    if (ui->newBlockBeforeCB->isChecked()) {
        if (ui->defaultBlockFormatCB->isChecked())
            ui->textEdit->textCursor().insertBlock(QTextBlockFormat());
        else
            ui->textEdit->textCursor().insertBlock();
    }
}

void Widget::on_plainTextCB_activated(int index)
{
    if (index < m_texts.size()) {
        auto it = m_texts.constBegin();
        std::advance(it, index);
        ui->plainTextEdit->setPlainText(it.value());
    }
}

void Widget::on_richTextCB_activated(int index)
{
    if (index < m_texts.size()) {
        auto it = m_texts.constBegin();
        std::advance(it, index);
        if (it.key().contains(u"markdown", Qt::CaseInsensitive))
            ui->textEdit->setMarkdown(it.value());
        else if (it.key().contains(u"html", Qt::CaseInsensitive))
            ui->textEdit->setHtml(it.value());
        else
            ui->textEdit->setPlainText(it.value());
    }
}

void Widget::on_textEdit_cursorPositionChanged()
{
    QTextBlock block = ui->textEdit->textCursor().block();
    ui->blockDesc->setText(u"%1 of %2"_s
                           .arg(block.blockNumber())
                           .arg(ui->textEdit->document()->blockCount()));
    QTextList *list = block.textList();
    if (list) {
        QTextBlock first = list->item(0);
        ui->listDesc->setText(u"index %1: \"%2\" is in list\nwith style %3 and %4 items starting with %5: %6 %7"_s
                              .arg(list->itemNumber(block))
                              .arg(block.text())
                              .arg(list->format().style())
                              .arg(list->count())
                              .arg(list->format().start())
                              .arg(list->format().style() > QTextListFormat::ListDecimal ? u""_s : list->itemText(first))
                              .arg(first.text()) );
    } else {
        ui->listDesc->setText(u"not in a list"_s);
    }
}

void Widget::on_saveButton_clicked()
{
    m_fileDialog.open();
}

void Widget::onSave(const QString &file)
{
    QFile f(file);
    if (f.open(QFile::WriteOnly)) {
        if (m_fileDialog.selectedMimeTypeFilter() == u"text/markdown")
            f.write(ui->textEdit->toMarkdown().toUtf8());
        else if (m_fileDialog.selectedMimeTypeFilter() == u"text/html")
            f.write(ui->textEdit->toHtml().toUtf8());
        else if (m_fileDialog.selectedMimeTypeFilter() == u"text/plain")
            f.write(ui->textEdit->toPlainText().toUtf8());
        else if (m_fileDialog.selectedMimeTypeFilter() == u"application/vnd.oasis.opendocument.text") {
            QBuffer buffer;
            QTextDocumentWriter writer(&buffer, "ODF");
            writer.write(ui->textEdit->document());
            buffer.close();
            f.write(buffer.data());
        }
        f.close();
    }
}

