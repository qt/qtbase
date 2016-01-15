/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // Changing font size and indent values to verify fix for QTBUG-5111.
    connect(ui->spinBoxFontPointSize, SIGNAL(valueChanged(int)), this, SLOT(setFontPointSize(int)));
    connect(ui->spinBoxIndentWidth, SIGNAL(valueChanged(int)), this, SLOT(setIndentWidth(int)));

    textCursor = new QTextCursor(ui->textEdit->document());

    // Initialize text list with different formats and layouts
    QTextListFormat listFormat;
    // disc
    listFormat.setStyle(QTextListFormat::ListDisc);
    textCursor->insertList(listFormat);
    textCursor->insertText("disc one");
    textCursor->insertText("\ndisc two");
    // 1., 2.
    listFormat.setStyle(QTextListFormat::ListDecimal);
    textCursor->insertList(listFormat);
    textCursor->insertText("decimal one");
    textCursor->insertText("\ndecimal two");
    // a., b.
    listFormat.setStyle(QTextListFormat::ListLowerAlpha);
    textCursor->insertList(listFormat);
    textCursor->insertText("lower alpha one");
    textCursor->insertText("\nlower alpha two");
    // A., B.
    listFormat.setStyle(QTextListFormat::ListUpperAlpha);
    textCursor->insertList(listFormat);
    textCursor->insertText("upper alpha one");
    textCursor->insertText("\nupper alpha two");
    // Indent 1
    listFormat.setStyle(QTextListFormat::ListDisc);
    listFormat.setIndent(1);
    textCursor->insertList(listFormat);
    textCursor->insertText("indent 1 one");
    textCursor->insertText("\nindent 2 two");
    // Indent 2
    listFormat.setIndent(2);
    textCursor->insertList(listFormat);
    textCursor->insertText("indent 2 one");
    textCursor->insertText("\nindent 2 two");
    // Indent 3
    listFormat.setIndent(3);
    textCursor->insertList(listFormat);
    textCursor->insertText("indent 3 one");
    textCursor->insertText("\nindent 3 two");
    // right to left: disc
    listFormat.setIndent(1);
    listFormat.setStyle(QTextListFormat::ListDisc);
    textCursor->insertList(listFormat);
    textCursor->insertText(QChar( 0x05d0)); // use Hebrew aleph to create a right-to-left layout
    textCursor->insertText("\n" + QString(QChar( 0x05d0)));
    // right to left: 1., 2.
    listFormat.setStyle(QTextListFormat::ListLowerAlpha);
    textCursor->insertList(listFormat);
    textCursor->insertText(QChar( 0x05d0)); // use Hebrew aleph to create a right-to-left layout
    textCursor->insertText("\n" + QString(QChar( 0x05d0)));

    QFont font;
    setFontPointSize(font.pointSize());
    ui->textEdit->setFont(font);

    setIndentWidth(static_cast<int>(ui->textEdit->document()->indentWidth()));
}

void Widget::setFontPointSize(int value)
{
    ui->textEdit->selectAll();
    ui->textEdit->setFontPointSize(value);
}

void Widget::setIndentWidth(int value)
{
    ui->textEdit->document()->setIndentWidth(value);
}

Widget::~Widget()
{
    delete ui;
    delete textCursor;
}
