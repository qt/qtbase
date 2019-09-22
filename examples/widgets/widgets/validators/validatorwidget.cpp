/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "validatorwidget.h"

#include <QIntValidator>

ValidatorWidget::ValidatorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    connect(localeSelector, &LocaleSelector::localeSelected,
            this, &ValidatorWidget::setLocale);
    connect(localeSelector, &LocaleSelector::localeSelected,
            this, &ValidatorWidget::updateValidator);
    connect(localeSelector, &LocaleSelector::localeSelected,
            this, &ValidatorWidget::updateDoubleValidator);

    connect(minVal, &QSpinBox::editingFinished,
            this, &ValidatorWidget::updateValidator);
    connect(maxVal, &QSpinBox::editingFinished,
            this, &ValidatorWidget::updateValidator);
    connect(editor, &QLineEdit::editingFinished,
            ledWidget, &LEDWidget::flash);

    connect(doubleMaxVal, &QDoubleSpinBox::editingFinished,
            this, &ValidatorWidget::updateDoubleValidator);
    connect(doubleMinVal, &QDoubleSpinBox::editingFinished,
            this, &ValidatorWidget::updateDoubleValidator);
    connect(doubleDecimals, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ValidatorWidget::updateDoubleValidator);
    connect(doubleFormat, QOverload<int>::of(&QComboBox::activated),
            this, &ValidatorWidget::updateDoubleValidator);
    connect(doubleEditor, &QLineEdit::editingFinished,
            doubleLedWidget, &LEDWidget::flash);

    updateValidator();
    updateDoubleValidator();
}

void ValidatorWidget::updateValidator()
{
    QIntValidator *v = new QIntValidator(minVal->value(), maxVal->value(), this);
    v->setLocale(locale());
    delete editor->validator();
    editor->setValidator(v);

    QString s = editor->text();
    int i = 0;
    if (v->validate(s, i) == QValidator::Invalid) {
        editor->clear();
    } else {
        editor->setText(s);
    }
}

void ValidatorWidget::updateDoubleValidator()
{
    QDoubleValidator *v
        = new QDoubleValidator(doubleMinVal->value(), doubleMaxVal->value(),
                                doubleDecimals->value(), this);
    v->setNotation(static_cast<QDoubleValidator::Notation>(doubleFormat->currentIndex()));
    v->setLocale(locale());
    delete doubleEditor->validator();
    doubleEditor->setValidator(v);

    QString s = doubleEditor->text();
    int i = 0;
    if (v->validate(s, i) == QValidator::Invalid) {
        doubleEditor->clear();
    } else {
        doubleEditor->setText(s);
    }
}
