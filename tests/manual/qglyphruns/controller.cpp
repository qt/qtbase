// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "controller.h"
#include "ui_controller.h"

Controller::Controller(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Controller)
{
    ui->setupUi(this);

    connect(ui->cbWrap, &QCheckBox::toggled, this, &Controller::updateViews);
    connect(ui->gbRange, &QGroupBox::toggled, this, &Controller::updateViews);
    connect(ui->teSourceString, &QPlainTextEdit::textChanged, this, &Controller::updateViews);
    connect(ui->hsLineWidth, &QSlider::valueChanged, this, &Controller::updateViews);
    connect(ui->inspector, &GlyphRunInspector::updateBounds, ui->view, &View::setVisualizedBounds);
    connect(ui->sbFrom, &QSpinBox::valueChanged, this, &Controller::updateViews);
    connect(ui->sbTo, &QSpinBox::valueChanged, this, &Controller::updateViews);
    connect(ui->teSourceString, &QPlainTextEdit::selectionChanged, this, &Controller::updateRange);
    connect(ui->fcbFont, &QFontComboBox::currentFontChanged, this, &Controller::updateViews);
}

Controller::~Controller()
{
    delete ui;
}

void Controller::updateRange()
{
    if (ui->gbRange->isChecked()) {
        QTextCursor cursor = ui->teSourceString->textCursor();
        if (cursor.hasSelection()) {
            ui->sbFrom->setValue(cursor.selectionStart());
            ui->sbTo->setValue(cursor.selectionEnd() - 1);

            updateViews();
        }
    }
}

void Controller::updateViews()
{
    QString s = ui->teSourceString->toPlainText();
    ui->sbFrom->setMaximum(s.length());
    ui->sbTo->setMaximum(s.length());

    s.replace('\n', QChar::LineSeparator);
    ui->view->updateLayout(s,
                           qreal(ui->hsLineWidth->value()) * ui->hsLineWidth->width() / 100.0f,
                           ui->cbWrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::ManualWrap,
                           ui->fcbFont->currentFont());

    int start = ui->gbRange->isChecked() ? ui->sbFrom->value() : -1;
    int length = ui->gbRange->isChecked() ? ui->sbTo->value() - start + 1 : -1;
    ui->inspector->updateLayout(ui->view->layout(), start, length);
}
