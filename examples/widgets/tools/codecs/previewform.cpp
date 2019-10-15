/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include "previewform.h"

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QTextCodec>
#include <QTextStream>

// Helpers for creating hex dumps
static void indent(QTextStream &str, int indent)
{
    for (int i = 0; i < indent; ++i)
        str << ' ';
}

static void formatHex(QTextStream &str, const QByteArray &data)
{
    const int fieldWidth = str.fieldWidth();
    const QTextStream::FieldAlignment alignment = str.fieldAlignment();
    const int base = str.integerBase();
    const QChar padChar = str.padChar();
    str.setIntegerBase(16);
    str.setPadChar(QLatin1Char('0'));
    str.setFieldAlignment(QTextStream::AlignRight);

    const unsigned char *p = reinterpret_cast<const unsigned char *>(data.constBegin());
    for (const unsigned char *end = p + data.size(); p < end; ++p) {
        str << ' ';
        str.setFieldWidth(2);
        str << unsigned(*p);
        str.setFieldWidth(fieldWidth);
    }
    str.setFieldAlignment(alignment);
    str.setPadChar(padChar);
    str.setIntegerBase(base);
}

static void formatPrintableCharacters(QTextStream &str, const QByteArray &data)
{
    for (const char c : data) {
        switch (c) {
        case '\0':
            str << "\\0";
            break;
        case '\t':
            str << "\\t";
            break;
        case '\r':
            str << "\\r";
            break;
        case '\n':
            str << "\\n";
            break;
        default:
            if (c >= 32 && uchar(c) < 127)
                str << ' ' << c;
            else
                str << "..";
            break;
        }
    }
}

static QString formatHexDump(const QByteArray &data)
{
    enum { lineWidth = 16 };
    QString result;
    QTextStream str(&result);
    str.setIntegerBase(16);
    str.setPadChar(QLatin1Char('0'));
    const int fieldWidth = str.fieldWidth();
    const QTextStream::FieldAlignment alignment = str.fieldAlignment();
    for (int a = 0, size = data.size(); a < size; a += lineWidth) {
        str.setFieldAlignment(QTextStream::AlignRight);
        str.setFieldWidth(8);
        str << a;
        str.setFieldWidth(fieldWidth);
        str.setFieldAlignment(alignment);

        const int end = qMin(a + lineWidth, size);
        const QByteArray line = data.mid(a, end - a);

        formatHex(str, line);
        indent(str, 3 * (lineWidth - line.size()));

        str << ' ';
        formatPrintableCharacters(str, line);
        indent(str, 2 * (lineWidth - line.size()));
        str << '\n';
    }
    return result;
}

PreviewForm::PreviewForm(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    encodingComboBox = new QComboBox;

    QLabel *encodingLabel = new QLabel(tr("&Encoding:"));
    encodingLabel->setBuddy(encodingComboBox);

    textEdit = new QPlainTextEdit;
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    textEdit->setReadOnly(true);
    hexDumpEdit = new QPlainTextEdit;
    hexDumpEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    hexDumpEdit->setReadOnly(true);
    hexDumpEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    okButton = buttonBox->button(QDialogButtonBox::Ok);

    connect(encodingComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &PreviewForm::updateTextEdit);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(encodingLabel, 0, 0);
    mainLayout->addWidget(encodingComboBox, 0, 1);
    tabWidget = new QTabWidget;
    tabWidget->addTab(textEdit, tr("Preview"));
    tabWidget->addTab(hexDumpEdit, tr("Hex Dump"));
    mainLayout->addWidget(tabWidget, 1, 0, 1, 2);
    statusLabel = new QLabel;
    mainLayout->addWidget(statusLabel, 2, 0, 1, 2);
    mainLayout->addWidget(buttonBox, 3, 0, 1, 2);

    const QRect screenGeometry = screen()->geometry();
    resize(screenGeometry.width() * 2 / 5, screenGeometry.height() / 2);
}

void PreviewForm::setCodecList(const QVector<QTextCodec *> &list)
{
    encodingComboBox->clear();
    for (const QTextCodec *codec : list) {
        encodingComboBox->addItem(QLatin1String(codec->name()),
                                  QVariant(codec->mibEnum()));
    }
}

void PreviewForm::reset()
{
    decodedStr.clear();
    textEdit->clear();
    hexDumpEdit->clear();
    statusLabel->clear();
    statusLabel->setStyleSheet(QString());
    okButton->setEnabled(false);
    tabWidget->setCurrentIndex(0);
}

void PreviewForm::setEncodedData(const QByteArray &data)
{
    reset();
    encodedData = data;
    hexDumpEdit->setPlainText(formatHexDump(data));
    updateTextEdit();
}

void PreviewForm::updateTextEdit()
{
    int mib = encodingComboBox->itemData(
                      encodingComboBox->currentIndex()).toInt();
    const QTextCodec *codec = QTextCodec::codecForMib(mib);
    const QString name = QLatin1String(codec->name());

    QTextCodec::ConverterState state;
    decodedStr = codec->toUnicode(encodedData.constData(), encodedData.size(), &state);

    bool success = true;
    if (state.remainingChars) {
        success = false;
        const QString message =
            tr("%1: conversion error at character %2")
            .arg(name).arg(encodedData.size() - state.remainingChars + 1);
        statusLabel->setText(message);
        statusLabel->setStyleSheet(QStringLiteral("background-color: \"red\";"));
    } else if (state.invalidChars) {
        statusLabel->setText(tr("%1: %n invalid characters", nullptr, state.invalidChars).arg(name));
        statusLabel->setStyleSheet(QStringLiteral("background-color: \"yellow\";"));
    } else {
        statusLabel->setText(tr("%1: %n bytes converted", nullptr, encodedData.size()).arg(name));
        statusLabel->setStyleSheet(QString());
    }
    if (success)
        textEdit->setPlainText(decodedStr);
    else
        textEdit->clear();
    okButton->setEnabled(success);
}
