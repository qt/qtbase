// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    connect(doubleDecimals, &QSpinBox::valueChanged,
            this, &ValidatorWidget::updateDoubleValidator);
    connect(doubleFormat, &QComboBox::activated,
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
