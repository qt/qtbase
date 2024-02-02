// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "miscellaneous.h"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>

MiscWidget::MiscWidget()
{
    QGridLayout *l = new QGridLayout;

    createLineEdit("Text to quote:", &textToQuoteLabel, &textToQuote);
    createLineEdit("Standard quotes:", &standardQuotedTextLabel, &standardQuotedText);
    createLineEdit("Alternate quotes:", &alternateQuotedTextLabel, &alternateQuotedText);
    textToQuote->setText("some text");
    createLineEdit("Text direction:", &textDirectionLabel, &textDirection);
    createLineEdit("List to separated string:", &listToSeparatedStringLabel, &listToSeparatedStringText);

    l->addWidget(textToQuoteLabel, 0, 0);
    l->addWidget(textToQuote, 0, 1);
    l->addWidget(standardQuotedTextLabel, 0, 2);
    l->addWidget(standardQuotedText, 0, 3);
    l->addWidget(alternateQuotedTextLabel, 1, 2);
    l->addWidget(alternateQuotedText, 1, 3);
    l->addWidget(textDirectionLabel, 2, 0);
    l->addWidget(textDirection, 2, 1, 1, 3);
    l->addWidget(listToSeparatedStringLabel, 3, 0);
    l->addWidget(listToSeparatedStringText, 3, 1, 1, 3);

    connect(textToQuote, SIGNAL(textChanged(QString)), this, SLOT(updateQuotedText(QString)));

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(l);
    v->addStretch();
}

void MiscWidget::updateQuotedText(QString str)
{
    standardQuotedText->setText(locale().quoteString(str));
    alternateQuotedText->setText(locale().quoteString(str, QLocale::AlternateQuotation));
}

void MiscWidget::updateListToSeparatedStringText()
{
    QStringList test;
    test << "aaa" << "bbb" << "ccc" << "ddd";
    listToSeparatedStringText->setText(locale().createSeparatedList(test));
}

void MiscWidget::localeChanged(QLocale locale)
{
    setLocale(locale);
    updateQuotedText(textToQuote->text());
    updateListToSeparatedStringText();
    textDirection->setText(locale.textDirection() == Qt::LeftToRight ? "Left To Right" : "Right To Left");
}

void MiscWidget::createLineEdit(const QString &label, QLabel **labelWidget, QLineEdit **lineEditWidget)
{
    QLabel *lbl = new QLabel(label);
    QLineEdit *le = new QLineEdit;
    le->setReadOnly(true);
    lbl->setBuddy(le);
    if (labelWidget)
        *labelWidget = lbl;
    if (lineEditWidget)
        *lineEditWidget = le;
}
