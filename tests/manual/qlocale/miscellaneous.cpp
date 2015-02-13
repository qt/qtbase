/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
