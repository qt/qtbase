// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "stylesheeteditor.h"

#include <QFile>
#include <QRegularExpression>
#include <QStyleFactory>

StyleSheetEditor::StyleSheetEditor(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    connect(ui.styleCombo, &QComboBox::textActivated, this, &StyleSheetEditor::setStyleName);
    connect(ui.styleSheetCombo, &QComboBox::textActivated, this, &StyleSheetEditor::setStyleSheetName);
    connect(ui.styleTextEdit, &QTextEdit::textChanged, this, &StyleSheetEditor::setModified);
    connect(ui.applyButton, &QAbstractButton::clicked, this, &StyleSheetEditor::apply);

    QRegularExpression regExp("^.(.*)\\+?Style$");
    QString defaultStyle = QApplication::style()->metaObject()->className();
    QRegularExpressionMatch match = regExp.match(defaultStyle);

    if (match.hasMatch())
        defaultStyle = match.captured(1);

    ui.styleCombo->addItems(QStyleFactory::keys());
    ui.styleCombo->setCurrentIndex(ui.styleCombo->findText(defaultStyle, Qt::MatchContains));
    ui.styleSheetCombo->setCurrentIndex(ui.styleSheetCombo->findText("Coffee"));
    loadStyleSheet("Coffee");
}

void StyleSheetEditor::setStyleName(const QString &styleName)
{
    qApp->setStyle(styleName);
    ui.applyButton->setEnabled(false);
}

void StyleSheetEditor::setStyleSheetName(const QString &sheetName)
{
    loadStyleSheet(sheetName);
}

void StyleSheetEditor::setModified()
{
    ui.applyButton->setEnabled(true);
}

void StyleSheetEditor::apply()
{
    qApp->setStyleSheet(ui.styleTextEdit->toPlainText());
    ui.applyButton->setEnabled(false);
}

void StyleSheetEditor::loadStyleSheet(const QString &sheetName)
{
    QFile file(":/qss/" + sheetName.toLower() + ".qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());

    ui.styleTextEdit->setPlainText(styleSheet);
    qApp->setStyleSheet(styleSheet);
    ui.applyButton->setEnabled(false);
}
