/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
