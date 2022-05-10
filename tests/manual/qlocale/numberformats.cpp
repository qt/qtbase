// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "numberformats.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QVBoxLayout>

NumberFormatsWidget::NumberFormatsWidget()
{
    QGridLayout *l = new QGridLayout;

    QLabel *numbersLabel = new QLabel("Numbers:");
    number1 = createLineEdit();
    number2 = createLineEdit();
    number3 = createLineEdit();

    measurementLabel = new QLabel("Measurement units:");
    measurementSystem = createLineEdit();

    l->addWidget(numbersLabel, 0, 0);
    l->addWidget(number1, 0, 1);
    l->addWidget(number2, 0, 2);
    l->addWidget(number3, 0, 3);

    l->addWidget(measurementLabel, 1, 0);
    l->addWidget(measurementSystem, 1, 1, 1, 3);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(l);
    v->addStretch();
}

void NumberFormatsWidget::localeChanged(QLocale locale)
{
    number1->setText(locale.toString(-123456));
    number2->setText(locale.toString(1234.56, 'f', 2));
    number3->setText(locale.toString(1234.56, 'e', 4));

    measurementSystem->setText(
                locale.measurementSystem() == QLocale::ImperialSystem ? "US" : "Metric");
}

QLineEdit *NumberFormatsWidget::createLineEdit()
{
    QLineEdit *le = new QLineEdit;
    le->setReadOnly(true);
    return le;
}
